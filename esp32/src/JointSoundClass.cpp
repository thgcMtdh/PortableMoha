#include "JointSoundClass.h"

SoundSourceClass::SoundSourceClass(int id, float speed, float minSpeed, float interceptPitch, float interceptVolume, uint8_t* buf, int size) : id(id), speed(speed), minSpeed(minSpeed), interceptPitch(interceptPitch), interceptVolume(interceptVolume), buf(buf), size(size) {}
SoundSourceClass::SoundSourceClass(const SoundSourceClass& obj) : id(obj.id), speed(obj.speed), minSpeed(obj.minSpeed), interceptPitch(obj.interceptPitch), interceptVolume(obj.interceptVolume), buf(obj.buf), size(obj.size) {}
SoundSourceClass::~SoundSourceClass() {}

JointClass::JointClass(int soundId, float position) : soundId(soundId), position(position) {}
JointClass::JointClass(const JointClass& obj) : soundId(obj.soundId), position(obj.position) {}
JointClass::~JointClass() {}

WheelClass::WheelClass(float position, float pitch, float volume) : position(position), pitch(pitch), volume(volume) {}
WheelClass::WheelClass(const WheelClass& obj) : position(obj.position), pitch(obj.pitch), volume(obj.volume) {}
WheelClass::~WheelClass() {}

JointSoundClass::JointSoundClass(const float listeningPointHeight, const bool loopback) : _height(listeningPointHeight), _loopback(loopback) {}
JointSoundClass::~JointSoundClass()
{
  _soundVector.clear();
  _jointDeque.clear();
  _wheelVector.clear();
  _playerVector.clear();
}

int JointSoundClass::addSoundSource(int id, float speed, float minSpeed, float interceptPitch, float interceptVolume, uint8_t* buf, int size)
{
  _soundVector.push_back(SoundSourceClass(id, speed, minSpeed, interceptPitch, interceptVolume, buf, size));
  return 1;
}

int JointSoundClass::addJoint(int soundId, float position)
{
  for (auto& x : _soundVector) {
    if (x.id == soundId) {  // soundIdで指定されたidをもつ音源データが存在するか確認
      _jointDeque.push_back(JointClass(soundId, position));     // 新規作成
      std::sort(_jointDeque.begin(), _jointDeque.end(), [](const JointClass& lhs, const JointClass& rhs) {
        return lhs.position < rhs.position;
      });  // positionの小さい順に並べ替え
      return 1;
    }
  }
  return 0;
}

int JointSoundClass::addForwardJoint(int soundId, float position)
{
  for (auto& x : _soundVector) {
    if (x.id == soundId) {  // soundIdで指定されたidをもつ音源データが存在するか確認
      if (position < _jointDeque.front().position) {  // 指定された位置が最前方であるか確認
        _jointDeque.push_front(JointClass(soundId, position));  // 先頭に挿入
        return 1;
      }
    }
  }
  return 0;
}

int JointSoundClass::addWheel(float position, float pitch, float volume)
{
  _wheelVector.push_back(WheelClass(position, pitch, volume));  // 新規作成
  std::sort(_wheelVector.begin(), _wheelVector.end(), [](const WheelClass& lhs, const WheelClass& rhs) {
    return lhs.position < rhs.position;
  });          // positionの小さい順に並べ替え
  return 1;
}

