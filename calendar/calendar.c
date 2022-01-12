/**
 * @file      calendar.c
 * @author    Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * @brief     Source file with basic implementation of Gregorian Calendar
 * @version   1.2
 * @date      2021-18-01
 * @copyright Copyright (c) 2021
 */
#include <string.h>
#include <stdio.h>
#include "calendar.h"

/** @addtogroup   CalendarPrivate Calendar Private
 *  @ingroup Calendar Calendar
 * @{
 */
/**
 *  @brief Gregorian calendar data
 */
typedef struct GregorianDefinition
{
    char    *name;          ///<Name of the month
    uint16_t ordinal;       ///<Number of the month
    uint16_t days_in_month; ///<Number of days in the month
} gregorian_struct;
/**
 *  @brief  Vector of strings with the names of the days of the week
 */
const char weekday_names[WEEKDAY_QTY][10] = {
    "SUNDAY",    //
    "MONDAY",    //
    "TUESDAY",   //
    "WEDNESDAY", //
    "THURSDAY",  //
    "FRIDAY",    //
    "SATURDAY"   //
};
/**
 * @brief     Month definitions
 */
const gregorian_struct gregorian[12] = {
    {  "January",  1, 31}, //
    { "February",  2, 28}, //
    {    "March",  3, 31}, //
    {    "April",  4, 30}, //
    {      "May",  5, 31}, //
    {     "June",  6, 30}, //
    {     "July",  7, 31}, //
    {   "August",  8, 31}, //
    {"September",  9, 30}, //
    {  "October", 10, 31}, //
    { "November", 11, 30}, //
    { "December", 12, 31}  //
};

static const uint32_t TIME_SECONDS_IN_DAY         = 86400;    ///< Amount of seconds in a day
static const uint32_t TIME_SECONDS_IN_NORMAL_YEAR = 31536000; ///< Number of seconds in a year
static const uint32_t TIME_SECONDS_IN_LEAP_YEAR   = 31622400; ///< Number of seconds in a leap year

/** @}*/ // End of CalendarPrivate

/**
 *  @brief      Determines year is a leap year
 *  @param[in]  year:  the year of interest
 *  @return     1 if a leap year, 0 otherwise
 */
int8_t cal_leapyear(uint16_t year)
{
    if (((year % 4 == 0) && (year % 100)) || (year % 400 == 0))
    {
        /* Leap year */
        return 1;
    }

    /* Not a leap year */
    return 0;
}

/**
 *  @brief      Determines day of the week based on Sakamoto's methods
 *  @param[in]  y: year
 *  @param[in]  m: month
 *  @param[in]  d: day
 *  @return     Day of the week @ref weekday_e
 */
