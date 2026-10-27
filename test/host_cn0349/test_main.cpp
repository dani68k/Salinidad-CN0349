#include <cmath>
#include <cfloat>
#include <iostream>
#include <stdexcept>
#include "../../src/cn0349/CN0349Manager.h"
#include "../../src/cn0349/ImpedanceMath.h"
#include "../../src/terminal/Terminal.h"
#define CHECK(x) do { if(!(x)) throw std::runtime_error(#x); } while(0)
using E=CN0349ImpedanceError;
using S=CN0349ImpedanceState;
using P=CN0349Preset;
struct Fixture {
  Stream serial; TwoWire wire; AD5934 ad{wire}; ADG715 muxDriver{wire};
  MuxApp mux{muxDriver,ad,serial}; ImpedanceApp app{ad,mux,serial};
  Terminal terminal{serial,wire}; CN0349Manager manager{app,serial};
  Fixture() { terminal.begin(true); app.begin(terminal); mux.begin(terminal); CHECK(manager.begin(terminal,true)); CHECK(manager.runDiagnostics()); }
  void tick() { hostClock()+=10; manager.update(); }
  void finish() { for(int i=0;i<2000 && manager.isBusy();++i) tick(); CHECK(!manager.isBusy()); }
  void command(const std::string& text) { serial.input=text+"\n"; while(serial.available()) terminal.update(); }
  void calibrate(P p) { CHECK(manager.startCalibration(p)==CN0349CalibrationError::None); finish(); CHECK(manager.calibrationState()==CN0349CalibrationState::Complete); }
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
}

