#include "CarDataClass.h"

#ifdef ARDUINO_ARCH_ESP32
#include <ArduinoJson.h>
#else
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <iostream>
#include <string>

#include "nlohmann/json.hpp"
#endif

CarDataClass::CarDataClass() {
  clearCarData();
}
CarDataClass::~CarDataClass() {
  delete _carDataStr;
}

int CarDataClass::setCarDataFromFile(char* carDataPath) {
#ifdef ARDUINO_ARCH_ESP32
  // ESP32でSDカードからファイルを開く

  // JSON形式の文字列をparseする
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, _carDataStr);
  if (error) {
    Serial.println(error.c_str());
    return 0;
  }

  // 転記する

#else
  // ファイルパスを設定しファイルを開く
  size_t pathLen = strlen("../data_in_SD") + strlen(carDataPath) + 1;  // ファイルパスの長さを取得(+1はnull文字ぶん)
  char* filePath = new char[pathLen];                                  // パスが入るメモリ領域を確保
  snprintf(filePath, pathLen, "../data_in_SD%s", carDataPath);         // 親フォルダの名前をパスに付加
  FILE* fp = fopen(filePath, "rb");                                    // ファイルを開く
  if (!fp) {
    printf("couldn't open file");
    delete[] filePath;
    return 0;
  }

  // ファイルサイズ取得
  size_t fileSize = 0;  // 読み込んだCarDataファイルのサイズ(バイナリとしてのサイズなのでnull文字は含まない)
  struct stat statBuf;
  if (stat(filePath, &statBuf) == 0) {
    fileSize = statBuf.st_size;
  } else {
    printf("file size get failure");
    return 0;
  }
  delete[] filePath;

  // ファイルに記録されたデータを_carDataStrにコピー
  _carDataStr = new char[fileSize + 1];  // ファイルサイズ+null文字ぶんのメモリ領域を確保
  memset(_carDataStr, 0, fileSize + 1);  // ゼロ埋め
  fread(_carDataStr, 1, fileSize, fp);   // コピー
  fclose(fp);                            // 閉じる
  
  // Windows上ではArduinoJsonのdeserializeが動かないのでこちらを使ってparseする
  std::string str(_carDataStr);
  nlohmann::json j = nlohmann::json::parse(str);

  // 転記する
  _wheelDiameter = j["wheelDiameter"].get<float>();
  _smallGear = j["smallGear"].get<int>();
  _largeGear = j["largeGear"].get<int>();
  _pole = j["pole"].get<int>();
  _acc0 = j["acc0"].get<float>();
  _brk0 = j["brk0"].get<float>();
  _regenLostFreq = j["regenLostFreq"].get<float>();
  _modulationMax = j["modulationMax"].get<float>();
  _modulationMaxFreq = j["modulationMaxFreq"].get<float>();
  _pmNum = j["pulseMode"].size();
  for (size_t i = 0; i < _pmNum; i++) {
    _listFs[i]     = !j["pulseMode"][i]["fs"].is_null()     ? j["pulseMode"][i]["fs"].get<float>()     : 0;
    _listFc1[i]    = !j["pulseMode"][i]["fc1"].is_null()    ? j["pulseMode"][i]["fc1"].get<float>()    : 0;
    _listFc2[i]    = !j["pulseMode"][i]["fc2"].is_null()    ? j["pulseMode"][i]["fc2"].get<float>()    : 0;
    _listFrand1[i] = !j["pulseMode"][i]["frand1"].is_null() ? j["pulseMode"][i]["frand1"].get<float>() : 0;
    _listFrand2[i] = !j["pulseMode"][i]["frand2"].is_null() ? j["pulseMode"][i]["frand2"].get<float>() : 0;
    _listMode[i]   = !j["pulseMode"][i]["mode"].is_null()   ? j["pulseMode"][i]["mode"].get<int>()     : 0;
    _listNpulse[i] = !j["pulseMode"][i]["Npulse"].is_null() ? j["pulseMode"][i]["Npulse"].get<int>()   : 0;
  }
#endif

  return 1;
}

void CarDataClass::clearCarData() {
  delete _carDataStr;
  _wheelDiameter = 0;
  _smallGear = 0;
  _largeGear = 0;
  _pole = 0;
  _acc0 = 0;
  _brk0 = 0;
  _regenLostFreq = 0;
  _modulationMax = 0;
  _modulationMaxFreq = 0;
  _pmNum = 0;
  for (size_t i = 0; i < CARDATA_MAX_PULSEMODE_NUM; i++) {
    _listFs[i]     = 0;
    _listFc1[i]    = 0;
    _listFc2[i]    = 0;
    _listFrand1[i] = 0;
    _listFrand2[i] = 0;
    _listMode[i]   = 0;
    _listNpulse[i] = 0;
  }
}
