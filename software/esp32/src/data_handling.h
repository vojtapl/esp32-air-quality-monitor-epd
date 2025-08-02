#pragma once
#include "ESPNtpClient.h"
#include "SD_MMC.h"
#include "config.h"
#include "sd_card.h"
#include "utils.h"
#include <ArduinoLog.h>
#include <SensirionI2cSen66.h>
#include <cstdint>
#include <hV_Colours565.h>
extern char buf[BUF_SIZE];

#define DATA_LINE_LEN 88

typedef enum {
  DRAW_IDX_PM1,
  DRAW_IDX_PM2P5,
  DRAW_IDX_PM4,
  DRAW_IDX_PM10,
  DRAW_IDX_HUMIDITY,
  DRAW_IDX_TEMPERATURE,
  DRAW_IDX_VOC,
  DRAW_IDX_NOX,
  DRAW_IDX_CO2,
} draw_idx_t;

bool val_is_invalid_pm(float val);
bool val_is_invalid_humidity(float val);
bool val_is_invalid_temperature(float val);
bool val_is_invalid_index(uint16_t val);
bool val_is_invalid_co2(uint16_t val);
bool get_data_invalid(draw_idx_t type, float val);

int get_draw_unit(char *fmt_buffer, draw_idx_t idx);
int get_draw_name(char *fmt_buffer, draw_idx_t idx);

class Sen66Values
{

public:
  float pm1 = 0.;
  float pm2p5 = 0.;
  float pm4 = 0.;
  float pm10 = 0.;
  float humidity = 0.;
  float temperature = 0.;
  uint16_t voc = 0.;
  uint16_t nox = 0.;
  uint16_t co2 = 0.;
  SensirionI2cSen66 sen66;

  Sen66Values() {};

  void begin(SensirionI2cSen66 sen66) { this->sen66 = sen66; }

  int get_str(char *fmt_buffer)
  {
    int len = sprintf(
        fmt_buffer,
        "%04.1f, %04.1f, %04.1f, %04.1f, %04.2f, %05.3f, %03u, %03u, %05u\n",
        pm1, pm2p5, pm4, pm10, humidity, temperature, voc, nox, co2);
    return len;
  }

  bool get_data_invalid(draw_idx_t idx)
  {
    switch (idx) {
    case DRAW_IDX_PM1:
      return val_is_invalid_pm(pm1);
      break;
    case DRAW_IDX_PM2P5:
      return val_is_invalid_pm(pm2p5);
      break;
    case DRAW_IDX_PM4:
      return val_is_invalid_pm(pm4);
      break;
    case DRAW_IDX_PM10:
      return val_is_invalid_pm(pm10);
      break;
    case DRAW_IDX_HUMIDITY:
      return val_is_invalid_humidity(humidity);
      break;
    case DRAW_IDX_TEMPERATURE:
      return val_is_invalid_temperature(temperature);
      break;
    case DRAW_IDX_VOC:
      return val_is_invalid_index(voc);
      break;
    case DRAW_IDX_NOX:
      return val_is_invalid_index(nox);
      break;
    case DRAW_IDX_CO2:
      return val_is_invalid_co2(co2);
      break;
    }
    return false;
  }

  void log(void)
  {
    Log.infoln("SEN66 vals:");
    Log.info("\tPM1               = ");
    Serial.printf("%.1f μg/m³\n", pm1);
    Log.info("\tPM2.5             = ");
    Serial.printf("%.1f μg/m³\n", pm2p5);
    Log.info("\tPM4               = ");
    Serial.printf("%.1f μg/m³\n", pm4);
    Log.info("\tPM10              = ");
    Serial.printf("%.1f μg/m³\n", pm10);
    Log.info("\trelative humidity = ");
    Serial.printf("%.2f %%\n", humidity);
    Log.info("\ttemperature       = ");
    Serial.printf("%.3f °C\n", temperature);
    Log.info("\tVOC index         = ");
    Serial.printf("%u\n", voc);
    Log.info("\tNOx index         = ");
    Serial.printf("%u\n", nox);
    Log.info("\tCO2               = ");
    Serial.printf("%u ppm\n", co2);
  }

