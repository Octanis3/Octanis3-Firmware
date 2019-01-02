/*
 * rtc.h
 *
 *  Created on: 24 Dec 2018
 *      Author: raffael
 */

#ifndef FW_RTC_H_
#define FW_RTC_H_


void rtc_config();
void rtc_set_clock(uint32_t unix_timestamp);
void rtc_update_system_time();

// set new values for pause and resume times:
void rtc_set_pause_times(uint8_t p_hour, uint8_t p_min, uint8_t r_hour, uint8_t r_min);
void rtc_set_pause_times_compact(uint32_t value);
uint32_t rtc_get_pause_times_compact();
uint8_t rtc_get_p_min();
uint8_t rtc_get_r_min();
uint8_t rtc_get_p_hour();
uint8_t rtc_get_r_hour();

// polling function to check if it is time to shut down the system:
int rtc_is_it_time_to_pause();

// turns off the system tick and sets an alarm to wake up again in the evening:
void rtc_pause_system();
//turn on system again
void rtc_resume_system();


#endif /* FW_RTC_H_ */
