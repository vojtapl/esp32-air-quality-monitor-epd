#pragma once

#include "SD_MMC.h"
#include "data_handling.h"
#include <ESPNtpClient.h>
#include <FS.h>
#include <PDLS_EXT3_Basic_Global.h>
#include <cstdint>

typedef enum {
  SCREEN_PM1,
  SCREEN_PM2P5,
  SCREEN_PM4,
  SCREEN_PM10,
  SCREEN_HUMIDITY,
  SCREEN_TEMPERATURE,
  SCREEN_VOC,
  SCREEN_NOX,
  SCREEN_CO2,
  SCREEN_OVERVIEW,

  SCREEN_NUM,
  SCREEN_LAST = SCREEN_OVERVIEW,
  SCREEN_FIRST = SCREEN_PM1,
} screen_t;

uint8_t past_threshold_idx = 0;
const uint8_t past_threshold_cnt = 4;
float past_thresholds[past_threshold_cnt] = {
    -1,
    -6,
    -12,
    -24,
};

extern char buf[BUF_SIZE];
screen_t screen = SCREEN_OVERVIEW;

const uint8_t PAD = 2;
const uint8_t Y_TOP_BAR_LINE = 16;
const uint8_t AX_ARROWHEAD_OFFSET = 3;
const uint8_t TICK_HALF_HEIGHT = 2;
const uint8_t X_AX_NUM_TICKS = 4;
const uint8_t Y_AX_NUM_TICKS = 4;

void draw_bmp(Screen_EPD_EXT3 screen, const uint8_t *bmp, uint ox, uint oy,
              uint width, uint height, const uint16_t color);
void draw_overview(Screen_EPD_EXT3 &myScreen, Sen66Values &sen66Values,
                   BQ27441 &bat);
void draw_graph(Screen_EPD_EXT3 &myScreen, draw_idx_t type, float x_min,
                float x_max);
void draw_postsign(Screen_EPD_EXT3 &myScreen);
const uint8_t *select_img_bat(BQ27441 &bat);
const uint8_t *select_img_wifi();
const uint8_t *select_img_sd_card();
const uint16_t select_color_bat(BQ27441 &bat);
const uint16_t select_color_wifi();
const uint16_t select_color_sd_card();
int get_screen_name(char *buf, screen_t screen);

inline void epd_turn_off();
inline void epd_turn_on();

void draw_top_bar(Screen_EPD_EXT3 &myScreen, BQ27441 &bat);
void draw(Screen_EPD_EXT3 &myScreen, Sen66Values &sen66Values, BQ27441 &bat);
void fetch_graph_data_from_sd_card(draw_idx_t type, const float x_min,
                                   const float x_max, uint16_t x_ax_draw_min_x,
                                   uint16_t x_ax_draw_max_x,
                                   float draw_buffer_min[],
                                   float draw_buffer_max[], float &global_min,
                                   float &global_max);
static void draw_graph_x_axis(Screen_EPD_EXT3 &myScreen, uint16_t x_ax_min_x,
                              uint16_t x_ax_max_x, uint16_t x_ax_y);
static void draw_graph_x_axis_ticks(Screen_EPD_EXT3 &myScreen,
                                    uint16_t x_ax_min_x,
                                    uint16_t x_ax_draw_max_x, uint16_t x_ax_y,
                                    float x_min, float x_max);
static void draw_graph_y_axis(Screen_EPD_EXT3 &myScreen, draw_idx_t type,
                              uint16_t y_ax_x, uint16_t y_ax_min_y,
                              uint16_t y_ax_max_y);
static void draw_graph_y_axis_ticks(Screen_EPD_EXT3 &myScreen, uint16_t y_ax_x,
                                    uint16_t y_ax_draw_min_y,
                                    uint16_t y_ax_max_y, float global_min,
                                    float global_max);
static void draw_graph_plot_data(Screen_EPD_EXT3 &myScreen, uint16_t x_ax_min_x,
                                 uint16_t x_ax_num_px, uint16_t y_ax_min_y,
                                 uint16_t y_ax_max_y, float draw_buffer_min[],
                                 float draw_buffer_max[], float global_min,
                                 float global_max);
static void draw_graph_title(Screen_EPD_EXT3 &myScreen, draw_idx_t type);
static uint8_t get_line_read_offset(draw_idx_t type);
char *top_bar_get_bat_info(BQ27441 &bat);

