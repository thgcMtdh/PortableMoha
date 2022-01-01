#pragma once

#include "constant.h"

class CarDataClass {
 private:
  static const size_t CARDATA_MAX_NAME_SIZE = 32;
  static const size_t CARDATA_MAX_PULSEMODE_NUM = 16;

 public:
  CarDataClass();
  ~CarDataClass();
  
  /// @brief 指定されたパスからJSONファイルを読み込み、車両データを設定する
  /// @param[in] carDataPath SDカードのrootから見たJSONファイルへのパス. たとえば "/cardata/JRxxxSeries.json" など.
  ///                        先頭にrootを表す "/" をつけ忘れないよう注意
  /// @retval 1:success, 0:failure
  int setCarDataFromFile(char* carDataPath);

  /// @brief セットされている車両データを消去する
  void clearCarData();


  // 車両データを表すjson文字列へのポインタ
  char* _carDataStr;

  // 駆動系や性能に関するデータ
  float _wheelDiameter;
  int _smallGear;
  int _largeGear;
  int _pole;
  float _acc0;
  float _brk0;
  float _regenLostFreq;
  float _modulationMax;
  float _modulationMaxFreq;

  // パルスモードに関するデータ
  size_t _pmNum = 0;  // パルスモードの数
  float _listFs[CARDATA_MAX_PULSEMODE_NUM];
  float _listFc1[CARDATA_MAX_PULSEMODE_NUM];
  float _listFc2[CARDATA_MAX_PULSEMODE_NUM];
  float _listFrand1[CARDATA_MAX_PULSEMODE_NUM];
  float _listFrand2[CARDATA_MAX_PULSEMODE_NUM];
  int _listMode[CARDATA_MAX_PULSEMODE_NUM];
  int _listNpulse[CARDATA_MAX_PULSEMODE_NUM];
};

  typedef enum Mode {
    ASYNC,
    SYNC,
    SYNC_W3P,
    CARDATA_MODE_NUM
  } Mode;
