/* NOTE:
 * - epd power not used in the PDLS library version used
 */
#include "config.h"
#include "config_gpio.h"
#include "config_wifi.h"
#include "data_handling.h"
#include "draw.h"
#include "graphics.h"
#include "sd_card.h"
#include <AiEsp32RotaryEncoder.h>
#include <Arduino.h>
#include <ArduinoLog.h>
#include <ESPNtpClient.h>
#include <PDLS_EXT3_Basic_Global.h>
#include <SPI.h>
#include <SensirionI2cSen66.h>
#include <SparkFunBQ27441.h>
#include <WiFi.h>
#include <Wire.h>
#include <cstdint>
#include <esp_wifi.h>
#include <hV_Configuration.h>
#include <hV_List_Screens.h>

char buf[BUF_SIZE] = {0};
extern screen_t screen;
extern uint8_t past_threshold_idx;
extern const uint8_t past_threshold_cnt;

static bool setup_wifi();
static void setup_pins(void);
static bool setup_battery_fuel_gauge(BQ27441 &bat, TwoWire &i2c_inst);
static bool setup_sen66(SensirionI2cSen66 &Sen66_local, TwoWire &i2c_inst);
void setup_epd(Screen_EPD_EXT3 &myScreen);
void setup_encoder();

void on_button_short_click();
void on_button_long_click();
void handle_rotary_button();
void rotary_loop();
void IRAM_ATTR isr_read_encoder();
static inline void epd_force_refresh();

bool find_abs_range(float &min, float &max);

SensirionI2cSen66 Sen66;
BQ27441 Battery;
TwoWire i2c = TwoWire(0);
Screen_EPD_EXT3 myScreen(eScreen_EPD_266_JS_0C, epd_pins);
Sen66Values sen66Values;
AiEsp32RotaryEncoder encoder = AiEsp32RotaryEncoder(
    ROTARY_ENCODER_A, ROTARY_ENCODER_B, ROTARY_ENCODER_BTN, -1, 2);

void setup()
{
  Serial.begin(SERIAL_BAUDRATE);

  Log.setPrefix(print_prefix);
  Log.setSuffix(print_suffix);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial, true, true);
  Log.verboseln("Setup start");

  setup_pins();

  if (!i2c.begin(I2C_SDA, I2C_SCL, I2C_CLOCK)) {
    Log.errorln("Unable to initialize I2C bus");
    return;
  }

  if (!setup_sen66(Sen66, i2c)) {
    Log.errorln("Unable to setup SEN66");
    return;
  }

  if (!setup_battery_fuel_gauge(Battery, i2c)) {
    Log.errorln("Unable to setup battery fuel gauge");
    return;
  }

  if (!setup_wifi()) {
    return;
  }
  if (!wait_for_wifi_connection()) {
    return;
  }

  NTP.setTimeZone(TZ_Europe_Prague);
  NTP.begin("cz.pool.ntp.org");
  while (NTP.getLastNTPSync() == 0) {
    delay(100);
  }
  Log.infoln("NTP synced");

  setup_epd(myScreen);

  if (!sd_card_in_socket()) {
    Log.infoln("SD card not in socket");
  } else {
    if (!setup_sd_card()) {
      return;
    }
  }

  setup_encoder();

  Log.verboseln("Sen66 measurement started");
  int sen66_error = Sen66.startContinuousMeasurement();
  if (sen66_error != NO_ERROR) {
    char sen66_error_msg[128];
    Log.error("Error trying to execute startContinuousMeasurement: ");
    errorToString(sen66_error, sen66_error_msg, sizeof(sen66_error_msg));
    Log.errorln(sen66_error_msg);
    return;
  }

  Log.verboseln("Setup end");
}

int64_t last_update_epd = 0;
int64_t last_update_sen66 = 0;

