#pragma once

#ifdef ARDUINO_ARCH_ESP32
#include <Arduino.h>
#else
#include <stdint.h>
#include <stdio.h>
#endif

#include "constant.h"
#include "CarDataClass.h"
#include "Filter.h"

class VVVFSoundClass {
 private:
  const CarDataClass& _carData;

  size_t _pmIndex[3];  // 各相が何番目のパルスモードにいるか

  float _Vs;  // モータ電圧(0 to 1. 全電圧1パルスモードのとき1)
  float _phaseSin[3];  // 各相の信号波位相
  float _ampSin[3];  // 各相の信号波瞬時値(-1 to 1)

  float _fc;  // 非同期キャリア周波数(ランダム変調の場合はその中心). この値は同期モードでは意味を持たない
  float _frand;  // ランダム変調幅. この値は同期モードでは意味を持たない
  float _fdeviation;  // ランダムに決定される、キャリア周波数の中心からのずれ
  float _phaseCarrier;  // 非同期搬送波の位相
  float _ampCarrier;  // 非同期搬送波の瞬時値(-1 to 1)

  bool _invPhaseV[3];  // 各相の相電圧出力. 0または1
  int _invLineV[3];  // 各線間電圧出力. -1,0,1 のどれか. 順番はU-V, V-W, W-Uの順
  FirstLPF _firstLPF0;  // U-V線間のLPF
  FirstLPF _firstLPF1;  // V-W線間のLPF
  int _volume;  // 再生時の音量(0-32767)
  
  size_t getPulsemodeIndex(const float fs);
  inline void calcAsyncTriangle(const float fs, const size_t pmIndex);
  inline void asyncPWM(const size_t i_phase);
  inline void calcSyncTriangle(const float phaseSin, const int Npulse);
  inline void calcSyncNot3xPTriangle(const float phaseSin, const int Npulse);
  inline void syncPWM(const size_t i_phase);

 public:
  /// @brief VVVF音生成クラス
  /// @param[in] carData 音を鳴らしたい車両の車両データ
  VVVFSoundClass(const CarDataClass& carData);
  ~VVVFSoundClass();

  /// @brief 現在の変数をクリアし波形計算を初期状態に戻す. carDataの参照先は変化しないので、
  ///        車両を変更したい場合にはCarData::setCarDataFromFile()を用いる
  void clear();

  /// @brief 音量を設定する
  /// @param[in] volume 音量(0-32767)
  /// @retval 1:success, 0:fail
  int setVolume(int volume);

  /// @brief モーター音のカットオフ周波数を指定する
  /// @param[in] fcutoff カットオフ周波数[Hz]
  /// @retval 1:success, 0:fail
  int setCutoffFreq(float fcutoff);

  /// @brief ある速度における音データをsize[bytes]ぶん生成する
  /// @param[out] buf 生成したPCMデータが格納されるバッファへのポインタ
  /// @param[in] size 生成する音データのサイズ(bytes). サンプル数は size/4 個になる
  /// @param[in] fs 各サンプリング点における信号波周波数[Hz]を size/4 個ぶん格納した配列
  /// @retval 1:success, 0:fail
  int generateSound(uint8_t* buf, int size, float* fs);
};
