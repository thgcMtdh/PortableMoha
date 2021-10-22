#include "JointSoundClass.h"

JointSoundClass::JointSoundClass() :
  _speed(0.0), _loopback(true)
{
  deleteSoundSource();
  deleteJoint();
  deleteWheel();
}

JointSoundClass::~JointSoundClass()
{
  deleteSoundSource();
  deleteJoint();
  deleteWheel();
  for (int p_i = 0; p_i < MAX_PLAYER_NUM; p_i++) {
    if (pPlayers[p_i] != NULL) {
      delete pPlayers[p_i];
      pPlayers[p_i] = NULL;
    }
  }
}

int JointSoundClass::addSoundSource(int id, float speed, float minSpeed, float interceptPitch, float interceptVolume, uint8_t* buf, int len)
{
  if (id < 0 || id >= MAX_SOURCE_NUM) { return 0; }
  if (!buf) { return 0; }

  soundSourceArray[id].id = id;
  soundSourceArray[id].speed = speed;
  soundSourceArray[id].minSpeed = minSpeed;
  soundSourceArray[id].buf = buf;
  soundSourceArray[id].length = len;
  soundSourceArray[id].interceptPitch = interceptPitch;
  soundSourceArray[id].interceptVolume = interceptVolume;
  return 1;
}

int JointSoundClass::addJoint(int id, int soundId, float position)
{
  if (id < 0 || id >= MAX_JOINT_NUM) { return 0; }             // IDが範囲内か確認
  if (soundId < 0 || soundId >= MAX_SOURCE_NUM) { return 0; }  // soundIdが範囲内か確認
  if (!soundSourceArray[soundId].buf) { return 0; }            // 該当する音源があるか確認

  jointArray[id].id = id;
  jointArray[id].position = position;
  jointArray[id].sound = &soundSourceArray[soundId];

  return 1;
}

int JointSoundClass::addWheel(int id, float position, float pitch, float volume)
{
  if (id <0 || id >= MAX_WHEEL_NUM) { return 0; }

  wheelArray[id].id = id;
  wheelArray[id].position = position;
  wheelArray[id].pitch = pitch;
  wheelArray[id].volume = volume;
  updateMaxMinWheelPosition();
  return 1;
}


int JointSoundClass::deleteSoundSource(int id)
{
  if (id < 0 || id > MAX_SOURCE_NUM) { return 0; }

  // id未指定のときid==MAX_SOURCE_NUMとなる. このとき全てのsoundSourceを消去
  if (id == MAX_SOURCE_NUM) {
    for (int i = 0; i < MAX_SOURCE_NUM; i++) {
      deleteSoundSource(i);
    }
  // id指定時、当該idのsoundSourceを削除
  } else {
    soundSourceArray[id].id = MAX_SOURCE_NUM;
    soundSourceArray[id].speed = 0.0;
    soundSourceArray[id].buf = NULL;
  }
  return 1;
}

int JointSoundClass::deleteJoint(int id)
{
  if (id < 0 || id > MAX_JOINT_NUM) { return 0; }

  // id未指定のときid==MAX_JOINT_NUMとなる. このとき全てのjointを消去
  if (id == MAX_JOINT_NUM) {
    for (int i = 0; i < MAX_JOINT_NUM; i++) {
      deleteJoint(i);
    }
  // id指定時、当該idのjointを削除
  } else {
    jointArray[id].id = MAX_JOINT_NUM;
    jointArray[id].position = 0.0;
    jointArray[id].sound = NULL;
  }
  return 1;
}

int JointSoundClass::deleteWheel(int id)
{
  if (id < 0 || id > MAX_WHEEL_NUM) { return 0; }

  // id未指定のときid==MAX_WHEEL_NUMとなる. このとき全てのjointを消去
  if (id == MAX_WHEEL_NUM) {
    for (int i = 0; i < MAX_WHEEL_NUM; i++) {
      deleteWheel(i);
    }
  // id指定時、当該idのjointを削除
  } else {
    wheelArray[id].id = MAX_WHEEL_NUM;
    wheelArray[id].position = 0.0;
    wheelArray[id].pitch = 1.0;
    wheelArray[id].volume = 1.0;
    updateMaxMinWheelPosition();
  }
  return 1;
}

