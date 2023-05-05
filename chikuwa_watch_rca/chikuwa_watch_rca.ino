//
// ちくわウォッチ M5 Stack Core2用 (RCAビデオ出力可能)
//
// © 2023 CHIKUWA TEIKOKU

#include <M5Unified.h>
#include <M5UnitRCA.h>
#include <WiFi.h>
#include <time.h>
#include <sntp.h>

// ご自身のWiFi環境に合わせて書き換えてください。
#define WIFI_SSID           "******"
#define WIFI_PASSWORD       "******"

// 日本じゃない場合は書き換えてください。
#define NTP_TIMEZONE        "JST-9"
#define NTP_SERVER1         "ntp.nict.jp"
#define NTP_SERVER2         "ntp.jst.mfeed.ad.jp"
#define NTP_SERVER3         "0.pool.ntp.org"

// 好きな色に変えてもいいです。
#define WATCHFACE_FCOLOR_1  WHITE
#define WATCHFACE_BCOLOR_1  BLUE
#define MOMOMARU_COLOR_1    MAGENTA
#define WATCHFACE_FCOLOR_2  LIGHTGREY
#define WATCHFACE_BCOLOR_2  BLACK
#define MOMOMARU_COLOR_2    MAGENTA
#define WATCHFACE_FCOLOR_3  LIGHTGREY
#define WATCHFACE_BCOLOR_3  BLACK
#define MOMOMARU_COLOR_3    ORANGE

// オリジナルキャラに変えちゃうのもありです
// 横16 x 縦11 x 2パターンです。1の部分がドットとして描画されます。
int momomaru_ptn[2][11][16] =
{
  {
    { 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 },
    { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0 },
    { 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0 },
    { 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0 },
    { 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0 },
    { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }
  },
  {
    { 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 },
    { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0 },
    { 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0 },
    { 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0 },
    { 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }
  }
};

// MSX0の向きに慣れたので180°回転させています。
// RCAモジュールを付けた場合は電源アダプターの穴が上にきます。
#define LCD_ROTATION    3

// 25x15
#define FONT_WIDTH_S    12
#define FONT_HEIGHT_S   16
// 10x5
#define FONT_WIDTH_M    32
#define FONT_HEIGHT_M   48
// 5x2.5
#define FONT_WIDTH_L    64
#define FONT_HEIGHT_L   96

M5UnitRCA gfx_rca( 320  // logical_width
                  ,250  // logical_height
                  ,320  // output_width
                  ,250  // output_height
                  ,M5UnitRCA::signal_type_t::NTSC_J  // signal_type
                  ,M5UnitRCA::use_psram_t::psram_half_use  // use_psram
                  ,26  // GPIO pin
                  ,200  // output level
);

uint16_t watchface_fcolor = WATCHFACE_FCOLOR_1;
uint16_t watchface_bcolor = WATCHFACE_BCOLOR_1;
uint16_t momomaru_color = MOMOMARU_COLOR_1;

int momomaru_anim_ptn = 0;

int redraw_wait_counter = 0;

int color_mode = 0;
bool pressed = false;

const char* week[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

int wifi_retry_count = 0;


//
// インターネットから時刻を取得してRTCに記憶します。
//
void get_internet_time()
{
  configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

  M5.Lcd.print("NTP connecting");

  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED)
  {
    M5.Lcd.print(".");
    delay(1000);
  }
  M5.Lcd.println("\nNTP connected.");

  time_t t = time(nullptr) + 1;
  while (t > time(nullptr));
  M5.Rtc.setDateTime(localtime(&t));
}

//
// WiFiに接続します。
// 何回か失敗したらやめます。
// 接続に成功したときだけインターネット時刻を取得しにいきます。
//
void setup_wifi()
{
  WiFi.disconnect();
  delay(500);
//  WiFi.begin(ssid, password);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  M5.Lcd.setTextSize(2);
  M5.Lcd.print("WiFi connecting");

  while (1)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      M5.Lcd.println("\nConnected.");
      get_internet_time();
      break;
    }
    M5.Lcd.print(".");
    wifi_retry_count++;
    if (wifi_retry_count > 50) break;
    delay(500);
  }

  WiFi.disconnect();
}

//
// 左上のモモ丸くんを描画します。
//
void draw_momomaru()
{
  for (int y = 0; y < 11; y++)
  {
    for (int x = 0; x < 16; x++)
    {
      if (momomaru_ptn[momomaru_anim_ptn][y][x] == 1)
      {
        M5.Lcd.fillRect(10 + x * 8, 10 + y * 8, 8, 8, momomaru_color);
        gfx_rca.fillRect(10 + x * 8, 10 + y * 8, 8, 8, momomaru_color);
      }
      else
      {
        M5.Lcd.fillRect(10 + x * 8, 10 + y * 8, 8, 8, watchface_bcolor);
        gfx_rca.fillRect(10 + x * 8, 10 + y * 8, 8, 8, watchface_bcolor);
      }
    }
  }
}

