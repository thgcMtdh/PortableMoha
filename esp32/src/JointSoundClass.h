#pragma once

#ifdef ARDUINO_ARCH_ESP32
#include <Arduino.h>
#else
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#endif

#include <math.h>

#define SAMPLINGRATE 44100
#define T_SAMPLE (1.0/SAMPLINGRATE)

#define MAX_SOURCE_NUM 1
#define MAX_WHEEL_NUM 8
#define MAX_JOINT_NUM 4
#define MAX_PLAYER_NUM 16


typedef struct _soundSource
{
  int id;       // user defined ID (must be 0-(MAX_SOURCE_NUM-1)). you can add different sounds with different IDs.
  float speed;  // train speed [km/h] when the sound was recorded.
  float minSpeed;         // 時速何km/h以上で走行中にこの音を出すか？
  float interceptPitch;   // 録音した速度における音程を基準に、速度ゼロのとき音域はもとの何倍か
  float interceptVolume;  // 録音した速度における音量を基準に、速度ゼロのとき音量はもとの何倍か
  int length;   // size of the sound data [byte]
  // pointer to the data buffer of the joint sound.
  // data must be formatted as 44.1kHz, two-channel, 16-bit PCM
  uint8_t* buf;
} soundSource_t;

typedef struct _wheel
{
  int id;          // user defined ID (must be 0-(MAX_WHEEL_NUM-1)). you can add a number of wheels.
  float position;  // 聴取点からみた車輪位置[m]. 進行方向前方にある場合は負, 後方にある場合は正.
  float pitch;     // 音程を変える場合に設定. 周波数を何倍するか.
  float volume;    // 音量を変える場合に設定. 振幅を何倍するか.
} wheel_t;

typedef struct _joint
{
  int id;                // user defined ID (must be 0-(MAX_JOINT_NUM-1)). you can add joints with different IDs.
  float position;        // joint position when looking from listener.
  soundSource_t* sound;  // pointer to the sound source which is played when a wheel is passing.
} joint_t;


/// ジョイントと車輪の組み合わせによって生じる音を計算し、再生用のPCMデータを生成するクラス
class PlayerClass
{
public:
  /// @param joint 再生するジョイントへのポインタ
  /// @param wheel 再生する車輪へのポインタ
  PlayerClass(const joint_t* pJoint, const wheel_t* pWheel);

  ~PlayerClass();

  /// @brief 再生の開始/一時停止を行う
  /// @param play 1 : 再生, 0 : 一時停止
  void setPlaying(bool play);

  /// @brief 再生が終了したかどうかを返す. 終了した場合は破棄してほしい
  /// @retval 1 : 再生終了済, 0 : 未完了
  bool getIsFinished(void);

  /// @brief 速度に応じて、ひとつのサンプルを生成
  /// @param speed 現在の走行速度 [km/h]
  /// @param height 音源(レール上面を仮定)から聴取点までの距離 [m]
  /// @retval 16-bit stereo PCM 形式のデータ. 符号付16bit整数で、先頭16bitが L, 後方16bitが R.
  uint8_t* createSample(float speed, float height);

  const joint_t* _pJoint;  // 生成対象のジョイント
  const wheel_t* _pWheel;  // 生成対象の車輪

  uint8_t _buf[4];  // 1サンプルぶんのPCMデータを保存するバッファ. 符号付16bit整数で、先頭16bitが L, 後方16bitが R.

  float _playingSpeed;     // 再生速度は元の音源の何倍であるか
  float _playingPosition;    // 音源の再生位置[サンプル目]
  bool _isPlaying;         // 現在再生中か？
  bool _isFinished;        // 最後まで再生したか？
};

class JointSoundClass
{
public:
  /// ジョイント音生成プロセス全体を管理するクラス
  JointSoundClass(const float listenigPointHeight, const bool loopback);
  ~JointSoundClass();

  int addSoundSource(int id, float speed, float minSpeed, float interceptPitch, float interceptVolume, uint8_t* buf, int len);
  int addJoint(int id, int soundId, float position);
  int addWheel(int id, float postion, float pitch=1.0, float volume=1.0);
  int deleteSoundSource(int id=MAX_SOURCE_NUM);
  int deleteJoint(int id=MAX_JOINT_NUM);
  int deleteWheel(int id=MAX_WHEEL_NUM);

  void updateMaxMinWheelPosition(void);

  /// @brief ある速度における音データをlen[bytes]ぶん生成する
  /// @param buf buffer to be filled with PCM data stream
  /// @param len size(in bytes) of data block to be copied to buf. -1 is an indication to user that data buffer shall be flushed
  /// @retval 1:success, 0:fail
  int generateSound(uint8_t* buf, int len, float speed);

private:
  const float _height;   // distance from sound source to listening point [m]
  const bool _loopback;  // joint loopback enable flag
  soundSource_t soundSourceArray[MAX_SOURCE_NUM];
  joint_t jointArray[MAX_JOINT_NUM];
  wheel_t wheelArray[MAX_WHEEL_NUM];
  PlayerClass* pPlayers[MAX_PLAYER_NUM];
  float _maxWheelPosition;
  float _minWheelPosition;
};
