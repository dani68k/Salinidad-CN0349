const UUID = {
  service: '8d7b0001-6f7a-4f31-9c4d-2b6e6a7f3a10', status: '8d7b0002-6f7a-4f31-9c4d-2b6e6a7f3a10',
  setup: '8d7b0003-6f7a-4f31-9c4d-2b6e6a7f3a10', setupCommand: '8d7b0004-6f7a-4f31-9c4d-2b6e6a7f3a10',
  setupResult: '8d7b0005-6f7a-4f31-9c4d-2b6e6a7f3a10', calibration: '8d7b0006-6f7a-4f31-9c4d-2b6e6a7f3a10',
  measurement: '8d7b0007-6f7a-4f31-9c4d-2b6e6a7f3a10', controlCommand: '8d7b0008-6f7a-4f31-9c4d-2b6e6a7f3a10',
  controlResult: '8d7b0009-6f7a-4f31-9c4d-2b6e6a7f3a10', advanced: '8d7b000a-6f7a-4f31-9c4d-2b6e6a7f3a10',
  ecDiagnostic: '8d7b000b-6f7a-4f31-9c4d-2b6e6a7f3a10'
};
const VERSION = 1, decoder = new TextDecoder(), $ = selector => document.querySelector(selector);
const ui = { compatibility: $('#compatibility'), connect: $('#connect'), disconnect: $('#disconnect'), connection: $('#connection'), connectionDot: $('#connection-dot'),
  instrument: $('#instrument'), cal: $('#cal'), busy: $('#busy'), error: $('#error'), form: $('#setup-form'), interval: $('#interval'),
  range: $('#range'), model: $('#model'), cellK: $('#cell-k'), apply: $('#apply'), cancel: $('#cancel'), pending: $('#pending'),
  calRun: $('#cal-run'), calGeneral: $('#cal-general'), calBody: $('#calibration-body'), sensors: $('#sensors'),
  setSensors: $('#set-sensors'), once: $('#once'), start: $('#start'), stop: $('#stop'), cycle: $('#cycle'),
  ptResult: $('#pt-result'), ecResult: $('#ec-result'), ecTitle: $('#ec-title'), ecDisplay: $('#ec-display'),
  chart: $('#measurement-chart'), historyCount: $('#history-count'), chartEcUnit: $('#chart-ec-unit'),
  chartScale: [...document.querySelectorAll('input[name="chart-scale"]')], chartScaleError: $('#chart-scale-error'),
  temperatureMin: $('#temperature-min'), temperatureMax: $('#temperature-max'),
  conductivityMin: $('#conductivity-min'), conductivityMax: $('#conductivity-max') };
let device, ch = {}, appliedSetup = null, state = null, busy = false, applying = false, requestId = 0;
let pendingSetup = null, pendingControl = null, previousState = null, latestEc = null, measurementHistory = [];
let chartScaleMode = 'auto', latestEcDiagnostic = null, previousCalibrationCount = null, pendingEc = null, pendingEcTimer = null;
const controlErrors = ['OK','INVALID_LENGTH','UNSUPPORTED_VERSION','INVALID_OPERATION','NOT_READY','BUSY','INVALID_ARGUMENT','HARDWARE','NOT_CALIBRATED','TIMEOUT','MEASUREMENT_FAILED','CANCELLED','INTERNAL_ERROR'];
const instrumentErrors = ['OK','BUSY','INVALID_ARGUMENT','HARDWARE','NOT_CALIBRATED','TIMEOUT','MEASUREMENT_FAILED','CANCELLED'];
const conductivityErrors = ['OK','NOT_RUN','INVALID_MODEL','INVALID_IMPEDANCE','INVALID_K','INVALID_R2','NON_POSITIVE_RESISTANCE','OVERFLOW','CAL_INVALID','CAL_MISMATCH','INVALID_MAGNITUDE','NON_POSITIVE_ADMITTANCE','INVALID_DENOMINATOR'];