int JointSoundClass::setLoopback(bool loopback)
{
  _loopback = loopback;
  return 1;
}

void JointSoundClass::updateMaxMinWheelPosition()
{
  float maxPosition = -1000.0;
  float minPosition = 1000.0;
  for (int i = 0; i < MAX_WHEEL_NUM; i++) {
    if (wheelArray[i].id < MAX_WHEEL_NUM) {  // wheel i が存在している場合
      maxPosition = fmax(wheelArray[i].position, maxPosition);
      minPosition = fmin(wheelArray[i].position, minPosition);
    }
  }
  _maxWheelPosition = maxPosition;
  _minWheelPosition = minPosition;
}

int JointSoundClass::generateSound(uint8_t* buf, int len, float speed)
{
  // 必要なサンプル数ぶんの計算を行う
  for (int s_i = 0; s_i < len / 4; s_i++) {  // s_i : sample index のつもり

    // -- joint通過判定 --
    // jointの位置を進める
    float traveledDistance = speed/3.6 * T_SAMPLE;  // サンプリング時間の間に進んだ距離[m]
    for (int j_i = 0; j_i < MAX_JOINT_NUM; j_i++) {
      if (jointArray[j_i].id < MAX_JOINT_NUM) {         // jointが存在している場合
        jointArray[j_i].position += traveledDistance;  // 位置を進める
      }
    }

    // それぞれのwheelについて、jointを跨いだかどうか判定する
    for (int w_i = 0; w_i < MAX_WHEEL_NUM; w_i++) {
      for (int j_i = 0; j_i < MAX_JOINT_NUM; j_i++) {
        if (jointArray[j_i].position - traveledDistance < wheelArray[w_i].position && wheelArray[w_i].position < jointArray[j_i].position) {
          // 跨いでいた場合、最低速度以上か判定
          if (speed > jointArray[j_i].sound->minSpeed) {

            // playerを生成し、再生を開始
            for (int p_i = 0; p_i < MAX_PLAYER_NUM; p_i++) {
              if (pPlayers[p_i] == NULL) {
                pPlayers[p_i] = new PlayerClass(&jointArray[j_i], &wheelArray[w_i]);
                pPlayers[p_i]->setPlaying(true);
                break;
              }
            }

          }
        }
      }
    }
    
    // -- 進行方向最後方(position最大)のジョイントについて、すべてのwheelを通り過ぎていたら位置を更新 --
    // 最後方, 2番目に後方, 最前方のジョイントを取得
    float maxPosition = -1.0e6;
    float secondPosition = -1.0e6;
    float minPosition = 1.0e6;
    joint_t* pMaxPositionJoint = NULL;
    joint_t* pSecondPositionJoint = NULL;
    joint_t* pMinPositionJoint = NULL;
    for (int i = 0; i < MAX_JOINT_NUM; i++) {
      if (jointArray[i].id < MAX_JOINT_NUM) {            // joint i が存在している場合
        if (jointArray[i].position > maxPosition) {      // 最大値を更新
          secondPosition = maxPosition;
          maxPosition = jointArray[i].position;
          pMaxPositionJoint = &jointArray[i];
        } else if (jointArray[i].position > secondPosition) {  // 2番目を更新
          secondPosition = jointArray[i].position;
          pSecondPositionJoint = &jointArray[i];
        }
        if (jointArray[i].position < minPosition) {      // 最小値を更新
          minPosition = jointArray[i].position;
          pMinPositionJoint = &jointArray[i];
        }
      }

      // 最後方のジョイントがすべてのwheelを通過済みの場合
      if (maxPosition > _maxWheelPosition) {
        if (_loopback) {  // ループバック有効時、最前方に戻す
          pMaxPositionJoint->position = minPosition - (maxPosition - secondPosition);
        } else {          // ループバック無効時、消去
          deleteJoint(pMaxPositionJoint->id);
        }
      }
    }

    // -- 各playerについてサンプル生成し、bufへ出力 --
    int16_t* bufL = (int16_t*)(&buf[4 * s_i]);
    int16_t* bufR = (int16_t*)(&buf[4 * s_i + 2]);
    for (int p_i = 0; p_i < MAX_PLAYER_NUM; p_i++) {
      if (pPlayers[p_i] != NULL) {
        uint8_t* sample = pPlayers[p_i]->createSample(speed);
        *bufL += sample[4 * s_i]     << 8 + sample[4 * s_i + 1];
        *bufR += sample[4 * s_i + 2] << 8 + sample[4 * s_i + 3];
      }
    }

    // -- 再生終了したplayerは破棄 --
    for (int p_i = 0; p_i < MAX_PLAYER_NUM; p_i++) {
      if (pPlayers[p_i] != NULL && pPlayers[p_i]->getIsFinished() == true) {
        delete pPlayers[p_i];
        pPlayers[p_i] = NULL;
      }
    }
  }
  return 1;
}