void draw(Screen_EPD_EXT3 &myScreen, Sen66Values &sen66Values, BQ27441 &bat)
{
  myScreen.setOrientation(1);
  myScreen.clear();
  myScreen.setPenSolid(true);

  epd_turn_on();
  draw_top_bar(myScreen, bat);

  switch (screen) {
  case SCREEN_PM1:
  case SCREEN_PM2P5:
  case SCREEN_PM4:
  case SCREEN_PM10:
  case SCREEN_HUMIDITY:
  case SCREEN_TEMPERATURE:
  case SCREEN_VOC:
  case SCREEN_NOX:
  case SCREEN_CO2:
    draw_graph(myScreen, (draw_idx_t)screen,
               past_thresholds[past_threshold_idx], 0);
    break;

  case SCREEN_OVERVIEW:
  default:
    draw_overview(myScreen, sen66Values, bat);
    break;
  }

  myScreen.flush();
  epd_turn_off();
}

void draw_top_bar(Screen_EPD_EXT3 &myScreen, BQ27441 &bat)
{
  // top bar from left to right
  myScreen.selectFont(Font_Terminal12x16);
  char *datetime_str =
      NTP.getTimeDateString(time(nullptr), "%02d/%02m %02H:%02M");
  myScreen.gText(0, 0, datetime_str);

  const uint8_t *img_sd = select_img_sd_card();
  const uint16_t color_sd = select_color_sd_card();
  draw_bmp(myScreen, img_sd, 173, 0, 14, 16, color_sd);

  const uint8_t *wifi_img = select_img_wifi();
  const uint16_t wifi_color = select_color_wifi();
  draw_bmp(myScreen, wifi_img, 196, 0, 19, 16, wifi_color);

  const uint8_t *img_bat = select_img_bat(bat);
  const uint16_t color_bat = select_color_bat(bat);
  draw_bmp(myScreen, img_bat, 225, 0, 24, 16, color_bat);

  myScreen.gText(250, 0, top_bar_get_bat_info(bat));

  myScreen.line(0, 16, 294, 16, myColours.black);
}

void draw_overview(Screen_EPD_EXT3 &myScreen, Sen66Values &sen66Values,
                   BQ27441 &bat)
{
  // constants of value screen positions
  const uint8_t y0 = 18; // start a bit lower than the top bar division line
  const uint8_t x0 = 0;

  // o - offset
  const uint8_t o_val_x = 20;
  const uint8_t o_val_y = 20;

  const uint8_t o_name_x = 40;
  const uint8_t o_name_y = 2;

  myScreen.setOrientation(1);
  const uint16_t width = myScreen.screenSizeX();
  const uint16_t height = myScreen.screenSizeY();

  const uint8_t NUM_COLS = 3;
  const uint8_t NUM_ROWS = 3;

  // value screen
  for (int row_idx = 0; row_idx < NUM_ROWS; ++row_idx) {
    for (int col_idx = 0; col_idx < NUM_COLS; ++col_idx) {
      draw_idx_t val_idx = (draw_idx_t)(col_idx + NUM_COLS * row_idx);

      uint16_t curr_x = x0 + (col_idx * (width - x0)) / NUM_COLS;
      uint16_t curr_y = y0 + (row_idx * (height - y0)) / NUM_ROWS;

      myScreen.selectFont(Font_Terminal8x12);
      get_draw_name(buf, val_idx);
      myScreen.gText(curr_x, curr_y, buf);

      myScreen.selectFont(Font_Terminal12x16);
      uint16_t color;
      if (sen66Values.get_data_invalid(val_idx)) {
        sprintf(buf, "N/A");
        color = myColours.red;
      } else {
        sen66Values.get_draw_value(buf, val_idx);
        color = sen66Values.get_draw_color(val_idx);
      }
      myScreen.gText(curr_x + o_val_x, curr_y + o_val_y, buf, color);

      myScreen.selectFont(Font_Terminal6x8);
      get_draw_unit(buf, val_idx);
      myScreen.gText(curr_x + o_name_x, curr_y + o_name_y, buf);
    }
  }
}

void draw_bmp(Screen_EPD_EXT3 scr, const uint8_t *bmp, uint ox, uint oy,
              uint width, uint height, const uint16_t color)
{
  // Number of bytes per row in bitmap data (row padded bits)
  int bytesPerRow = (width + 7) / 8;

  for (int y = 0; y < height; y++) {
    for (int byteIndex = 0; byteIndex < bytesPerRow; byteIndex++) {
      uint8_t byte = bmp[y * bytesPerRow + byteIndex];

      for (int bit = 0; bit < 8; bit++) {
        int x = byteIndex * 8 + bit;
        if (x >= width) {
          break; // ignore padding bits
        }

        bool pixelOn = byte & (1 << bit);
        if (pixelOn) {
          scr.point(ox + x, oy + y, color);
        }
      }
    }
  }
}

