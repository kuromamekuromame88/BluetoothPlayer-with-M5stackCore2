#include <Arduino.h>
#include <M5Core2.h>
#define LGFX_AUTODETECT         // 自動認識
#define LGFX_USE_V1             // LovyanGFX v1.0.0を有効に
#include <LovyanGFX.hpp>        // LovyanGFXのヘッダを準備
#include <LGFX_AUTODETECT.hpp>
#include <BluetoothA2DPSink.h>

LGFX lcd;  // LovyanGFXインスタンス

BluetoothA2DPSink a2dp_sink;
boolean pauseMode = false;
int volume = 30;
int lastvolume;
int lastStatus = 1;
String currentTitle = "";
String currentArtist = "";

// メタデータコールバック
void avrc_metadata_callback(uint8_t attr_id, const uint8_t *text) {
  switch (attr_id) {
    case ESP_AVRC_MD_ATTR_TITLE:
      currentTitle = String((const char *)text);
      if(!currentTitle) currentTitle = "Not found";
      Serial.printf("曲名: ");
      Serial.println(currentTitle);
      break;
    case ESP_AVRC_MD_ATTR_ARTIST:
      currentArtist = String((const char *)text);
      if(!currentArtist) currentArtist = "Not found";
      Serial.printf("アーティスト名: ");
      Serial.println(currentArtist);
      break;
  }
}

// UI描画
void drawUI() {
  lcd.fillScreen(TFT_BLACK);

  // タイトル
  lcd.setTextFont(&fonts::Font0);
  lcd.setTextSize(2);
  lcd.setTextColor(TFT_CYAN);
  lcd.setCursor(10, 10);
  lcd.println("M5Core2 Bluetooth Speaker");

  lcd.setTextSize(1);
  lcd.setTextColor(TFT_WHITE);
  lcd.drawLine(0, 35, 320, 35, TFT_DARKGREY);

  // 曲名・アーティスト表示
  lcd.setCursor(10, 50);
  lcd.setTextFont(&fonts::lgfxJapanGothic_16);
  lcd.printf("曲名 : %s\n", currentTitle.c_str());
  lcd.setCursor(10, 80);
  lcd.printf("アーティスト: %s\n", currentArtist.c_str());

  lcd.drawLine(0, 110, 320, 110, TFT_DARKGREY);

  // 音量表示
  lcd.setCursor(10, 130);
  lcd.printf("音量: %3d%%", volume);

  // 音量バー
  int barWidth = map(volume, 0, 100, 0, 200);
  lcd.drawRect(100, 130, 200, 10, TFT_WHITE);
  lcd.fillRect(100, 130, barWidth, 10, TFT_GREEN);

  lcd.setCursor(30, 200);
  lcd.printf(pauseMode ? "再生": "停止");

  lcd.setCursor(145, 200);
  lcd.printf("音＋");

  lcd.setCursor(250, 200);
  lcd.printf("音ー");
}

//音量バー操作
void updateVolumeBar() {
  int barWidth = map(volume, 0, 100, 0, 200);
  lcd.fillRect(60, 130, 240, 20, TFT_BLACK);
  lcd.drawRect(100, 130, 200, 10, TFT_WHITE);
  lcd.fillRect(100, 130, barWidth, 10, TFT_GREEN);
  lcd.setFont(&fonts::lgfxJapanGothic_16);
  lcd.setCursor(10, 130);
  lcd.printf("音量: %3d%% ", volume);
}

void updatePlayingStatus(int Status) {
  lcd.fillRect(30, 200, 30, 20, TFT_BLACK);
  lcd.setCursor(30, 200);
  switch (Status){
    case 2:
      lcd.printf("停止");
      break;
    case 0:
    case 1:
      lcd.printf("再生");
      break;
    default:
      lcd.printf("Error");
  }
}

void updatePlayingMusicInfo() {
  // 曲名・アーティスト表示
  lcd.fillRect(10, 50, 300, 45, TFT_BLACK);
  lcd.setCursor(10, 50);
  lcd.setTextFont(&fonts::lgfxJapanGothic_16);
  lcd.printf("曲名 : %s\n", currentTitle.c_str());
  lcd.setCursor(10, 80);
  lcd.printf("アーティスト: %s\n", currentArtist.c_str());
}

void setup() {
  M5.begin();
  M5.Axp.SetSpkEnable(true);
  Serial.begin(115200);

  lcd.init();
  lcd.setBrightness(128);
  lcd.fillScreen(TFT_BLACK);

  // 内蔵NS4168用 I2S設定
  i2s_pin_config_t i2s_pin_config = {
    .bck_io_num   = 12,
    .ws_io_num    = 0,
    .data_out_num = 2,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };
  a2dp_sink.set_pin_config(i2s_pin_config);

  // メタデータコールバック
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.set_auto_reconnect(true);
  a2dp_sink.start("M5Core2 Speaker");
  lcd.setTextFont(&fonts::lgfxJapanGothic_16);
  lcd.setTextColor(TFT_WHITE);
  lcd.setCursor(20, 20);
  lcd.printf("接続待機中");
  while(!a2dp_sink.get_connection_state()) {
    lcd.printf(".");
    delay(1000);
  }
  lcd.printf("接続完了！");
  delay(500);
  drawUI();
  a2dp_sink.set_volume(volume);
}

void loop() {
  M5.update();

  // Aボタン: 再生／一時停止
  if (M5.BtnA.wasPressed()) {
    if(!pauseMode) {
      a2dp_sink.pause();
      Serial.println("一時停止");
      pauseMode = true;
      updatePlayingStatus(0);
    }else {
      a2dp_sink.play();
      Serial.println("再生");
      pauseMode = false;
      updatePlayingStatus(2);
    }
  }

  // Bボタン: 音量UP
  if (M5.BtnB.wasPressed()) {
    volume = min(100, volume + 5);
    a2dp_sink.set_volume(volume);
    updateVolumeBar();
  }

  // Cボタン: 音量DOWN
  if (M5.BtnC.wasPressed()) {
    volume = max(0, volume - 5);
    a2dp_sink.set_volume(volume);
    updateVolumeBar();
  }

  // 曲情報が更新されたら再描画
  static String lastTitle = "";
  static String lastArtist = "";
  if (lastTitle != currentTitle || lastArtist != currentArtist) {
    lastTitle = currentTitle;
    lastArtist = currentArtist;
    updatePlayingMusicInfo();
  }

  //デバイスとの音量の同期
  if(volume != a2dp_sink.get_volume()){
    volume = a2dp_sink.get_volume();
    updateVolumeBar();
  }

  //再生ステータスのUIの同期
  if(lastStatus != a2dp_sink.get_audio_state()){
    updatePlayingStatus(a2dp_sink.get_audio_state());
    lastStatus = a2dp_sink.get_audio_state();
  }

  delay(50);
}