function error(value) { ui.error.textContent = value ? (value.message || String(value)) : ''; }
function connected() { return Boolean(device?.gatt?.connected); }
function ready() { return connected() && state === 'READY' && !busy && !applying && !pendingControl; }
function setConnection(text, active) { ui.connection.textContent = active ? 'CONNECTED' : text; ui.connection.dataset.state = active ? 'on' : 'off'; ui.connectionDot.dataset.state = active ? 'on' : 'off'; ui.connect.disabled = active; ui.connect.hidden = active; ui.disconnect.disabled = !active; ui.disconnect.hidden = !active; }
function setupDraft() { return { sensors: appliedSetup?.sensors || 3, interval: Number(ui.interval.value), range: Number(ui.range.value), model: Number(ui.model.value), cellK: Number(ui.cellK.value) }; }
function sameSetup(a,b) { return a && b && a.interval===b.interval && a.range===b.range && a.model===b.model && Math.abs(a.cellK-b.cellK)<=Math.max(1e-6,Math.abs(a.cellK)*1e-6); }
function availability() {
  const canEdit = ready(); for (const item of ui.form.elements) item.disabled = !canEdit;
  ui.apply.disabled = !canEdit || !appliedSetup || sameSetup(setupDraft(), appliedSetup); ui.calRun.disabled = !canEdit;
  ui.sensors.disabled = !canEdit; ui.setSensors.disabled = !canEdit || !appliedSetup || Number(ui.sensors.value)===appliedSetup.sensors;
  ui.once.disabled = !canEdit; ui.start.disabled = !canEdit; ui.stop.disabled = !connected() || !busy || Boolean(pendingControl);
}
function pendingLabel() { const changed=appliedSetup&&!sameSetup(setupDraft(),appliedSetup); ui.pending.textContent=changed?'CHANGES PENDING':'NO CHANGES'; ui.pending.classList.toggle('changed',Boolean(changed)); availability(); }

function renderAdvanced(value) {
  const ids=['mclk','start','step','increments','settling','range','vpp','pga','mode'];
  let values=ids.map(()=> '\u2014');
  if (value) {
    if (value.byteLength!==20 || value.getUint8(0)!==VERSION) throw Error('ADVANCED invalid');
    const range=value.getUint8(17), gain=value.getUint8(18), vpp=value.getUint8(19), increments=value.getUint16(13,true);
    values=[value.getUint32(1,true),value.getUint32(5,true),value.getUint32(9,true),increments,value.getUint16(15,true),range===1?'Range 1':'\u2014',vpp?`${vpp/10} V`:'\u2014',gain===1?'x1':'\u2014',increments===0?'Single frequency':'Sweep'];
  }
  ids.forEach((id,i)=> { $(`#advanced-${id}`).textContent=values[i]; });
}

function decodeSetup(value) { const v=new DataView(value.buffer,value.byteOffset,value.byteLength); if(value.byteLength!==12||v.getUint8(0)!==VERSION) throw Error('SETUP no válido'); return {sensors:v.getUint8(1),interval:v.getUint32(2,true),range:v.getUint8(6),model:v.getUint8(7),cellK:v.getFloat32(8,true)}; }
function renderSetup(s) { appliedSetup=s; ui.interval.value=s.interval; ui.range.value=s.range; ui.model.value=s.model; ui.cellK.value=s.cellK; ui.sensors.value=s.sensors; pendingLabel(); renderEc(); }
function encodeSetup(s,id) { if(!Number.isInteger(s.interval)||s.interval<1||s.interval>2147483647||!Number.isFinite(s.cellK)||s.cellK<=0) throw Error('Setup no válido'); const b=new ArrayBuffer(13),v=new DataView(b); v.setUint8(0,VERSION);v.setUint8(1,id);v.setUint8(2,s.sensors);v.setUint32(3,s.interval,true);v.setUint8(7,s.range);v.setUint8(8,s.model);v.setFloat32(9,s.cellK,true);return b; }
function wait(kind,id,op=0) { return new Promise((resolve,reject)=>{ const holder={id,op,resolve,reject,timer:setTimeout(()=>{ if(kind==='setup')pendingSetup=null;else pendingControl=null; reject(Error('Timeout de confirmación BLE'));},5000)}; if(kind==='setup')pendingSetup=holder;else pendingControl=holder; }); }
function finishPending(holder,code,names) { clearTimeout(holder.timer); code===0?holder.resolve():holder.reject(Error(names[code]||`ERROR_${code}`)); }