void loop()
{
  timeval current_time;
  gettimeofday(&current_time, nullptr);
  if (abs(last_update_sen66 - current_time.tv_sec) > UPDATE_PERIOD_SEN66) {
    if (sen66Values.update()) {
      sen66Values.log();

      if (SD_MMC.cardType() != CARD_NONE) {
        save_data_to_sd_card(sen66Values);
      }

      log_charge_status(Battery);
      last_update_sen66 = current_time.tv_sec;
    }
  }

  if (abs(last_update_epd - current_time.tv_sec) > UPDATE_PERIOD_EPD) {
    draw(myScreen, sen66Values, Battery);
    last_update_epd = current_time.tv_sec;
  }

  if ((SD_MMC.cardType() == CARD_NONE) && sd_card_in_socket()) {
    setup_sd_card();
  }
  if (!sd_card_in_socket()) {
    SD_MMC.end();
  }

  rotary_loop();

  delay(100);
}

static bool setup_wifi()
{
  // power consumption optimization (esp. as it is currently only for ntp)
  esp_wifi_set_ps(WIFI_PS_MAX_MODEM);

  WiFi.disconnect(true); // disconnect form wifi to set new wifi connection
  WiFi.mode(WIFI_STA);

  const uint8_t mac_address[] = MAC_ADDRESS;
  if (!change_mac_address(&mac_address[0])) {
    return false;
  }

  Log.infoln("Connecting to network: %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME,
             EAP_PASSWORD);

  return true;
}

static void setup_pins(void)
{
  pinMode(SD_DET, INPUT_PULLUP);
  pinMode(ALARM_LOW_BAT, INPUT);
  pinMode(EPD_POWER, OUTPUT);
}

static bool setup_battery_fuel_gauge(BQ27441 &Bat, TwoWire &i2c_inst)
{
  if (!Bat.begin(i2c_inst)) {
    Log.errorln("Unable to communicate with BQ27441.");
    return false;
  }

  // Write config params only if needed
  if (Bat.itporFlag()) {
    if (!Bat.setCapacity(BATTERY_INIT_CAPACITY_mAh)) {
      Log.errorln("Unable to set battery init capacity");
      return false;
    }
    if (!Bat.setDesignEnergy(BATTERY_INIT_CAPACITY_mAh *
                             BATTERY_NOMINAL_VOLTAGE_V)) {
      Log.errorln("Unable to set battery design energy");
      return false;
    }
    if (!Bat.setTerminateVoltage(BATTERY_TERMINAL_VOLTAGE_mV)) {
      Log.errorln("Unable to set battery terminate voltage");
      return false;
    }
    // if (!Bat.setTaperRate()){ // not specified in datasheet of BQ25185
    //   Log.errorln("Unable to set battery taper rate");
    //   return false;
    // }
    if (!Bat.exitConfig()) {
      Log.errorln("Unable to exit battery config");
      return false;
    }
  } else {
    Log.infoln("Using existing BQ27441 configuration.");
  }

  print_battery_stats(Battery);

  return true;
}

static bool setup_sen66(SensirionI2cSen66 &Sen66_local, TwoWire &i2c_inst)
{
  Sen66_local.begin(i2c_inst,
                    SEN66_I2C_ADDR_6B); // NOTE: only SEN60 has a different one

  CHECK_SEN66_CALL(Sen66_local.deviceReset());

  Log.infoln("SEN 66 info:");
  int8_t sen66_serial_number[32] = {0};
  CHECK_SEN66_CALL(Sen66_local.getSerialNumber(sen66_serial_number,
                                               sizeof(sen66_serial_number)));
  Log.info("\tserial number: %s\n", (const char *)sen66_serial_number);

  int8_t sen66_product_name[32] = {0};
  CHECK_SEN66_CALL(Sen66_local.getProductName(sen66_product_name,
                                              sizeof(sen66_product_name)));
  Log.info("\tproduct name: %s\n", (const char *)sen66_product_name);

  uint8_t version_major = 0;
  uint8_t version_minor = 0;
  CHECK_SEN66_CALL(Sen66_local.getVersion(version_major, version_minor));
  Log.info("\tversion: %u.%u\n", version_major, version_minor);

  sen66Values.begin(Sen66);

  return true;
}