weekday_e dayofweek(uint16_t y, uint16_t m, uint16_t d)
{
    const uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    y -= m < 3;
    return (weekday_e)((y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7);
}
/**
 *  @brief      Encodes calendar data
 *  @param[out] time_out : raw time value of the calendar data
 *  @param[in]  x: the calendar data to encode
 *  @return     0 if encoded properly, or -1 if error
 */
int8_t cal_encode(uint32_t *time_out, calendar_struct *x)
{
    uint32_t i;

    (void)gregorian[0].name;    // avoid warnings
    (void)gregorian[0].ordinal; // avoid warnings
    /* Epoch is 2000-01-01 00:00:00 */
    *time_out = 0;

    /* Validate input */
    if (x->year < 2000)
    {
        return -1;
    }
    if ((x->month < 1) || (x->month > 12))
    {
        return -1;
    }
    if (x->day < 1)
    {
        return -1;
    }
    if (x->day > ((x->month == 2) ? (gregorian[x->month - 1].days_in_month + cal_leapyear(x->year)) : gregorian[x->month - 1].days_in_month))
    {
        return -1;
    }

    /* Count years */
    i = 2000;
    while (i < x->year)
    {
        *time_out += cal_leapyear(i++) ? TIME_SECONDS_IN_LEAP_YEAR : TIME_SECONDS_IN_NORMAL_YEAR;
    }

    /* Count months */
    i = 1;
    while (i < (uint32_t)x->month)
    {
        *time_out += TIME_SECONDS_IN_DAY * (uint32_t)gregorian[i - 1].days_in_month;
        if ((i == 2) && (cal_leapyear(x->year)))
        {
            /* Extra day in Feb each leap year */
            *time_out += TIME_SECONDS_IN_DAY;
        }
        i++;
    }

    /* Convert days to seconds */
    *time_out += TIME_SECONDS_IN_DAY * ((uint32_t)x->day - 1);

    /* Rest is easy */
    *time_out += (uint32_t)x->hours * 3600;
    *time_out += (uint32_t)x->minutes * 60;
    *time_out += (uint32_t)x->seconds;

    return 0;
}
/**
 *  @brief      Decodes calendar data
 *  @param[in]  time_in: raw time value to decode
 *  @param[out] x: the calendar data based on time_in
 *  @return     0 if encoded properly, or -1 if error
 */
int8_t cal_decode(uint32_t time_in, calendar_struct *x)
{
    uint32_t time;
    int      i;

    (void)gregorian[0].name;    // avoid warnings
    (void)gregorian[0].ordinal; // avoid warnings

    /* Initialize time structure */
    time = time_in;
    memset(x, 0, sizeof(calendar_struct));
    x->year  = 2000;
    x->month = 1;
    x->day   = 1;

    /* Take off years */
    while (time >= (cal_leapyear(x->year) ? TIME_SECONDS_IN_LEAP_YEAR : TIME_SECONDS_IN_NORMAL_YEAR))
    {
        if (cal_leapyear(x->year))
        {
            /* Leap year: 366 days = 60 * 60 * 24 * 366 seconds*/
            time -= TIME_SECONDS_IN_LEAP_YEAR;
        }
        else
        {
            /* Normal year: 365 days */
            time -= TIME_SECONDS_IN_NORMAL_YEAR;
        }
        x->year++;
    }

    /* Now we have only days left. We are at 01-01-year */
    /* Months are zero-indexed for simplicity. January == 0 */
    for (i = 0; i < 12; i++)
    {
        int days;
        days = gregorian[i].days_in_month;
        if (cal_leapyear(x->year) && x->month == 2)
        {
            /* February, is 29 days in the month */
            days++;
        }
        if (time >= ((uint32_t)days * TIME_SECONDS_IN_DAY))
        {
            time -= (uint32_t)days * TIME_SECONDS_IN_DAY;
            x->month++;
        }
        else
        {
            /* No more months to take off. Exit loop */
            break;
        }
    }

    /* Sanity check */
    if (i == 12)
    {
        return -1;
    }
    if ((time / TIME_SECONDS_IN_DAY) > (uint32_t)gregorian[i].days_in_month)
    {
        /* More days left than days in month! */
        return -1;
    }

    /* Determine days */
    x->day = (uint16_t)(1 + (time / TIME_SECONDS_IN_DAY));

    /* Determine HM:MM:SS */
    x->seconds = (uint16_t)((time % 60));
    x->minutes = (uint16_t)((time % 3600) / 60);
    x->hours   = (uint16_t)((time % TIME_SECONDS_IN_DAY) / 3600);

    return 0;
}
/**
 *  @brief      Determines if date and time of calendar are valid
 *  @param[in]  calendar: calendar to check
 *  @return     true if calendar is valid, false otherwise
 *  @note       The function corrects/fills the weekday field
 */
bool cal_check_valid(calendar_struct *calendar)
{
    bool valid_date = false;
    bool valid_time = false;
    if (calendar->year >= 1900 && calendar->year <= 9999)
    {
        switch (calendar->month)
        {
            case 1:
            case 3:
            case 5:
            case 7:
            case 8:
            case 10:
            case 12:
                if ((calendar->day >= 1) && (calendar->day <= 31))
                    valid_date = true;
                break;
            case 4:
            case 6:
            case 9:
            case 11:
                if ((calendar->day >= 1) && (calendar->day <= 30))
                    valid_date = true;
                break;
            case 2:
                if ((calendar->day >= 1) && (calendar->day <= 28))
                    valid_date = true;
                if (cal_leapyear(calendar->year) && (calendar->day == 29))
                    valid_date = true;
                break;
        }
    }

    if (valid_date)
    {
        calendar->weekday = dayofweek(calendar->year, calendar->month, calendar->day);
    }

    if ((calendar->hours <= 23) && (calendar->minutes <= 59) && (calendar->seconds <= 59))
    {
        valid_time = true;
    }

    if ((valid_date) && (valid_time))
        return true;
    else
        return false;
}
/**
 * @brief      Get the weekday name
 * @param[in]  day: day of the week
 * @param[out] name: pointer to receive day ot the week name (string)
 * @param[in]  name_max_size: Maximum name size to be filled
 * @return     Number of bytes filled in the output buffer
 */
int32_t get_weekday_name(weekday_e day, char *name, uint32_t name_max_size)
{
    int32_t copy_size = 0;
    if (day < (uint8_t)WEEKDAY_QTY)
    {
        copy_size = snprintf(name, name_max_size, "%s", &weekday_names[day][0]);
    }
    else
    {
        copy_size = snprintf(name, name_max_size, "ERR");
    }
    return copy_size;
}
