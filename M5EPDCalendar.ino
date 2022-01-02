#include <M5EPD.h>
#include <WiFi.h>
#include <Preferences.h>

#include "auth.h"

Preferences pref;
#define prefname "calendar"

M5EPD_Canvas canvas(&M5.EPD);

#define SCREEN_WIDTH 540
#define SCREEN_HEIGHT 960

static void
showMacAddr() {
  byte mac[6];

  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[0],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.println(mac[5],HEX);
}

static String connectingMessage = String("Connecting...");

void showMessage(String mesg)
{
  unsigned w = canvas.textWidth(mesg);

  canvas.fillCanvas(0);
  canvas.drawString(mesg, (SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT / 2);
  canvas.pushCanvas(0,0,UPDATE_MODE_DU4);
}

void connectWiFi()
{
  const unsigned waitTime = 500, waitTimeOut = 60000;
  unsigned i;
  
  WiFi.begin(WIFIAP, WIFIPW);
  showMessage(connectingMessage);

  for (i = 0 ; WiFi.status() != WL_CONNECTED && i < waitTimeOut ; i += waitTime) {
    delay(waitTime);
  }
  // if you are connected, print your MAC address:
  showMacAddr();

  canvas.fillCanvas(0);
}

bool NTPSync()
{
  const unsigned long NTP_INTERVAL = (24 * 60 * 60 * 1000);
  static unsigned long lastNTPconfiguration = 0;
  bool retval = false;

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (WiFi.status() == WL_CONNECTED) {
    // check NTP
    if (!lastNTPconfiguration || NTP_INTERVAL < millis() - lastNTPconfiguration) {
      configTime(9 * 3600, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");
      lastNTPconfiguration = millis();
      for (time_t t = 0 ; t == 0 ; t = time(NULL)); // wait for NTP to synchronize
      retval = true;
    }
  }
  return retval;
}

static int date = -1;
static bool firstBoot = true;
static bool wakedup = false;

void setup()
{
  configTime(9 * 3600, 0, nullptr); // set time zone as JST-9
  // The above is performed without network connection.

  pref.begin(prefname, false);
  date = pref.getInt("date", -1);
  firstBoot = pref.getBool("firstboot", true);
  pref.clear(); // clear the preferences for unexpected reset
  pref.end();

  //  Check power on reason before calling M5.begin()
  //  which calls RTC.begin() which clears the timer flag.
  Wire.begin(21, 22);                  
  uint8_t data = M5.RTC.readReg(0x01);
  wakedup = ((data & 4) == 4); // true means waked up by RTC, not by power button
  if (!wakedup) {
    firstBoot = true;
  }

  // M5.enableMainPower();
  M5.begin();
  // M5.enableEPDPower();
  M5.EPD.SetRotation(90); // electric paper display
  M5.TP.SetRotation(90); // touch panel
  M5.EPD.Clear(true); 
  M5.RTC.begin(); // real time clock

  canvas.createCanvas(SCREEN_WIDTH, SCREEN_HEIGHT);
  canvas.setTextSize(3);

  if (firstBoot) {
    showMessage(String("First boot."));
    firstBoot = false;
    delay(2000);
  }
  if (wakedup) {
    showMessage(String("Waked up."));
    delay(2000);
  }

  connectWiFi();
  showMessage(String("Synchronizing time..."));
  if (NTPSync()) {
    showMessage(String("Time synchromized."));
  }
  else {
    showMessage(String("Synchromization failed."));
  }
}

void loop()
{
  static bool slept = false;

  if (slept) {
    // M5.EPD.Clear(true);
    slept = false;
    showMessage(connectingMessage);
  }
  if (1 || M5.BtnP.wasPressed()) {
    delay(1000);
    slept = true;
    showMessage(String("Sleeping..."));
    delay(1000);

    pref.begin(prefname, false);
    pref.putInt("date", date);
    pref.putBool("firstboot", false);
    pref.end();
    
    M5.shutdown();

    delay(1000);
  }
  M5.update();
  delay(100);
}

