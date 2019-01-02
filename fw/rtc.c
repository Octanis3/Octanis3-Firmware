/*
 * rtc.c
 *
 *  Created on: 24 Dec 2018
 *      Author: raffael
 */

#include "../Board.h"
#include <xdc/cfg/global.h> //needed for semaphore

#include <time.h>
#include <ti/sysbios/hal/Seconds.h>
#include <msp430.h>

#include "rtc.h"
#include "logger.h"

#define DEFAULT_RESUME_HOUR    16  // UTC time! i.e. default resume time is 17h in winter
#define DEFAULT_PAUSE_HOUR     8  // UTC time! i.e. default system shutdown time is 9 am in winter



// RTC times when the system will pause operation (usually in the morning)
static uint8_t pause_hour = DEFAULT_PAUSE_HOUR;
static uint8_t pause_minute = 0;
// RTC times when the system will resume operating (usually at break of dawn)
static uint8_t resume_hour = DEFAULT_RESUME_HOUR;
static uint8_t resume_minute = 0;


uint8_t rtc_get_p_min() {return pause_minute;}
uint8_t rtc_get_r_min() {return resume_minute;}
uint8_t rtc_get_p_hour() {return pause_hour;}
uint8_t rtc_get_r_hour() {return resume_hour;}

void rtc_set_pause_times(uint8_t p_hour, uint8_t p_min, uint8_t r_hour, uint8_t r_min)
{
    // assert all values are within allowed bounds.
    if(p_hour > 23)
        p_hour = DEFAULT_PAUSE_HOUR;
    if(r_hour > 23)
        r_hour = DEFAULT_RESUME_HOUR;
    if(p_min > 59)
        p_min = 0;
    if(r_min > 59)
        r_min = 0;
    // assert that resume hour is strictly bigger than pause hour.
    if(r_hour>p_hour)
    {
        pause_hour = p_hour;
        resume_hour = r_hour;
    }
    else
    {
        pause_hour = DEFAULT_PAUSE_HOUR;
        resume_hour = DEFAULT_RESUME_HOUR;
    }
    pause_minute = p_min;
    resume_minute = r_min;

    log_set_rtc_pause_times(); //and back up the values!
}

void rtc_set_pause_times_compact(uint32_t value)
{
    rtc_set_pause_times((value   )  & 0x000000FF,
                        (value>>8)  & 0x000000FF,
                        (value>>16) & 0x000000FF,
                        (value>>24) & 0x000000FF);
}

uint32_t rtc_get_pause_times_compact()
{
    uint32_t value = 0;

    value = resume_minute;
    value = (value << 8) + resume_hour;
    value = (value << 8) + pause_minute;
    value = (value << 8) + pause_hour;

    return value;
}


void rtc_calibration()
{
    // compensate for LFX deviation:
        RTCCTL01 = RTCHOLD;

        // measured 511.824689 Hz! --> -342 ppm --> RTCCAL = 342/4.34 = 79 --> maxed out!!

        RTCCTL23 = RTCCAL5 + RTCCAL4 + RTCCAL3 + RTCCAL2 + RTCCAL1 + RTCCAL0;
        RTCCTL23 |= RTCCALS;



/*********** OLD VALUES: ********/
//        // Here we compensate for 11.1ppm = 5*2.17 --> 5 = 0b101
//
//        RTCCTL23 = RTCCAL2 + RTCCAL0;
//        RTCCTL23 &= ~RTCCALS;

        RTCCTL01 &= ~(RTCHOLD);                 // Start RTC
}

void rtc_config()
{
    rtc_calibration();
}

void rtc_set_clock(uint32_t unix_timestamp)
{
    struct tm *ltm;
    /* retrieve current time relative to Jan 1, 1970 */
    /*
     * Use overridden time() function to get the current time.
     * Use standard C RTS library functions with return from time().
     * Assumes Seconds_set() has been called as above
     */
    time_t t1 = unix_timestamp + 2208988800; // add offset from year 1900 to 1970.
    ltm = localtime(&t1);

    RTCCTL01 =  /* | RTCBCD*/ RTCHOLD; //RTCBCD = 0 --> binary mode!
                                            // RTC enable, binary mode, RTC hold

    // we dont care about the date, only about the time of day
    RTCYEAR = ltm->tm_year+1900;   // Year – low byte. Valid values of Year are 0 to 4095.
    RTCMON = ltm->tm_mon+1;     // Month. Valid values are 1 to 12.                               // Month = 0x04 = April
    RTCDAY = ltm->tm_mday;    // Day of month. Valid values are 1 to 31.                        // Day = 0x05 = 5th
    RTCDOW = ltm->tm_wday;    // Day of week. Valid values are 0 to 6.                 // Day of week = 0x01 = Monday
    RTCHOUR = ltm->tm_hour;   // Hours. Valid values are 0 to 23. // ((unix_timestamp % 86400) - (unix_timestamp % 3600))/3600; // Hour
    RTCMIN = ltm->tm_min;     // Minutes. Valid values are 0 to 59. //  ((unix_timestamp % 3600) - (unix_timestamp % 60))/60;      // Minute
    RTCSEC = ltm->tm_sec;     // Seconds. Valid values are 0 to 59. //(unix_timestamp % 60);                            // Seconds


    RTCCTL01 &= ~(RTCHOLD);                 // Start RTC

    rtc_update_system_time();
}