void conductivityTests() {
  Fixture f;
  f.command("cn0349 conductivity model list");
  f.command("cn0349 conductivity model set SERIES_R2_EXPERIMENTAL");
  CHECK(f.manager.conductivityModel()==Conductivity::Model::Raw);
  CHECK(f.serial.output.find("topology not physically verified")!=std::string::npos);
  CHECK(f.manager.startConductivity()==E::InvalidCalibration);
  for(int i=0;i<4;++i) f.calibrate(static_cast<P>(i));
  CHECK(f.manager.selectEcRange(CN0349EcRange::Low)==E::None);
  CHECK(f.manager.selectEcCalibration(P::CalLow1K)==E::None);
  f.command("cn0349 conductivity config");
  f.command("cn0349 conductivity start");
  CHECK(f.manager.isEcRunning());
  CHECK(!f.manager.selectConductivityModel(Conductivity::Model::Raw));
  f.finish();
  auto result=f.manager.getConductivityResult();
  CHECK(result.valid && result.ec.acquisition.valid && result.ec.standbyVerified);
  CHECK(result.ec.acquisition.measurementMask==0x82);
  CHECK(result.ec.acquisition.calibrationPreset==P::CalLow1K);
  CHECK(result.conversion.model==Conductivity::Model::Raw);
  CHECK(result.conversion.nominalKPerCm==1.0f && result.conversion.r2Ohms==100.0f);
  CHECK(fabs(result.conversion.cellResistanceOhms-result.ec.acquisition.apparentImpedanceOhms)<0.001f);
  CHECK(fabs(result.conversion.microSiemensPerCm-1.0e6f/result.conversion.cellResistanceOhms)<0.01f);
  f.command("cn0349 conductivity result");
  CHECK(f.serial.output.find("CONDUCTIVITY - EXPERIMENTAL / NOT CALIBRATED")!=std::string::npos);
  CHECK(f.serial.output.find("REAL/IMAG/DFT magnitude")!=std::string::npos);
  CHECK(f.serial.output.find("Estimated conductivity uS/cm")!=std::string::npos);
  CHECK(f.manager.selectEcRange(CN0349EcRange::High)==E::None);
  CHECK(f.manager.getConductivityResult().ec.acquisition.measurementMask==0x82);
  f.wire.setDft(0,0);
  CHECK(f.manager.startConductivity()==E::IncompatibleCalibration);
  CHECK(f.manager.getConductivityResult().ec.acquisition.measurementMask==0x82);
  CHECK(f.manager.selectEcCalibration(P::CalHigh100R)==E::None);
  CHECK(f.manager.startConductivity()==E::None); f.finish();
  result=f.manager.getConductivityResult();
  CHECK(!result.valid && !result.ec.acquisition.valid);
  CHECK(result.conversion.nominalKPerCm==1.0f && result.conversion.r2Ohms==100.0f);
  f.wire.setDft(600,800);
  CHECK(f.manager.startConductivity()==E::None);
  CHECK(f.manager.cancelConductivity()); f.finish();
  CHECK(!f.manager.getConductivityResult().valid);
}
void measurementTests() {
  Fixture f;
  CHECK(f.manager.getImpedanceResult().state==S::Idle);
  f.command("i2c scan"); f.command("cn0349 impedance start"); CHECK(f.manager.impedanceRequestError()==E::Busy);
  for(int i=0;i<120;++i) f.terminal.update();
  CHECK(f.manager.startImpedance()==E::InvalidCalibration);
  CHECK(f.manager.selectImpedanceCalibration(static_cast<P>(99))==E::InvalidCalibration);
  CHECK(f.manager.selectImpedanceCalibration(P::CalLow1K)==E::IncompatibleCalibration);
  for(int i=0;i<4;++i) f.calibrate(static_cast<P>(i));
  CN0349Calibration before[4]; for(int i=0;i<4;++i) CHECK(f.manager.getCalibration(static_cast<P>(i),before[i]));
  for(P p : {P::CalHigh100R,P::CalHigh1K}) {
    CHECK(f.manager.selectImpedanceCalibration(p)==E::None);
    CHECK(f.manager.startImpedance()==E::None); CHECK(f.wire.mask==0x41);
    CHECK(f.manager.getStatus().activeOperation==CN0349Operation::Impedance);
    CHECK(f.manager.startImpedance()==E::Busy); CHECK(!f.manager.startSingle());
    CHECK(!f.manager.clearCalibrations()); CHECK(f.manager.selectImpedanceCalibration(p)==E::Busy);
    f.finish(); auto r=f.manager.getImpedanceResult(); CHECK(r.valid && r.state==S::Complete);
    CHECK(r.real==600 && r.imag==800 && r.magnitude==1000);
    CHECK(fabs(r.apparentImpedanceOhms-(p==P::CalHigh100R?100:1000))<0.01f);
    CHECK(f.wire.regs[0x80]==0xB1 && f.wire.regs[0x81]==8);
  }
  CHECK(f.manager.configureSingle({P::CalLow1K,3000,10})); CHECK(f.manager.startImpedance()==E::InvalidFrequency);
  CHECK(f.manager.configureSingle({P::CalLow1K,2000,11})); CHECK(f.manager.startImpedance()==E::IncompatibleCalibration);
  CHECK(f.manager.configureSingle({P::CalLow1K,2000,10}));
  CHECK(f.manager.startImpedance()==E::None); CHECK(f.manager.cancelImpedance()); f.finish();
  CHECK(f.manager.getImpedanceResult().state==S::Cancelled);
  CN0349ImpedanceResult saved; CHECK(f.manager.getLastValidImpedanceResult(saved) && saved.valid);
  CHECK(f.manager.startImpedance()==E::None); f.command("ad5934 measure stop"); f.finish();
  CHECK(f.manager.getImpedanceResult().state==S::Cancelled);
  f.wire.setDft(0,0); CHECK(f.manager.startImpedance()==E::None); f.finish();
  CHECK(f.manager.getImpedanceResult().error==E::InvalidData); CHECK(f.manager.getLastValidImpedanceResult(saved));
  f.wire.setDft(600,800); f.wire.dftTimeout=true;
  CHECK(f.manager.startImpedance()==E::None); f.finish(); CHECK(f.manager.getImpedanceResult().state==S::Failed);
  CHECK(f.wire.regs[0x80]==0xB1); f.wire.dftTimeout=false; CHECK(f.manager.recover());
  CHECK(f.manager.startSingle()); f.finish(); CN0349RawResult raw; CHECK(f.manager.getSingleResult(raw));
  CHECK(f.manager.startSweep()); f.finish(); CN0349SweepResult sweep; CHECK(f.manager.getSweepResult(sweep)); CHECK(sweep.pointsValid==9);
  CHECK(f.manager.runDiagnostics());
  for(int i=0;i<4;++i) { CN0349Calibration c; CHECK(f.manager.getCalibration(static_cast<P>(i),c)); CHECK(c.calibrationId==before[i].calibrationId && c.gainFactorPerOhm==before[i].gainFactorPerOhm); }
  for(const char* c : {"cn0349 impedance help","cn0349 impedance config","cn0349 impedance cal list","cn0349 impedance cal set CAL_HIGH_100R","cn0349 impedance cal set CAL_HIGH_1K","cn0349 impedance status","cn0349 impedance result"}) f.command(c);
  f.command("cn0349 impedance cal set nonexistent"); CHECK(f.manager.impedanceRequestError()==E::InvalidCalibration);
  f.command("cn0349 impedance cal set CAL_LOW_1K"); CHECK(f.manager.impedanceRequestError()==E::IncompatibleCalibration);
  unsigned transactions=f.wire.transactions; f.command("cn0349 impedance status"); f.command("cn0349 impedance config"); f.command("cn0349 impedance result"); CHECK(transactions==f.wire.transactions);
  f.command("cn0349 impedance start"); CHECK(f.manager.isBusy()); f.command("cn0349 impedance stop"); f.finish(); CHECK(f.manager.getImpedanceResult().state==S::Cancelled);
}
void failureTests() {
  Fixture f; f.calibrate(P::CalHigh100R); f.wire.muxMismatch=true;
  CHECK(f.manager.startImpedance()==E::AcquisitionFailed); f.finish(); CHECK(f.manager.getImpedanceResult().state==S::Failed); CHECK(f.wire.regs[0x80]==0xB1);
  f.wire.muxMismatch=false; CHECK(f.manager.recover()); CHECK(f.manager.startImpedance()==E::None);
  f.wire.standbyFailure=true; f.finish(); CHECK(!f.manager.getImpedanceResult().valid); CHECK(f.manager.getImpedanceResult().state==S::Failed);
}
void temperatureTests() {
  Fixture f;
  CHECK(!f.manager.getTemperatureResult().valid);
  CHECK(f.manager.startTemperature()==E::InvalidCalibration);
  CHECK(f.manager.getTemperatureResult().acquisition.error==E::InvalidCalibration);
  CHECK(f.manager.getTemperatureResult().conversion.error==Pt100::Error::NotAttempted);
  f.calibrate(P::CalHigh100R); f.calibrate(P::CalHigh1K);
  CHECK(f.manager.startTemperature()==E::None);
  CHECK(f.manager.getTemperatureResult().acquisition.state==S::Running);
  CHECK(f.manager.startTemperature()==E::Busy);
  f.finish(); auto first=f.manager.getTemperatureResult();
  CHECK(first.valid && fabs(first.conversion.celsius)<0.001);
  CHECK(first.acquisition.real==600 && first.acquisition.imag==800);
  CHECK(!first.cableCompensationApplied);
  CHECK(f.manager.selectImpedanceCalibration(P::CalHigh1K)==E::None);
  // Selecting/recalibrating/using impedance must never reinterpret saved temperature.
  CHECK(f.manager.getTemperatureResult().acquisition.calibrationId==first.acquisition.calibrationId);
  CHECK(f.manager.startImpedance()==E::None); f.finish();
  CHECK(f.manager.getTemperatureResult().conversion.celsius==first.conversion.celsius);
  f.command("cn0349 temperature start"); f.finish();
  auto outside=f.manager.getTemperatureResult();
  CHECK(outside.acquisition.valid && !outside.valid);
  CHECK(outside.acquisition.calibrationPreset==P::CalHigh1K);
  CHECK(outside.acquisition.calibrationId!=first.acquisition.calibrationId);
  CHECK(outside.conversion.error==Pt100::Error::OutOfRange);
  CHECK(f.manager.selectImpedanceCalibration(P::CalHigh100R)==E::None);
  CHECK(f.manager.startTemperature()==E::None); f.command("cn0349 temperature stop"); f.finish();
  CHECK(f.manager.getTemperatureResult().acquisition.state==S::Cancelled);
  CHECK(!f.manager.getTemperatureResult().valid);
  f.wire.dftTimeout=true; CHECK(f.manager.startTemperature()==E::None); f.finish();
  CHECK(f.manager.getTemperatureResult().acquisition.error==E::AcquisitionFailed);
  CHECK(f.manager.getTemperatureResult().conversion.error==Pt100::Error::NotAttempted);
  CHECK(!f.manager.getTemperatureResult().valid);
  f.wire.dftTimeout=false; CHECK(f.manager.recover());
  f.wire.mask=0x09; f.wire.muxMismatch=true; CHECK(f.manager.startTemperature()==E::AcquisitionFailed); f.finish();
  CHECK(!f.manager.getTemperatureResult().valid);
  CHECK(f.manager.getTemperatureResult().acquisition.hardwareError!=ImpedanceDiagnostics::ErrorCode::None);
  f.wire.muxMismatch=false; CHECK(f.manager.recover());
  CHECK(f.manager.startTemperature()==E::None); f.finish();
  auto saved=f.manager.getTemperatureResult(); CHECK(saved.valid);
  f.calibrate(P::CalHigh100R);
  CHECK(f.manager.getTemperatureResult().acquisition.calibrationId==saved.acquisition.calibrationId);
  CHECK(f.manager.getTemperatureResult().acquisition.gainFactor==saved.acquisition.gainFactor);
  unsigned transactions=f.wire.transactions;
  f.command("cn0349 temperature help"); f.command("cn0349 temperature status"); f.command("cn0349 temperature result");
  CHECK(transactions==f.wire.transactions);
  CHECK(f.serial.output.find("EXPERIMENTAL / NOT ABSOLUTELY CALIBRATED")!=std::string::npos);
  // Defense against inconsistent acquisition flags: never convert on upstream failure.
  auto bad=saved.acquisition; bad.valid=false; CHECK(!convertPt100Acquisition(bad).valid);
  bad=saved.acquisition; bad.state=S::Failed; CHECK(!convertPt100Acquisition(bad).valid);
  bad=saved.acquisition; bad.error=E::InvalidCalibration; CHECK(!convertPt100Acquisition(bad).valid);
  bad=saved.acquisition; bad.hardwareError=ImpedanceDiagnostics::ErrorCode::I2cNack;
  CHECK(!convertPt100Acquisition(bad).valid);
}
void startupTests() {
  Fixture f;
  f.wire.regs[0x80]=0;  // Recovery must restore a usable profile before calibration.
  CHECK(f.manager.prepareMeasurements());
  CHECK(f.manager.isCalibrationRunning() && f.manager.isBusy());
  CHECK(!f.manager.prepareMeasurements());
  CHECK(f.manager.startTemperature()==E::Busy);
  CHECK(f.serial.output.find("Sistema preparado para medir.")==std::string::npos);
  f.finish();
  CHECK(f.manager.calibrationState()==CN0349CalibrationState::Complete);
  for(int i=0;i<4;++i) {
    CN0349Calibration c; CHECK(f.manager.getCalibration(static_cast<P>(i),c));
    CHECK(c.valid && c.sampleCount==5 && c.frequencyHz==2000);
    CHECK(c.calibrationId==static_cast<unsigned>(i+1));
  }
  CHECK(f.serial.output.find("Sistema preparado para medir.")!=std::string::npos);
  CHECK(f.manager.impedanceCalibration()==P::CalHigh100R);
  CHECK(f.manager.startTemperature()==E::None); f.finish(); CHECK(f.manager.getTemperatureResult().valid);
  Fixture cancelled; CHECK(cancelled.manager.prepareMeasurements());
  CHECK(cancelled.manager.cancelCalibration()); cancelled.finish();
  CHECK(cancelled.manager.calibrationState()==CN0349CalibrationState::Cancelled);
  CHECK(cancelled.serial.output.find("Sistema preparado para medir.")==std::string::npos);
  Fixture failed; CHECK(failed.manager.prepareMeasurements()); failed.wire.dftTimeout=true; failed.finish();
  CHECK(failed.manager.calibrationState()==CN0349CalibrationState::Failed);
  CHECK(failed.serial.output.find("Sistema preparado para medir.")==std::string::npos);
  Fixture recovery; recovery.wire.regs[0x80]=0x21; recovery.wire.standbyFailure=true;
  CHECK(!recovery.manager.prepareMeasurements());
  CHECK(recovery.manager.calibrationState()==CN0349CalibrationState::Idle);
  CHECK(!recovery.manager.isCalibrationRunning());
  CHECK(recovery.serial.output.find("Sistema preparado para medir.")==std::string::npos);
  Fixture manual; manual.command("cn0349 cal run all"); manual.finish();
  for(int i=0;i<4;++i) { CN0349Calibration c; CHECK(manual.manager.getCalibration(static_cast<P>(i),c)); }
}
void continuousTemperatureTests() {
  Fixture f;
  CHECK(f.manager.startContinuousTemperature()==E::InvalidCalibration);
  CHECK(!f.manager.isContinuousTemperature());
  f.calibrate(P::CalHigh100R);
  // Use exact 1 ms ticks to verify start-to-start timing across millis rollover.
  hostClock()=UINT32_MAX-500;
  const uint32_t started=hostClock();
  f.serial.output.clear();
  f.command("cn0349 temperature continuous");
  CHECK(f.manager.isContinuousTemperature()); CHECK(f.manager.isBusy());
  CHECK(f.manager.startContinuousTemperature()==E::Busy);
  CHECK(f.manager.selectImpedanceCalibration(P::CalHigh1K)==E::Busy);
  for(int i=0;i<999;++i) { ++hostClock(); f.manager.update(); }
  CHECK(static_cast<uint32_t>(hostClock()-started)==999);
  CHECK(f.manager.getTemperatureResult().valid);
  CHECK(f.serial.output.find("temperature: ")!=std::string::npos);
  CHECK(f.serial.output.find("\xC2\xBA" "C\n")!=std::string::npos);
  for(const char* verbose : {"ADG715 ya confirma", "Solicitado RTD", "Adquisicion DFT unica iniciada", "Single Measurement", "REAL", "Z_ohm", "Pt100 impedance:", "finalizado en standby"})
    CHECK(f.serial.output.find(verbose)==std::string::npos);
  CHECK(f.manager.getTemperatureResult().acquisition.state==S::Complete);
  const unsigned beforeScan=f.wire.transactions;
  f.command("i2c scan"); CHECK(f.wire.transactions==beforeScan);
  ++hostClock(); f.manager.update();
  CHECK(f.manager.getTemperatureResult().acquisition.state==S::Running);
  f.serial.output.clear();
  f.command("cn0349 temperature result");
  CHECK(f.serial.output.find("REAL/IMAG:")!=std::string::npos);
  f.command("cn0349 temperature stop"); f.finish();
  CHECK(!f.manager.isContinuousTemperature());
  CHECK(f.manager.getTemperatureResult().acquisition.state==S::Cancelled);
  auto snapshot=f.manager.getTemperatureResult(); unsigned transactions=f.wire.transactions;
  hostClock()+=5000; f.manager.update(); CHECK(transactions==f.wire.transactions);
  CHECK(f.manager.getTemperatureResult().acquisition.state==snapshot.acquisition.state);
  CHECK(f.manager.startContinuousTemperature()==E::None);
  // Finish first sample without waiting for the continuous bus reservation.
  for(int i=0;i<200 && f.manager.getTemperatureResult().acquisition.state==S::Running;++i) f.tick();
  CHECK(f.manager.getTemperatureResult().valid);
  CHECK(f.manager.cancelTemperature()); CHECK(!f.manager.isBusy());
  CHECK(f.manager.getTemperatureResult().valid);  // stop in idle gap preserves sample
  CHECK(f.manager.startContinuousTemperature()==E::None);
  f.wire.dftTimeout=true;
  f.finish(); CHECK(!f.manager.isContinuousTemperature()); CHECK(!f.manager.getTemperatureResult().valid);
  CHECK(f.manager.getTemperatureResult().acquisition.error==E::AcquisitionFailed);
  f.wire.dftTimeout=false; CHECK(f.manager.recover());
  CHECK(f.manager.startContinuousTemperature()==E::None); CHECK(f.manager.cancel()); f.finish();
  CHECK(!f.manager.isContinuousTemperature());
  CHECK(f.manager.recover());
  f.serial.output.clear();
  CHECK(f.manager.startTemperature()==E::None); f.finish();
  CHECK(f.serial.output.find("Single Measurement")!=std::string::npos);
}
void ecTests() {
  Fixture f;
  using R=CN0349EcRange;
  CHECK(f.manager.getEcResult().acquisition.state==S::Idle);
  CHECK(f.manager.startEc()==E::InvalidCalibration);
  CHECK(f.manager.selectEcCalibration(P::CalLow1K)==E::IncompatibleCalibration);
  CHECK(f.manager.selectEcRange(static_cast<R>(99))==E::InvalidData);
  CHECK(f.manager.selectEcRange(R::Low)==E::None);
  CHECK(f.manager.startEc()==E::IncompatibleCalibration);
  CHECK(f.manager.selectEcCalibration(P::CalLow1K)==E::None);
  CHECK(f.manager.startEc()==E::InvalidCalibration);
  CHECK(f.manager.selectEcCalibration(static_cast<P>(99))==E::InvalidCalibration);
  for(int i=0;i<4;++i) f.calibrate(static_cast<P>(i));
  CN0349EcResult first{};
  for(R range : {R::High,R::Low}) {
    CHECK(f.manager.selectEcRange(range)==E::None);
    const P presets[2] = {range==R::High?P::CalHigh100R:P::CalLow1K,
                          range==R::High?P::CalHigh1K:P::CalLow10K};
    for(P preset : presets) {
      CHECK(f.manager.selectEcCalibration(preset)==E::None);
      CHECK(f.manager.startEc()==E::None);
      const uint8_t expected=range==R::High?0x81:0x82;
      CHECK(f.wire.mask==expected);
      CHECK(f.manager.getStatus().activeOperation==CN0349Operation::Ec);
      CHECK(f.manager.getStatus().state==CN0349State::RunningEc);
      CHECK(f.manager.startEc()==E::Busy);
      CHECK(f.manager.selectEcRange(range)==E::Busy);
      CHECK(!f.manager.startSingle());
      f.finish();
      auto r=f.manager.getEcResult();
      CHECK(r.acquisition.valid && r.acquisition.state==S::Complete);
      CHECK(r.ad5934FinalState==ImpedanceApp::MeasurementState::Complete);
      CHECK(r.standbyVerified && !r.saturationKnown);
      CHECK(r.range==range && r.acquisition.measurementMask==expected);
      CHECK(r.rfbOhms==(range==R::High?100:1000));
      CHECK(r.acquisition.calibrationPreset==preset && r.acquisition.calibrationId>0);
      CHECK(r.acquisition.real==600 && r.acquisition.imag==800 && r.acquisition.magnitude==1000);
      CHECK(fabs(r.acquisition.apparentImpedanceOhms-(preset==P::CalHigh100R?100:preset==P::CalHigh1K?1000:preset==P::CalLow1K?1000:10000))<0.1f);
      if(range==R::High && preset==P::CalHigh100R) first=r;
      CHECK(f.wire.regs[0x80]==0xB1 && f.wire.regs[0x81]==8);
    }
  }
  CHECK(f.manager.selectEcRange(R::High)==E::None);
  CHECK(f.manager.getEcResult().range==R::Low);
  CHECK(f.manager.getEcResult().acquisition.calibrationPreset==P::CalLow10K);
  CHECK(f.manager.selectEcCalibration(P::CalHigh100R)==E::None);
  CHECK(f.manager.getEcResult().acquisition.calibrationPreset==P::CalLow10K);
  f.command("cn0349 ec range list"); f.command("cn0349 ec config");
  f.command("cn0349 ec cal list"); f.command("cn0349 ec help");
  f.command("cn0349 ec status"); f.command("cn0349 ec result");
  CHECK(f.serial.output.find("EC PROBE - EXPERIMENTAL / NOT ABSOLUTELY CALIBRATED")!=std::string::npos);
  CHECK(f.serial.output.find("Saturation not reliably detectable")!=std::string::npos);
  CHECK(f.serial.output.find("AD5934 final state/standby verified: COMPLETE / yes")!=std::string::npos);
  f.command("cn0349 ec range set LOW"); CHECK(f.manager.ecRange()==R::Low);
  f.command("cn0349 ec cal set CAL_HIGH_100R");
  CHECK(f.manager.ecRequestError()==E::IncompatibleCalibration);
  f.command("cn0349 ec cal set CAL_LOW_1K");
  CHECK(f.manager.ecRequestError()==E::None && f.manager.ecCalibration()==P::CalLow1K);
  f.command("cn0349 ec range set HIGH");
  CHECK(f.manager.ecRange()==R::High);
  CHECK(f.manager.selectEcCalibration(P::CalHigh100R)==E::None);
  CHECK(f.manager.configureSingle({P::CalLow1K,3000,10})); CHECK(f.manager.startEc()==E::InvalidFrequency);
  CHECK(f.manager.configureSingle({P::CalLow1K,2000,11})); CHECK(f.manager.startEc()==E::IncompatibleCalibration);
  CHECK(f.manager.configureSingle({P::CalLow1K,2000,10}));
  f.command("cn0349 ec start"); CHECK(f.manager.isEcRunning());
  f.command("cn0349 ec stop"); f.finish();
  CHECK(f.manager.getEcResult().acquisition.state==S::Cancelled);
  CN0349EcResult saved; CHECK(f.manager.getLastValidEcResult(saved));
  CHECK(saved.acquisition.calibrationPreset==P::CalLow10K);
  f.wire.setDft(0,0); CHECK(f.manager.startEc()==E::None); f.finish();
  CHECK(f.manager.getEcResult().acquisition.error==E::InvalidData);
  CHECK(f.manager.getLastValidEcResult(saved));
  f.wire.setDft(600,800); f.wire.dftTimeout=true;
  CHECK(f.manager.startEc()==E::None); f.finish();
  CHECK(f.manager.getEcResult().acquisition.state==S::Failed && !f.manager.getEcResult().acquisition.valid);
  f.wire.dftTimeout=false; CHECK(f.manager.recover());
  f.wire.mask=0x09; f.wire.muxMismatch=true;
  CHECK(f.manager.startEc()==E::AcquisitionFailed); f.finish();
  CHECK(!f.manager.getEcResult().acquisition.valid);
  f.wire.muxMismatch=false; CHECK(f.manager.recover());
  CHECK(f.manager.startEc()==E::None); f.finish(); CHECK(f.manager.getEcResult().acquisition.valid);
  f.wire.regs[0x80]=0x21; f.wire.standbyFailure=true;
  CHECK(f.manager.startEc()==E::AcquisitionFailed); f.finish();
  CHECK(!f.manager.getEcResult().acquisition.valid && !f.manager.getEcResult().standbyVerified);
  f.wire.standbyFailure=false; CHECK(f.manager.recover());
  CHECK(f.manager.selectImpedanceCalibration(P::CalHigh100R)==E::None);
  CHECK(f.manager.startTemperature()==E::None); f.finish();
  auto temp=f.manager.getTemperatureResult(); CHECK(temp.valid);
  CHECK(f.manager.startEc()==E::None); f.finish();
  CHECK(f.manager.getTemperatureResult().acquisition.calibrationId==temp.acquisition.calibrationId);
  CHECK(f.manager.getTemperatureResult().conversion.celsius==temp.conversion.celsius);
  CHECK(f.manager.getImpedanceResult().measurementMask==0x41);
  CHECK(first.acquisition.measurementMask==0x81);
  unsigned transactions=f.wire.transactions;
  f.command("cn0349 ec result"); f.command("cn0349 ec status");
  CHECK(transactions==f.wire.transactions);
}
int main() {
  try { mathTests(); std::cout<<"math: PASS\n"; measurementTests(); std::cout<<"manager/terminal/regression: PASS\n"; failureTests(); std::cout<<"readback/standby failures: PASS\n"; temperatureTests(); std::cout<<"temperature/provenance/errors/terminal: PASS\n"; startupTests(); std::cout<<"startup/recovery/all-calibrations: PASS\n"; continuousTemperatureTests(); std::cout<<"continuous temperature/timing/stop/errors: PASS\n"; ecTests(); std::cout<<"EC routing/calibration/errors/RTD regression: PASS\n"; conductivityTests(); std::cout<<"conductivity RAW/model/snapshot/errors: PASS\n"; }
  catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<"\n"; return 1; }
}
