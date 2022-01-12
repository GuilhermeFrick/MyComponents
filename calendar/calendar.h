/**
 * @file      calendar.h
 * @author    Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * @brief     Header file with basic implementation of Gregorian Calendar
 * @version   1.2
 * @date      2021-18-01
 * @copyright Copyright (c) 2021
 */
#ifndef __CALENDAR_H__
#define __CALENDAR_H__

/** @addtogroup  Calendar Calendar
 * @{
 */
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif


    /**
     *  @brief Days of the week
     */
    typedef enum WeekdayDefinition
    {
        SUNDAY      = 0, ///< Sunday
        MONDAY      = 1, ///< Monday
        TUESDAY     = 2, ///< Tuesday
        WEDNESDAY   = 3, ///< Wednesday
        THURSDAY    = 4, ///< Thursday
        FRIDAY      = 5, ///< Friday
        SATURDAY    = 6, ///< Saturday
        WEEKDAY_QTY = 7  ///< Number of days per week
    } weekday_e;

    /**
     *  @brief Structure used for the Gregorian Calendar
     */
    typedef struct CalendarDefinition
    {
        uint16_t  seconds; ///< Seconds value
        uint16_t  minutes; ///< Minutes value
        uint16_t  hours;   ///< Hours value
        uint16_t  day;     ///< Day value
        uint16_t  month;   ///< Month value
        uint16_t  year;    ///< Year value
        weekday_e weekday; ///< Day of the week
    } calendar_struct;

    int8_t    cal_leapyear(uint16_t year);
    weekday_e dayofweek(uint16_t y, uint16_t m, uint16_t d);
    int8_t    cal_encode(uint32_t *time_out, calendar_struct *x);
    int8_t    cal_decode(uint32_t time_in, calendar_struct *x);
    bool      cal_check_valid(calendar_struct *calendar);
    int32_t   get_weekday_name(weekday_e day, char *name, uint32_t name_max_size);

#ifdef __cplusplus
}
#endif
/** @}*/ // End of Calendar
#endif
