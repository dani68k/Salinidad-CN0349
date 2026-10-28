#include <cmath>
#include <cfloat>
#include <iostream>
#include <stdexcept>
#include "../../src/cn0349/CN0349Manager.h"
#include "../../src/cn0349/ImpedanceMath.h"
#include "../../src/terminal/Terminal.h"
#define CHECK(x) do { if (!(x)) throw std::runtime_error(std::string(__FUNCTION__)+":"+std::to_string(__LINE__)+" " #x); } while(0)
using E = InstrumentError;
using S = MeasurementSensors;
using P = CN0349Preset;
struct Fixture {
  Stream serial; TwoWire wire; AD5934 ad{wire}; ADG715 muxDriver{wire};
  MuxApp mux{muxDriver,ad}; ImpedanceApp app{ad,mux}; CN0349Manager manager{app};
  Terminal terminal{serial,manager};
  void begin() { CHECK(manager.begin(true)==E::None); terminal.begin(); }
  void tick(uint32_t dt=10) { hostClock()+=dt; manager.update(); }
  void finish() { for(int i=0;i<4000 && manager.isBusy();++i) tick(); CHECK(!manager.isBusy()); }
  void ready() { begin(); finish(); CHECK(manager.getStatus().validCalibrations==4); terminal.update(); }
  void command(const std::string& text) { serial.input=text+"\n"; while(serial.available()) terminal.update(); }
  void cycle() { const auto before=manager.getStatus().publishedCycles;
    for(int i=0;i<4000 && manager.getStatus().publishedCycles==before;++i) tick();
    CHECK(manager.getStatus().publishedCycles==before+1);
  }
  MeasurementCycle result() { MeasurementCycle r; CHECK(manager.getMeasurementResult(r)); return r; }
  void standby() { CHECK((wire.regs[0x80]&0xf0)==0xb0 && wire.regs[0x81]==8); }
};
void mathTests() {
  float z=-1, phase=-1;
  CHECK(CN0349Math::apparentImpedance(500,0.00001f,-170,170,z,phase)); CHECK(fabs(z-200)<0.001f); CHECK(phase==20);
  CHECK(CN0349Math::apparentImpedance(500,0.00001f,170,-170,z,phase)); CHECK(phase==-20);
  CHECK(CN0349Math::apparentImpedance(500,0.00001f,180,0,z,phase)); CHECK(phase==180);
  for(float bad : {0.0f,-1.0f,NAN,INFINITY}) {
    CHECK(!CN0349Math::apparentImpedance(bad,1,0,0,z,phase));
    CHECK(!CN0349Math::apparentImpedance(1,bad,0,0,z,phase));
  }
  CHECK(!CN0349Math::apparentImpedance(1,1,NAN,0,z,phase));
  CHECK(!CN0349Math::apparentImpedance(FLT_MAX,FLT_MAX,0,0,z,phase));
  CHECK(!CN0349Math::apparentImpedance(FLT_MIN,FLT_MIN,0,0,z,phase));
  CHECK(CN0349Math::sameEffectiveFrequency(1999.99658203125f,1999.9964599609375f));
  CHECK(!CN0349Math::sameEffectiveFrequency(2000,2001));
  for(float ohms : {100.0f,500.0f,1000.0f,2000.0f,10000.0f}) {
    auto c=Conductivity::convert(ohms,1.0f,Conductivity::Model::Raw,100.0f);
    CHECK(c.valid && c.error==Conductivity::Error::None && c.cellResistanceOhms==ohms);
    CHECK(fabs(c.microSiemensPerCm-1.0e6f/ohms)<0.001f);
    CHECK(fabs(c.milliSiemensPerCm-1000.0f/ohms)<0.001f);
  }
  auto series=Conductivity::convert(600,1,Conductivity::Model::SeriesR2Experimental,100);
  CHECK(series.valid && series.cellResistanceOhms==500 && series.microSiemensPerCm==2000);
  CHECK(Conductivity::convert(100,1,Conductivity::Model::SeriesR2Experimental,100).error==Conductivity::Error::NonPositiveCellResistance);
  CHECK(Conductivity::convert(100,1,static_cast<Conductivity::Model>(99),100).error==Conductivity::Error::InvalidModel);
  for(float bad : {0.0f,-1.0f,NAN,INFINITY})
    CHECK(!Conductivity::convert(bad,1,Conductivity::Model::Raw,100).valid);
  for(float bad : {0.0f,-1.0f,NAN,INFINITY})
    CHECK(Conductivity::convert(100,bad,Conductivity::Model::Raw,100).error==Conductivity::Error::InvalidCellConstant);
  CHECK(Conductivity::convert(1.0e-38f,FLT_MAX,Conductivity::Model::Raw,100).error==Conductivity::Error::Overflow);
  auto ad=Conductivity::calibrateAnalogDevices(1000,10000,0.0001,0.001,1000,2000,1,1,42);
  CHECK(ad.valid && ad.error==Conductivity::AdCalibrationError::None);
  CHECK(fabs(ad.gainFactorSiemensPerCode-1e-7)<1e-15 && fabs(ad.systemOffsetCode)<1e-12);
  auto low=Conductivity::convertAnalogDevices(1000,1,100,ad,2000,1000,1,1);
  auto high=Conductivity::convertAnalogDevices(10000,1,100,ad,2000,1000,1,1);
  CHECK(low.valid && fabs(low.measuredAdmittanceSiemens-0.0001)<1e-12);
  CHECK(high.valid && fabs(high.measuredAdmittanceSiemens-0.001)<1e-12);
  auto synthetic=Conductivity::convertAnalogDevices(5000,1,100,ad,2000,1000,1,1);
  CHECK(synthetic.valid && fabs(synthetic.measuredAdmittanceSiemens-0.0005)<1e-12);
  CHECK(fabs(synthetic.cellAdmittanceSiemens-0.000526315789)<1e-12);
  CHECK(fabs(synthetic.microSiemensPerCm-526.315789)<0.0001 && synthetic.calibrationId==42);
  auto adHigh=Conductivity::calibrateAnalogDevices(1000,10000,0.001,0.01,100,2000,1,1,43);
  CHECK(adHigh.valid);
  CHECK(Conductivity::convertAnalogDevices(1000,1,100,adHigh,2000,100,1,1).valid);
  CHECK(Conductivity::convertAnalogDevices(10000,1,100,adHigh,2000,100,1,1).error==Conductivity::Error::InvalidDenominator);
  CHECK(Conductivity::convertAnalogDevices(11000,1,100,adHigh,2000,100,1,1).error==Conductivity::Error::InvalidDenominator);
  CHECK(!Conductivity::calibrateAnalogDevices(1,1,.0001,.001,1000,2000,1,1,1).valid);
  CHECK(!Conductivity::calibrateAnalogDevices(2,1,.0001,.001,1000,2000,1,1,1).valid);
  CHECK(!Conductivity::calibrateAnalogDevices(NAN,2,.0001,.001,1000,2000,1,1,1).valid);
  CHECK(Conductivity::convertAnalogDevices(-1,1,100,ad,2000,1000,1,1).error==Conductivity::Error::NonPositiveAdmittance);
  CHECK(Conductivity::convertAnalogDevices(NAN,1,100,ad,2000,1000,1,1).error==Conductivity::Error::InvalidMagnitude);
  CHECK(Conductivity::convertAnalogDevices(5000,0,100,ad,2000,1000,1,1).error==Conductivity::Error::InvalidCellConstant);
  CHECK(Conductivity::convertAnalogDevices(5000,-1,100,ad,2000,1000,1,1).error==Conductivity::Error::InvalidCellConstant);
  CHECK(Conductivity::convertAnalogDevices(5000,1,100,ad,2001,1000,1,1).error==Conductivity::Error::CalibrationMismatch);
  CHECK(Conductivity::convertAnalogDevices(5000,1,100,ad,2000,100,1,1).error==Conductivity::Error::CalibrationMismatch);
  Conductivity::AdCalibration absent;
  CHECK(Conductivity::convertAnalogDevices(5000,1,100,absent,2000,1000,1,1).error==Conductivity::Error::CalibrationInvalid);
  auto corrupt=ad; corrupt.gainFactorSiemensPerCode=-1;
  CHECK(Conductivity::convertAnalogDevices(5000,1,100,corrupt,2000,1000,1,1).error==Conductivity::Error::CalibrationInvalid);
  corrupt=ad; corrupt.gainFactorSiemensPerCode=INFINITY;
  CHECK(Conductivity::convertAnalogDevices(5000,1,100,corrupt,2000,1000,1,1).error==Conductivity::Error::CalibrationInvalid);
  corrupt=ad; corrupt.systemOffsetCode=NAN;
  CHECK(Conductivity::convertAnalogDevices(5000,1,100,corrupt,2000,1000,1,1).error==Conductivity::Error::CalibrationInvalid);
}


