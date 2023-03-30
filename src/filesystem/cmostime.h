#ifndef _TIMEE_H_
#define _TIMEE_H_

#define CURRENT_YEAR        2023                        // Change this each year!
#include "../lib-header/portio.h"

extern int century_register;                                // Set by ACPI table parsing code if possible
 
extern int second;
extern int minute;
extern int hour;
extern int day;
extern int month;
extern int year;
 
 
enum {
      cmos_address = 0x70,
      cmos_data    = 0x71
};
 
int get_update_in_progress_flag();
 
unsigned char get_RTC_register(int reg); 
 
void read_rtc();

uint16_t getDateEncode();

int getYearDecode(uint16_t date);

int getMonthDecode(uint16_t date);

int getDayDecode(uint16_t date);

// only store hours and minutes. Ga muat kalo sama sekon?
uint16_t getTimeEncode();

int getHourDecode(uint16_t time);

int getMinuteDecode(uint16_t time);


#endif