PlayerClass::PlayerClass(const joint_t* pJoint, const wheel_t* pWheel) :
  _pJoint(pJoint), _pWheel(pWheel), _playingSpeed(1.0), _playingPosition(0), _isPlaying(false), _isFinished(false)
{
  for (int i = 0; i < sizeof(_buf); i++) _buf[i] = 0x0;
}

PlayerClass::~PlayerClass()
{
}

void PlayerClass::setPlaying(bool play)
{
  _isPlaying = play;
}

bool PlayerClass::getIsFinished()
{
  return _isFinished;
}

uint8_t* PlayerClass::createSample(float speed)
{
  if (_isPlaying && !_isFinished) {
    // 再生速度(=音の高さ)を計算
    float speedRatio = speed / _pJoint->sound->speed;
    _playingSpeed = _pJoint->sound->interceptPitch + (1 - _pJoint->sound->interceptPitch) * speedRatio;
    _playingSpeed *= _pWheel->pitch;

    // 何サンプル目を再生するかに変換
    _playingPosition += _playingSpeed;
    int i1 = static_cast<int>(_playingPosition);
    int i2 = i1 + 1;
    // インデックスが長さを越えている場合、再生終了したので停止
    if (i2 >= _pJoint->sound->length / 2 / 2) {  // ステレオで /2, 2byte/sampleなので /2
      _isFinished = true;
      _isPlaying = false;
    // LとRの2つについて、線形補完してサンプル生成
    } else {
      int16_t sample1, sample2, result;
      // L
      sample1 = *((int16_t*)(&_pJoint->sound->buf[4*i1]));  // 4*i1のアドレスをint16_t型とみなして読み込む
      sample2 = *((int16_t*)(&_pJoint->sound->buf[4*i2]));
      result = static_cast<int16_t>((1.0 - _playingPosition) * sample1 + _playingPosition * sample2);
      _buf[0] = result & 0xff;
      _buf[1] = result >> 8;
      // R
      sample1 = *((int16_t*)(&_pJoint->sound->buf[4*i1 + 2]));
      sample2 = *((int16_t*)(&_pJoint->sound->buf[4*i2 + 2]));
      result = static_cast<int16_t>((1.0 - _playingPosition) * sample1 + _playingPosition * sample2);
      _buf[2] = result & 0xff;
      _buf[3] = result >> 8;
      return _buf;
    }
  }
  _buf[0] = 0;
  _buf[1] = 0;
  _buf[2] = 0;
  _buf[3] = 0;
  return _buf;
}


#ifndef ARDUINO_ARCH_ESP32

// for debug on desktop PC
#include <stdio.h>

JointSoundClass jointSound;

void setup() {
  // テスト用に正弦波を生成
  const size_t LEN = 16;
  uint8_t buf0[4 * LEN];  // ステレオでx2, 16bitなのでx2
  for (int i=0; i<LEN; i++) {
    int sinVal = 32767*sin(2.0 * M_PI * i/LEN);
    int cosVal = 32767*cos(2.0 * M_PI * i/LEN);
    buf0[4*i]     = sinVal & 0xff;
    buf0[4*i + 1] = sinVal >> 8;
    buf0[4*i + 2] = cosVal & 0xff;
    buf0[4*i + 3] = cosVal >> 8;
    printf("%d, %d\n", *((int16_t*)(&buf0[4*i])), *((int16_t*)(&buf0[4*i+2])));
  }

  // jointSound.addSoundSource(0, 24.9, 12.0, 0.5, 1.0);
}

void loop() {

}

int main() {
  setup();
  loop();
}

#endif
