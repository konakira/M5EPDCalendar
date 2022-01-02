#include <M5EPD.h>
#include <WiFi.h>
#include <Preferences.h>

#include "auth.h"

Preferences pref;
#define prefname "calendar"

M5EPD_Canvas canvas(&M5.EPD);

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540

#define PADX 10
#define PADY 10

#define COLWIDTH (SCREEN_WIDTH / 7)
#define COLHEIGHT 50

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
  canvas.fillCanvas(0);
  canvas.setTextSize(3);
  unsigned w = canvas.textWidth(mesg);
  canvas.drawString(mesg, (SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT / 2);
  canvas.pushCanvas(0,0,UPDATE_MODE_DU4);
}

#define MIN_VOLTAGE 3300
#define MAX_VOLTAGE 4350

void
showBatteryStatus()
{
  double vbat = M5.getBatteryVoltage();
  //  int charging = M5.Axp.GetBatCurrent();
  unsigned batstat = (unsigned)((vbat - MIN_VOLTAGE) * 100 / (MAX_VOLTAGE - MIN_VOLTAGE));
  char buf[6];
  sprintf(buf, "%d%%", batstat);

  canvas.setTextSize(3);
  canvas.drawString(buf, SCREEN_WIDTH - canvas.textWidth(buf) - PADX, SCREEN_HEIGHT - 30);
  canvas.pushCanvas(0,0,UPDATE_MODE_DU4);
}

static int date = -1;
static time_t lastNTPTime = 0;
const unsigned long NTP_INTERVAL = (30 * 24 * 60 * 60); // NTP sync once in 30 days


String months[] = {String("January "), String("February "), String("March "),
		   String("April "), String("May "), String("June "),
		   String("July "), String("August "), String("September "),
		   String("October "), String("November "), String("December ")};

String wdays[] = {String("Sunday, "), String("Monday, "), String("Tuesday, "), String("Wednesday, "),
		  String("Thursday, "), String("Friday, "), String("Saturday, ")};

String shortwdays[] = {String("SUN"), String("MON"), String("TUE"), String("WED"),
		       String("THU"), String("FRI"), String("SAT"), String("SUN")};

void dateText(char *buf, unsigned d)
{
  if (d < 10) {
    buf[0] = '0' + d;
    buf[1] = '\0';
  }
  else {
    buf[0] = '0' + d / 10;
    buf[1] = '0' + d % 10;
    buf[2] = 0;
  }
}

unsigned getLastDayOfMonth(time_t t)
{
  int mday;
  struct tm timeInfo;
  localtime_r(&t, &timeInfo);
  mday = timeInfo.tm_mday;

  t += (32 - mday) * 24 * 3600;
  localtime_r(&t, &timeInfo);
  return 32 - timeInfo.tm_mday;
}