void draw_postsign(Screen_EPD_EXT3 &myScreen)
{
  const uint16_t width = myScreen.screenSizeX();

  myScreen.selectFont(Font_Terminal8x12);
  // const uint8_t font_height = 12;
  const uint8_t font_width = 8;

  uint8_t prev_idx = screen > 0 ? screen - 1 : SCREEN_LAST;
  uint8_t next_idx = screen < SCREEN_LAST ? screen + 1 : SCREEN_FIRST;

  int written = sprintf(buf, "<");
  written += get_screen_name(&buf[written], (screen_t)prev_idx);

  // middle part
  written += sprintf(&buf[written], "|");

  // right part
  written += get_screen_name(&buf[written], (screen_t)next_idx);
  written += sprintf(&buf[written], ">");

  myScreen.gText(width - font_width * written, Y_TOP_BAR_LINE + PAD, buf);
}

void fetch_graph_data_from_sd_card(draw_idx_t type, const float x_min,
                                   const float x_max, uint16_t x_ax_draw_min_x,
                                   uint16_t x_ax_draw_max_x,
                                   float draw_buffer_min[],
                                   float draw_buffer_max[], float &global_min,
                                   float &global_max)
{

  // draw the data
  float curr_min = std::numeric_limits<float>::max();
  float curr_max = std::numeric_limits<float>::min();

  timeval curr_time;
  gettimeofday(&curr_time, nullptr);
  uint32_t t_curr = curr_time.tv_sec;

  uint32_t t_min = t_curr > x_min * 3600 ? t_curr + x_min * 3600 : 0;
  uint32_t t_max = t_curr > x_max * 3600 ? t_curr + x_max * 3600 : 0;

  global_max = std::numeric_limits<float>::min();
  global_min = std::numeric_limits<float>::max();

  File file = SD_MMC.open(DATA_PATH, FILE_READ, false);

  const uint8_t data_offset = get_line_read_offset(type);

  uint32_t seek_min = ((t_curr - t_min) / UPDATE_PERIOD_SEN66) * DATA_LINE_LEN;
  size_t seek_init = file.size() > seek_min ? file.size() - seek_min : 0;
  if (!file.seek(seek_init, fs::SeekSet)) {
    Log.errorln("seek error, curr file pos: %d/%d", file.position(),
                file.size());
    goto close_file_and_return;
  }

  uint16_t x_px_prev = 0;
  while (file.available() >= DATA_LINE_LEN) {

    int read_t = file.readBytesUntil(',', buf, sizeof(buf));
    buf[read_t] = '\0';

    uint32_t t = 0;
    if (sscanf(buf, "%d", &t) != 1) {
      Log.errorln(
          "Unable to read time value from buffer: %s, curr file pos: %d/%d",
          buf, file.position(), file.size());
      break;
    }

    if ((t < t_min) || (t > t_max)) {
      // skip to another line
      if (!file.seek(DATA_LINE_LEN - read_t - 1, fs::SeekCur)) {
        Log.errorln("seek error, curr file pos: %d/%d", file.position(),
                    file.size());
        break;
      }
      continue;
    }

    if (!file.seek(data_offset, fs::SeekCur)) {
      Log.errorln("seek error, curr file pos: %d/%d", file.position(),
                  file.size());
    }

    int read_val = file.readBytesUntil(',', buf, sizeof(buf));
    buf[read_val] = '\0';

    float curr_val = std::numeric_limits<float>::min();
    if (sscanf(buf, "%f", &curr_val) != 1) {
      Log.errorln("Unable to read time value");
      break;
    }

    if (!file.seek(DATA_LINE_LEN - read_t - data_offset - read_val - 2,
                   SeekCur)) {
      Log.errorln("seek error, curr file pos: %d/%d", file.position(),
                  file.size());
      break;
    }

    uint16_t x_px_curr = uint16_t(map_bounded<uint32_t>(
        t, t_min, t_max, x_ax_draw_min_x, x_ax_draw_max_x));

    curr_min = std::min<float>(curr_val, curr_min);
    curr_max = std::max<float>(curr_val, curr_max);

    if (x_px_prev == 0) {
      x_px_prev = x_px_curr;
    }

    if (x_px_curr != x_px_prev) {
      global_min = std::min<float>(global_min, curr_min);
      global_max = std::max<float>(global_max, curr_max);

      draw_buffer_max[x_px_prev - x_ax_draw_min_x] = curr_max;
      draw_buffer_min[x_px_prev - x_ax_draw_min_x] = curr_min;

      // reset
      curr_min = std::numeric_limits<float>::max();
      curr_max = std::numeric_limits<float>::min();
      x_px_prev = x_px_curr;
    }
  }

close_file_and_return:
  file.close();
}

