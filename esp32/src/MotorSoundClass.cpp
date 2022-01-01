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
  _isEngagementPlay = false;
}

int MotorSoundClass::setVolume(int volume) {
  if (volume < 0 || volume > 32767) {
    return 0;
  }
  _volume = volume;
  return 1;
}

int MotorSoundClass::setEngagementPlay(bool isPlay) {
  _isEngagementPlay = isPlay;
  return 0;
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
    // 各ギアの回転数を計算
    float rpsLargeGear = speed[i] / 3.6 / PI / _carData._wheelDiameter;  // v=rω=2πrfよりf=v/2πr=v/πΦ
    float rpsSmallGear = rpsLargeGear * gr;
    float rpsEngage = rpsSmallGear * _carData._smallGear;

    // 各ギアの位相を計算 
    _phaseLargeGear += rpsLargeGear * 2 * PI * T_SAMPLE;
    _phaseSmallGear += rpsSmallGear * 2 * PI * T_SAMPLE;
    _phaseEngage += rpsEngage * 2 * PI * T_SAMPLE;
    // 2π以下にする
    if (_phaseLargeGear > 2*PI) { _phaseLargeGear -= (2*PI);}
    if (_phaseSmallGear > 2*PI) { _phaseSmallGear -= (2*PI);}
    if (_phaseEngage > 2*PI) { _phaseEngage -= (2*PI);}
    // _phaseLargeGear -= (int)_phaseLargeGear;
    // _phaseSmallGear -= (int)_phaseSmallGear;
    // _phaseEngage -= (int)_phaseEngage;

    // 振幅を計算
    float ampSmallGear = (rpsSmallGear < 30) ? 0.0 : (rpsSmallGear - 30) / 30;
    if (ampSmallGear > 1.0) ampSmallGear = 1.0;
    float ampEngage = (rpsEngage < 30) ? 0.0 : (rpsEngage - 30) / 300;
    if (ampEngage > 1.0) ampEngage = 1.0;

    // 瞬時値を計算
    // float ampLargeGear = sin(_phaseLargeGear);
    float valSmallGear = 0.0;
    // valSmallGear += sinRough(1*_phaseSmallGear)/4;
    // valSmallGear += sinRough(2*_phaseSmallGear)/2;
    valSmallGear += sinRough(4*_phaseSmallGear)/2;
    // valSmallGear += sinRough(6*_phaseSmallGear)/2;
    valSmallGear += sinRough(8*_phaseSmallGear)/2;
    // valSmallGear += sinRough(10*_phaseSmallGear)/4;
    valSmallGear += sinRough(12*_phaseSmallGear)/3;
    // valSmallGear += sinRough(14*_phaseSmallGear)/8;
    valSmallGear += sinRough(16*_phaseSmallGear)/5;
    // valSmallGear += sinRough(18*_phaseSmallGear)/12;
    valSmallGear += sinRough(20*_phaseSmallGear)/7;
    // valSmallGear += sinRough(22*_phaseSmallGear)/12;
    valSmallGear += sinRough(24*_phaseSmallGear)/9;
    float valEngage = 0.0;
    if (_isEngagementPlay) {
      // valEngage += sinRough(_phaseEngage);
      // valEngage += sinRough(2*_phaseEngage)/2;
      // valEngage += sinRough(3*_phaseEngage)/4;
      // valEngage += sinRough(4*_phaseEngage)/6;
      // valEngage += sinRough(5*_phaseEngage)/8;
      valEngage += sinRough(_phaseEngage);
      valEngage += sinRough(2*_phaseEngage)/2;
      valEngage += sinRough(3*_phaseEngage);
      valEngage += sinRough(5*_phaseEngage);
      // valEngage += sinRough(7*_phaseEngage);
    }
    float output = (valSmallGear * ampSmallGear + valEngage * ampEngage)/2.0;
    output = firstHPF1.update(output, T_SAMPLE);
    // output = firstHPF2.update(output, T_SAMPLE);
    output = firstLPF.update(output, T_SAMPLE);

    // 出力先アドレスを出力バッファの適切な位置に指定
    int16_t* pResultL = reinterpret_cast<int16_t*>(&buf[4*i]);
    int16_t* pResultR = reinterpret_cast<int16_t*>(&buf[4*i+2]);

    // 出力
    *pResultL = output * _volume;
    *pResultR = output * _volume;
  }
  return 1;
}
