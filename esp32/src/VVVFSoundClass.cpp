#include "VVVFSoundClass.h"

#ifndef ARDUINO_ARCH_ESP32
#include <math.h>
#endif
#include "Filter.h"

VVVFSoundClass::VVVFSoundClass(const CarDataClass& carData) : _carData(carData) {
  clear();
}
VVVFSoundClass::~VVVFSoundClass() {}

void VVVFSoundClass::clear() {
  _pmIndex[0] = 0;

  _Vs = 0.0;
  _phaseSin[0] = 0.0;  _phaseSin[1] = 0.0;  _phaseSin[2] = 0.0;
  _ampSin[0] = 0.0;  _ampSin[1] = 0.0;  _ampSin[2] = 0.0;

  _fc = 0.0;
  _frand = 0.0;
  _fdeviation = 0.0;
  _phaseCarrier = 1.001;  // 初回のcalcAsyncTriangleでfcを更新するために、1より大きい値に初期化しておく
  _ampCarrier = 0.0;

  _invPhaseV[0] = 0;  _invPhaseV[1] = 0;  _invPhaseV[2] = 0;
  _invLineV[0] = 0;  _invLineV[1] = 0;  _invLineV[2] = 0;
  _firstLPF0.clear(0.0);
  _firstLPF1.clear(0.0);
  _volume = 0;
}

int VVVFSoundClass::setVolume(int volume) {
  if (volume < 0 || volume > 32767) {
    return 0;
  }
  _volume = volume;
  return 1;
}

int VVVFSoundClass::setCutoffFreq(float fcutoff) {
  if (fcutoff <= 0.0) {
    return 0;
  }
  _firstLPF0.setTau(1.0 / fcutoff);
  _firstLPF1.setTau(1.0 / fcutoff);
  return 1;
}

int VVVFSoundClass::generateSound(uint8_t* buf, int size, float* speed) {
  // speed から fs へ換算する係数
  float coeffSpdToFs = 1.0/3.6 / (PI*_carData._wheelDiameter) * (_carData._largeGear/_carData._smallGear) * _carData._pole/2;

  // size/4個分のサンプルを生成する
  for (size_t i = 0; i < size / 4; i++) {
    float fs = speed[i] * coeffSpdToFs + 2.0;  // すべり周波数として2.0を付加

    // 信号波位相を計算
    _phaseSin[2] += fs * T_SAMPLE;  // 位相をサンプリング時間分進める
    _phaseSin[1] = _phaseSin[2] + 1.0/3.0;
    _phaseSin[0] = _phaseSin[2] + 2.0/3.0;
    _phaseSin[0] -= (int)_phaseSin[0];  // 整数部を引いて0から1に収める
    _phaseSin[1] -= (int)_phaseSin[1];
    _phaseSin[2] -= (int)_phaseSin[2];

    // 信号波電圧を計算
    if (fs > _carData._modulationMaxFreq) {
      _Vs = _carData._modulationMax;  // 最大電圧に達する周波数を超えている場合
    } else {
      _Vs = 0.03 + 0.97 * _carData._modulationMax * fs / _carData._modulationMaxFreq;  // V/f一定で上昇.ブーストを3%とる
    }

    // 現在の周波数におけるパルスモードを取得
	  size_t pmIndexRef = getPulsemodeIndex(fs);

    // U,V,Wの各相について、パルスモードを変更
    for (size_t i_p = 0; i_p < 3; i_p++) {
		
      // 現在のパルスモードが、現在の周波数において適用されるべきパルスモードと異なるとき(変更が必要)
      if (_pmIndex[i_p] != pmIndexRef) {
        // async -> async で変化するとき
        if (_carData._listMode[_pmIndex[i_p]] == ASYNC && _carData._listMode[pmIndexRef] == ASYNC) {
          _pmIndex[i_p] = pmIndexRef;  // すぐにパルスモードを変更
        // syncが絡むとき
        } else {
          // if (_phaseSin[i_p] < 1.0/360.0) {  // 信号波位相が0に近いときに限り変更
            _pmIndex[i_p] = pmIndexRef;
          // }
        }
      }
    }

    // 非同期の相が1つ以上ある場合、非同期キャリアを計算
    for (size_t i_p = 0; i_p < 3; i_p++) {
      if (_carData._listMode[_pmIndex[i_p]] == ASYNC) {
        calcAsyncTriangle(fs, _pmIndex[i_p]);
        break;
      }
    }

    // 各相について、非同期または同期PWMを行う
    for (size_t i_p = 0; i_p < 3; i_p++) {
      switch (_carData._listMode[_pmIndex[i_p]]) {
      case ASYNC:  // 非同期PWM
        asyncPWM(i_p);  break;
      case SYNC:
        syncPWM(i_p);  break;
      
      default:
        break;
      }
    }

    // printf("fs:%f, phaseSin[0]:%f, Vs:%f, pmIndex[0]:%d, fc:%f, phaseCarrier:%f, ampCarrier:%f\n", fs[i], _phaseSin[0], _Vs, _pmIndex[0], _fc, _phaseCarrier, _ampCarrier);

    // 出力先アドレスを出力バッファの適切な位置に指定
    int16_t* pResultL = reinterpret_cast<int16_t*>(&buf[4*i]);
    int16_t* pResultR = reinterpret_cast<int16_t*>(&buf[4*i+2]);
    // 線間電圧を計算
    _invLineV[0] = _invPhaseV[0] - _invPhaseV[1];
    _invLineV[1] = _invPhaseV[1] - _invPhaseV[2];
    // _invLineV[2] = _invPhaseV[2] - _invPhaseV[0];
    // 出力(LPFを通さない方が、ジョイント音と合わせた際に綺麗)
    *pResultL = _invLineV[0] * _volume;  //_firstLPF0.update(_invLineV[0] * _volume, T_SAMPLE);
    *pResultR = _invLineV[1] * _volume;  // _firstLPF1.update(_invLineV[1] * _volume, T_SAMPLE);
  }
  return 1;
}

