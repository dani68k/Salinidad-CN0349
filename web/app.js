const DEVICE_NAME = 'CN0349';
const SERVICE_UUID = '8d7b0001-6f7a-4f31-9c4d-2b6e6a7f3a10';
const STATUS_UUID = '8d7b0002-6f7a-4f31-9c4d-2b6e6a7f3a10';

const ui = {
  compatibility: document.querySelector('#compatibility'),
  connect: document.querySelector('#connect'),
  disconnect: document.querySelector('#disconnect'),
  connection: document.querySelector('#connection'),
  instrument: document.querySelector('#instrument'),
  cal: document.querySelector('#cal'),
  busy: document.querySelector('#busy'),
  error: document.querySelector('#error')
};

let device = null;
let statusCharacteristic = null;
const decoder = new TextDecoder('utf-8');

function setConnection(text, connected) {
  ui.connection.textContent = text;
  ui.connection.dataset.state = connected ? 'on' : 'off';
  ui.connect.disabled = connected;
  ui.disconnect.disabled = !connected;
}

function showError(error) {
  ui.error.textContent = error ? (error.message || String(error)) : 'Ninguno';
}

function showStatus(value) {
  try {
    const status = JSON.parse(decoder.decode(value));
    if (typeof status.state !== 'string' || typeof status.cal !== 'number' || typeof status.busy !== 'boolean') {
      throw new Error('STATUS contiene campos no válidos');
    }
    ui.instrument.textContent = status.state;
    ui.cal.textContent = `${status.cal}/4`;
    ui.busy.textContent = status.busy ? 'Sí' : 'No';
    showError(null);
  } catch (error) {
    showError(error);
  }
}

function onStatus(event) {
  showStatus(event.target.value);
}

function resetStatus() {
  statusCharacteristic = null;
  ui.instrument.textContent = 'Sin datos';
  ui.cal.textContent = '—';
  ui.busy.textContent = '—';
}

function onDisconnected() {
  setConnection('Desconectado', false);
  resetStatus();
}

async function connect() {
  showError(null);
  setConnection('Seleccionando dispositivo…', false);
  ui.connect.disabled = true;
  try {
    device = await navigator.bluetooth.requestDevice({
      filters: [{ name: DEVICE_NAME }],
      optionalServices: [SERVICE_UUID]
    });
    device.addEventListener('gattserverdisconnected', onDisconnected);
    setConnection('Conectando…', false);
    ui.connect.disabled = true;
    const server = await device.gatt.connect();
    const service = await server.getPrimaryService(SERVICE_UUID);
    statusCharacteristic = await service.getCharacteristic(STATUS_UUID);
    showStatus(await statusCharacteristic.readValue());
    await statusCharacteristic.startNotifications();
    statusCharacteristic.addEventListener('characteristicvaluechanged', onStatus);
    setConnection(`Conectado a ${device.name || DEVICE_NAME}`, true);
  } catch (error) {
    showError(error);
    if (device?.gatt?.connected) device.gatt.disconnect();
    setConnection('Desconectado', false);
  }
}

function disconnect() {
  showError(null);
  if (device?.gatt?.connected) device.gatt.disconnect();
  else onDisconnected();
}

if (!navigator.bluetooth) {
  ui.compatibility.hidden = false;
  ui.compatibility.textContent = location.protocol === 'file:'
    ? 'Web Bluetooth no está disponible desde file:// en este navegador. Prueba un navegador compatible o sirve esta carpeta mediante localhost.'
    : 'Este navegador no ofrece Web Bluetooth. Usa un navegador compatible con Bluetooth habilitado.';
  ui.connect.disabled = true;
} else if (!window.isSecureContext) {
  ui.compatibility.hidden = false;
  ui.compatibility.textContent = 'Web Bluetooth requiere un contexto seguro. Si file:// está bloqueado, abre la página desde localhost.';
}

ui.connect.addEventListener('click', connect);
ui.disconnect.addEventListener('click', disconnect);
