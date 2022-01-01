#pragma once

#include "constant.h"
#include "CarDataClass.h"
#ifndef ARDUINO_ARCH_ESP32
#include <math.h>
#endif

class MotorSoundClass {
private:
  const CarDataClass& _carData;
  float _phaseLargeGear;  // 大歯車の回転角(0 to 1)
  float _phaseSmallGear;  // 小歯車の回転角(0 to 1)
  float _phaseEngage;  // 噛み合い周波数の位相角(0 to 1)
  int _volume;  // 音量(0-32767)

  /// @brief 簡単なsin波生成
  /// @param[in] phase 位相(0 to 2pi)
  /// @retval sin(phase). (-1 to 1)
  inline float sinRough(float phase) {
    if (phase < 0.0) {phase = PI - phase;}  // 0以下の場合は0以上に持ってくる
    while (phase > 2.0 * PI) {phase -= 2.0 * PI;}  // 0から2piの範囲に収める
    
    if (phase < 0.5 * PI) {  // 計算
      return minimaxSin(phase);
    } else if (phase < 1.5 * PI) {
      return minimaxSin(PI - phase);
    } else {
      return minimaxSin(phase - 2.0*PI);
    }
  }

  inline float minimaxSin(float phase) {
    // https://interface.cqpub.co.jp/wp-content/uploads/if08_148.pdf
    // HP内の x を 2Θ/π でおきかえて、-π/2 < Θ < π/2 に対応
    const float a1 = 1.0;
    const float a3 = -0.1666482838;
    const float a5 = 0.008306325184;
    return phase*(a1 + phase*phase*(a3 + phase*phase*a5));
    // const float a7 = -0.0001836365274;
    // return phase*(a1 + phase*phase*(a3 + phase*phase*(a5 + a7*phase*phase)));
  }

  /// @brief のこぎり波生成
  /// @param[in] phase 位相(0 to 1)
  /// @retval amp 波形の瞬時値. (-1 to 1)
  inline float sawTooth(const float phase) {
    return phase * 2.0 - 1.0;
    // return sin(phase) + 1.0/2.0*sin(2*phase) + 1.0/4.0*sin(3*phase) + 1.0/8.0*sin(4*phase);
  }

public:
  /// @brief モーター音生成クラス
  /// @param[in] carData 音を鳴らしたい車両の車両データ
  MotorSoundClass(const CarDataClass& carData);
  ~MotorSoundClass();

  /// @brief 現在の変数をクリアし波形計算を初期状態に戻す. carDataの参照先は変化しないので、
  ///        車両を変更したい場合にはCarData::setCarDataFromFile()を用いる
  void clear();

  /// @brief 音量を設定する
  /// @param[in] volume 音量(0-32767)
  /// @retval 1:success, 0:fail
  int setVolume(int volume);

  /// @brief ある速度における音データをsize[bytes]ぶん生成する
  /// @param[out] buf 生成したPCMデータが格納されるバッファへのポインタ
  /// @param[in] size 生成する音データのサイズ(bytes). サンプル数は size/4 個になる
  /// @param[in] speed 各サンプリング点における走行速度[km/h]を size/4 個ぶん格納した配列
  /// @retval 1:success, 0:fail
  int generateSound(uint8_t* buf, int size, float* speed);
};
