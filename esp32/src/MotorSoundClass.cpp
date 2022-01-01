#include "MotorSoundClass.h"
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

  // size/4 個ぶんのサンプルを生成する
  for (size_t i = 0; i < size/4; i++) {
    // 各ギアの位相を計算
    float deltaPhaseLargeGear = speed[i] / 3.6 * T_SAMPLE / (PI * _carData._wheelDiameter);
    _phaseLargeGear += deltaPhaseLargeGear;
    _phaseSmallGear += deltaPhaseLargeGear * gr;
    _phaseEngage += deltaPhaseLargeGear * gr * _carData._smallGear;
    // 1以下にする
    _phaseLargeGear -= (int)_phaseLargeGear;
    _phaseSmallGear -= (int)_phaseSmallGear;
    _phaseEngage -= (int)_phaseEngage;

    // 振幅を計算
    float ampLargeGear = sin(2.0 * PI * _phaseLargeGear);
    float ampSmallGear = sin(2.0 * PI * _phaseSmallGear);
    float ampEngage = sin(2.0 * PI * _phaseEngage);
    float output = (ampLargeGear + ampSmallGear + ampEngage) * 1.0/3.0;

    // 出力先アドレスを出力バッファの適切な位置に指定
    int16_t* pResultL = reinterpret_cast<int16_t*>(&buf[4*i]);
    int16_t* pResultR = reinterpret_cast<int16_t*>(&buf[4*i+2]);

    // 出力
    *pResultL = output * _volume;
    *pResultR = *pResultL;
  }
  return 1;
}
