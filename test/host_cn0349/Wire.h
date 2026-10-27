#pragma once
#include "Arduino.h"
#include <vector>
class TwoWire {
 public:
  uint8_t regs[256]{};
  uint8_t mask=0x12, address=0, pointer=0;
  bool muxMismatch=false, dftTimeout=false, standbyFailure=false;
  unsigned transactions=0;
  std::vector<uint8_t> tx, rx;
  TwoWire() { regs[0x80]=0xB1; regs[0x81]=8; regs[0x8F]=6; setDft(600,800); }
  void setDft(int16_t r,int16_t i) { regs[0x94]=uint16_t(r)>>8; regs[0x95]=r&255; regs[0x96]=uint16_t(i)>>8; regs[0x97]=i&255; }
  void beginTransmission(uint8_t a) { address=a; tx.clear(); }
  size_t write(uint8_t b) { tx.push_back(b); return 1; }
  size_t write(const uint8_t* b,size_t n) { tx.insert(tx.end(),b,b+n); return n; }
  uint8_t endTransmission(bool=true) {
    ++transactions;
    if(address!=0x0D && address!=0x48) return 2;
    if(tx.empty()) return 0;
    if(address==0x48) { if(!muxMismatch) mask=tx[0]; return 0; }
    if(tx[0]==0xB0) { pointer=tx.at(1); return 0; }
    if(standbyFailure && tx[0]==0x80 && (tx.at(1)&0xF0)==0xB0) return 2;
    for(size_t i=1;i<tx.size();++i) regs[uint8_t(tx[0]+i-1)]=tx[i];
    return 0;
  }
  size_t requestFrom(uint8_t a,size_t n,bool) {
    ++transactions; rx.clear();
    if(a==0x48) rx.push_back(mask);
    else if(a==0x0D) rx.push_back(pointer==0x8F && dftTimeout ? 0 : regs[pointer]);
    return rx.size()<n?rx.size():n;
  }
  int available() { return static_cast<int>(rx.size()); }
  int read() { if(rx.empty()) return -1; int b=rx.front(); rx.erase(rx.begin()); return b; }
};
