/**
 *  Name:      AutoIrrigation
 *  Author:    Alexandre Boni
 *  Created:   2013/05/27
 *  Modified:  2015/08/08
 *  Version:   0.3
 *  IDE:       Arduino 1.6.5-r2
 *  Profile:   Digispark(Default - 16.5mhz)
 *  Hardware:  Digispark (ATtiny85)
 *
 *  Pins:   P0(SDA) <-> DS1307 SDA
 *          P2(SCL) <-> DS1307 SCL
 *          P5 -> MOSFET G1
 *
 *  Release:
 *    0.3
 *          Code clean-up and time adjustment.
 *
 *    0.2
 *          Add minute in the system to be more accurate.
 *          Use a 16 bits uint with 8 MSB for hour and
 *          8 LSB for minute. Hour and minute are in HEX.
 *
 *    0.1
 *          Creation of this code.
 *
 *
 *
 */

/**
 * Includes
 */
#include <TinyWireM.h>
#include <DigiUSB.h>
#include "TinyRTClib.h"

/**
 * Defines
 */
#define Sunday    0
#define Monday    1
#define Tuesday   2
#define Wednesday 3
#define Thursday  4
#define Friday    5
#define Saturday  6

#define pinValve  5

/**
 *  Structure:      TargetDay
 *
 *  Description:    This structure is used to setup the targeted period
 *                  Set the day and hour to start
 *                  Set the day and hour to stop
 *                  Days should be in decimal format
 *                  Hours and minutes should be in hex format
 *                  But you can use your favorite format
 *
 */
typedef struct _TargetDay
{
  uint8_t StartDayOfWeek;  /* Day to start:0-6, 0=Sunday */
  uint8_t StopDayOfWeek;   /* Day to stop:0-6, 0=Sunday */
  uint16_t StartHour;      /* Hour to start:0xHHMM, hour and min in hex */
  uint16_t StopHour;       /* Hour to stop:0xHHMM, hour and min in hex */
}TargetDay;

/**
 * Prototypes
 */
boolean checkTarget(uint8_t day, uint16_t hour, TargetDay* target);

/**
 * Global variables
 * Note: These variables are declared as global due to Arduino functions (setup and loop)
 *       Currently, these variables are in the main()
 */
/* Variable used for Real Time Clock (RTC) */
RTC_DS1307 RTC;

/* Target array with every targeted days and hours */
TargetDay RainyDay[] = 
{
  {
    Tuesday,
    Tuesday,
    0x0300,   /* 3h00min */
    0x0400    /* 4h00min */
  },
  {
    Thursday,
    Thursday,
    0x031E,   /* 3h30min */
    0x041E    /* 4h30min */
  },
  {
    Saturday,
    Saturday,
    0x0400,   /* 4h00min */
    0x0500    /* 5h00min */
  },
#if 0
  {
    Sunday,
    Sunday,
    0x0105,  /* 01h05min */
    0x010A   /* 01h10min */
  }
#endif
};

/**
 * Setup function (in main)
 */
void setup ()
{
  TinyWireM.begin();
  RTC.begin();
  
  pinMode(pinValve, OUTPUT);
  
  /* Check if RTC is configured */
  if (! RTC.isrunning())
  {
#if 0
    RTC.adjust(DateTime(__DATE__, __TIME__));
#endif
    digitalWrite(pinValve, 0);
  }
  else
  {
    digitalWrite(pinValve, 1);
  }
}

/**
 * Loop function (in main)
 */
void loop ()
{
  DateTime  now         = RTC.now();
  uint8_t   past        = now.second();
  uint16_t  k           = 0;
  uint16_t  hour_ex     = 0x0000;
  boolean   runTarget   = false;
  boolean   startTarget = false;
  
  while(1)
  {
    past = now.minute();
    do
    {
      delay(1000);
      now = RTC.now();
    } while(now.minute() == past);
    
    hour_ex = ( (now.hour()&0xFF) << 8 ) + ( now.minute()&0xFF );
    for(k = 0; k < sizeof(RainyDay)/sizeof(TargetDay); k++)
    {
      runTarget = checkTarget(now.dayOfWeek(), hour_ex, &RainyDay[k]);
      if(runTarget == true)  break;
    }
        
    if(runTarget && !startTarget)       startTarget = true;
    else if(!runTarget && startTarget)  startTarget = false;
    
    digitalWrite(pinValve, startTarget);
  }
}

/**
 *  Procedure:      checkTarget
 *
 *  Description:    This function compares current day and hour (hour+minute)
 *                  with the TargetDay (target array).
 *
 *  Inputs:         day - Value between 0 and 6, 0 for Sunday and 6 for Saturday
 *                  hour - Value includes hour and minute
 *                         In hex format, 0xHHMM with HH for Hour and MM for Minute
 *                         24h-mode should be used
 *                         i.e.: 16h32 = 0x1020, or 05h54 = 0x0536
 *                         Note that 12h is noon, and 00h is midnight (avoid 24h for midnight)
 *                  target - pointer of TargetDay structure
 *
 *  Outputs:        true - the day and hour are in one of targeted periods in TargetDay
 *                  false - otherwise
 *
 */
boolean checkTarget(uint8_t day, uint16_t hour, TargetDay* target)
{
  uint8_t startDay;
  uint8_t stopDay;
  
  if(target == NULL) return false;
  
  startDay = target->StartDayOfWeek;
  stopDay = target->StopDayOfWeek;
  
  if(stopDay < startDay)
  {
    if(day > stopDay)
    {
      stopDay += 7;
    }
    else if(day <= stopDay)
    {
      day += 7;
      stopDay += 7;
    }
  }
  
  if( (day >= startDay)   &&
      (day <= stopDay)    &&
      ( (((day-startDay)*24) << 8) + hour >= target->StartHour ) &&
      (hour < (((stopDay-day)*24) << 8) + target->StopHour)
  )
  {
    return true;
  }
  return false;
}

