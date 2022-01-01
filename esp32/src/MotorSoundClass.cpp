#include "MotorSoundClass.h"

#include "Filter.h"

#ifndef ARDUINO_ARCH_ESP32
#include <math.h>
#endif

MotorSoundClass::MotorSoundClass(const CarDataClass& carData) : _carData(carData) {
  clear();
}

MotorSoundClass::~MotorSoundClass() {
}

void MotorSoundClass::clear() {
  _phaseLargeGear = 0.0;
  _phaseSmallGear = 0.0;
  _phaseEngage = 0.0;
  _volume = 0;
}

int MotorSoundClass::setVolume(int volume) {
  if (volume < 0 || volume > 32767) {
    return 0;
  }
  _volume = volume;
  return 1;
}

int MotorSoundClass::generateSound(uint8_t* buf, int size, float* speed) {
  float gr = _carData._largeGear / _carData._smallGear;
  FirstHPF firstHPF1, firstHPF2;
  firstHPF1.setTau(1.0/300.0);
  firstHPF2.setTau(1.0/300.0);
  FirstLPF firstLPF;
  firstLPF.setTau(1.0/2000.0);

  // size/4 個ぶんのサンプルを生成する
  for (size_t i = 0; i < size/4; i++) {
    // 各ギアの位相を計算
    float deltaPhaseLargeGear = speed[i] / 1.8 * T_SAMPLE / _carData._wheelDiameter;  // dx=rdΘよりdΘ=dx/r
    _phaseLargeGear += deltaPhaseLargeGear;
    _phaseSmallGear += deltaPhaseLargeGear * gr;
    _phaseEngage += deltaPhaseLargeGear * gr * _carData._smallGear;
    // 2π以下にする
    if (_phaseLargeGear > 2*PI) { _phaseLargeGear -= (2*PI);}
    if (_phaseSmallGear > 2*PI) { _phaseSmallGear -= (2*PI);}
    if (_phaseEngage > 2*PI) { _phaseEngage -= (2*PI);}
    // _phaseLargeGear -= (int)_phaseLargeGear;
    // _phaseSmallGear -= (int)_phaseSmallGear;
    // _phaseEngage -= (int)_phaseEngage;

    // 振幅を計算
    // float ampLargeGear = sin(_phaseLargeGear);
    float ampSmallGear = 0.0;
    // ampSmallGear += sinRough(1*_phaseSmallGear)/4;
    ampSmallGear += sinRough(-2*_phaseSmallGear)/2;
    ampSmallGear += sinRough(4*_phaseSmallGear)/2;
    ampSmallGear += sinRough(-6*_phaseSmallGear)/2;
    ampSmallGear += sinRough(8*_phaseSmallGear)/2;
    ampSmallGear += sinRough(10*_phaseSmallGear)/4;
    ampSmallGear += sinRough(-12*_phaseSmallGear)/6;
    ampSmallGear += sinRough(-14*_phaseSmallGear)/8;
    ampSmallGear += sinRough(16*_phaseSmallGear)/10;
    ampSmallGear += sinRough(-18*_phaseSmallGear)/12;
    ampSmallGear += sinRough(20*_phaseSmallGear)/14;
    float ampEngage = 0.0;
    ampEngage += sinRough(_phaseEngage);
    ampEngage += sinRough(2*_phaseEngage)/2;
    ampEngage += sinRough(3*_phaseEngage)/4;
    ampEngage += sinRough(4*_phaseEngage)/6;
    ampEngage += sinRough(5*_phaseEngage)/8;
    float output = (ampSmallGear + ampEngage)/2.0;
    output = firstHPF1.update(output, T_SAMPLE);
    output = firstHPF2.update(output, T_SAMPLE);
    output = firstLPF.update(output, T_SAMPLE);

    // 出力先アドレスを出力バッファの適切な位置に指定
    int16_t* pResultL = reinterpret_cast<int16_t*>(&buf[4*i]);
    int16_t* pResultR = reinterpret_cast<int16_t*>(&buf[4*i+2]);

    // 出力
    *pResultL = output * _volume;
    *pResultR = *pResultL;
  }
  return 1;
}