//
// 文字情報をLCDに表示します。
//
void draw_watchface_lcd()
{
  auto rtc_date_time = M5.Rtc.getDateTime();

  M5.Lcd.setTextColor(watchface_fcolor, watchface_bcolor);

  M5.Lcd.setCursor(142 + 0, 32, 1);
  M5.Lcd.setTextSize(2);
  M5.Lcd.printf("%04d/%02d/", rtc_date_time.date.year, rtc_date_time.date.month);

  M5.Lcd.setCursor(142 + 96, 32, 7);
  M5.Lcd.setTextSize(1);
  M5.Lcd.printf("%02d", rtc_date_time.date.date);

  M5.Lcd.setCursor(142 + 96 + 16, 32 + 50, 1);
  M5.Lcd.setTextSize(2);
  M5.Lcd.printf("%s", week[rtc_date_time.date.weekDay]);

  M5.Lcd.setCursor(24, 96 + 16, 7);
  M5.Lcd.setTextSize(2);
  M5.Lcd.printf("%02d:%02d", rtc_date_time.time.hours, rtc_date_time.time.minutes);

  M5.Lcd.setCursor(24, 224, 1);
  M5.Lcd.setTextSize(2);
  M5.Lcd.print("(C)2023 CHIKUWA TEIKOKU");

  M5.Lcd.setCursor(236, 0, 1);
  M5.Lcd.setTextSize(2);
  M5.Lcd.printf("B%3d%%", M5.Power.getBatteryLevel());
}

//
// 文字情報をRCAに表示します。
//
void draw_watchface_rca()
{
  auto rtc_date_time = M5.Rtc.getDateTime();

  gfx_rca.setTextColor(watchface_fcolor, watchface_bcolor);

  gfx_rca.setCursor(142 + 0, 32, 1);
  gfx_rca.setTextSize(2);
  gfx_rca.printf("%04d/%02d/", rtc_date_time.date.year, rtc_date_time.date.month);

  gfx_rca.setCursor(142 + 96, 32, 7);
  gfx_rca.setTextSize(1);
  gfx_rca.printf("%02d", rtc_date_time.date.date);

  gfx_rca.setCursor(142 + 96 + 16, 32 + 50, 1);
  gfx_rca.setTextSize(2);
  gfx_rca.printf("%s", week[rtc_date_time.date.weekDay]);

  gfx_rca.setCursor(24, 96 + 16, 7);
  gfx_rca.setTextSize(2);
  gfx_rca.printf("%02d:%02d", rtc_date_time.time.hours, rtc_date_time.time.minutes);

  gfx_rca.setCursor(24, 224, 1);
  gfx_rca.setTextSize(2);
  gfx_rca.print("(C)2023 CHIKUWA TEIKOKU");

  gfx_rca.setCursor(240, 4, 1);
  gfx_rca.setTextSize(2);
  gfx_rca.printf("B%3d%%", M5.Power.getBatteryLevel());
}

//
// 起動時に1回だけ実行されます。
//
void setup()
{
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  cfg.output_power = true;
  cfg.external_rtc  = true;
  M5.begin(cfg);

  M5.Lcd.setRotation(LCD_ROTATION);

  M5.Lcd.fillScreen(watchface_bcolor);
  M5.Lcd.setTextColor(watchface_fcolor, watchface_bcolor);

  setup_wifi();

  M5.Lcd.fillScreen(watchface_bcolor);

  gfx_rca.init();
  gfx_rca.fillScreen(watchface_bcolor);
  gfx_rca.setTextColor(watchface_fcolor, watchface_bcolor);

  draw_momomaru();
  draw_watchface_lcd();
  draw_watchface_rca();
}

//
// 動いている間ずっと実行されます。
//
void loop()
{
  vTaskDelay(1);
  M5.update();

  if (M5.Touch.getDetail().isPressed())
  {
    if (!pressed)
    {
      pressed = true;
      color_mode++;
      if (color_mode > 2) color_mode = 0;

      switch (color_mode)
      {
        case 0:
          watchface_fcolor = WATCHFACE_FCOLOR_1;
          watchface_bcolor = WATCHFACE_BCOLOR_1;
          momomaru_color = MOMOMARU_COLOR_1;
          break;
        case 1:
          watchface_fcolor = WATCHFACE_FCOLOR_2;
          watchface_bcolor = WATCHFACE_BCOLOR_2;
          momomaru_color = MOMOMARU_COLOR_2;
          break;
        case 2:
          watchface_fcolor = WATCHFACE_FCOLOR_3;
          watchface_bcolor = WATCHFACE_BCOLOR_3;
          momomaru_color = MOMOMARU_COLOR_3;
          break;
      }

      M5.Lcd.fillScreen(watchface_bcolor);
      gfx_rca.fillScreen(watchface_bcolor);
      draw_momomaru();
      draw_watchface_lcd();
      draw_watchface_rca();
    }
  }
  else
  {
    pressed = false;
  }

  redraw_wait_counter++;

  if (redraw_wait_counter >= 10)
  {
    redraw_wait_counter = 0;
    momomaru_anim_ptn = 1 - momomaru_anim_ptn;
    draw_momomaru();
    draw_watchface_lcd();
    draw_watchface_rca();
  }

  vTaskDelay(50);
}