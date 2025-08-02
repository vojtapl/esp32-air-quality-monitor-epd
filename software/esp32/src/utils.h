#pragma once

#include "config_gpio.h"
#include <ArduinoLog.h>
#include <SparkFunBQ27441.h>
#include <WiFi.h>
#include <cstdint>
#include <esp_wifi.h>
#define CHECK_SEN66_CALL(...)                                                  \
  do {                                                                         \
    uint16_t sen66_error = __VA_ARGS__;                                        \
    if (sen66_error != NO_ERROR) {                                             \
      Log.error("Error trying to execute " #__VA_ARGS__ ": ");                 \
      char sen66_error_msg[64] = {"\0"};                                       \
      errorToString(sen66_error, sen66_error_msg, sizeof(sen66_error_msg));    \
      Log.errorln(sen66_error_msg);                                            \
      return false;                                                            \
    }                                                                          \
  } while (0)

float calc_dew_point(float t, float rel_hum);
float calc_humidex(float t, float rel_hum);

inline bool sd_card_in_socket();

uint16_t get_bat_temp_in_deg_c(BQ27441 &Bat, temp_measure type);
void print_battery_stats(BQ27441 &bat);

void print_mac_address(void);
bool change_mac_address(const uint8_t mac_addr[6]);
bool wait_for_wifi_connection(void);

void get_charge_discharge_time(BQ27441 &Bat, uint8_t &h, uint8_t &min);
void log_charge_status(BQ27441 &Bat);

void print_suffix(Print *_logOutput, int logLevel);
void print_prefix(Print *_logOutput, int logLevel);
void print_log_level(Print *_logOutput, int logLevel);
void print_timestamp(Print *_logOutput);

template <typename T> T map(T x, T in_min, T in_max, T out_min, T out_max);
template <typename T>
T map_bounded(const T x, const T in_min, const T in_max, const T out_min,
              const T out_max);
// use when out_max < out_min
template <typename T>
T map_rev_bounded(const T x, const T in_min, const T in_max, const T out_min,
                  const T out_max);
template <typename T>
T get_x_tick_at_idx(const T tick_min, const T tick_max, const T tick_cnt,
                    const T idx);
// y tick needs subtracting to align properly on the origin
template <typename T>
T get_y_tick_at_idx(const T tick_min, const T tick_max, const T tick_cnt,
                    const T idx);

float calc_dew_point(float t, float rel_hum)
{
  const float b = 18.678;
  const float c = 257.14;
  const float d = 234.5;

  float gamma_m = log((rel_hum / 100) * exp((b - t / d) * (t / (c + t))));
  return (c * gamma_m) / (b - gamma_m);
}

float calc_humidex(float t, float rel_hum)
{
  float t_dew = calc_dew_point(t, rel_hum);
  return t +
         0.5555 *
             (6.11 * exp(5417.7530 * (1 / 273.15 - 1 / (273.15 + t_dew))) - 10);
}

inline bool sd_card_in_socket() { return digitalRead(SD_DET) == LOW; }

uint16_t get_bat_temp_in_deg_c(BQ27441 &Bat, temp_measure type)
{
  return floor(Bat.temperature(type) * 1.0) / 10 - 273.15;
}

void print_battery_stats(BQ27441 &Bat)
{
  // Read battery stats from the BQ27441
  Log.infoln("Battery stats:");
  Log.infoln("\tstate of charge       = %u %%", Bat.soc());
  Log.infoln("\tU                     = %u mV", Bat.voltage());
  Log.infoln("\tI_avg                 = %d mA", Bat.current(AVG));
  Log.infoln("\tQ_full                = %u mAh", Bat.capacity(FULL));
  Log.infoln("\tQ_remaining           = %u mAh", Bat.capacity(REMAIN));
  Log.infoln("\tP_avg                 = %d mW", Bat.power());
  Log.infoln("\tstate of health       = %d %%", Bat.soh());
  Log.infoln("\tt_bat                 = %u °C",
             get_bat_temp_in_deg_c(Bat, BATTERY));
  Log.infoln("\tt_internal            = %u °C",
             get_bat_temp_in_deg_c(Bat, INTERNAL_TEMP));
  Log.infoln("\tfast charging allowed : %s", Bat.chgFlag() ? "yes" : "no");
  Log.infoln("\tfully charged         : %s", Bat.fcFlag() ? "yes" : "no");
  Log.infoln("\tdischarging           : %s", Bat.dsgFlag() ? "yes" : "no");
}
void print_mac_address(void)
{
  uint8_t baseMac[6];
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret == ESP_OK) {
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n", baseMac[0], baseMac[1],
                  baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
  } else {
    Serial.println("");
    Log.errorln("Failed to read MAC address");
  }
}

bool change_mac_address(const uint8_t mac_addr[6])
{
  esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, mac_addr);
  if (err != ESP_OK) {
    Log.errorln("Error changing Mac Address: ", esp_err_to_name(err));
    return false;
  }
  Log.verbose("Success changing Mac Address to: ");
  print_mac_address();
  return true;
}

