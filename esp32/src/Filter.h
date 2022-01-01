#pragma once

class FirstLPF {
  private:
  float _tau;   // 1次遅れ時定数[s]
  float _x[2];  // 現在およびひとつ前のx. 0が最新、1がひとつ前
  float _y[2];  // 現在およびひとつ前のy

  public:
  FirstLPF() : _tau(0.0) {
    _x[0] = 0.0; _x[1] = 0.0; _y[0] = 0.0; _y[1] = 0.0;
  }
  ~FirstLPF() {};
  
  /// @brief 一次遅れ時定数を設定
  /// @param[in] tau 一次遅れ時定数[s]
  void setTau(const float tau) {
    _tau = tau;
  }

  /// @brief 内部変数をリセット
  /// @param[in] init xとyをリセットする初期値
  void clear(const float init) {
    _x[0] = init; _x[1] = init; _y[0] = init; _y[1] = init;
  }

  /// @brief 入力から出力を計算
  /// @param[in] x 入力
  /// @param[in] dt 直前の入力からの経過時間[s]
  /// @retval y 出力
  inline float update(const float x, const float dt) {
    _x[1] = _x[0];
    _y[1] = _y[0];
    _x[0] = x;
    _y[0] = 1.0/(2.0*_tau + dt) * ((2.0*_tau - dt) * _y[1] + dt * (_x[0] + _x[1]));
    return _y[0];
  }
};

class FirstHPF {
  private:
  float _tau;   // 1次進み時定数[s]
  float _x[2];  // 現在およびひとつ前のx. 0が最新、1がひとつ前
  float _y[2];  // 現在およびひとつ前のy

  public:
  FirstHPF() : _tau(0.0) {
    _x[0] = 0.0; _x[1] = 0.0; _y[0] = 0.0; _y[1] = 0.0;
  }
  ~FirstHPF() {};
  
  /// @brief 一次進み時定数を設定
  /// @param[in] tau 一次進み時定数[s]
  void setTau(const float tau) {
    _tau = tau;
  }

  /// @brief 内部変数をリセット
  /// @param[in] init xとyをリセットする初期値
  void clear(const float init) {
    _x[0] = init; _x[1] = init; _y[0] = init; _y[1] = init;
  }

  /// @brief 入力から出力を計算
  /// @param[in] x 入力
  /// @param[in] dt 直前の入力からの経過時間[s]
  /// @retval y 出力
  inline float update(const float x, const float dt) {
    _x[1] = _x[0];
    _y[1] = _y[0];
    _x[0] = x;
    _y[0] = 1.0/(2.0*_tau + dt) * ((2.0*_tau - dt) * _y[1] + 2.0*_tau * (_x[0] - _x[1]));
    return _y[0];
  }
};

/// @brief 高域を遅延させるオールパスフィルタ
class FirstAPF {
  private:
  float _omegab;  // ブレイク角周波数[rad/s]: 遅延が90°になる角周波数
  float _x[2];  // 現在およびひとつ前のx. 0が最新、1がひとつ前
  float _y[2];  // 現在およびひとつ前のy

  public:
  FirstAPF() : _omegab(0.0) {
    _x[0] = 0.0; _x[1] = 0.0; _y[0] = 0.0; _y[1] = 0.0;
  }
  ~FirstAPF() {};
  
  /// @brief ブレイク周波数を設定
  /// @param[in] freqb ブレイク周波数[Hz]
  void setTau(const float freqb) {
    _omegab = 2.0 * PI * freqb;
  }

  /// @brief 内部変数をリセット
  /// @param[in] init xとyをリセットする初期値
  void clear(const float init) {
    _x[0] = init; _x[1] = init; _y[0] = init; _y[1] = init;
  }

  /// @brief 入力から出力を計算
  /// @param[in] x 入力
  /// @param[in] dt 直前の入力からの経過時間[s]
  /// @retval y 出力
  inline float update(const float x, const float dt) {
    _x[1] = _x[0];
    _y[1] = _y[0];
    _x[0] = x;
    _y[0] = (2 - dt*_omegab)/(2 + dt*_omegab) * (_y[1] - _x[0]) + _x[1];
    return _y[0];
  }
};