const uint8_t *select_img_bat(BQ27441 &Bat)
{
  const uint8_t *img_bat = nullptr;
  uint16_t soc = Bat.soc();

  if (Bat.fcFlag()) {
    img_bat = image_battery_full;
  } else if (!Bat.dsgFlag()) {
    img_bat = image_battery_charger_connected;
  } else if (soc < 10) {
    img_bat = image_battery_0;
  } else if (soc < 20) {
    img_bat = image_battery_10;
  } else if (soc < 30) {
    img_bat = image_battery_20;
  } else if (soc < 40) {
    img_bat = image_battery_30;
  } else if (soc < 50) {
    img_bat = image_battery_40;
  } else if (soc < 60) {
    img_bat = image_battery_50;
  } else if (soc < 70) {
    img_bat = image_battery_60;
  } else if (soc < 80) {
    img_bat = image_battery_70;
  } else if (soc < 90) {
    img_bat = image_battery_80;
  } else {
    img_bat = image_battery_90;
  }

  return img_bat;
}

const uint16_t select_color_bat(BQ27441 &Bat)
{
  uint16_t color = myColours.black;
  if (Bat.dsgFlag() && (Bat.soc() < 10)) {
    color = myColours.red;
  }
  return color;
}

const uint8_t *select_img_wifi()
{
  const uint8_t *wifi_img = nullptr;
  if (WiFi.status() != WL_CONNECTED) {
    wifi_img = image_wifi_not_connected;
  } else {
    // TODO:, FIXME: RSSI thresholds are pure guesses
    uint8_t rssi = WiFi.RSSI();
    if (rssi < 100) {
      wifi_img = image_wifi_25;
    } else if (rssi < 50)
      wifi_img = image_wifi_50;
    else {
      wifi_img = image_wifi_100;
    }
  }

  return wifi_img;
}

const uint16_t select_color_wifi()
{
  if (WiFi.status() != WL_CONNECTED) {
    return myColours.red;
  }
  return myColours.black;
}

const uint8_t *select_img_sd_card()
{
  const uint8_t *img_sd = nullptr;
  if (SD_MMC.cardType() == CARD_NONE) {
    img_sd = image_micro_sd_no_card;
  } else {
    img_sd = image_micro_sd;
  }
  return img_sd;
}

const uint16_t select_color_sd_card()
{
  if (SD_MMC.cardType() == CARD_NONE) {
    return myColours.red;
  }
  return myColours.black;
}

inline void epd_turn_off() { digitalWrite(EPD_POWER, LOW); }
inline void epd_turn_on() { digitalWrite(EPD_POWER, HIGH); }

void on_button_short_click()
{
  Log.verboseln("button SHORT press");
  past_threshold_idx = (past_threshold_idx + 1) % past_threshold_cnt;
  epd_force_refresh();
}

static inline void epd_force_refresh() { last_update_epd = 0; }

void on_button_long_click()
{
  Log.verboseln("button LONG press");
  epd_force_refresh();
}

void handle_rotary_button()
{
  static unsigned long lastTimeButtonDown = 0;
  static bool wasButtonDown = false;

  bool isEncoderButtonDown = encoder.isEncoderButtonDown();

  if (isEncoderButtonDown) {
    if (!wasButtonDown) {
      lastTimeButtonDown = millis();
    }
    wasButtonDown = true;
    return;
  }

  if (wasButtonDown) {
    if (millis() - lastTimeButtonDown >= BTN_PRESS_THRESHOLD_LONG) {
      on_button_long_click();
    } else if (millis() - lastTimeButtonDown >= BTN_PRESS_THRESHOLD_SHORT) {
      on_button_short_click();
    }
  }
  wasButtonDown = false;
}

void rotary_loop()
{
  if (encoder.encoderChanged()) {
    Log.verboseln("Encoder value: %d", encoder.readEncoder());
    screen = (screen_t)encoder.readEncoder();
    epd_force_refresh();
  }
  handle_rotary_button();
}

void IRAM_ATTR isr_read_encoder() { encoder.readEncoder_ISR(); }

void setup_epd(Screen_EPD_EXT3 &myScreen)
{
  SPI.begin(SPI_SCLK, -1, SPI_MOSI, -1);
  myScreen.begin();
  Log.infoln("EPD: %s %ix%i\n", myScreen.WhoAmI().c_str(),
             myScreen.screenSizeX(), myScreen.screenSizeY());
}

void setup_encoder()
{
  encoder.begin();
  encoder.setup(isr_read_encoder);
  encoder.setEncoderValue(screen);
  encoder.setBoundaries(0, SCREEN_LAST, true);
  encoder.disableAcceleration();
}
