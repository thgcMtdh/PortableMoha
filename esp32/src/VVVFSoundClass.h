#pragma once

#ifdef ARDUINO_ARCH_ESP32
#include <Arduino.h>
#else
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#endif

#include <ArduinoJson.h>

#include "constant.h"

class VVVFSoundClass {
 private:
  /* data */
 public:
  VVVFSoundClass(/* args */);
  ~VVVFSoundClass();

  /// @brief パルスモード等の車両データを設定する
  /// @param[in] carParamsStr 車両データ書式(carParamsFormat.md参照)に従ったJSON文字列
  /// @retval 1:success, 0:fail
  int setPulseMode(char* carParamsStr);

  /// @brief ある速度における音データをsize[bytes]ぶん生成する
  /// @param[out] buf   生成したPCMデータが格納されるバッファへのポインタ
  /// @param[in] size  生成する音データのサイズ(bytes). サンプル数は size/4 個になる
  /// @param[in] speed 各サンプリング点における走行速度[km/h]を size/4 個ぶん格納した配列
  /// @retval 1:success, 0:fail
  int generateSound(uint8_t* buf, int size, float* speed);
};