bool wait_for_wifi_connection(void)
{
  uint8_t counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    counter++;
    if (counter >= 60) {
      Serial.println(""); // add a newline after the dots
      Log.fatalln("Unable to connect to WiFi within 30 seconds!");
      return false;
    }
  }
  Serial.println(""); // add a newline after the dots

  Log.infoln("WiFi connected with IP address: %s", WiFi.localIP().toString());
  return true;
}

void print_prefix(Print *_logOutput, int logLevel)
{
  print_timestamp(_logOutput);
  print_log_level(_logOutput, logLevel);
}

void print_timestamp(Print *_logOutput)
{
  // Division constants
  const unsigned long MSECS_PER_SEC = 1000;
  const unsigned long SECS_PER_MIN = 60;
  const unsigned long SECS_PER_HOUR = 3600;
  const unsigned long SECS_PER_DAY = 86400;

  // Total time
  const unsigned long msecs = millis();
  const unsigned long secs = msecs / MSECS_PER_SEC;

  // Time in components
  const unsigned long MilliSeconds = msecs % MSECS_PER_SEC;
  const unsigned long Seconds = secs % SECS_PER_MIN;
  const unsigned long Minutes = (secs / SECS_PER_MIN) % SECS_PER_MIN;
  const unsigned long Hours = (secs % SECS_PER_DAY) / SECS_PER_HOUR;

  // Time as string
  char timestamp[20];
  sprintf(timestamp, "%02lu:%02lu:%02lu.%03lu ", Hours, Minutes, Seconds,
          MilliSeconds);
  _logOutput->print(timestamp);
}

void print_log_level(Print *_logOutput, int logLevel)
{
  /// Show log description based on log level
  switch (logLevel) {
  default:
  case 0:
    _logOutput->print("SILENT  ");
    break;
  case 1:
    _logOutput->print("FATAL   ");
    break;
  case 2:
    _logOutput->print("ERROR   ");
    break;
  case 3:
    _logOutput->print("WARNING ");
    break;
  case 4:
    _logOutput->print("INFO    ");
    break;
  case 5:
    _logOutput->print("TRACE   ");
    break;
  case 6:
    _logOutput->print("VERBOSE ");
    break;
  }
}

void print_suffix(Print *_logOutput, int logLevel) { _logOutput->print(""); }

void log_charge_status(BQ27441 &Bat)
{
  const char *state = "charging";
  if (Bat.dsgFlag()) {
    state = "discharging";
  } else if (Bat.fcFlag()) {
    state = "charged";
  }

  uint8_t t_remaining_h = 0;
  uint8_t t_remaining_min = 0;
  get_charge_discharge_time(Bat, t_remaining_h, t_remaining_min);

  Log.verboseln("Battery: state of charge = %d mAh / %d mAh = %u %%, %s, I = "
                "%d mA, t_remaining = %u h %u min",
                Bat.capacity(REMAIN), Bat.capacity(AVAIL_FULL), Bat.soc(),
                state, Bat.current(), t_remaining_h, t_remaining_min);
}

template <typename T> T map(T x, T in_min, T in_max, T out_min, T out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

template <typename T>
T map_bounded(const T x, const T in_min, const T in_max, const T out_min,
              const T out_max)
{
  T res = map<T>(x, in_min, in_max, out_min, out_max);
  if (res < out_min) {
    return out_min;
  } else if (res > out_max) {
    return out_max;
  }
  return res;
}

// use when out_max < out_min
template <typename T>
T map_rev_bounded(const T x, const T in_min, const T in_max, const T out_min,
                  const T out_max)
{
  T res = map<T>(x, in_min, in_max, out_min, out_max);
  if (res < out_max) {
    return out_max;
  } else if (res > out_min) {
    return out_min;
  }
  return res;
}

template <typename T>
T get_x_tick_at_idx(const T tick_min, const T tick_max, const T tick_cnt,
                    const T idx)
{
  T tick_len = (tick_max - tick_min) / tick_cnt;
  T tick_x = tick_min + idx * tick_len;
  return tick_x;
}

// y tick needs subtracting to align properly on the origin
template <typename T>
T get_y_tick_at_idx(const T tick_min, const T tick_max, const T tick_cnt,
                    const T idx)
{
  T tick_len = (tick_max - tick_min) / tick_cnt;
  T tick_x = tick_max - idx * tick_len;
  return tick_x;
}

void get_charge_discharge_time(BQ27441 &Bat, uint8_t &h, uint8_t &mins)
{
  uint16_t capacity = 0;
  if (Bat.fcFlag()) {
    h = 255;
    mins = 255;
  } else {
    if (Bat.dsgFlag()) {
      capacity = Bat.capacity(REMAIN);
    } else {
      capacity = Bat.capacity(AVAIL_FULL) - Bat.capacity(REMAIN);
    }
    float t_remaining = (capacity * 1.f) / abs(Bat.current());

    h = floor(t_remaining);
    mins = (t_remaining - floor(t_remaining)) * 60;
  }
}