async function applySetup(event) { event.preventDefault(); if(!ready())return; applying=true;availability();error(null); try{const id=requestId=++requestId&255, packet=encodeSetup(setupDraft(),id),ack=wait('setup',id);await ch.setupCommand.writeValueWithResponse(packet);await ack;renderSetup(decodeSetup(await ch.setup.readValue()));}catch(e){if(pendingSetup){clearTimeout(pendingSetup.timer);pendingSetup=null;}error(e);}finally{applying=false;availability();} }
async function control(op,arg=0) { if(pendingControl)throw Error('Hay otra orden pendiente'); const id=requestId=++requestId&255,b=Uint8Array.of(VERSION,id,op,arg),ack=wait('control',id,op); availability(); try{await ch.controlCommand.writeValueWithResponse(b);await ack;}catch(e){if(pendingControl){clearTimeout(pendingControl.timer);pendingControl=null;}throw e;}finally{availability();} }

function showStatus(value) { try{const s=JSON.parse(decoder.decode(value));previousState=state;state=s.state;busy=s.busy;if((previousState!==state&&state==='CALIBRATING')||(previousCalibrationCount!==null&&s.cal<previousCalibrationCount)){latestEcDiagnostic=null;clearTimeout(pendingEcTimer);pendingEc=null;if(latestEc){latestEc.diagnostic=0;renderEc();}}previousCalibrationCount=s.cal;ui.instrument.textContent=state;ui.cal.textContent=`${s.cal}/4`;ui.busy.textContent=busy?'Yes':'No';availability();if(previousState==='CALIBRATING'&&state!=='CALIBRATING')readCalibration();}catch(e){error(e);} }
function setupResult(event){const v=event.target.value;if(!pendingSetup||v.byteLength!==3||v.getUint8(1)!==pendingSetup.id)return;const p=pendingSetup;pendingSetup=null;finishPending(p,v.getUint8(2),['OK','INVALID_LENGTH','UNSUPPORTED_VERSION','INVALID_VALUE','NOT_READY','BUSY','INTERNAL_ERROR']);}
function controlResult(event){const v=event.target.value;if(!pendingControl||v.byteLength!==4||v.getUint8(1)!==pendingControl.id||v.getUint8(2)!==pendingControl.op)return;const p=pendingControl;pendingControl=null;finishPending(p,v.getUint8(3),controlErrors);}

function decodeCalibration(value){if(value.byteLength!==76)throw Error('CALIBRATION incompleta');const v=new DataView(value.buffer,value.byteOffset,value.byteLength);if(v.getUint8(0)!==VERSION)throw Error('Versión CALIBRATION no compatible');const refs=[],names=['HIGH 100R','HIGH 1K','LOW 1K','LOW 10K'];let o=6;for(const name of names){refs.push({name,state:v.getUint8(o),mag:v.getFloat32(o+1,true),gf:v.getFloat32(o+5,true)});o+=9;}for(const name of ['AD HIGH','AD LOW']){refs.push({name,state:v.getUint8(o)?2:0,gf:v.getFloat64(o+1,true),nos:v.getFloat64(o+9,true),ad:true});o+=17;}return{state:v.getUint8(1),run:v.getUint32(2,true),refs};}
function renderCalibration(c){const general=['ABSENT','CALIBRATING','VALID','PARTIAL','INVALID'][c.state]||'UNKNOWN';ui.calGeneral.textContent=`${general} · RUN ${c.run}`;ui.calBody.innerHTML=c.refs.map(r=>`<tr><td>${r.name}</td><td>${['INVALID','RUNNING','VALID'][r.state]||'—'}</td><td>${r.ad?'—':r.mag.toFixed(3)}</td><td>${r.gf?r.gf.toExponential(8):'—'}</td><td>${r.ad&&r.state===2?r.nos.toFixed(3):'—'}</td></tr>`).join('');}
async function readCalibration(){if(!ch.calibration)return;try{renderCalibration(decodeCalibration(await ch.calibration.readValue()));}catch(e){error(e);}}