void startupTests() {
  Fixture f; CHECK(f.manager.getStatus().state==InstrumentState::NotInitialized);
  CHECK(f.manager.startSingleMeasurement()==E::Hardware); f.begin();
  CHECK(f.manager.getStatus().state==InstrumentState::Calibrating);
  CHECK(f.manager.startSingleMeasurement()==E::Busy);
  CHECK(f.manager.setMeasurementSensors(S::Ec)==E::Busy);
  CHECK(f.manager.runCalibration()==E::Busy);
  f.finish(); f.standby(); CHECK(f.manager.getStatus().calibrationRunId==1);
  CHECK(f.manager.getSetup().sensors==S::Both && f.manager.getSetup().intervalMs==1000);
  CHECK(f.manager.getSetup().ecCalibration==P::CalLow10K);
  for(uint8_t i=0;i<4;++i) {
    CN0349Calibration c; CHECK(f.manager.getCalibrationStatus(static_cast<P>(i),c));
    CHECK(c.valid && c.compatibleWithCurrentConfig && c.sampleCount==5 && c.calibrationId==i+1);
    CHECK(c.frequencyHz==2000 && c.mclkHz==1000000 && c.excitationRange==1 && c.pgaGain==1 && c.settlingCycles==10);
    CHECK(fabs(c.gainFactorPerOhm - 1/(c.meanMagnitude*c.rcalOhms))<1e-9);
  }
  CN0349Calibration invalid; CHECK(!f.manager.getCalibrationStatus(static_cast<P>(99),invalid));
  Conductivity::AdCalibration ad; CHECK(f.manager.getAdCalibrationStatus(CN0349EcRange::Low,ad));
  CHECK(ad.valid && fabs(ad.gainFactorSiemensPerCode-1e-7)<1e-15 && fabs(ad.systemOffsetCode)<1e-9);
  CHECK(f.manager.getAdCalibrationStatus(CN0349EcRange::High,ad));
  CHECK(ad.valid && fabs(ad.gainFactorSiemensPerCode-1e-5)<1e-13 && fabs(ad.systemOffsetCode)<1e-9);
  f.terminal.update(); CHECK(f.serial.output=="ATLANTES CN0349\nCAL OK 4/4\nAD LOW OK\nAD HIGH OK\nREADY\n");
  Fixture missing; missing.wire.adAbsent=true; CHECK(missing.manager.begin(true)==E::Hardware);
  CHECK(!missing.manager.isBusy()); missing.terminal.begin(); CHECK(missing.serial.output=="ATLANTES CN0349\nERR HARDWARE\n");
  Fixture running; running.ready(); CHECK(running.manager.startSingleMeasurement()==E::None);
  for(int i=0;i<35;++i) running.tick();
  Fixture reboot; reboot.wire=running.wire; reboot.ready(); reboot.standby();
  CHECK(reboot.manager.getSetup().sensors==S::Both && reboot.manager.getSetup().intervalMs==1000);
  CHECK(reboot.manager.getStatus().publishedCycles==0);
  Fixture mux; mux.wire.muxAbsent=true; CHECK(mux.manager.begin(true)==E::Hardware);
  Fixture bus; CHECK(bus.manager.begin(false)==E::Hardware);
}
void calibrationTests() {
  Fixture f; f.wire.zeroDftMask=0x09; f.begin(); f.finish();
  CHECK(f.manager.getStatus().validCalibrations==3); f.standby();
  CN0349Calibration c; CHECK(f.manager.getCalibrationStatus(P::CalHigh100R,c));
  CHECK(!c.valid && c.error==E::MeasurementFailed && c.state==CalibrationState::Invalid);
  CHECK(!f.manager.getStatus().adHighValid && f.manager.getStatus().adLowValid);
  CHECK(f.manager.startSingleMeasurement()==E::NotCalibrated);
  CHECK(f.manager.setMeasurementSensors(S::Ec)==E::None);
  CHECK(f.manager.startSingleMeasurement()==E::None); f.finish(); CHECK(f.result().valid);
  f.terminal.update(); CHECK(f.serial.output.find("CAL PARTIAL 3/4")!=std::string::npos);
  f.wire.zeroDftMask=0xff; CHECK(f.manager.runCalibration()==E::None); f.finish();
  CHECK(f.manager.getStatus().validCalibrations==4);
  const auto oldId=c.calibrationId;
  f.wire.timeoutMask=0x09; CHECK(f.manager.runCalibration()==E::None); f.finish();
  f.manager.getCalibrationStatus(P::CalHigh100R,c);
  CHECK(!c.valid && c.calibrationId>oldId && c.error==E::Timeout);
  CHECK(f.manager.getStatus().validCalibrations==3); f.standby();
  f.wire.timeoutMask=0xff; CHECK(f.manager.runCalibration()==E::None); f.finish();
  CHECK(f.manager.getStatus().validCalibrations==4);
  CHECK(f.manager.startContinuousMeasurement()==E::None); CHECK(f.manager.runCalibration()==E::None);
  CHECK(!f.manager.getStatus().continuous); f.finish(); f.standby();
  CHECK(f.manager.getStatus().validCalibrations==4 && !f.manager.getStatus().continuous);
  const auto cycles=f.manager.getStatus().publishedCycles; for(int i=0;i<200;++i) f.tick();
  CHECK(f.manager.getStatus().publishedCycles==cycles);
}
void selectionAndCycleTests() {
  for(S sensors : {S::Pt100,S::Ec,S::Both}) {
    Fixture f; f.ready(); CHECK(f.manager.setMeasurementSensors(sensors)==E::None);
    f.wire.routes.clear(); CHECK(f.manager.startSingleMeasurement()==E::None);
    CHECK(f.manager.startSingleMeasurement()==E::Busy); f.finish(); f.standby();
    auto r=f.result(); CHECK(r.valid && r.cycleId==1 && r.setup.sensors==sensors && r.finishedMs-r.startedMs>0);
    if(sensors!=S::Ec) { CHECK(r.pt100.valid && r.pt100.acquisition.measurementMask==0x41);
      CHECK(fabs(r.pt100.temperature.celsius)<0.001f); CHECK(r.pt100.acquisition.real==600 && r.pt100.acquisition.imag==800); }
    if(sensors!=S::Pt100) { CHECK(r.ec.valid && r.ec.acquisition.measurementMask==0x82);
      CHECK(fabs(r.ec.conductivity.microSiemensPerCm-100)<0.01f); }
    if(sensors==S::Both) { CHECK(f.wire.routes.size()==2 && f.wire.routes[0]==0x41 && f.wire.routes[1]==0x82); }
    CHECK(f.manager.takeMeasurementResult(r)); CHECK(!f.manager.takeMeasurementResult(r));
    const auto preserved=r;
    auto setup=f.manager.getSetup(); setup.nominalKPerCm=2; setup.ecRange=CN0349EcRange::High; setup.ecCalibration=P::CalHigh1K;
    CHECK(f.manager.setSetup(setup)==E::None);
    CHECK(f.manager.runCalibration()==E::None); f.finish();
    r=f.result(); CHECK(r.cycleId==preserved.cycleId && r.ec.acquisition.gainFactor==preserved.ec.acquisition.gainFactor);
    CHECK(r.setup.nominalKPerCm==1);
    CHECK(f.manager.startSingleMeasurement()==E::None); f.finish();
    r=f.result(); CHECK(r.cycleId==2); if(sensors!=S::Pt100) CHECK(r.ec.acquisition.measurementMask==0x81 && r.ec.acquisition.calibrationPreset==P::CalHigh1K);
  }
  Fixture compatibility; compatibility.ready();
  CHECK(compatibility.app.configureSingle(MuxApp::CalibrationPreset::CalHigh100R,2001,10));
  CHECK(compatibility.manager.startSingleMeasurement()==E::NotCalibrated);
  CHECK(compatibility.app.configureSingle(MuxApp::CalibrationPreset::CalHigh100R,2000,11));
  CHECK(compatibility.manager.startSingleMeasurement()==E::NotCalibrated);
  CHECK(compatibility.manager.runCalibration()==E::None); compatibility.finish();
  CHECK(compatibility.manager.startSingleMeasurement()==E::None); compatibility.finish(); CHECK(compatibility.result().valid);
  Fixture f; f.ready(); auto setup=f.manager.getSetup();
  setup.sensors=static_cast<S>(0); CHECK(f.manager.setSetup(setup)==E::InvalidArgument);
  setup=f.manager.getSetup(); setup.nominalKPerCm=NAN; CHECK(f.manager.setSetup(setup)==E::InvalidArgument);
  setup=f.manager.getSetup(); setup.ecModel=Conductivity::Model::SeriesR2Experimental; CHECK(f.manager.setSetup(setup)==E::None);
  setup=f.manager.getSetup(); setup.ecModel=static_cast<Conductivity::Model>(99); CHECK(f.manager.setSetup(setup)==E::InvalidArgument);
  setup=f.manager.getSetup(); setup.ecCalibration=P::CalHigh100R; CHECK(f.manager.setSetup(setup)==E::InvalidArgument);
}
void analogDevicesIntegrationTests() {
  Fixture f; f.ready(); auto setup=f.manager.getSetup(); setup.ecModel=Conductivity::Model::AnalogDevices;
  setup.sensors=S::Both; CHECK(f.manager.setSetup(setup)==E::None);
  CHECK(f.manager.startSingleMeasurement()==E::None); f.finish(); auto r=f.result();
  CHECK(r.valid && r.pt100.valid && r.ec.valid && r.ec.conductivity.model==Conductivity::Model::AnalogDevices);
  CHECK(r.ec.acquisition.apparentImpedanceOhms==0 && r.ec.conductivity.calibrationId!=0);
  CHECK(fabs(r.ec.conductivity.measuredAdmittanceSiemens-0.0001)<1e-12);
  CHECK(fabs(r.ec.conductivity.cellAdmittanceSiemens-0.000101010101)<1e-12);
  CHECK(fabs(r.ec.conductivity.microSiemensPerCm-101.010101)<0.0001);
  const auto first=r.ec.conductivity;
  CHECK(f.manager.runCalibration()==E::None); f.finish();
  r=f.result(); CHECK(r.ec.conductivity.calibrationId==first.calibrationId);
  setup=f.manager.getSetup(); setup.sensors=S::Ec; setup.ecRange=CN0349EcRange::High;
  setup.ecCalibration=P::CalHigh1K; setup.nominalKPerCm=2; CHECK(f.manager.setSetup(setup)==E::None);
  f.wire.setDft(30,40); CHECK(f.manager.startSingleMeasurement()==E::None); f.finish(); r=f.result();
  CHECK(r.valid && fabs(r.ec.conductivity.measuredAdmittanceSiemens-0.0005)<1e-12);
  CHECK(fabs(r.ec.conductivity.microSiemensPerCm-1052.631578)<0.001);
  CHECK(f.manager.startContinuousMeasurement()==E::None); f.cycle(); CHECK(f.result().valid);
  CHECK(f.manager.stopMeasurement()==E::None); f.finish();
  Fixture invalid; invalid.wire.zeroDftMask=0x12; invalid.begin(); invalid.finish();
  setup=invalid.manager.getSetup(); setup.sensors=S::Ec; setup.ecModel=Conductivity::Model::AnalogDevices;
  CHECK(invalid.manager.setSetup(setup)==E::None);
  CHECK(invalid.manager.startSingleMeasurement()==E::NotCalibrated);
}
void continuousTests() {
  for(S sensors : {S::Pt100,S::Ec,S::Both}) {
    Fixture f; f.ready(); CHECK(f.manager.setMeasurementSensors(sensors)==E::None);
    hostClock()=0xfffffff0UL; const uint32_t start=millis();
    CHECK(f.manager.startContinuousMeasurement()==E::None); f.cycle();
    CHECK(f.result().startedMs==start); CHECK(f.manager.setMeasurementSensors(S::Ec)==E::Busy);
    CHECK(f.manager.setSetup(f.manager.getSetup())==E::Busy);
    const auto n=f.manager.getStatus().publishedCycles;
    hostClock()=start+999; f.manager.update(); CHECK(f.manager.getStatus().state==InstrumentState::Idle);
    hostClock()=start+1000; f.manager.update(); CHECK(f.manager.getStatus().state==InstrumentState::Measuring);
    CHECK(f.manager.getStatus().publishedCycles==n); f.cycle(); CHECK(f.result().startedMs==uint32_t(start+1000));
    CHECK(f.manager.stopMeasurement()==E::None); f.finish(); f.standby();
    const auto tx=f.wire.transactions; for(int i=0;i<500;++i) f.tick(); CHECK(f.wire.transactions==tx);
  }
  Fixture f; f.ready(); auto setup=f.manager.getSetup(); setup.intervalMs=1; CHECK(f.manager.setSetup(setup)==E::None);
  CHECK(f.manager.startContinuousMeasurement()==E::None); f.cycle(); CHECK(f.result().intervalExceeded);
  CHECK(f.manager.getStatus().intervalExceededCount==1); const auto finish=millis(); f.tick();
  f.cycle(); CHECK(f.result().startedMs==finish+10); CHECK(f.manager.getStatus().overwrittenResults==1);
  CHECK(f.manager.stopMeasurement()==E::None); f.finish();
  // Sustained run exercises repeated transitions and result consumption.
  Fixture longRun; longRun.ready(); setup=longRun.manager.getSetup(); setup.intervalMs=1;
  CHECK(longRun.manager.setSetup(setup)==E::None); CHECK(longRun.manager.startContinuousMeasurement()==E::None);
  for(int i=0;i<1000;++i) { longRun.cycle(); MeasurementCycle r; CHECK(longRun.manager.takeMeasurementResult(r)); CHECK(r.valid && r.cycleId==uint32_t(i+1)); }
  CHECK(longRun.manager.getStatus().overwrittenResults==0); CHECK(longRun.manager.stopMeasurement()==E::None); longRun.finish();
}
void stopAndFailureTests() {
  for(S selection : {S::Pt100,S::Ec}) {
    Fixture f; f.ready(); CHECK(f.manager.setMeasurementSensors(selection)==E::None);
    CHECK(f.manager.startContinuousMeasurement()==E::None); f.tick();
    CHECK(f.manager.stopMeasurement()==E::None); f.finish(); f.standby();
    const auto r=f.result(); CHECK(r.cancelled && !r.valid);
    const auto sensor=selection==S::Pt100 ? r.pt100 : r.ec;
    CHECK(sensor.acquisition.state==CN0349ImpedanceState::Cancelled && sensor.error==E::Cancelled);
  }
  for(uint8_t mask : {0x41,0x82}) {
    Fixture f; f.ready(); CHECK(f.manager.startContinuousMeasurement()==E::None);
    for(int i=0;i<200 && (f.wire.mask!=mask || f.app.measurementState()!=ImpedanceApp::MeasurementState::Acquiring);++i) f.tick();
    CHECK(f.wire.mask==mask); CHECK(f.manager.stopMeasurement()==E::None);
    f.finish(); f.standby(); CHECK(!f.result().valid && f.result().cancelled);
    CHECK(f.manager.startSingleMeasurement()==E::None); f.finish(); CHECK(f.result().valid);
  }
  for(uint8_t mask : {0x41,0x82}) {
    Fixture f; f.ready(); f.wire.timeoutMask=mask; CHECK(f.manager.startSingleMeasurement()==E::None); f.finish();
    auto r=f.result(); CHECK(!r.valid && r.error==E::Timeout);
    if(mask==0x41) CHECK(!r.pt100.valid && r.ec.valid); else CHECK(r.pt100.valid && !r.ec.valid);
    f.standby(); f.wire.timeoutMask=0xff;
    // Manual calibration recovers a failed final acquisition's degraded state.
    CHECK(f.manager.runCalibration()==E::None); f.finish();
    CHECK(f.manager.startSingleMeasurement()==E::None); f.finish(); CHECK(f.result().valid);
  }
  Fixture pending; pending.ready(); pending.command("measure start");
  pending.command("cal run"); pending.wire.adAbsent=true;
  pending.finish(); pending.serial.output.clear(); pending.terminal.update();
  CHECK(pending.manager.getStatus().validCalibrations==0);
  CHECK(pending.serial.output.find("ERR HARDWARE")!=std::string::npos);
  Fixture zero; zero.ready(); zero.wire.zeroDftMask=0x82;
  CHECK(zero.manager.startSingleMeasurement()==E::None); zero.finish();
  CHECK(zero.result().pt100.valid && !zero.result().ec.valid && !zero.result().valid);
  Fixture range; range.ready(); range.wire.setDft(6,8);
  CHECK(range.manager.startSingleMeasurement()==E::None); range.finish();
  CHECK(!range.result().pt100.valid && range.result().pt100.acquisition.valid && range.result().ec.valid);
  Fixture mux; mux.ready(); mux.wire.muxMismatch=true;
  CHECK(mux.manager.startSingleMeasurement()==E::None); mux.finish(); CHECK(!mux.result().valid); mux.standby();
  Fixture standby; standby.ready(); CHECK(standby.manager.startSingleMeasurement()==E::None);
  standby.wire.standbyFailure=true; standby.finish(); CHECK(!standby.result().valid && !standby.manager.getStatus().hardwareReady);
  Fixture bus; bus.ready(); CHECK(bus.manager.startSingleMeasurement()==E::None); bus.wire.busError=true;
  bus.finish(); CHECK(!bus.result().valid); bus.wire.busError=false;
  CHECK(bus.manager.runCalibration()==E::None); bus.finish(); CHECK(bus.manager.getStatus().validCalibrations==4);
}
void terminalTests() {
  Fixture f; f.ready(); f.serial.output.clear();
  const auto tx=f.wire.transactions;
  for(const auto command : {"help","status","setup show","cal status","cal show","measure show"}) f.command(command);
  CHECK(f.wire.transactions==tx);
  CHECK(f.serial.output.find("LEGACY HIGH_100R | GF ")!=std::string::npos);
  CHECK(f.serial.output.find("AD LOW | NL ")!=std::string::npos);
  CHECK(f.serial.output.find("S/code | NOS ")!=std::string::npos);
  f.command("  MeAsUrE SET ec PT100  "); CHECK(f.manager.getSetup().sensors==S::Both);
  for(const auto bad : {"measure set", "measure set ec ec", "measure set pt100 bad", "measure set ec pt100 ec", "setup sensor ec", "setup interval -1", "setup interval 0", "setup interval 2147483648", "setup interval 1x", "setup ec k nan", "setup ec k inf", "setup ec k 1x", "setup ec model invalid", "ad5934 measure start", "cn0349 temperature start"}) {
    f.serial.output.clear(); f.command(bad); CHECK(f.serial.output=="ERR INVALID_ARGUMENT\n");
  }
  f.command("measure set EC"); CHECK(f.manager.getSetup().sensors==S::Ec);
  f.command("measure set PT100 EC"); CHECK(f.manager.getSetup().sensors==S::Both);
  f.command("measure set pt100"); CHECK(f.manager.getSetup().sensors==S::Pt100); f.command("measure once");
  f.command("setup interval 2000"); CHECK(f.serial.output.find("ERR BUSY")!=std::string::npos);
  f.command("status"); CHECK(f.manager.isBusy()); f.finish(); f.serial.output.clear(); f.terminal.update();
  CHECK(f.serial.output.find("PT100 | ")!=std::string::npos && f.serial.output.find(" | OK")!=std::string::npos);
  CHECK(f.serial.output.find("REAL")==std::string::npos && f.serial.output.find("DFT")==std::string::npos);
  f.command("setup ec range high"); CHECK(f.manager.getSetup().ecCalibration==P::CalHigh100R);
  f.command("setup ec k 2.5"); CHECK(f.manager.getSetup().nominalKPerCm==2.5);
  f.command("measure set ec"); f.command("measure start"); f.command("measure stop");
  f.finish(); f.terminal.update(); CHECK(f.serial.output.find("OK STOPPED")!=std::string::npos);
  f.command("setup ec range low"); f.command("measure set pt100 ec"); f.wire.timeoutMask=0x82; f.command("measure once"); f.finish();
  f.serial.output.clear(); f.terminal.update(); CHECK(f.serial.output.find("PT100 | ")!=std::string::npos);
  CHECK(f.serial.output.find("EC | RAW | ERR TIMEOUT")!=std::string::npos && f.serial.output.find(" | OK")==std::string::npos);
  f.serial.output.clear(); f.command(std::string(100,'a')); CHECK(f.serial.output=="ERR INVALID_ARGUMENT\n");
  f.command("status"); CHECK(f.serial.output.find("CAL 4/4")!=std::string::npos);
  f.wire.timeoutMask=0xff; f.command("cal run"); f.finish(); f.terminal.update();
  CHECK(f.manager.getStatus().validCalibrations==4);
  f.serial.output.clear(); f.serial.input="status\r\n"; f.terminal.update();
  CHECK(f.serial.output.find("CAL 4/4")!=std::string::npos && f.serial.output.find("ERR")==std::string::npos);
}
int main() {
  try {
    mathTests(); std::cout<<"math PASS\n";
    startupTests(); std::cout<<"startup PASS\n";
    calibrationTests(); std::cout<<"calibration PASS\n";
    selectionAndCycleTests(); std::cout<<"selection/cycles PASS\n";
    analogDevicesIntegrationTests(); std::cout<<"analog devices PASS\n";
    continuousTests(); std::cout<<"continuous/timing PASS\n";
    stopAndFailureTests(); std::cout<<"stop/errors PASS\n";
    terminalTests(); std::cout<<"terminal PASS\n";
  } catch(const std::exception& e) { std::cerr<<e.what()<<"\n"; return 1; }
}
