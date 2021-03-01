
#include "time_manager.h"

ESP32Time rtc;

/**
 * Set the RTC time using the epoch time.
 * @param: uint64_t seconds elapsed time
 */
void set_rtc_time(uint64_t epoch_time){
  // setting time zone is not working. 
  // rtc.setTimeZone();

  // let's try to fix this by manually subtracting the number of milliseconds for PST from GMT.
  // since PST is 8 hours behind GMT we subtract 8 hours worth of milliseconds. Also, during 
  // during daylight savings, we need to add 1 hour worth of milliseconds which will add another 
  // requirements of knowing when daylight saving starts and ends. This is left for now.
  uint64_t timiZoneMillis = 8 * 60 * 60  * 1000;
  rtc.setTimeEpoch(epoch_time - timiZoneMillis);
}

/**
 * Print current RTC time to Serial.
 */
void show_current_rtc_time() {
  struct tm timeinfo = rtc.getTimeStruct();
	char s[51];

  // long date format.
	strftime(s, 50, "%A, %B %d %Y %H:%M:%S", &timeinfo);
  Serial.printf("Current RTC time: %s\n", s);

}

/**
 * Get current RTC time as string.
 * @param: const char * to store the time string.
 */
void get_rtc_time_as_string(char * buffer) {
  if(buffer == NULL) {
    Serial.println("get_rtc_time_as_string: null buffer");
  }

  // get the time
  struct tm timeinfo = rtc.getTimeStruct();
	
  // long date format.
	strftime(buffer, 50, "%A_%B_%d_%Y_%H_%M_%S", &timeinfo);
  return;

  // return rtc.getTime("%d-%m-%Y:%H-%M-%S");
  // Serial.println(rtc.getTime());          //  (String) 15:24:38
  // Serial.println(rtc.getDate());          //  (String) Sun, Jan 17 2021
  // Serial.println(rtc.getDate(true));      //  (String) Sunday, January 17 2021
  // Serial.println(rtc.getDateTime());      //  (String) Sun, Jan 17 2021 15:24:38
  // Serial.println(rtc.getDateTime(true));  //  (String) Sunday, January 17 2021 15:24:38
  // Serial.println(rtc.getTimeDate());      //  (String) 15:24:38 Sun, Jan 17 2021
  // Serial.println(rtc.getTimeDate(true));  //  (String) 15:24:38 Sunday, January 17 2021
  // Serial.println(rtc.getMicros());        //  (long)    723546
  // Serial.println(rtc.getMillis());        //  (long)    723
  // Serial.println(rtc.getEpoch());         //  (long)    1609459200
  // Serial.println(rtc.getSecond());        //  (int)     38    (0-59)
  // Serial.println(rtc.getMinute());        //  (int)     24    (0-59)
  // Serial.println(rtc.getHour());          //  (int)     3     (0-12)
  // Serial.println(rtc.getHour(true));      //  (int)     15    (0-23)
  // Serial.println(rtc.getAmPm());          //  (String)  pm
  // Serial.println(rtc.getAmPm(true));      //  (String)  PM
  // Serial.println(rtc.getDay());           //  (int)     17    (1-31)
  // Serial.println(rtc.getDayofWeek());     //  (int)     0     (0-6)
  // Serial.println(rtc.getDayofYear());     //  (int)     16    (0-365)
  // Serial.println(rtc.getMonth());         //  (int)     0     (0-11)
  // Serial.println(rtc.getYear());          //  (int)     2021
  // return rtc.getTimeDate(true);
}

// void printLocalTime()
// {
//   struct tm timeinfo;
//   if (!getLocalTime(&timeinfo)) {
//     Serial.println("Failed to obtain time");
//     return;
//   }
//   Serial.print(&timeinfo, "%A, %B %d %Y %H:%M:%S");//Friday, February 22 2019 22:37:45
// }


/*
  %a Abbreviated weekday name
  %A Full weekday name
  %b Abbreviated month name
  %B Full month name
  %c Date and time representation for your locale
  %d Day of month as a decimal number (01-31)
  %H Hour in 24-hour format (00-23)
  %I Hour in 12-hour format (01-12)
  %j Day of year as decimal number (001-366)
  %m Month as decimal number (01-12)
  %M Minute as decimal number (00-59)
  %p Current locale's A.M./P.M. indicator for 12-hour clock
  %S Second as decimal number (00-59)
  %U Week of year as decimal number,  Sunday as first day of week (00-51)
  %w Weekday as decimal number (0-6; Sunday is 0)
  %W Week of year as decimal number, Monday as first day of week (00-51)
  %x Date representation for current locale
  %X Time representation for current locale
  %y Year without century, as decimal number (00-99)
  %Y Year with century, as decimal number
  %z %Z Time-zone name or abbreviation, (no characters if time zone is unknown)
  %% Percent sign
  You can include text literals (such as spaces and colons) to make a neater display or for padding between adjoining columns.
  You can suppress the display of leading zeroes  by using the "#" character  (%#d, %#H, %#I, %#j, %#m, %#M, %#S, %#U, %#w, %#W, %#y, %#Y)
*/