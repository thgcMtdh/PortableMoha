#include "JointSoundClass.h"

// 車両定数
const float CAR_L = 20.0;      // 車両長[m]
const float CAR_D = 13.8;      // 台車間距離[m]
const float CAR_W = 2.1;       // 車軸間距離[m]
const float PERSON_POS = 6.0;  // 聴取者が車両中心から何mの場所にいるか[m]
const float EAR_HEIGHT = 2.0;  // レール面(音源)から耳までの距離[m]
const float ALPHA_WALL = 0.2;  // 壁の向こうの車輪からの音は何倍になるか

float duration = 10;
float speed = 30;

JointSoundClass jointSound(EAR_HEIGHT, true);

void printBuf(uint8_t* buf, int SAMPLENUM) {
  for (int i = 0; i < SAMPLENUM; i++) {
    printf("%d, %d\n", *((int16_t*)&buf[4 * i]), *((int16_t*)&buf[4 * i + 2]));
  }
}

void debug_setup() {
  // 音源の追加

  char fileName[] = "4-3-1_24.915kmh_encoded.wav";
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

  jointSound.addSoundSource(0, 24.9, 12.0, 0.5, 1.0, data, dataSize);

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

  // 音を出してみる
  size_t outSize = duration * SAMPLINGRATE * 4;
  uint8_t* output;
  output = new uint8_t[outSize];
  for (int i = 0; i < outSize; i++) {
    output[i] = 0x0;
  }
  jointSound.generateSound(output, outSize, speed);
  // printf("output=\n");
  // printBuf(output, outSize/4);

  fp = fopen("out.raw", "wb");
  fwrite(output, 1, outSize, fp);
  fclose(fp);

  delete data;
  delete output;
}

void debug_loop() {}

#ifndef ARDUINO_ARCH_ESP32

int main(int argc, char** argv) {
  if (argc != 3) {
    return 0;
  }

  duration = atof(argv[1]);
  speed = atof(argv[2]);
  if (duration <= 0 || speed <= 0) {
    return 0;
  }

  debug_setup();
  debug_loop();
}

#endif