void rtc_update_system_time()
{
    struct tm ltm;
    time_t unix_timestamp;

    // we dont care about the date, only about the time of day
    ltm.tm_year = RTCYEAR - 1900;   // Year – low byte. Valid values of Year are 0 to 4095.
    ltm.tm_mon = RTCMON - 1;     // Month. Valid values are 1 to 12.                               // Month = 0x04 = April
    ltm.tm_mday = RTCDAY;    // Day of month. Valid values are 1 to 31.                        // Day = 0x05 = 5th
    ltm.tm_wday = RTCDOW;    // Day of week. Valid values are 0 to 6.                 // Day of week = 0x01 = Monday
    ltm.tm_hour = RTCHOUR;   // Hours. Valid values are 0 to 23. // ((unix_timestamp % 86400) - (unix_timestamp % 3600))/3600; // Hour
    ltm.tm_min = RTCMIN;     // Minutes. Valid values are 0 to 59. //  ((unix_timestamp % 3600) - (unix_timestamp % 60))/60;      // Minute
    ltm.tm_sec = RTCSEC;     // Seconds. Valid values are 0 to 59. //(unix_timestamp % 60);                            // Seconds

    unix_timestamp = mktime(&ltm) - 2208988800; // add offset from year 1900 to 1970.

    Seconds_set(unix_timestamp);
}

static enum rtc_state_ {
    NIGHT_SHIFT,
    DAY_BREAK,
    USER_INTERRUPT
} rtc_state = NIGHT_SHIFT;

int rtc_is_it_time_to_pause()
{
    int retval = 0;
    if(RTCHOUR > pause_hour && RTCHOUR < resume_hour)
        retval = 1;
    else if(RTCHOUR >= pause_hour && RTCHOUR < resume_hour && RTCMIN >= pause_minute)
    {
        retval = 1;
    }

    if(retval == 1)
    {
        if(rtc_state == NIGHT_SHIFT)
            retval = 2; // to indicate that the fram content shall be flushed to SD card.
        rtc_state = DAY_BREAK;
    }

    return retval;
}

//static uint32_t seconds_till_resume = 0;

void rtc_pause_system()
{
    if(resume_hour > 23)
        resume_hour = DEFAULT_RESUME_HOUR;
    if(resume_minute > 59)
        resume_minute = 0;

    RTCCTL01 |= RTCAIE  /* | RTCBCD*/ | RTCHOLD; //RTCBCD = 0 --> binary mode!

    //configure the alarm to wake up
    RTCADAY = 0;
    RTCADOW = 0;
    RTCAHOUR = RTCAE | resume_hour;     // RTC Hour Alarm
    RTCAMIN = RTCAE | resume_minute;    // RTC Minute Alarm

    RTCCTL01 &= ~(RTCHOLD);             // Start RTC

//    seconds_till_resume = ((((RTCAHOUR & 0x7f) + 24) - RTCHOUR) % 24)*3600;
//    seconds_till_resume = seconds_till_resume +
//                          (((RTCAMIN & 0x7f) - RTCMIN) % 60)*60;
//    seconds_till_resume = seconds_till_resume - RTCSEC;
    Clock_tickStop();
}

void rtc_resume_system()
{
    // restore system timer value
    //Seconds_set(Seconds_get()+seconds_till_resume);
    Clock_tickStart();
    rtc_update_system_time();
    if(rtc_state == DAY_BREAK) // as the state was not changed in the ISR to NIGHT_SHIFT,
                               // it means the system was woken up by the user.
    {
        rtc_state = USER_INTERRUPT;
    }
}

// IV 0FFCEh
// reacts to: RTCRDYIFG, RTCTEVIFG, RTCAIFG, RT0PSIFG, RT1PSIFG, RTCOFIFG, (RTCIV)
void rtc_isr()
{
    switch(__even_in_range(RTCIV, RTCIV_RT1PSIFG))
    {
        case RTCIV_NONE:      break;        // No interrupts
        case RTCIV_RTCOFIFG:  break;        // RTCOFIFG
        case RTCIV_RTCRDYIFG: break;        // RTCRDYIFG
        case RTCIV_RTCTEVIFG: break;        // RTCEVIFG
        case RTCIV_RTCAIFG:
            //Resume system:
            rtc_state = NIGHT_SHIFT;
            Semaphore_post((Semaphore_Handle)semSystemPause);
            break;        // RTCAIFG
        case RTCIV_RT0PSIFG:  break;        // RT0PSIFG
        case RTCIV_RT1PSIFG:  break;        // RT1PSIFG
        default: break;
    }
}