/// @brief 周波数fsに対応するパルスモードを取得する
/// @param[in] fs 信号波周波数[Hz]
/// @retval パルスモードのインデックス(0,1,...,_pmNum-1)
size_t VVVFSoundClass::getPulsemodeIndex(const float fs) {
  int i = 0;
  for (i = _carData._pmNum-1; i>=0; i--) {  // search _carData._listMode[_pmIndex[i_p]] index
    if (fs >= _carData._listFs[i]) break;
  }
  return i;
}

/// @brief 非同期キャリア波形を計算する
/// @param[in] fs 信号波周波数[Hz]
/// @retval None (VVVFSoundClass の _fc, _frand, _phaseCarrier, _ampCarrier が更新される)
inline void VVVFSoundClass::calcAsyncTriangle(const float fs, const size_t pmIndex) {

  // サンプリング時刻分位相を進める
  _phaseCarrier += _fc * T_SAMPLE;

  // 1周を超えたとき
  if (_phaseCarrier > 1.0) {
    _phaseCarrier -= 1.0;  // 位相は0-1のあいだなので戻す

    // キャリア周波数を再計算
    float fs1 = _carData._listFs[pmIndex];  // 一次関数の左端と右端
    float fs2 = (pmIndex==_carData._pmNum-1)? fs1 + 1.0 : _carData._listFs[pmIndex+1];
    _fc = _carData._listFc1[pmIndex] + (_carData._listFc2[pmIndex] - _carData._listFc1[pmIndex]) * (fs - fs1) / (fs2 - fs1);
    _frand = _carData._listFrand1[pmIndex] + (_carData._listFrand2[pmIndex] - _carData._listFrand1[pmIndex]) * (fs - fs1) / (fs2 - fs1);
    _fc += _frand * ((float)rand()/RAND_MAX * 2.0 - 1.0);  // ずれ幅をランダムに更新
  }

  // 瞬時値を計算
  if (_phaseCarrier < 0.5) {
    _ampCarrier = 4.0 * _phaseCarrier - 1.0;
  } else {
    _ampCarrier =  -4.0 * _phaseCarrier + 3.0;
  }
}

/// @brief 1相ぶんの非同期PWMを行う
/// @param[in] i_phase 相番号. 0,1,2のどれか
/// @retval None (VVVFSoundClass の_invPhaseV に出力される)
inline void VVVFSoundClass::asyncPWM(const size_t i_phase) {
  _invPhaseV[i_phase] = (_Vs * sin(2 * PI * _phaseSin[i_phase]) >= _ampCarrier);
}

/// @brief 同期PWMに用いるキャリア波形を計算する
/// @param[in] phaseSin 信号波の位相
/// @param[in] Npulse パルス数
/// @retval ampSyncCarrier キャリア波形の瞬時値
inline void VVVFSoundClass::calcSyncTriangle(const float phaseSin, const int Npulse) {
  _phaseCarrier = phaseSin * Npulse;
  _phaseCarrier -= (int)_phaseCarrier;
  if (_phaseCarrier < 0.25) {
    _ampCarrier = -4.0 * _phaseCarrier;
  } else if (_phaseCarrier < 0.75) {
    _ampCarrier = -2.0 + 4.0 * _phaseCarrier;
  } else {
    _ampCarrier = 4.0 - 4.0 * _phaseCarrier;
  }
}

inline void VVVFSoundClass::calcSyncNot3xPTriangle(const float phaseSin, const int Npulse) {
  switch (Npulse) {
  case 5:
    _phaseCarrier = 3.0*phaseSin;
    _phaseCarrier -= (int)_phaseCarrier;
    if (_phaseCarrier < 1.0/8.0) {
      _ampCarrier = -8.0 * _phaseCarrier;
    } else if (_phaseCarrier < 2.0/8.0) {
      _ampCarrier = 8.0 * _phaseCarrier - 2.0;
    } else if (_phaseCarrier < 3.0/8.0) {
      _ampCarrier = -8.0 * _phaseCarrier + 2.0;
    } else if (_phaseCarrier < 5.0/8.0) {
      _ampCarrier = 8.0 * _phaseCarrier - 4.0;
    } else if (_phaseCarrier < 6.0/8.0) {
      _ampCarrier = -8.0 * _phaseCarrier + 6.0;
    } else if (_phaseCarrier < 7.0/8.0) {
      _ampCarrier = 8.0 * _phaseCarrier - 6.0;
    } else {
      _ampCarrier = -8.0 * _phaseCarrier + 8.0;
    }
    break;
  default:
    break;
  }
}

/// @brief 1相ぶんの正弦波同期PWMを行う
/// @param[in] i_phase 相番号. 0,1,2のどれか
/// @retval None (VVVFSoundClass の_invPhaseV に出力される)
inline void VVVFSoundClass::syncPWM(const size_t i_phase) {
  int Npulse = _carData._listNpulse[_pmIndex[i_phase]];
  if ((Npulse / 3) * 3 != Npulse) {
    calcSyncNot3xPTriangle(_phaseSin[0], Npulse);
  } else {
    calcSyncTriangle(_phaseSin[0], Npulse);
  }
  _invPhaseV[i_phase] = (_Vs * sin(2 * PI * _phaseSin[i_phase]) >= _ampCarrier);

}