function renderHistory() {
  const ns='http://www.w3.org/2000/svg',width=Math.max(ui.chart.clientWidth,320),height=Math.max(ui.chart.clientHeight,140),left=62,right=66,top=18,bottom=44;
  const plotWidth=width-left-right,plotHeight=height-top-bottom;
  const make=(tag,attrs={},text='')=>{const el=document.createElementNS(ns,tag);for(const [key,val] of Object.entries(attrs))el.setAttribute(key,String(val));if(text)el.textContent=text;return el;};
  const ecUnit=ui.ecDisplay.value==='ms'?'mS/cm':'µS/cm',ecScale=ui.ecDisplay.value==='ms'?'milliSiemens':'microSiemens';
  ui.chartEcUnit.textContent=ecUnit;
  ui.historyCount.textContent=`${measurementHistory.length} / 50 cycles`;
  ui.chart.replaceChildren();
  const svg=make('svg',{viewBox:`0 0 ${width} ${height}`,role:'img','aria-label':`Last ${measurementHistory.length} measurement cycles; temperature in degrees Celsius and conductivity in ${ecUnit}`});
  svg.append(make('title',{},'Temperature and conductivity history'));
  const tempValues=measurementHistory.map(p=>p.temperature).filter(Number.isFinite);
  const ecValues=measurementHistory.map(p=>p[ecScale]).filter(Number.isFinite);
  const range=values=>{if(!values.length)return[0,10];let min=Math.min(...values),max=Math.max(...values);if(min===max){const pad=Math.max(Math.abs(min)*0.05,1);min-=pad;max+=pad;}else{const pad=(max-min)*0.08;min-=pad;max+=pad;}return[min,max];};
  const temperatureMin=Number(ui.temperatureMin.value),temperatureMax=Number(ui.temperatureMax.value);
  const conductivityMin=Number(ui.conductivityMin.value),conductivityMax=Number(ui.conductivityMax.value);
  const validFixedRanges=[temperatureMin,temperatureMax,conductivityMin,conductivityMax].every(Number.isFinite)&&temperatureMax>temperatureMin&&conductivityMax>conductivityMin;
  if(chartScaleMode==='fixed'&&!validFixedRanges){ui.chartScaleError.textContent='Fixed limits must be valid numbers with maximum greater than minimum.';return;}
  ui.chartScaleError.textContent='';
  const tempRange=chartScaleMode==='fixed'?[temperatureMin,temperatureMax]:range(tempValues);
  const ecFactor=ui.ecDisplay.value==='ms'?1000:1;
  const ecRange=chartScaleMode==='fixed'?[conductivityMin/ecFactor,conductivityMax/ecFactor]:range(ecValues);
  const y=(value,r)=>top+(r[1]-Math.min(r[1],Math.max(r[0],value)))/(r[1]-r[0])*plotHeight;
  for(let i=0;i<5;i++){
    const fraction=i/4,gy=top+fraction*plotHeight;
    svg.append(make('line',{x1:left,y1:gy,x2:width-right,y2:gy,class:'chart-grid'}));
    const tv=tempRange[1]-fraction*(tempRange[1]-tempRange[0]),ev=ecRange[1]-fraction*(ecRange[1]-ecRange[0]);
    svg.append(make('text',{x:left-9,y:gy+4,'text-anchor':'end',class:'chart-axis'},tv.toFixed(1)));
    svg.append(make('text',{x:width-right+9,y:gy+4,'text-anchor':'start',class:'chart-axis'},ev.toFixed(ui.ecDisplay.value==='ms'?3:1)));
  }
  const firstCycle=measurementHistory[0]?.cycle,lastCycle=measurementHistory[measurementHistory.length-1]?.cycle;
  svg.append(make('text',{x:left,y:height-10,'text-anchor':'start',class:'chart-axis'},firstCycle?`Cycle ${firstCycle}`:'Cycle'));
  svg.append(make('text',{x:width-right,y:height-10,'text-anchor':'end',class:'chart-axis'},lastCycle?`Cycle ${lastCycle}`:'Cycle'));
  svg.append(make('text',{x:18,y:top+plotHeight/2,transform:`rotate(-90 18 ${top+plotHeight/2})`,'text-anchor':'middle',class:'chart-axis-title'},'Temperature (°C)'));
  svg.append(make('text',{x:width-12,y:top+plotHeight/2,transform:`rotate(90 ${width-12} ${top+plotHeight/2})`,'text-anchor':'middle',class:'chart-axis-title'},`Conductivity (${ecUnit})`));
  if(!measurementHistory.length){
    svg.append(make('text',{x:width/2,y:top+plotHeight/2,'text-anchor':'middle',class:'chart-empty'},'Waiting for measurement data...'));
    ui.chart.setAttribute('aria-label','Measurement history chart; waiting for data');
    ui.chart.append(svg);
    return;
  }
  if(!tempValues.length&&!ecValues.length)svg.append(make('text',{x:width/2,y:top+plotHeight/2,'text-anchor':'middle',class:'chart-empty'},'No valid measurements yet'));
  const x=i=>left+(measurementHistory.length===1?plotWidth/2:i/(measurementHistory.length-1)*plotWidth);
  const draw=(key,r,lineClass,pointClass,digits)=>{
    let path='',active=false;
    measurementHistory.forEach((point,i)=>{
      const value=point[key];
      if(!Number.isFinite(value)){active=false;return;}
      const px=x(i),py=y(value,r);
      path+=`${active?'L':'M'}${px.toFixed(2)},${py.toFixed(2)} `;
      active=true;
      const dot=make('circle',{cx:px,cy:py,r:3.3,class:pointClass});
      dot.append(make('title',{},`Cycle ${point.cycle}: ${value.toFixed(digits)}`));
      svg.append(dot);
    });
    if(path)svg.insertBefore(make('path',{d:path.trim(),class:lineClass}),svg.querySelector('circle'));
  };
  draw('temperature',tempRange,'chart-line-temperature','chart-point-temperature',2);
  draw(ecScale,ecRange,'chart-line-conductivity','chart-point-conductivity',ui.ecDisplay.value==='ms'?3:1);
  ui.chart.setAttribute('aria-label',`Chart with ${measurementHistory.length} cycles; temperature and conductivity readings`);
  ui.chart.append(svg);
}
function renderEc(){
  const models=['RAW','SERIES','AD'],model=latestEc?.model??appliedSetup?.model;
  ui.ecTitle.textContent=`Conductivity with ${models[model]||'—'}`;
  const diagnosis=latestEc?.state===1?latestEc.diagnostic:0;
  ui.ecResult.dataset.rangeDiagnostic=({2:'near',3:'below',4:'above'})[diagnosis]||'';
  const recommendation=$('#ec-range-recommendation');
  const suggestHigh=diagnosis===4&&latestEc?.range===0;
  const suggestLow=diagnosis===3&&latestEc?.range===1;
  recommendation.hidden=!(suggestHigh||suggestLow);
  recommendation.textContent=suggestHigh?'Recommended: switch to HIGH':suggestLow?'Recommended: try LOW':'';
  if(latestEc){
    const {state,error:ecError,detail,microSiemens,milliSiemens}=latestEc;
    const failure=ecError===6&&detail>1?(conductivityErrors[detail]||'MEASUREMENT_FAILED'):(instrumentErrors[ecError]||'ERROR');
    ui.ecResult.textContent=state===0?'Not selected':state===1?`${ui.ecDisplay.value==='us'?`${microSiemens.toFixed(1)} µS/cm`:ui.ecDisplay.value==='ms'?`${milliSiemens.toFixed(3)} mS/cm`:`${microSiemens.toFixed(1)} µS/cm · ${milliSiemens.toFixed(3)} mS/cm`}`:failure;
  }
  renderHistory();
}
function showEcDiagnostic(value){
  if(value.byteLength!==7||value.getUint8(0)!==VERSION)return;
  const cycle=value.getUint32(1,true),diagnostic=value.getUint8(5),range=value.getUint8(6);
  if(!cycle||diagnostic>4||range>1){latestEcDiagnostic=null;return;}
  latestEcDiagnostic={cycle,diagnostic,range};
  if(pendingEc?.cycle===cycle){clearTimeout(pendingEcTimer);pendingEc.diagnostic=diagnostic;pendingEc.range=range;latestEc=pendingEc;pendingEc=null;renderEc();return;}
  if(latestEc?.cycle===cycle){latestEc.diagnostic=diagnostic;latestEc.range=range;renderEc();}
}
function showMeasurement(value){
  try{
    if(value.byteLength!==26)return;
    const v=new DataView(value.buffer,value.byteOffset,value.byteLength);
    if(v.getUint8(0)!==VERSION)return;
    const cycle=v.getUint32(2,true);
    if(!cycle)return;
    const overall=v.getUint8(6);
    if(overall===2)return;
    const overallError=v.getUint8(7);
    ui.cycle.textContent=`Cycle ${cycle} · ${overall===1?'OK':instrumentErrors[overallError]||'ERROR'}`;
    const ptState=v.getUint8(8),ptError=v.getUint8(9),temperature=v.getFloat32(10,true);
    ui.ptResult.textContent=ptState===0?'Not selected':ptState===1?`${temperature.toFixed(2)} °C`:instrumentErrors[ptError]||'ERROR';
    const diagnostic=latestEcDiagnostic?.cycle===cycle?latestEcDiagnostic:null;
    const measurementEc={cycle,diagnostic:diagnostic?.diagnostic??0,range:diagnostic?.range,state:v.getUint8(14),error:v.getUint8(15),detail:v.getUint8(17),model:v.getUint8(16),microSiemens:v.getFloat32(18,true),milliSiemens:v.getFloat32(22,true)};
    if(measurementEc.state===1&&ch.ecDiagnostic&&!diagnostic){
      const alreadyWaiting=Boolean(pendingEc);
      pendingEc=measurementEc;
      if(!alreadyWaiting)pendingEcTimer=setTimeout(()=>{if(pendingEc){latestEc=pendingEc;pendingEc=null;renderEc();}},1000);
    }else{clearTimeout(pendingEcTimer);pendingEc=null;latestEc=measurementEc;}
    const point={cycle,temperature:ptState===1&&Number.isFinite(temperature)?temperature:null,microSiemens:measurementEc.state===1&&Number.isFinite(measurementEc.microSiemens)?measurementEc.microSiemens:null,milliSiemens:measurementEc.state===1&&Number.isFinite(measurementEc.milliSiemens)?measurementEc.milliSiemens:null};
    if(measurementHistory[measurementHistory.length-1]?.cycle===cycle)measurementHistory[measurementHistory.length-1]=point;
    else measurementHistory.push(point);
    if(measurementHistory.length>50)measurementHistory.shift();
    renderEc();
  }catch(e){error(e);}
}