static void draw_graph_x_axis(Screen_EPD_EXT3 &myScreen, uint16_t x_ax_min_x,
                              uint16_t x_ax_max_x, uint16_t x_ax_y)
{
  const uint16_t width = myScreen.screenSizeX();
  const uint16_t height = myScreen.screenSizeY();

  myScreen.selectFont(Font_Terminal6x8);
  const uint8_t font_height = 8;
  const uint8_t font_width = 6;

  // x-axis name and units
  myScreen.gText(width - 2 * font_width, height - 3 * font_height - PAD, "t");
  myScreen.gText(width - 3 * font_width, height - 2 * font_height, "[h]");

  // x-axis line
  myScreen.line(x_ax_min_x, x_ax_y, x_ax_max_x, x_ax_y, myColours.black);

  // x-axis arrowhead
  myScreen.line(x_ax_max_x, x_ax_y, x_ax_max_x - AX_ARROWHEAD_OFFSET,
                x_ax_y - AX_ARROWHEAD_OFFSET, myColours.black);
  myScreen.line(x_ax_max_x, x_ax_y, x_ax_max_x - AX_ARROWHEAD_OFFSET,
                x_ax_y + AX_ARROWHEAD_OFFSET, myColours.black);
}

static void draw_graph_x_axis_ticks(Screen_EPD_EXT3 &myScreen,
                                    uint16_t x_ax_min_x,
                                    uint16_t x_ax_draw_max_x, uint16_t x_ax_y,
                                    float x_min, float x_max)
{
  const uint16_t height = myScreen.screenSizeY();

  myScreen.selectFont(Font_Terminal6x8);
  const uint8_t font_height = 8;
  const uint8_t font_width = 6;

  // x-axis ticks
  for (uint8_t i = 0; i <= X_AX_NUM_TICKS; i++) {
    uint16_t x_tick_x = get_x_tick_at_idx<uint16_t>(x_ax_min_x, x_ax_draw_max_x,
                                                    X_AX_NUM_TICKS, i);
    float x_tick_val =
        get_x_tick_at_idx<float>(x_min, x_max, X_AX_NUM_TICKS, i);

    myScreen.line(x_tick_x, x_ax_y + TICK_HALF_HEIGHT, x_tick_x,
                  x_ax_y - TICK_HALF_HEIGHT, myColours.black);

    int written = sprintf(buf, "%.1f", x_tick_val);
    uint16_t o_x = (written * font_width) / 2;
    myScreen.gText(x_tick_x - o_x, height - font_height, buf);
  }
}

static void draw_graph_y_axis(Screen_EPD_EXT3 &myScreen, draw_idx_t type,
                              uint16_t y_ax_x, uint16_t y_ax_min_y,
                              uint16_t y_ax_max_y)
{
  myScreen.selectFont(Font_Terminal6x8);
  const uint8_t font_height = 8;
  const uint8_t font_width = 6;

  // y-axis name and unit
  int written = get_draw_unit(buf, type);
  myScreen.gText(0, Y_TOP_BAR_LINE + 2 * PAD + font_height, buf);
  int written_name = get_draw_name(buf, type);
  myScreen.gText(abs(written - written_name) * (font_width / 2),
                 Y_TOP_BAR_LINE + PAD, buf);

  // y-axis line
  myScreen.line(y_ax_x, y_ax_max_y, y_ax_x, y_ax_min_y, myColours.black);

  // y-axis arrowhead
  myScreen.line(y_ax_x, y_ax_min_y, y_ax_x + AX_ARROWHEAD_OFFSET,
                y_ax_min_y + AX_ARROWHEAD_OFFSET, myColours.black);
  myScreen.line(y_ax_x, y_ax_min_y, y_ax_x - AX_ARROWHEAD_OFFSET,
                y_ax_min_y + AX_ARROWHEAD_OFFSET, myColours.black);
}