int JointSoundClass::generateSound(uint8_t* buf, int size, float speed)
{
  // エラーチェック
  if (_soundVector.empty() || _jointDeque.empty() || _wheelVector.empty()) {
    return 0;  // sound, joint, wheel それぞれ1つ以上あるか確認
  }
  if (_loopback && _jointDeque.size() < 2) {
    return 0;  // loopback==trueのときはjoint数2つ以上が必要
  }

  // 必要なサンプル数ぶんの計算を行う
  for (int s_i = 0; s_i < size / 4; s_i++) {  // s_i : sample index のつもり

    // printf("sample %d\n", s_i);

    // -- joint通過判定 --
    // jointの位置を進める
    float traveledDistance = speed/3.6 * T_SAMPLE;  // サンプリング時間の間に進んだ距離[m]
    for (auto& rJoint : _jointDeque) {
      rJoint.position += traveledDistance;  // jointの位置を進める
      // printf("joint pos = %f\n", rJoint.position);
    }

    // それぞれのwheelについて、jointを跨いだかどうか判定する
    for (auto& rWheel : _wheelVector) {
      for (auto& rJoint : _jointDeque) {
        if ((rJoint.position - traveledDistance) < rWheel.position && rWheel.position < rJoint.position) {
          // printf("joint passed\n");
          for (auto& rSoundSource : _soundVector) {  // このジョイントに紐づけられた音源への参照を取得
            if (rSoundSource.id == rJoint.soundId) {
              // playerを生成し、再生を開始
              // printf("created player\n");
              _playerVector.push_back(PlayerClass(&rJoint, &rWheel, &rSoundSource, _height));
              _playerVector.back().setPlaying(1);
            }
          }
        }
      }
    }
    
    // -- 進行方向最後方(position最大)のジョイントについて、すべてのwheelを通り過ぎていたら位置を更新 --
    // 最後方のジョイントがすべてのwheelを通過済みの場合
    if (_jointDeque.back().position > _wheelVector.back().position) {
      // ループバック有効時、最後方のジョイントを最前方へ戻す
      if (_loopback) {
        float frontPosition = _jointDeque.front().position;         // 最前方のジョイントの位置を取得
        float backPosition = _jointDeque.back().position;           // 最後方のジョイントの位置を取得
        float back2Position = (*(_jointDeque.end() - 2)).position;  // 最後方から2番目のジョイントの位置を取得
        
        _jointDeque.push_front(JointClass(_jointDeque.back()));  // 最後方のジョイントを最前方へコピー
        _jointDeque.front().position = frontPosition - (backPosition - back2Position);  // 位置を設定
      }
      // 削除
      _jointDeque.pop_back();
    }

    // -- 各playerについてサンプル生成し、bufへ出力 --
    int16_t* oneBufL = reinterpret_cast<int16_t*>(&buf[4 * s_i]);
    int16_t* oneBufR = reinterpret_cast<int16_t*>(&buf[4 * s_i + 2]);
    for (auto& rPlayer : _playerVector) {
      uint8_t* sample = rPlayer.createSample(speed);
      *oneBufL += *(reinterpret_cast<int16_t*>(&sample[0]));
      *oneBufR += *(reinterpret_cast<int16_t*>(&sample[2]));
    }

    // -- 再生終了したplayerは破棄 --
    auto itr = _playerVector.begin();
    while (itr != _playerVector.end()) {
      if ((*itr)._isFinished) {
        _playerVector.erase(itr);
        // printf("deleted player\n");
      } else {
        ++itr;
      }
    }
  }
  return 1;
}

PlayerClass::PlayerClass(JointClass* pJoint, WheelClass* pWheel, SoundSourceClass* pSoundSource, float height) :
  _pJoint(pJoint), _pWheel(pWheel), _pSoundSource(pSoundSource), _height(height), _playingSpeed(1.0), _playingPosition(0.0), _isPlaying(false), _isFinished(false)
{
  for (int i = 0; i < sizeof(_buf); i++) _buf[i] = 0x0;
}

PlayerClass::PlayerClass(const PlayerClass& obj) :
  _pJoint(obj._pJoint), _pWheel(obj._pWheel), _pSoundSource(obj._pSoundSource), _height(obj._height), _playingSpeed(obj._playingSpeed), _playingPosition(obj._playingPosition), _isPlaying(obj._isPlaying), _isFinished(obj._isFinished)
{
  for (int i = 0; i < 4; i++) { _buf[i] = obj._buf[i]; }
}

