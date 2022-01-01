#include <M5EPD.h>

M5EPD_Canvas canvas(&M5.EPD);

void showInitialMessage()
{
  canvas.drawString("Press PWR Btn for sleep!", 45, 350);
  canvas.drawString("after 5 sec wakeup!", 70, 450);
  canvas.pushCanvas(0,0,UPDATE_MODE_DU4);
}

static String sleepingMessage = String("Sleeping...");

void setup()
{
  // M5.enableMainPower();
  M5.begin();
  // M5.enableEPDPower();
  M5.EPD.SetRotation(90); // electric paper display
  M5.TP.SetRotation(90); // touch panel
  M5.EPD.Clear(true); 
  M5.RTC.begin(); // real time clock

  canvas.createCanvas(540, 960);
  canvas.setTextSize(3);
  showInitialMessage();
}

void loop()
{
  static bool slept = false;

  if (slept) {
    // M5.EPD.Clear(true);
    slept = false;
    showInitialMessage();
  }
  if (M5.BtnP.wasPressed()) {
    canvas.drawString(sleepingMessage, 45, 550);
    canvas.pushCanvas(0,0,UPDATE_MODE_DU4);
    delay(1000);
    sleepingMessage = String("I'm in second sleeping.");
    slept = true;
    M5.shutdown(10);
    delay(1000);
  }
  M5.update();
  delay(100);
}

