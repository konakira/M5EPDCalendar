#include <M5EPD.h>
#include <WiFi.h>
#include <Preferences.h>

#include "auth.h"

Preferences pref;
#define prefname "calendar"

M5EPD_Canvas canvas(&M5.EPD);

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540

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
  canvas.setTextSize(64);
  canvas.setTextDatum(MC_DATUM);
  canvas.drawString(mesg, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
  canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
}

void showTime(time_t t)
{
  struct tm timeInfo;
  localtime_r(&t, &timeInfo);

  String now = String(1900 + timeInfo.tm_year) + String("/") +
    String(1 + timeInfo.tm_mon) + String("/") +
    String(timeInfo.tm_mday) + String(" ") +
    String(timeInfo.tm_hour) + String(":") +
    String(timeInfo.tm_min) + String(":") +
    String(timeInfo.tm_sec);

  showMessage(now);
  delay(1000);
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

  canvas.setTextSize(32);
  canvas.setTextDatum(BR_DATUM);
  canvas.drawString(buf, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
  canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
}

static unsigned day = 0;
static time_t lastNTPTime = 0;
static bool useRTCTime = false;
const unsigned long NTP_INTERVAL = (30 * 24 * 60 * 60); // NTP sync once in 30 days

String months[] = {String("January "), String("February "), String("March "),
		   String("April "), String("May "), String("June "),
		   String("July "), String("August "), String("September "),
		   String("October "), String("November "), String("December ")};

String wdays[] = {String("Sunday, "), String("Monday, "), String("Tuesday, "), String("Wednesday, "),
		  String("Thursday, "), String("Friday, "), String("Saturday, ")};

String shortwdays[] = {String("SUN"), String("MON"), String("TUE"), String("WED"),
		       String("THU"), String("FRI"), String("SAT"), String("SUN")};

void dayText(char *buf, unsigned d)
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
  day = timeInfo.tm_mday;

  dayText(buf, day);
  today = wdayname + monthname + String(buf);

  // Show info for Today
  canvas.setTextDatum(TC_DATUM);
  canvas.setTextSize(64);
  canvas.drawString(today, SCREEN_WIDTH / 2, posy);

  posy += 90;
  posx = 0;

  canvas.setTextSize(32);
  // showing week name header
  for (int i = 1 ; i <= 7 ; i++) {
    canvas.drawString(shortwdays[i], posx + COLWIDTH / 2, posy);
    posx += COLWIDTH;
  }
  posy += COLHEIGHT;

  // getting week of day
  int wod = (timeInfo.tm_wday + 35 - (timeInfo.tm_mday - 1)) % 7;
  if (wod == 0) {
    wod = 7;
  }

  #define REVPADDING 10
  
  canvas.setTextSize(48);
  for (unsigned i = 1 ; i <= lastday ; i++) {
    if (i == day) {
      canvas.setTextColor(0);
      canvas.fillRect((wod - 1) * COLWIDTH + REVPADDING, posy - 5,
		      COLWIDTH - 2 * REVPADDING, COLHEIGHT, 15);
    }

    dayText(buf, i);
    canvas.setTextDatum(TC_DATUM);
    canvas.drawString(buf, (wod - 1) * COLWIDTH + COLWIDTH / 2, posy);
    if (7 < ++wod) {
      wod = 1;
      posy += COLHEIGHT;
    }

    if (i == day) {
      canvas.setTextColor(15);
    }
  }
  canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
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
}

// reading RTC to set the system time via settimeofday().
void rtcTime()
{
  rtc_time_t rt;
  rtc_date_t rd;
  struct tm ti;

  M5.RTC.getTime(&rt);
  M5.RTC.getDate(&rd);
  {
    ti.tm_year = rd.year - 1900;
    ti.tm_mon = rd.mon - 1;
    ti.tm_mday = rd.day;
    ti.tm_hour = rt.hour;
    ti.tm_min = rt.min;
    ti.tm_sec = rt.sec;
    ti.tm_isdst = 0;
    ti.tm_wday = ti.tm_yday = 0; // mktime() ignores them.
  }
  time_t t = mktime(&ti);
  struct timeval tv;
  tv.tv_sec = t;
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);
}

// setting RTC from the system time
void setRtcTime(time_t t)
{
  rtc_time_t rt;
  rtc_date_t rd;
  struct tm ti;
  localtime_r(&t, &ti);
  {
    rd.year = ti.tm_year + 1900;
    rd.mon = ti.tm_mon + 1;
    rd.day = ti.tm_mday;
    rt.hour = ti.tm_hour;
    rt.min = ti.tm_min;
    rt.sec = ti.tm_sec;
  }
  M5.RTC.setTime(&rt);
  M5.RTC.setDate(&rd);
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
    setRtcTime(t);
    retval = true;
  }
  return retval;
}

