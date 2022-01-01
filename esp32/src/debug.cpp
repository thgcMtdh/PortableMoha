#include "debug.h"

#include "CarDataClass.h"
#include "JointSoundClass.h"
#include "VVVFSoundClass.h"
#include "MotorSoundClass.h"

// 車両定数
const float CAR_L = 20.0;      // 車両長[m]
const float CAR_D = 13.8;      // 台車間距離[m]
const float CAR_W = 2.1;       // 車軸間距離[m]
const float PERSON_POS = 6.0;  // 聴取者が車両中心から何mの場所にいるか[m]
const float EAR_HEIGHT = 2.0;  // レール面(音源)から耳までの距離[m]
const float ALPHA_WALL = 0.4;  // 壁の向こうの車輪からの音は何倍になるか

CarDataClass carData;
JointSoundClass jointSound(EAR_HEIGHT, true);
MotorSoundClass motorSound(carData);
VVVFSoundClass vvvfSound(carData);

void printBuf(uint8_t* buf, int SAMPLENUM) {
  for (int i = 0; i < SAMPLENUM; i++) {
    printf("%d, %d\n", *((int16_t*)&buf[4 * i]), *((int16_t*)&buf[4 * i + 2]));
  }
}

// srcの中身を、int16_t型だと思ってlen(bytes)ぶんdstに加える
inline void addPCMBuf(uint8_t* srcBuf, uint8_t* dstBuf, size_t len) {
  for (size_t i=0; i < len/4; i++) {
    *((int16_t*)&dstBuf[4*i  ]) += *((int16_t*)&srcBuf[4*i  ]);  // L
    *((int16_t*)&dstBuf[4*i+2]) += *((int16_t*)&srcBuf[4*i+2]);  // R
  }
}

void debug_setup() {
  char carDataPath[] = "/carParams_tobu100.json";
  carData.setCarDataFromFile(carDataPath);
  
  // --- ジョイント音 ---
  // 音源の追加
#ifdef ARDUINO_ARCH_ESP32
  // ESP32でSDカードから読み込む場合の処理
#else
  char fileName[] = "../data_in_SD/4-3-1_24.915kmh_encoded_2.wav";
  FILE* fp = fopen(fileName, "rb");
  uint8_t waste[40];
  uint32_t dataSize;
  uint8_t* data;
  fread(waste, 1, 40, fp);
  fread(&dataSize, 4, 1, fp);
  data = new uint8_t[dataSize];
  // printf("%d\n",dataSize);
  fread(data, 1, dataSize, fp);
  fclose(fp);
#endif
  jointSound.addSoundSource(0, 24.9, 12.0, 0.7, 1.0, data, dataSize);

  // ジョイントの追加
  jointSound.addJoint(0, -10.0);
  jointSound.addJoint(0, -35.0);
  jointSound.addJoint(0, -60.0);

  // 車輪の追加
  jointSound.addWheel(-CAR_L + CAR_D / 2 - CAR_W / 2 - PERSON_POS, 1.0, ALPHA_WALL);
  jointSound.addWheel(-CAR_L + CAR_D / 2 + CAR_W / 2 - PERSON_POS, 1.0, ALPHA_WALL);
  jointSound.addWheel(-CAR_D / 2 - CAR_W / 2 - PERSON_POS, 1.0, 1.0);
  jointSound.addWheel(-CAR_D / 2 + CAR_W / 2 - PERSON_POS, 0.98, 1.0);
  jointSound.addWheel(CAR_D / 2 - CAR_W / 2 - PERSON_POS, 1.0, 1.0);
  jointSound.addWheel(CAR_D / 2 + CAR_W / 2 - PERSON_POS, 0.97, 1.0);
  jointSound.addWheel(CAR_L - CAR_D / 2 - CAR_W / 2 - PERSON_POS, 1.0, ALPHA_WALL);
  jointSound.addWheel(CAR_L - CAR_D / 2 + CAR_W / 2 - PERSON_POS, 1.0, ALPHA_WALL);

  jointSound.setVolume(8000);

  // --- VVVF音 ---
  vvvfSound.setVolume(2000);
  vvvfSound.setCutoffFreq(1000);

  // --- モーター音 ---
  motorSound.setVolume(2000);
  motorSound.setEngagementPlay(true);

  // 音を出してみる
  float duration = 30.0;
  size_t outSize = duration * SAMPLINGRATE * 4;
  uint8_t* buf = new uint8_t[outSize];
  uint8_t* output = new uint8_t[outSize];
  float* speed = new float[outSize/4];
  for (int i = 0; i < outSize; i++) {
    buf[i] = 0x0;
    output[i] = 0x0;
    speed[i/4] = T_SAMPLE * i/4  * carData._acc0 + 1;
    if (speed[i/4] > 80.0) {
      speed[i/4] = 80.0;
    }
  }

  motorSound.generateSound(buf, outSize, speed);
  addPCMBuf(buf, output, outSize);

  jointSound.generateSound(buf, outSize, speed);
  addPCMBuf(buf, output, outSize);

  vvvfSound.generateSound(buf, outSize, speed);
  addPCMBuf(buf, output, outSize);

  fp = fopen("out.raw", "wb");
  fwrite(output, 1, outSize, fp);
  fclose(fp);

  // delete data;
  delete output;
  delete speed;
  
}

void debug_loop() {}

#ifndef ARDUINO_ARCH_ESP32

int main(int argc, char** argv) {
  /*if (argc != 3) {
    return 0;
  }

  duration = atof(argv[1]);
  speed = atof(argv[2]);
  if (duration <= 0 || speed <= 0) {
    return 0;
  }
  */

  debug_setup();
  debug_loop();
}

#endif
