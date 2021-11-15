#pragma once

#ifdef ARDUINO_ARCH_ESP32
#include <Arduino.h>
#else
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#define PI 3.1415926535897932384626433832795
#endif

#include <algorithm>
#include <deque>
#include <vector>

const float SAMPLINGRATE = 44100;
const float T_SAMPLE = 1 / SAMPLINGRATE;

class SoundSourceClass {
 public:
  SoundSourceClass(int id, float speed, float minSpeed, float interceptPitch, float interceptVolume, uint8_t* buf, int size);
  SoundSourceClass(const SoundSourceClass& obj);
  ~SoundSourceClass();

  int id;
  float speed;
  float minSpeed;
  float interceptPitch;
  float interceptVolume;
  uint8_t* buf;
  int size;
};

class JointClass {
 public:
  JointClass(int soundId, float position);
  JointClass(const JointClass& obj);
  ~JointClass();

  int soundId;
  float position;
};

class WheelClass {
 public:
  WheelClass(float position, float pitch = 1.0, float volume = 1.0);
  WheelClass(const WheelClass& obj);
  ~WheelClass();

  float position;
  float pitch;
  float volume;
};

/// ジョイントと車輪の組み合わせによって生じる音を計算し、再生用のPCMデータを生成するクラス
class PlayerClass {
 public:
  /// @param pJoint 音源になるジョイントを指すポインタ
  /// @param pWheel 音源になる車輪を指すポインタ
  /// @param pSoundSource 再生する音源を指すポインタ
  /// @param height 音源(レール上面を仮定)から聴取点までの距離 [m]
  PlayerClass(JointClass* pJoint, WheelClass* pWheel, SoundSourceClass* pSoundSource, float height);
  PlayerClass(const PlayerClass& obj);

  ~PlayerClass();

  /// @brief 再生の開始/一時停止を行う
  /// @param play 1 : 再生, 0 : 一時停止
  void setPlaying(bool play);

  /// @brief 再生が終了したかどうかを返す. 終了した場合は破棄してほしい
  /// @retval 1 : 再生終了済, 0 : 未完了
  bool getIsFinished(void);

  /// @brief 速度に応じて、ひとつのサンプルを生成
  /// @param speed 現在の走行速度 [km/h]
  /// @retval 16-bit stereo PCM 形式のデータ. 符号付16bit整数で、先頭16bitが L, 後方16bitが R.
  uint8_t* createSample(float speed);

  JointClass* _pJoint;              // 生成対象のジョイント
  WheelClass* _pWheel;              // 生成対象の車輪
  SoundSourceClass* _pSoundSource;  // 再生する音源
  float _height;                    // 音源(レール上面を仮定)から聴取点までの距離 [m]

  uint8_t _buf[4];  // 1サンプルぶんのPCMデータを保存するバッファ. 符号付16bit整数で、先頭16bitが L, 後方16bitが R.

  float _playingSpeed;     // 再生速度は元の音源の何倍であるか
  float _playingPosition;  // 音源の再生位置[サンプル目]
  bool _isPlaying;         // 現在再生中か？
  bool _isFinished;        // 最後まで再生したか？
};

class JointSoundClass {
 public:
  JointSoundClass(const float listenigPointHeight, const bool loopback);
  ~JointSoundClass();

  /// @brief 初期設定時に音源を追加する
  /// @param id             user defined ID (must be 0-(MAX_SOURCE_NUM-1)). you can add different sounds with different IDs.
  /// @param speed          train speed [km/h] when the sound was recorded.
  /// @param minSpeed       時速何km/h以上で走行中にこの音を出すか？
  /// @param interceptPitch 録音した速度における音程を基準に、速度ゼロのとき音域はもとの何倍か
  /// @param inerceptVolume 録音した速度における音量を基準に、速度ゼロのとき音量はもとの何倍か
  /// @param buf            pointer to the data buffer of the joint sound.
  ///                       data must be formatted as 44.1kHz, two-channel, 16-bit PCM
  /// @param size           size of the sound data [byte]
  /// @retval 1:success, 0:fail
  int addSoundSource(int id, float speed, float minSpeed, float interceptPitch, float interceptVolume, uint8_t* buf, int size);

  /// @brief 初期設定時にジョイントを追加する. 音声生成中に実行してはならない
  /// @param soundId  ID of the sound source which is played when a wheel is passing.
  /// @param position joint position when looking from listener.
  /// @retval 1: success, 0: fail
  int addJoint(int soundId, float position);

  /// @brief 音声生成中、ジョイントを最も前方(positionが最も負の方向)に動的に追加する
  ///        高速に処理できるが、追加位置は指定できず最前方のみとなる. 最前方でないジョイント位置を指定した場合エラーとなり、追加されない
  /// @param soundId  ID of the sound source which is played when a wheel is passing.
  /// @param position joint position when looking from listener.
  /// @retval 1: success, 0: fail
  int addForwardJoint(int soundId, float position);

  /// @brief 初期設定時に車輪を追加する. 音声生成中に実行してはならない
  /// @param position 聴取点からみた車輪位置[m]. 進行方向前方にある場合は負, 後方にある場合は正.
  /// @param pitch    [省略可] 音程を変える場合に設定. 周波数を何倍するか.
  /// @param volume   [省略可] 音量を変える場合に設定. 振幅を何倍するか.
  int addWheel(float postion, float pitch = 1.0, float volume = 1.0);

  /// @brief ある速度における音データをsize[bytes]ぶん生成する
  /// @param buf  buffer to be filled with PCM data stream
  /// @param size size(in bytes) of data block to be copied to buf. -1 is an indication to user that data buffer shall be flushed
  /// @retval 1:success, 0:fail
  int generateSound(uint8_t* buf, int size, float speed);

 private:
  const float _height;   // distance from sound source to listening point [m]
  const bool _loopback;  // joint loopback enable flag

  std::vector<SoundSourceClass> _soundVector;
  std::deque<JointClass> _jointDeque;
  std::vector<WheelClass> _wheelVector;
  std::vector<PlayerClass> _playerVector;
};

void debug_setup(void);
void debug_loop(void);