PlayerClass::~PlayerClass() {}

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
    float speedRatio = speed / _pSoundSource->speed;
    _playingSpeed = _pSoundSource->interceptPitch + (1 - _pSoundSource->interceptPitch) * speedRatio;  // 音程-速度特性は一次関数を仮定
    _playingSpeed *= _pWheel->pitch;  // 車輪固有の特性

    // 振幅を計算
    float amp = _height / sqrtf(_height*_height + _pWheel->position*_pWheel->position);  // 音源からの距離による減衰
    amp *= _pWheel->volume;  // 隣の車両にあるなど、車輪固有の減衰

    // 何サンプル目を再生するかに変換
    _playingPosition += _playingSpeed;
    int i1 = static_cast<int>(_playingPosition);
    int i2 = i1 + 1;
    // printf("_playingPosition, i1, i2 = %f, %d, %d\n", _playingPosition, i1, i2);
    // インデックスが長さを越えている場合、再生終了したので停止
    if (i2 >= _pSoundSource->size / 2 / 2) {  // ステレオで /2, 2byte/sampleなので /2
      _isFinished = true;
      _isPlaying = false;
    // LとRの2つについて、線形補完してサンプル生成
    } else {
      float alpha = _playingPosition - static_cast<float>(i1);  // 線形補間の位置(0～1). 0 は x1 側、1 は x2 側 
      int16_t sample1, sample2, result;
      // L
      sample1 = *(reinterpret_cast<int16_t*>(&_pSoundSource->buf[4*i1]));  // 4*i1のアドレスをint16_t型とみなして読み込む
      sample2 = *(reinterpret_cast<int16_t*>(&_pSoundSource->buf[4*i2]));
      result = static_cast<int16_t>(amp * ((1 - alpha) * sample1 + alpha * sample2));
      // printf("L sample1, sample2, result = %d, %d, %d\n", sample1, sample2, result);
      _buf[0] = result & 0xff;
      _buf[1] = result >> 8;
      // R
      sample1 = *(reinterpret_cast<int16_t*>(&_pSoundSource->buf[4*i1 + 2]));
      sample2 = *(reinterpret_cast<int16_t*>(&_pSoundSource->buf[4*i2 + 2]));
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


// 車両定数
const float CAR_L = 20.0;      // 車両長[m]
const float CAR_D = 13.8;      // 台車間距離[m]
const float CAR_W = 2.1;       // 車軸間距離[m]
const float PERSON_POS = 6.0;  // 聴取者が車両中心から何mの場所にいるか[m]
const float EAR_HEIGHT = 2.0;  // レール面(音源)から耳までの距離[m]
const float ALPHA_WALL = 0.2;  // 壁の向こうの車輪からの音は何倍になるか

JointSoundClass jointSound(EAR_HEIGHT, true);

void printBuf(uint8_t* buf, int SAMPLENUM) {
  for (int i=0; i<SAMPLENUM; i++) {
    printf("%d, %d\n", *((int16_t*)&buf[4*i]), *((int16_t*)&buf[4*i + 2]));
  }
}

void debug_setup(float duration, float speed) {

  // 音源の追加
  
  char fileName[] = "4-3-1_24.915kmh_encoded.wav";
  FILE* fp = fopen(fileName, "rb");
  uint8_t waste[40];
  uint32_t dataSize;
  uint8_t* data;
  fread(waste, 1, 40, fp);
  fread(&dataSize, 4, 1, fp);
  data = new uint8_t[dataSize];
  //printf("%d\n",dataSize);
  fread(data, 1, dataSize, fp);
  fclose(fp);
  
  jointSound.addSoundSource(0, 24.9, 12.0, 0.5, 1.0, data, dataSize);

  // ジョイントの追加
  jointSound.addJoint(0, -10.0);
  jointSound.addJoint(0, -35.0);
  jointSound.addJoint(0, -60.0);

  // 車輪の追加
  jointSound.addWheel(-CAR_L + CAR_D/2 - CAR_W/2 - PERSON_POS, 1.0, ALPHA_WALL);
  jointSound.addWheel(-CAR_L + CAR_D/2 + CAR_W/2 - PERSON_POS, 1.0, ALPHA_WALL);
  jointSound.addWheel(-CAR_D/2 - CAR_W/2 - PERSON_POS, 1.0, 1.0);
  jointSound.addWheel(-CAR_D/2 + CAR_W/2 - PERSON_POS, 0.98, 1.0);
  jointSound.addWheel(CAR_D/2 - CAR_W/2 - PERSON_POS, 1.0, 1.0);
  jointSound.addWheel(CAR_D/2 + CAR_W/2 - PERSON_POS, 0.97, 1.0);
  jointSound.addWheel(CAR_L - CAR_D/2 - CAR_W/2 - PERSON_POS, 1.0, ALPHA_WALL);
  jointSound.addWheel(CAR_L - CAR_D/2 + CAR_W/2 - PERSON_POS, 1.0, ALPHA_WALL);

  // 音を出してみる
  size_t outSize = (size_t)(duration * SAMPLINGRATE * 4);
  uint8_t* output;
  output = new uint8_t[outSize];
  for (int i=0; i<outSize; i++) { output[i] = 0x0; }
  jointSound.generateSound(output, outSize, speed);
  // printf("output=\n");
  // printBuf(output, outSize/4);

  fp = fopen("out.raw", "wb");
  fwrite(output, 1, outSize, fp);
  fclose(fp);
  
  delete data;
  delete output;
  
}

void debug_loop() {

}

#ifndef ARDUINO_ARCH_ESP32

int main(int argc, char** argv) {
  if (argc != 3) {
    return 0;
  }

  float duration = atof(argv[1]);
  float speed = atof(argv[2]);
  if (duration <= 0 || speed <= 0) {
    return 0;
  }

  debug_setup(duration, speed);
  debug_loop();
}

#endif
