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

#define DEFAULT_RESUME_HOUR    16  // UTC time! i.e. default resume time is 17h in winter
#define DEFAULT_PAUSE_HOUR     8  // UTC time! i.e. default system shutdown time is 9 am in winter



// RTC times when the system will pause operation (usually in the morning)
static uint8_t pause_hour = DEFAULT_PAUSE_HOUR;
static uint8_t pause_minute = 0;
// RTC times when the system will resume operating (usually at break of dawn)
static uint8_t resume_hour = DEFAULT_RESUME_HOUR;
static uint8_t resume_minute = 0;

void rtc_example()
{
    UInt32 t;
    time_t t1;
    struct tm *ltm;
    char *curTime;
    /* retrieve current time relative to Jan 1, 1970 */
    t = Seconds_get();
    /*
     * Use overridden time() function to get the current time.
     * Use standard C RTS library functions with return from time().
     * Assumes Seconds_set() has been called as above
     */
    t1 = time(NULL);
    ltm = localtime(&t1);
    curTime = asctime(ltm);
}

uint8_t rtc_get_p_min() {return pause_minute;}
uint8_t rtc_get_r_min() {return resume_minute;}
uint8_t rtc_get_p_hour() {return pause_hour;}
uint8_t rtc_get_r_hour() {return resume_minute;}

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
}

void rtc_calibration()
{
    // compensate for LFX deviation:
        RTCCTL01 = RTCHOLD;

        //TODO: make correct compensation
        // Here we compensate for 11.1ppm = 5*2.17 --> 5 = 0b101
        RTCCTL23 = RTCCAL2 + RTCCAL0;
        RTCCTL23 &= ~RTCCALS;

        RTCCTL01 &= ~(RTCHOLD);                 // Start RTC
}


void rtc_config()
{
    rtc_calibration();

}


void rtc_set_clock(int32_t unix_timestamp)
{

    RTCCTL01 =  /* | RTCBCD*/ RTCHOLD; //RTCBCD = 0 --> binary mode!
                                            // RTC enable, binary mode, RTC hold

    // we dont care about the date, only about the time of day
    RTCYEAR = 2019;                          // Year = 0x2010
    RTCMON = 1;                             // Month = 0x04 = April
    RTCDAY = 1;                             // Day = 0x05 = 5th
   // RTCDOW = 0x01;                          // Day of week = 0x01 = Monday
    RTCHOUR = ((unix_timestamp % 86400) - (unix_timestamp % 3600))/3600; // Hour
    RTCMIN = ((unix_timestamp % 3600) - (unix_timestamp % 60))/60;      // Minute
    RTCSEC = (unix_timestamp % 60);                            // Seconds


    RTCCTL01 &= ~(RTCHOLD);                 // Start RTC

    Seconds_set(unix_timestamp);
}

int rtc_is_it_time_to_pause()
{
    if(RTCHOUR > pause_hour && RTCHOUR < resume_hour)
        return 1;
    else if(RTCHOUR >= pause_hour && RTCHOUR < resume_hour && RTCMIN >= pause_minute)
    {
        return 1;
    }
    else return 0;
}

static uint32_t seconds_till_resume = 0;

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

    seconds_till_resume = ((((RTCAHOUR & 0x7f) + 24) - RTCHOUR) % 24)*3600;
    seconds_till_resume = seconds_till_resume +
                          (((RTCAMIN & 0x7f) - RTCMIN) % 60)*60;
    seconds_till_resume = seconds_till_resume - RTCSEC;
    Clock_tickStop();
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
            // restore system timer value
            Seconds_set(Seconds_get()+seconds_till_resume);
            //Resume system:
            Clock_tickStart();
            Semaphore_post((Semaphore_Handle)semSystemPause);
            break;        // RTCAIFG
        case RTCIV_RT0PSIFG:  break;        // RT0PSIFG
        case RTCIV_RT1PSIFG:  break;        // RT1PSIFG
        default: break;
    }
}