async function connect(){error(null);setConnection('CONNECTING…',false);ui.connect.disabled=true;try{device=await navigator.bluetooth.requestDevice({filters:[{name:'CN0349'}],optionalServices:[UUID.service]});device.addEventListener('gattserverdisconnected',disconnectEvent);const server=await device.gatt.connect(),service=await server.getPrimaryService(UUID.service);for(const [key,id] of Object.entries(UUID))if(key!=='service'&&key!=='advanced'&&key!=='ecDiagnostic')ch[key]=await service.getCharacteristic(id);try{ch.advanced=await service.getCharacteristic(UUID.advanced);ch.advanced.addEventListener('characteristicvaluechanged',e=>{try{renderAdvanced(e.target.value);}catch(problem){error(problem);}});await ch.advanced.startNotifications();renderAdvanced(await ch.advanced.readValue());}catch(e){ch.advanced=null;renderAdvanced(null);}try{ch.ecDiagnostic=await service.getCharacteristic(UUID.ecDiagnostic);ch.ecDiagnostic.addEventListener('characteristicvaluechanged',e=>showEcDiagnostic(e.target.value));await ch.ecDiagnostic.startNotifications();showEcDiagnostic(await ch.ecDiagnostic.readValue());}catch(_){ch.ecDiagnostic=null;latestEcDiagnostic=null;}showStatus(await ch.status.readValue());renderSetup(decodeSetup(await ch.setup.readValue()));await readCalibration();showMeasurement(await ch.measurement.readValue());for(const [key,handler] of [['status',e=>showStatus(e.target.value)],['setup',e=>{try{renderSetup(decodeSetup(e.target.value));}catch(problem){error(problem);}}],['setupResult',setupResult],['measurement',e=>showMeasurement(e.target.value)],['controlResult',controlResult]]){await ch[key].startNotifications();ch[key].addEventListener('characteristicvaluechanged',handler);}setConnection('CONNECTED',true);availability();}catch(e){error(e);if(device?.gatt?.connected)device.gatt.disconnect();setConnection('DISCONNECTED',false);}}
function disconnectEvent(){clearTimeout(pendingEcTimer);pendingEc=null;renderAdvanced(null);setConnection('DISCONNECTED',false);ch={};state=null;busy=false;appliedSetup=null;latestEc=null;latestEcDiagnostic=null;previousCalibrationCount=null;measurementHistory=[];ui.ecResult.dataset.rangeDiagnostic='';$('#ec-range-recommendation').hidden=true;ui.ptResult.textContent='—';ui.ecResult.textContent='—';ui.cycle.textContent='Waiting for data...';renderHistory();if(pendingSetup){clearTimeout(pendingSetup.timer);pendingSetup.reject(Error('BLE disconnected'));pendingSetup=null;}if(pendingControl){clearTimeout(pendingControl.timer);pendingControl.reject(Error('BLE disconnected'));pendingControl=null;}availability();}