  bool update()
  {
    // TODO: this or readNumberConcentrationValues (i.e. with scaling)
    float tmp_voc = 0;
    float tmp_nox = 0;

    CHECK_SEN66_CALL(sen66.readMeasuredValues(
        pm1, pm2p5, pm4, pm10, humidity, temperature, tmp_voc, tmp_nox, co2));

    this->voc = uint(round(tmp_voc));
    this->nox = uint(round(tmp_nox));

    // one should suffice
    if (get_data_invalid(DRAW_IDX_CO2)) {
      Log.warningln("Invalid sen66 vals (sometimes happens on startup)");
      return false;
    }
    return true;
  }

  int get_draw_value(char *fmt_buffer, draw_idx_t idx)
  {
    int ret = 0;

    switch (idx) {
    case DRAW_IDX_PM1:
      ret = sprintf(fmt_buffer, "%.1f", pm1);
      break;
    case DRAW_IDX_PM2P5:
      ret = sprintf(fmt_buffer, "%.1f", pm2p5);
      break;
    case DRAW_IDX_PM4:
      ret = sprintf(fmt_buffer, "%.1f", pm4);
      break;
    case DRAW_IDX_PM10:
      ret = sprintf(fmt_buffer, "%.1f", pm10);
      break;
    case DRAW_IDX_HUMIDITY:
      ret = sprintf(fmt_buffer, "%.2f", humidity);
      break;
    case DRAW_IDX_TEMPERATURE:
      ret = sprintf(fmt_buffer, "%.2f", temperature);
      break;
    case DRAW_IDX_VOC:
      ret = sprintf(fmt_buffer, "%u", voc);
      break;
    case DRAW_IDX_NOX:
      ret = sprintf(fmt_buffer, "%u", nox);
      break;
    case DRAW_IDX_CO2:
      ret = sprintf(fmt_buffer, "%u", co2);
      break;
    }
    return ret;
  }

  uint16_t get_draw_color(uint8_t idx)
  {
    hV_Colours565 colors = hV_Colours565();
    uint16_t color = colors.black;
    float humidex = 0;

    switch (idx) {
    case DRAW_IDX_PM1:
      if (pm1 < THRESH_BADNESS_1_PM1) {
        color = colors.black;
      } else if (pm1 < THRESH_BADNESS_2_PM1) {
        color = colors.lightRed;
      } else {
        color = colors.red;
      }
      break;
    case DRAW_IDX_PM2P5:
      if (pm2p5 < THRESH_BADNESS_1_PM2P5) {
        color = colors.black;
      } else if (pm2p5 < THRESH_BADNESS_2_PM2P5) {
        color = colors.lightRed;
      } else {
        color = colors.red;
      }
      break;
    case DRAW_IDX_PM4:
      if (pm4 < THRESH_BADNESS_1_PM4) {
        color = colors.black;
      } else if (pm4 < THRESH_BADNESS_2_PM4) {
        color = colors.lightRed;
      } else {
        color = colors.red;
      }
      break;
    case DRAW_IDX_PM10:
      if (pm10 < THRESH_BADNESS_1_PM10) {
        color = colors.black;
      } else if (pm10 < THRESH_BADNESS_2_PM10) {
        color = colors.lightRed;
      } else {
        color = colors.red;
      }
      break;
    case DRAW_IDX_HUMIDITY:
    case DRAW_IDX_TEMPERATURE:
      humidex = calc_humidex(temperature, humidity);
      if (humidex < THRESH_BADNESS_1_HUMIDEX) {
        color = colors.black;
      } else if (humidex < THRESH_BADNESS_2_HUMIDEX) {
        color = colors.lightRed;
      } else {
        color = colors.red;
      }
      break;
    case DRAW_IDX_VOC:
      if (voc < THRESH_BADNESS_1_VOC) {
        color = colors.black;
      } else if (voc < THRESH_BADNESS_2_VOC) {
        color = colors.lightRed;
      } else {
        color = colors.red;
      }
      break;
    case DRAW_IDX_NOX:
      if (nox < THRESH_BADNESS_1_NOX) {
        color = colors.black;
      } else if (nox < THRESH_BADNESS_2_VOC) {
        color = colors.lightRed;
      } else {
        color = colors.red;
      }
      break;
    case DRAW_IDX_CO2:
      if (co2 < THRESH_BADNESS_1_CO2) {
        color = colors.black;
      } else if (co2 < THRESH_BADNESS_2_CO2) {
        color = colors.lightRed;
      } else {
        color = colors.red;
      }
      break;
    default:
      color = colors.black;
      break;
    }

    return color;
  }
};

