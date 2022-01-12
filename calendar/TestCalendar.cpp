/**
 * @file      TestCalendar.c
 * @author    Luiz Guilherme Rizzatto Zucchi (luiz.zucchi@perto.com.br)
 * @brief     Source file with calendar unity test functions
 * @version   1.0
 * @date      2021-01-22
 * @copyright Copyright (c) 2020
 */
#include "uTest.hpp"
#include "calendar.h"

/** @addtogroup  TestCalendarPrivate Calendar Test Private
 *  @ingroup TestCalendar
 * @{
 */
static void TestCalLeapYear(void);
static void TestCalCheckValid(void);
static void TestCalEncodeDecode(void);
static void TestCalDayOfWeek(void);

static uTest *Test = nullptr;
/*! @}*/ // End of TestCalendarPrivate


/**
 * @brief     Function to test all calendar functions
 */
void TestCalendar(void)
{
    SetUp();
    TestCalLeapYear();
    TestCalCheckValid();
    TestCalEncodeDecode();
    TestCalDayOfWeek();
    TearDown();
}
/**
 * @brief  Test @ref cal_leapyear function
 */
void TestCalLeapYear(void)
{
    EXPECT_EQ(static_cast<int8_t>(false), cal_leapyear(2019));
    EXPECT_EQ(static_cast<int8_t>(true), cal_leapyear(2020));
    EXPECT_EQ(static_cast<int8_t>(true), cal_leapyear(2000));
    EXPECT_EQ(static_cast<int8_t>(false), cal_leapyear(2100));
}
/**
 * @brief  Test @ref cal_check_valid function
 */
void TestCalCheckValid(void)
{
    calendar_struct cal;

    cal.year    = 2019;
    cal.month   = 2;
    cal.day     = 29;
    cal.hours   = 12;
    cal.minutes = 43;
    cal.seconds = 28;
    EXPECT_EQ(false, cal_check_valid(&cal));

    cal.year    = 2020;
    cal.month   = 2;
    cal.day     = 29;
    cal.hours   = 12;
    cal.minutes = 43;
    cal.seconds = 28;
    EXPECT_EQ(true, cal_check_valid(&cal));

    cal.year    = 2019;
    cal.month   = 2;
    cal.day     = 26;
    cal.hours   = 12;
    cal.minutes = 43;
    cal.seconds = 28;
    EXPECT_EQ(true, cal_check_valid(&cal));
}
/**
 *  @brief  Test @ref cal_decode and @ref cal_encode functions
 */
void TestCalEncodeDecode(void)
{
    calendar_struct cal;
    uint32_t        encoded_val;

    EXPECT_EQ(static_cast<int8_t>(0), cal_decode(604518380, &cal));
    EXPECT_EQ(static_cast<uint16_t>(26), cal.day);
    EXPECT_EQ(static_cast<uint16_t>(2U), cal.month);
    EXPECT_EQ(static_cast<uint16_t>(2019), cal.year);
    EXPECT_EQ(static_cast<uint16_t>(17), cal.hours);
    EXPECT_EQ(static_cast<uint16_t>(46), cal.minutes);
    EXPECT_EQ(static_cast<uint16_t>(20), cal.seconds);
    EXPECT_EQ(static_cast<int8_t>(0), cal_encode(&encoded_val, &cal));
    EXPECT_EQ(604518380U, encoded_val);
}
/**
 *  @brief  Test @ref dayofweek function
 */
void TestCalDayOfWeek(void)
{
    EXPECT_EQ(TUESDAY, dayofweek(2019, 02, 26));
}