static void draw_graph_y_axis_ticks(Screen_EPD_EXT3 &myScreen, uint16_t y_ax_x,
                                    uint16_t y_ax_draw_min_y,
                                    uint16_t y_ax_max_y, float global_min,
                                    float global_max)
{
  myScreen.selectFont(Font_Terminal6x8);
  const uint8_t font_height = 8;
  const uint8_t font_width = 6;

  // >> y-axis ticks; down here as they are autoranged from read data
  for (uint8_t i = 0; i <= Y_AX_NUM_TICKS; ++i) {
    uint16_t y_tick_y = get_y_tick_at_idx<uint16_t>(y_ax_draw_min_y, y_ax_max_y,
                                                    Y_AX_NUM_TICKS, i);
    float y_tick_val = get_y_tick_at_idx<float>(
        global_min, global_max, Y_AX_NUM_TICKS, X_AX_NUM_TICKS - i);

    myScreen.line(y_ax_x - TICK_HALF_HEIGHT, y_tick_y,
                  y_ax_x + TICK_HALF_HEIGHT, y_tick_y, myColours.black);

    sprintf(buf, "%5.5g", y_tick_val);
    myScreen.gText(0, y_tick_y - font_height / 2, buf);
  }
  // << y-axis ticks
}

void draw_graph_sd_card_warning(Screen_EPD_EXT3 &myScreen)
{
  myScreen.selectFont(Font_Terminal8x12);
  uint8_t font_height = 12;
  uint8_t font_width = 8;

  const uint16_t width = myScreen.screenSizeX();
  const uint16_t height = myScreen.screenSizeY();

  const char *text_warn = "no data to graph, insert SD card";
  const uint8_t text_warn_len = strlen(text_warn);

  // center the text
  const uint16_t text_x = width / 2 - text_warn_len * font_width / 2;
  const uint16_t text_y =
      Y_TOP_BAR_LINE + (height - Y_TOP_BAR_LINE) / 2 - font_height / 2;

  myScreen.gText(text_x, text_y, text_warn, myColours.red);
}

static void draw_graph_plot_data(Screen_EPD_EXT3 &myScreen, uint16_t x_ax_min_x,
                                 uint16_t x_ax_num_px, uint16_t y_ax_min_y,
                                 uint16_t y_ax_max_y, float draw_buffer_min[],
                                 float draw_buffer_max[], float global_min,
                                 float global_max)
{
  // >> draw data from buf
  for (int i = 0; i <= x_ax_num_px; ++i) {
    float curr_max = draw_buffer_max[i];
    float curr_min = draw_buffer_min[i];
    if (isnan(curr_max) || isnan(curr_min)) {
      continue;
    }

    uint16_t y_curr_max = uint16_t(round(map_rev_bounded<float>(
        curr_max, global_min, global_max, y_ax_max_y, y_ax_min_y)));
    uint16_t y_curr_min = uint16_t(round(map_rev_bounded<float>(
        curr_min, global_min, global_max, y_ax_max_y, y_ax_min_y)));
    uint16_t curr_px = x_ax_min_x + i;
    myScreen.line(curr_px, y_curr_min, curr_px, y_curr_max, myColours.red);
  }
  // << draw data from buf
}

static void draw_graph_title(Screen_EPD_EXT3 &myScreen, draw_idx_t type)
{
  const uint16_t width = myScreen.screenSizeX();

  myScreen.selectFont(Font_Terminal8x12);
  uint8_t font_height = 12;
  uint8_t font_width = 8;

  get_draw_name(buf, type);
  uint16_t title_x = width / 2 - strlen(buf) * (font_width / 2);
  myScreen.gText(title_x, Y_TOP_BAR_LINE + PAD, buf);
}

