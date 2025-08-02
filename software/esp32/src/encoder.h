#include "config_gpio.h"
#include "oled.h"
// #include <FastInterruptEncoder.h>

#define ENCODER_READ_DELAY 300

static unsigned long encoder_timer = 0;

Encoder enc(ROTARY_ENCODER_A, ROTARY_ENCODER_B, SINGLE); // - Example for ESP32

unsigned long encodertimer = 0;

/* create a hardware timer */
hw_timer_t *timer = NULL;

void IRAM_ATTR Update_IT_callback() { enc.loop(); }

void setup_rotary_encoder(void)
{
  if (enc.init()) {
    Serial.println("Encoder Initialization OK");
  } else {
    Serial.println("Encoder Initialization Failed");
    while (1)
      ;
  }

  /* Use 1st timer of 4 */
  /* 1 tick take 1/(80MHZ/80) = 1us so we set divider 80 and count up */
  timer = timerBegin(0, 80, true);
  /* Attach onTimer function to our timer */
  timerAttachInterrupt(timer, &Update_IT_callback, true);
  /* Set alarm to call onTimer function every 100 ms -> 100 Hz */
  timerAlarmWrite(timer, 10000, true);
  /* Start an alarm */
  timerAlarmEnable(timer);
}

void rotary_encoder_loop()
{
  if ((millis() > (encodertimer + 1000)) || (millis() < encodertimer)) {
    Serial.println(enc.getTicks());
    encodertimer = millis();
  }
}