void shutdownToWakeup()
{
  Serial.println("Shutting down right now.");

  rtc_time_t wutime;
  wutime.hour = 0;
  wutime.min = 0;
  wutime.sec = 00; // sec ignored

  time_t t = time(NULL);
  struct tm ti;
  localtime_r(&t, &ti);

  if (ti.tm_hour == wutime.hour && ti.tm_min == wutime.min) {
    // wait for 60 seconds to prevent waking up just after shutdown().
    delay((60 - ti.tm_sec) * 1000); 
  }

  { // showing current time for debug purpose
    char buf[40];
    String mesg;

    localtime_r(&lastNTPTime, &ti);

    snprintf(buf, sizeof(buf), "Last time sync: %s %d", months[ti.tm_mon], ti.tm_mday);

    canvas.setTextSize(32);
    canvas.setTextDatum(BC_DATUM);
    canvas.drawString(buf, SCREEN_WIDTH / 2, SCREEN_HEIGHT - 1);
    Serial.println(buf);
    canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
    delay(1000);
  }

  // M5.shutdown(wutime) works only if modifying the source code of
  // .pio/libdeps/m5stack-fire/M5EPD/src/utility/BM8563.cpp to be commented out
  // the following portion around line 229:
  // out_buf[2] = 0x00;
  // out_buf[3] = 0x00;
  M5.shutdown(wutime);
  // this may fail but will take effect after unplugging USB cable,
  // so that, it is not necessary to shutdown() in loop().
}

void
showDateToSerial(time_t t)
{
  struct tm ti;
  localtime_r(&t, &ti);
  
  Serial.print(ti.tm_year + 1900);
  Serial.print("/");
  Serial.print(ti.tm_mon + 1);
  Serial.print("/");
  Serial.println(ti.tm_mday);
}

void setup()
{
  configTime(9 * 3600, 0, nullptr); // set time zone as JST-9
  // The above is performed without network connection.

  //  Check power on reason before calling M5.begin()
  //  which calls RTC.begin() which clears the timer flag.
  /*
  {
    Wire.begin(21, 22);                  
    uint8_t data = M5.RTC.readReg(0x01);
    bool wokeup = ((data & 4) == 4); // true means woke up by RTC, not by power button
  }
  */
  
  pref.begin(prefname, false);
  day = pref.getInt("day", 0);
  lastNTPTime = pref.getLong("lastntp", 0);
  useRTCTime = pref.getBool("usertctime", 0);
  pref.clear(); // clear the preferences for unexpected reset
  pref.end();
  
  // M5.enableMainPower();
  M5.begin();
  Serial.println("Program started.");
  
  // M5.enableEPDPower();
  M5.EPD.SetRotation(0); // electric paper display
  // M5.TP.SetRotation(0); // touch panel
  M5.EPD.Clear(true); 
  M5.RTC.begin(); // real time clock

  canvas.createCanvas(SCREEN_WIDTH, SCREEN_HEIGHT);
  canvas.loadFont("/font.ttf", SD); // Load font files from SD Card
  
  canvas.createRender(32, 256);
  canvas.createRender(48, 256);
  canvas.createRender(64, 256);

  time_t t;
  if (useRTCTime) { // obtain current time from RTC
    rtcTime();
    t = time(NULL);
    struct tm timeInfo;
    localtime_r(&t, &timeInfo);
  }
  else { // obtain current time from NTP server. This happens every NTP_Interval
    connectWiFi();
    Serial.println("Synchronizing time...");
    showMessage(String("Synchronizing time..."));
    if (NTPSync()) {
      Serial.println("Synchronization done...");
      showMessage(String("Synchronization done..."));
    }
    else {
      Serial.println("Synchronization failed.");
      showMessage(String("Synchromization failed."));
    }
    t = time(NULL); // obtain synchronized time
  }

  showDateToSerial(lastNTPTime);
  showDateToSerial(lastNTPTime + NTP_INTERVAL);

  // synchronize with NTP server 30 days after last sync
  useRTCTime = (t < lastNTPTime + NTP_INTERVAL); // this variable will be used at the next wake up

  showCalendar(t);

  pref.begin(prefname, false);
  pref.putInt("day", day);
  pref.putLong("lastntp", lastNTPTime);
  pref.putBool("usertctime", useRTCTime);
  pref.end();

  delay(500);
  shutdownToWakeup();
  delay(500);
}

void loop()
{
  struct tm ti;
  M5.update();
  time_t t = time(NULL);
  localtime_r(&t, &ti);
  if (day != ti.tm_mday) { // update display if day changed
    showCalendar(t);
  }
}