void draw_graph(Screen_EPD_EXT3 &myScreen, draw_idx_t type, float x_min,
                float x_max)
{
  int written = 0; // sprintf written cnt; not checked

  const uint16_t width = myScreen.screenSizeX();
  const uint16_t height = myScreen.screenSizeY();

  draw_postsign(myScreen);

  draw_graph_title(myScreen, type);

  // myScreen.selectFont(Font_Terminal6x8);
  const uint8_t font_height = 8;
  const uint8_t font_width = 6;

  const uint16_t y_ax_x = 6 * font_width + PAD + TICK_HALF_HEIGHT;
  const uint16_t y_ax_max_y = height - font_height - PAD - TICK_HALF_HEIGHT;
  const uint16_t y_ax_min_y = Y_TOP_BAR_LINE + PAD * (font_height + PAD);

  const uint16_t x_ax_y = y_ax_max_y;
  const uint16_t x_ax_min_x = y_ax_x;
  const uint16_t x_ax_max_x = width - 3 * font_width - PAD;

  const uint16_t y_ax_draw_min_y = y_ax_min_y + 2 * AX_ARROWHEAD_OFFSET;
  const uint16_t x_ax_draw_max_x = x_ax_max_x - 2 * AX_ARROWHEAD_OFFSET;

  draw_graph_x_axis(myScreen, x_ax_min_x, x_ax_max_x, x_ax_y);
  draw_graph_x_axis_ticks(myScreen, x_ax_min_x, x_ax_draw_max_x, x_ax_y, x_min,
                          x_max);

  draw_graph_y_axis(myScreen, type, y_ax_x, y_ax_min_y, y_ax_max_y);

  if (SD_MMC.cardType() == CARD_NONE) {
    draw_graph_sd_card_warning(myScreen);
    return;
  }

  // >> draw buf setup
  uint16_t x_ax_num_px = x_ax_draw_max_x - x_ax_min_x + 1;

  float draw_buffer_max[x_ax_num_px];
  std::fill(draw_buffer_max, draw_buffer_max + x_ax_num_px, NAN);

  float draw_buffer_min[x_ax_num_px];
  std::fill(draw_buffer_min, draw_buffer_min + x_ax_num_px, NAN);
  // << draw buf setup

  float global_min = 0;
  float global_max = 0;
  fetch_graph_data_from_sd_card(type, x_min, x_max, x_ax_min_x, x_ax_draw_max_x,
                                draw_buffer_min, draw_buffer_max, global_min,
                                global_max);

  // avoid eqality and then having the same num on all y-axis ticks
  if (abs(global_max - global_min) < 1e-4) {
    global_max += 0.5;
    global_min -= 0.5;
  }

  draw_graph_plot_data(myScreen, x_ax_min_x, x_ax_num_px, y_ax_draw_min_y,
                       y_ax_max_y, draw_buffer_min, draw_buffer_max, global_min,
                       global_max);
  draw_graph_y_axis_ticks(myScreen, y_ax_x, y_ax_draw_min_y, y_ax_max_y,
                          global_min, global_max);

  return;
}

int get_screen_name(char *buf, screen_t screen)
{
  switch (screen) {
  case SCREEN_PM1:
    return get_draw_name(buf, DRAW_IDX_PM1);
    break;

  case SCREEN_PM2P5:
    return get_draw_name(buf, DRAW_IDX_PM2P5);
    break;

  case SCREEN_PM4:
    return get_draw_name(buf, DRAW_IDX_PM4);
    break;

  case SCREEN_PM10:
    return get_draw_name(buf, DRAW_IDX_PM10);
    break;

  case SCREEN_HUMIDITY:
    return get_draw_name(buf, DRAW_IDX_HUMIDITY);
    break;

  case SCREEN_TEMPERATURE:
    return get_draw_name(buf, DRAW_IDX_TEMPERATURE);
    break;

  case SCREEN_VOC:
    return get_draw_name(buf, DRAW_IDX_VOC);
    break;

  case SCREEN_NOX:
    return get_draw_name(buf, DRAW_IDX_NOX);
    break;

  case SCREEN_CO2:
    return get_draw_name(buf, DRAW_IDX_CO2);
    break;

  case SCREEN_OVERVIEW:
    return sprintf(buf, "all");
    break;

  default:
    break;
  }

  return 0;
}

static uint8_t get_line_read_offset(draw_idx_t type)
{
  switch (type) {
  case DRAW_IDX_PM1:
    return 22;
  case DRAW_IDX_PM2P5:
    return 28;
  case DRAW_IDX_PM4:
    return 34;
  case DRAW_IDX_PM10:
    return 40;
  case DRAW_IDX_HUMIDITY:
    return 46;
  case DRAW_IDX_TEMPERATURE:
    return 53;
  case DRAW_IDX_VOC:
    return 61;
  case DRAW_IDX_NOX:
    return 66;
  case DRAW_IDX_CO2:
    return 71;
  }

  return 0;
}

char *top_bar_get_bat_info(BQ27441 &bat)
{
  uint8_t h = 0;
  uint8_t min = 0;
  get_charge_discharge_time(bat, h, min);
  if (min >= 30) {
    ++h;
  }

  if ((h == 255) || bat.fcFlag()) {
    sprintf(buf, "full");
  } else {
    sprintf(buf, "%d h", h);
  }

  return buf;
}