bool val_is_invalid_pm(float val)
{
  return !((0 <= val) && (val <= RANGE_MAX_PM));
}

bool val_is_invalid_humidity(float val)
{
  return !((0 <= val) && (val <= RANGE_MAX_RH));
}

bool val_is_invalid_temperature(float val)
{
  return !((0 <= val) && (val <= RANGE_MAX_T));
}

bool val_is_invalid_index(uint16_t val) { return val > RANGE_MAX_INDEX; }

bool val_is_invalid_co2(uint16_t val) { return val > RANGE_MAX_CO2; }

int get_draw_unit(char *fmt_buffer, draw_idx_t idx)
{
  int ret = 0;

  switch (idx) {
  case DRAW_IDX_PM1:
  case DRAW_IDX_PM2P5:
  case DRAW_IDX_PM4:
  case DRAW_IDX_PM10:
    ret = sprintf(fmt_buffer, "[ug/m3]");
    break;

  case DRAW_IDX_HUMIDITY:
    ret = sprintf(fmt_buffer, "[%%]");
    break;

  case DRAW_IDX_TEMPERATURE:
    ret = sprintf(fmt_buffer, "[deg C]");
    break;

  case DRAW_IDX_CO2:
    ret = sprintf(fmt_buffer, "[ppm]");
    break;

  case DRAW_IDX_VOC:
  case DRAW_IDX_NOX:
    ret = sprintf(fmt_buffer, "");
    return ret;
  }
  return ret;
}

int get_draw_name(char *fmt_buffer, draw_idx_t idx)
{
  const char *name = "\0";

  switch (idx) {
  case DRAW_IDX_PM1:
    name = "PM1";
    break;
  case DRAW_IDX_PM2P5:
    name = "PM2.5";
    break;
  case DRAW_IDX_PM4:
    name = "PM4";
    break;
  case DRAW_IDX_PM10:
    name = "PM10";
    break;
  case DRAW_IDX_HUMIDITY:
    name = "RH";
    break;
  case DRAW_IDX_TEMPERATURE:
    name = "t";
    break;
  case DRAW_IDX_VOC:
    name = "VOC";
    break;
  case DRAW_IDX_NOX:
    name = "NOX";
    break;
  case DRAW_IDX_CO2:
    name = "CO2";
    break;
  }
  return sprintf(fmt_buffer, "%s", name);
}

void save_data_to_sd_card(Sen66Values &sen66Values)
{
  timeval current_time;
  gettimeofday(&current_time, nullptr);
  char *date_str = NTP.getTimeDateString(current_time.tv_sec,
                                         "%02d/%02m/%04Y %02H:%02M:%02S");

  int written = sprintf(buf, "%ld, %s, ", current_time.tv_sec, date_str);
  written += sen66Values.get_str(&buf[written]);

  if (written == DATA_LINE_LEN) {
    Log.verbose("Writing: %s", buf); // NOTE: newline in the buffer
    append_file(SD_MMC, DATA_PATH, buf);
  } else {
    Log.errorln("Invalid data line length: %d, expected: %d", written,
                DATA_LINE_LEN);
  }
}

bool get_data_invalid(draw_idx_t type, float val)
{
  switch (type) {
  case DRAW_IDX_PM1:
  case DRAW_IDX_PM2P5:
  case DRAW_IDX_PM4:
  case DRAW_IDX_PM10:
    return val_is_invalid_pm(val);

  case DRAW_IDX_HUMIDITY:
    return val_is_invalid_humidity(val);

  case DRAW_IDX_TEMPERATURE:
    return val_is_invalid_temperature(val);

  case DRAW_IDX_VOC:
  case DRAW_IDX_NOX:
    return val_is_invalid_index(val);

  case DRAW_IDX_CO2:
    return val_is_invalid_co2(val);
  }

  return true;
}
