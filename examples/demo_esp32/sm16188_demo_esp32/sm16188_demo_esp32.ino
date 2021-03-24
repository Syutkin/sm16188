#include <sm16188.h>
#include <fonts/SystemFont5x7.h>
#include <fonts/Arial_black_16.h>

const gpio_num_t D1 = GPIO_NUM_22;
const gpio_num_t D2 = GPIO_NUM_23;

SM16188<D1, D2> sm16188;

#define DISPLAYS_ACROSS 5
#define DISPLAYS_DOWN 1

//Timer setup
//create a hardware timer of ESP32
hw_timer_t *timer = NULL;

void IRAM_ATTR triggerUpdate()
{
  sm16188.updateScreen();
}

void setup(void)
{
  // return the clock speed of the CPU
  uint32_t cpuClock = ESP.getCpuFreqMHz();

  // Use 1st timer of 4
  // divide cpu clock speed on its speed value by MHz to get 1us for each signal  of the timer
  timer = timerBegin(0, cpuClock, true);
  // Attach triggerUpdate function to our timer
  timerAttachInterrupt(timer, &triggerUpdate, true);
  // Set alarm to call triggerUpdate function
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer, 25000, true);

  // Start an alarm
  timerAlarmEnable(timer);

  //initialize sm16188
  sm16188.begin(DISPLAYS_ACROSS, DISPLAYS_DOWN);

  //clear/init the SM16188 pixels held in RAM
  sm16188.clearScreen(true); //true is normal (all pixels off), false is negative (all pixels on)
}

void loop(void)
{
  byte b;

  // 10 x 14 font clock, including demo of OR and NOR modes for pixels so that the flashing colon can be overlayed
  sm16188.clearScreen(true);
  sm16188.selectFont(Arial_Black_16);
  sm16188.drawChar(0, 3, '2', GRAPHICS_NORMAL);
  sm16188.drawChar(7, 3, '3', GRAPHICS_NORMAL);
  sm16188.drawChar(17, 3, '4', GRAPHICS_NORMAL);
  sm16188.drawChar(25, 3, '5', GRAPHICS_NORMAL);
  sm16188.drawChar(15, 3, ':', GRAPHICS_OR); // clock colon overlay on
  delay(1000);
  sm16188.drawChar(15, 3, ':', GRAPHICS_NOR); // clock colon overlay off
  delay(1000);
  sm16188.drawChar(15, 3, ':', GRAPHICS_OR); // clock colon overlay on
  delay(1000);
  sm16188.drawChar(15, 3, ':', GRAPHICS_NOR); // clock colon overlay off
  delay(1000);
  sm16188.drawChar(15, 3, ':', GRAPHICS_OR); // clock colon overlay on
  delay(1000);

  // drawMarquee not implemented
  //  sm16188.drawMarquee("Scrolling Text",14,(32*DISPLAYS_ACROSS)-1,0);
  //  long start=millis();
  //  long timer=start;
  //  boolean ret=false;
  //  while(!ret){
  //    if ((timer+30) < millis()) {
  //      ret=sm16188.stepMarquee(-1,0);
  //      timer=millis();
  //    }
  //  }

  // half the pixels on
  sm16188.drawTestPattern(PATTERN_ALT_0);
  delay(1000);
  // the other half on
  sm16188.drawTestPattern(PATTERN_ALT_1);
  delay(1000);

  // display some text
  sm16188.clearScreen(true);
  sm16188.selectFont(System5x7);

  for (byte x = 0; x < DISPLAYS_ACROSS; x++)
  {
    for (byte y = 0; y < DISPLAYS_DOWN; y++)
    {
      sm16188.drawString(2 + (32 * x), 1 + (16 * y), "freet", 5, GRAPHICS_NORMAL);
      sm16188.drawString(2 + (32 * x), 9 + (16 * y), "ronic", 5, GRAPHICS_NORMAL);
    }
  }

  delay(2000);

  // draw a border rectangle around the outside of the display
  sm16188.clearScreen(true);
  sm16188.drawBox(0, 0, (32 * DISPLAYS_ACROSS) - 1, (16 * DISPLAYS_DOWN) - 1, GRAPHICS_NORMAL);
  delay(1000);

  for (byte y = 0; y < DISPLAYS_DOWN; y++)
  {
    for (byte x = 0; x < DISPLAYS_ACROSS; x++)
    {
      // draw an X
      int ix = 32 * x;
      int iy = 16 * y;
      sm16188.drawLine(0 + ix, 0 + iy, 11 + ix, 15 + iy, GRAPHICS_NORMAL);
      sm16188.drawLine(0 + ix, 15 + iy, 11 + ix, 0 + iy, GRAPHICS_NORMAL);
      delay(1000);

      // draw a circle
      sm16188.drawCircle(16 + ix, 8 + iy, 5, GRAPHICS_NORMAL);
      delay(1000);

      // draw a filled box
      sm16188.drawFilledBox(24 + ix, 3 + iy, 29 + ix, 13 + iy, GRAPHICS_NORMAL);
      delay(1000);
    }
  }

  // stripe chaser
  for (b = 0; b < 20; b++)
  {
    sm16188.drawTestPattern((b & 1) + PATTERN_STRIPE_0);
    delay(200);
  }
  delay(200);
}