void
showCalendar(time_t t)
{
  struct tm timeInfo;
  int32_t posx = 0, posy = 30;
  String today;
  char buf[3];
  int lastday;

  Serial.println("showCalendar() called.");
  
  M5.EPD.Clear(true); 
  canvas.fillCanvas(0);

  showBatteryStatus();

  localtime_r(&t, &timeInfo);
  lastday = getLastDayOfMonth(t);

  String wdayname = wdays[timeInfo.tm_wday];
  String monthname = months[timeInfo.tm_mon];
  date = timeInfo.tm_mday;

  dateText(buf, date);
  today = wdayname + monthname + String(buf);

  // Show info for Today
  canvas.setTextSize(8);
  canvas.drawString(today, (SCREEN_WIDTH - canvas.textWidth(today)) / 2, posy);

  canvas.setTextSize(6);
  posy += 90;
  posx = 0;

  // showing week name header
  for (int i = 1 ; i <= 7 ; i++) {
    canvas.drawString(shortwdays[i], posx + (COLWIDTH - canvas.textWidth(shortwdays[i])) / 2, posy);
    posx += COLWIDTH;
  }
  posy += COLHEIGHT;

  // getting week of day
  int wod = (timeInfo.tm_wday + 35 - (timeInfo.tm_mday - 1)) % 7;
  if (wod == 0) {
    wod = 7;
  }

  #define REVPADDING 10
  
  for (unsigned i = 1 ; i <= lastday ; i++) {
    String s;

    if (i == date) {
      canvas.setTextColor(0);
      canvas.fillRect((wod - 1) * COLWIDTH + REVPADDING, posy - 5,
		      COLWIDTH - 2 * REVPADDING, COLHEIGHT, 15);
    }

    dateText(buf, i);
    s = String(buf);
    canvas.drawString(s, (wod - 1) * COLWIDTH + (COLWIDTH - canvas.textWidth(s)) / 2, posy);
    if (7 < ++wod) {
      wod = 1;
      posy += COLHEIGHT;
    }

    if (i == date) {
      canvas.setTextColor(15);
    }
  }
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

static const time_t recentPastTime = 1500000000UL; // 2017/7/14 2:40:00 JST

bool NTPSync()
{
  bool retval = false;

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (WiFi.status() == WL_CONNECTED) {
    // check NTP
    configTime(9 * 3600, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");
    time_t t;
    for (t = 0 ; t < recentPastTime ; t = time(NULL)); // wait for NTP to synchronize
    lastNTPTime = t;
    retval = true;
  }
  return retval;
}

void setup()
{
  bool firstBoot = false;
  configTime(9 * 3600, 0, nullptr); // set time zone as JST-9
  // The above is performed without network connection.

  pref.begin(prefname, false);
  date = pref.getInt("date", -1);
  lastNTPTime = pref.getLong("lastntp", 0);
  pref.clear(); // clear the preferences for unexpected reset
  pref.end();

  //  Check power on reason before calling M5.begin()
  //  which calls RTC.begin() which clears the timer flag.
  Wire.begin(21, 22);                  
  uint8_t data = M5.RTC.readReg(0x01);
  bool wokeup = ((data & 4) == 4); // true means woke up by RTC, not by power button

  // M5.enableMainPower();
  M5.begin();
  Serial.println("Program started.");
  
  if (!wokeup || date == -1) {
    firstBoot = true;
    Serial.println("first boot.");
  }
  firstBoot = true; // for maintenance reason.

  time_t t = time(NULL);
  struct tm timeInfo;
  localtime_r(&t, &timeInfo);

  if (!firstBoot && recentPastTime < t && date == timeInfo.tm_mday) {
    rtc_time_t wutime;
    Serial.println("Shutting down right now.");
    wutime.hour = wutime.min = wutime.sec = 0;
    M5.shutdown(wutime);
  }
  else {
    // M5.enableEPDPower();
    M5.EPD.SetRotation(0); // electric paper display
    M5.TP.SetRotation(0); // touch panel
    M5.EPD.Clear(true); 
    M5.RTC.begin(); // real time clock

    Serial.println("Some updates required.");
    
    if (firstBoot) {
      canvas.createCanvas(SCREEN_WIDTH, SCREEN_HEIGHT);
      canvas.setTextSize(3);
      
      showMessage(String("First boot."));
      delay(1000);

      Serial.println("Synchronizing time required.");
      connectWiFi();
      showMessage(String("Synchronizing time..."));
      if (NTPSync()) {
	showMessage(String("Time synchromized."));
      }
      else {
	showMessage(String("Synchromization failed."));
      }
    }
    delay(500);
    time_t t = time(NULL);
    showCalendar(t);
    canvas.pushCanvas(0,0,UPDATE_MODE_DU4);
    delay(500);

    pref.begin(prefname, false);
    pref.putInt("date", date);
    pref.putLong("lastntp", lastNTPTime);
    pref.end();
    rtc_time_t wutime;
    wutime.hour = wutime.min = wutime.sec = 0;
    M5.shutdown(wutime);

    delay(500);
  }
}

void loop()
{
  if (M5.BtnP.wasPressed()) {
    rtc_time_t wutime;
    wutime.hour = wutime.min = wutime.sec = 0;
    M5.shutdown(wutime);
  }
  M5.update();
}

