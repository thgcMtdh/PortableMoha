#include "JointSoundClass.h"

JointSoundClass::JointSoundClass(const float listeningPointHeight, const bool loopback) :
  _height(listeningPointHeight), _loopback(loopback)
{
  printf("constructor\n");
  deleteSoundSource();
  deleteJoint();
  deleteWheel();
}

JointSoundClass::~JointSoundClass()
{
  printf("destructor\n");
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
    soundSourceArray[id].minSpeed = 0.0;
    soundSourceArray[id].interceptPitch = 0.0;
    soundSourceArray[id].interceptVolume = 0.0;
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

  // id未指定のときid==MAX_WHEEL_NUMとなる. このとき全てのwheelを消去
  if (id == MAX_WHEEL_NUM) {
    for (int i = 0; i < MAX_WHEEL_NUM; i++) {
      deleteWheel(i);
    }
    _minWheelPosition = 0.0;
    _maxWheelPosition = 0.0;
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
  // printf("_maxwheelpos, _minwheelpos = %f, %f\n", _maxWheelPosition, _minWheelPosition);
}

int JointSoundClass::generateSound(uint8_t* buf, int len, float speed)
{
  // エラーチェック
  // sound, joint, wheel それぞれ1つ以上あるか確認
  // loopback==trueのときはjoint数2つ以上が必要

  // 必要なサンプル数ぶんの計算を行う
  for (int s_i = 0; s_i < len / 4; s_i++) {  // s_i : sample index のつもり

    // printf("sample %d\n", s_i);

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
      if (wheelArray[w_i].id < MAX_WHEEL_NUM) {  // wheel存在確認
        for (int j_i = 0; j_i < MAX_JOINT_NUM; j_i++) {
          if (jointArray[j_i].id < MAX_JOINT_NUM) {  // joint存在確認
            if ((jointArray[j_i].position - traveledDistance) < wheelArray[w_i].position && wheelArray[w_i].position < jointArray[j_i].position) {
              // 跨いでいた場合、最低速度以上か判定
              if (speed > jointArray[j_i].sound->minSpeed) {

                // playerを生成し、再生を開始
                for (int p_i = 0; p_i < MAX_PLAYER_NUM; p_i++) {
                  if (pPlayers[p_i] == NULL) {
                    pPlayers[p_i] = new PlayerClass(&jointArray[j_i], &wheelArray[w_i]);
                    pPlayers[p_i]->setPlaying(true);
                    // printf("player created.\n");
                    break;
                  }
                }

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
        // printf("jointArray[%d].postion = %.5f\n", i, jointArray[i].position);
      }
    }

    // printf("maxPos, secondPos, minPos = %.5f, %.5f, %.5f\n", maxPosition, secondPosition, minPosition);

    // 最後方のジョイントがすべてのwheelを通過済みの場合
    if (maxPosition > _maxWheelPosition) {
      if (_loopback) {  // ループバック有効時、最前方に戻す
        pMaxPositionJoint->position = minPosition - (maxPosition - secondPosition);
      } else {          // ループバック無効時、消去
        deleteJoint(pMaxPositionJoint->id);
      }
    }

    // -- 各playerについてサンプル生成し、bufへ出力 --
    int16_t* oneBufL = reinterpret_cast<int16_t*>(&buf[4 * s_i]);
    int16_t* oneBufR = reinterpret_cast<int16_t*>(&buf[4 * s_i + 2]);
    for (int p_i = 0; p_i < MAX_PLAYER_NUM; p_i++) {
      if (pPlayers[p_i] != NULL) {
        uint8_t* sample = pPlayers[p_i]->createSample(speed, _height);
        *oneBufL += *(reinterpret_cast<int16_t*>(&sample[0]));
        *oneBufR += *(reinterpret_cast<int16_t*>(&sample[2]));
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
  _pJoint(pJoint), _pWheel(pWheel), _playingSpeed(1.0), _playingPosition(0.0), _isPlaying(false), _isFinished(false)
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

uint8_t* PlayerClass::createSample(float speed, float height)
{
  if (_isPlaying && !_isFinished) {
    // 再生速度(=音の高さ)を計算
    float speedRatio = speed / _pJoint->sound->speed;
    _playingSpeed = _pJoint->sound->interceptPitch + (1 - _pJoint->sound->interceptPitch) * speedRatio;  // 音程-速度特性は一次関数を仮定
    _playingSpeed *= _pWheel->pitch;  // 車輪固有の特性

    // 振幅を計算
    float amp = height / sqrtf(height*height + _pWheel->position*_pWheel->position);  // 音源からの距離による減衰
    amp *= _pWheel->volume;  // 隣の車両にあるなど、車輪固有の減衰

    // 何サンプル目を再生するかに変換
    _playingPosition += _playingSpeed;
    int i1 = static_cast<int>(_playingPosition);
    int i2 = i1 + 1;
    // printf("_playingPosition, i1, i2 = %f, %d, %d\n", _playingPosition, i1, i2);
    // インデックスが長さを越えている場合、再生終了したので停止
    if (i2 >= _pJoint->sound->length / 2 / 2) {  // ステレオで /2, 2byte/sampleなので /2
      _isFinished = true;
      _isPlaying = false;
    // LとRの2つについて、線形補完してサンプル生成
    } else {
      float alpha = _playingPosition - static_cast<float>(i1);  // 線形補間の位置(0～1). 0 は x1 側、1 は x2 側 
      int16_t sample1, sample2, result;
      // L
      sample1 = *(reinterpret_cast<int16_t*>(&_pJoint->sound->buf[4*i1]));  // 4*i1のアドレスをint16_t型とみなして読み込む
      sample2 = *(reinterpret_cast<int16_t*>(&_pJoint->sound->buf[4*i2]));
      result = static_cast<int16_t>(amp * ((1 - alpha) * sample1 + alpha * sample2));
      // printf("L sample1, sample2, result = %d, %d, %d\n", sample1, sample2, result);
      _buf[0] = result & 0xff;
      _buf[1] = result >> 8;
      // R
      sample1 = *(reinterpret_cast<int16_t*>(&_pJoint->sound->buf[4*i1 + 2]));
      sample2 = *(reinterpret_cast<int16_t*>(&_pJoint->sound->buf[4*i2 + 2]));
      result = static_cast<int16_t>(amp * ((1 - alpha) * sample1 + alpha * sample2));
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

// 車両定数
const float CAR_L = 20.0;      // 車両長[m]
const float CAR_D = 13.8;      // 台車間距離[m]
const float CAR_W = 2.1;       // 車軸間距離[m]
const float PERSON_POS = 5.0;  // 聴取者が車両中心から何mの場所にいるか[m]
const float EAR_HEIGHT = 2.0;  // 車輪真上で測った、音源から耳までの距離[m]
const float ALPHA_WALL = 0.5;  // 壁の向こうの車輪からの音は何倍になるか

JointSoundClass jointSound(EAR_HEIGHT, true);

void printBuf(uint8_t* buf, int SAMPLENUM) {
  for (int i=0; i<SAMPLENUM; i++) {
    printf("%d, %d\n", *((int16_t*)&buf[4*i]), *((int16_t*)&buf[4*i + 2]));
  }
}

void setup() {
  // テスト用に正弦波を生成
  const size_t SAMPLENUM = 64;
  uint8_t buf0[4 * SAMPLENUM];  // ステレオでx2, 16bitなのでx2
  for (int i=0; i<SAMPLENUM; i++) {
    int sinVal = 32767*sin(2.0 * M_PI * i/SAMPLENUM);
    int cosVal = 32767*cos(2.0 * M_PI * i/SAMPLENUM);
    buf0[4*i]     = sinVal & 0xff;
    buf0[4*i + 1] = sinVal >> 8;
    buf0[4*i + 2] = cosVal & 0xff;
    buf0[4*i + 3] = cosVal >> 8;
  }

  // 音源の追加
  jointSound.addSoundSource(0, 24.9, 12.0, 0.5, 1.0, buf0, 4*SAMPLENUM);

  // ジョイントの追加
  jointSound.addJoint(0, 0, -25.001);
  jointSound.addJoint(1, 0, -50.0);
  jointSound.addJoint(2, 0, -75.0);

  // 車輪の追加
  jointSound.addWheel(0, -25.0);
  /*jointSound.addWheel(1, -100.0);
  jointSound.addWheel(2, -200.0);
  jointSound.addWheel(0, -CAR_L + CAR_D/2 - CAR_W/2 - PERSON_POS, 1.0, ALPHA_WALL);
  jointSound.addWheel(1, -CAR_L + CAR_D/2 + CAR_W/2 - PERSON_POS, 1.0, ALPHA_WALL);
  jointSound.addWheel(2, -CAR_D/2 - CAR_W/2 - PERSON_POS, 1.0, 1.0);
  jointSound.addWheel(3, -CAR_D/2 + CAR_W/2 - PERSON_POS, 1.0, 1.0);
  jointSound.addWheel(4, CAR_D/2 - CAR_W/2 - PERSON_POS, 1.0, 1.0);
  jointSound.addWheel(5, CAR_D/2 + CAR_W/2 - PERSON_POS, 1.0, 1.0);
  jointSound.addWheel(6, CAR_L - CAR_D/2 - CAR_W/2 - PERSON_POS, 1.0, ALPHA_WALL);
  jointSound.addWheel(7, CAR_L - CAR_D/2 + CAR_W/2 - PERSON_POS, 1.0, ALPHA_WALL);*/

  // 音を出してみる
  uint8_t output[4*SAMPLENUM];
  memset(output, 0, 4*SAMPLENUM);
  jointSound.generateSound(output, 4*SAMPLENUM, 20.0);
  printf("output=\n");
  printBuf(output, SAMPLENUM);
  
}

void loop() {

}

int main() {
  setup();
  loop();
}

#endif