ui.connect.onclick=connect;ui.disconnect.onclick=()=>device?.gatt?.disconnect();ui.form.oninput=pendingLabel;ui.form.onsubmit=applySetup;ui.cancel.onclick=()=>appliedSetup&&renderSetup(appliedSetup);
ui.ecDisplay.onchange=renderEc;
$('#clear-history').onclick=()=>{measurementHistory=[];renderHistory();};
for(const input of ui.chartScale)input.onchange=()=>{chartScaleMode=input.value;renderHistory();};
for(const input of [ui.temperatureMin,ui.temperatureMax,ui.conductivityMin,ui.conductivityMax])input.oninput=renderHistory;
const views=['measurement','setup','calibration'];function showView(name,updateHash=false){const active=views.includes(name)?name:'measurement';for(const view of views){$(`#${view}-view`).hidden=view!==active;$(`#nav-${view}`).setAttribute('aria-current',view===active?'page':'false');}document.title='EC WATER ANALYSIS';if(active==='measurement')renderHistory();if(updateHash)location.hash=`#/${active}`;}for(const view of views)$(`#nav-${view}`).onclick=event=>{event.preventDefault();showView(view,true);};window.addEventListener('hashchange',()=>{if(location.hash.startsWith('#/'))showView(location.hash.slice(2));});window.addEventListener('resize',()=>requestAnimationFrame(renderHistory));showView(location.hash.slice(2));
ui.calRun.onclick=async()=>{error(null);try{await control(1); }catch(e){error(e);}};
ui.setSensors.onclick=async()=>{error(null);try{await control(2,Number(ui.sensors.value));renderSetup(decodeSetup(await ch.setup.readValue()));}catch(e){error(e);}};
ui.once.onclick=async()=>{error(null);try{await control(3);}catch(e){error(e);}};ui.start.onclick=async()=>{error(null);try{await control(4);}catch(e){error(e);}};ui.stop.onclick=async()=>{error(null);try{await control(5);showMeasurement(await ch.measurement.readValue());}catch(e){error(e);}};
if(!navigator.bluetooth){ui.compatibility.hidden=false;ui.compatibility.textContent=location.protocol==='file:'?'Web Bluetooth no está disponible desde file:// en este navegador; usa uno compatible o localhost.':'Este navegador no ofrece Web Bluetooth.';ui.connect.disabled=true;}else if(!window.isSecureContext){ui.compatibility.hidden=false;ui.compatibility.textContent='Web Bluetooth requiere contexto seguro; prueba localhost.';}renderHistory();availability();
