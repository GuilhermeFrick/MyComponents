/**
 * @file test_filter.c
 * @author Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * @brief
 * @version 0.1
 * @date 2021-11-11
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "uTest.h"
#include "FreeRTOS.h"
#include "task.h"
#include "filter.h"
#include "test_filter.h"
#include <stdbool.h>
#include <stdio.h>

static void            TestSlidingWindow(size_t window_size);
static SlidingWindow_t win;
static int32_t *       Samples = NULL;
static void            TestCreation(size_t window_size);
static void            TestAppend(size_t window_size);
static void            TestGetLastItemsAppended(size_t window_size);
static void            TestCalcAverage_1(void);
static void            TestCalcAverage_2(void);
static void            TestItemPosition(size_t window_size);
static void            TestResetWindow(size_t window_size);
static void            TestMovingAverage(void);

void TestFilter(void)
{
    SetUp();
    TestCreation(32);
    TestAppend(32);
    TestGetLastItemsAppended(32);
    TestCalcAverage_1();
    TestCalcAverage_2();
    TestItemPosition(32);
    TestResetWindow(32);
    TestMovingAverage();
    TestFilterCleanup();
    TearDown();
}

/**
 * @brief
 *
 * @param window_size
 */
static void TestCreation(size_t window_size)
{
    bool is_cleared = false;
    ASSERT_EQ(FILTER_RET_OK, FilterSlidingWindowCreate(&win, sizeof(int32_t), window_size, NULL));
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowIsCleared(win, window_size, &is_cleared));
    EXPECT_TRUE(is_cleared);
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowDelete(&win));
}
/**
 * @brief
 *
 * @param window_size
 */
static void TestAppend(size_t window_size)
{
    bool is_cleared = false;
    ASSERT_EQ(FILTER_RET_OK, FilterSlidingWindowCreate(&win, sizeof(int32_t), window_size, NULL));
    for (int32_t i = 0; i < window_size; i++)
    {
        EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowAppend(win, &i));
        vTaskDelay(10);
    }
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowIsCleared(win, window_size, &is_cleared));
    ASSERT_EQ(false, is_cleared);
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowDelete(&win));
}
/**
 * @brief
 *
 * @param window_size
 */

static void TestGetLastItemsAppended(size_t window_size)
{
    Samples = TestMalloc(window_size * sizeof(int32_t));
    ASSERT_NE(NULL, Samples);
    ASSERT_EQ(FILTER_RET_OK, FilterSlidingWindowCreate(&win, sizeof(int32_t), window_size, NULL));
    for (int32_t i = 0; i < window_size; i++)
    {
        EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowAppend(win, &i));
        vTaskDelay(10);
    }
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowGetLastItems(win, window_size, Samples));

    for (int32_t i = 0; i < window_size; i++)
    {
        EXPECT_EQ(((window_size - 1) - i), Samples[i]);
    }

    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowDelete(&win));
    TestFree(Samples);
    Samples = NULL;
}

static void TestCalcAverage_1(void)
{
    static const int32_t Samples_1[]  = {8, 42, 56, 58, 98, 65, 235, 54, 78, 96, 54, 52, 33, 22, 55, 66};
    const float          avg_expected = 67.0f;
    ASSERT_EQ(FILTER_RET_OK, FilterSlidingWindowCreate(&win, sizeof(int32_t), 16, NULL));

    for (int32_t i = 0; i < 16; i++)
    {
        EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowAppend(win, (int32_t *)&Samples_1[i]));
        vTaskDelay(10);
    }
    float avg = 0.0f;
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowGetFloatAvg(win, 16, &avg));
    EXPECT_FLOAT_EQ(avg_expected, avg);
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowDelete(&win));
}

static void TestCalcAverage_2(void)
{
    static const int32_t Samples_2[]  = {-8, -1, -7, 22, 2, 13, -1, 54, -78, -96, -54, 52, 330, 22, -55, 66};
    const float          avg_expected = 16.3125f;
    ASSERT_EQ(FILTER_RET_OK, FilterSlidingWindowCreate(&win, sizeof(int32_t), 16, NULL));

    for (int32_t i = 0; i < 16; i++)
    {
        EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowAppend(win, (int32_t *)&Samples_2[i]));
        vTaskDelay(10);
    }
    float avg = 0.0f;
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowGetFloatAvg(win, 16, &avg));
    EXPECT_FLOAT_EQ(avg_expected, avg);
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowDelete(&win));
}

static void TestMovingAverage(void)
{
    int32_t              old_data;
    int64_t              accum       = 0;
    float                avg         = 0.0f;
    static const int32_t Samples_3[] = {-8, -1, -7, 22, 2,  13,  -1, 54, -78, -96, -54, 52, 330, 22, -55, 66,  8,
                                        42, 56, 58, 98, 65, 235, 54, 78, 96,  54,  52,  33, 22,  55, 66,  -25, 12};
    static const float   Averages[]  = {-0.25,    -0.28125, -0.5,     0.1875,   0.25,     0.65625,  0.625,    2.3125,   -0.125,
                                     -3.125,   -4.8125,  -3.1875,  7.125,    7.8125,   6.09375,  8.15625,  8.40625,  9.71875,
                                     11.46875, 13.28125, 16.34375, 18.375,   25.71875, 27.40625, 29.84375, 32.84375, 34.53125,
                                     36.15625, 37.1875,  37.875,   39.59375, 41.65625, 41.125,   41.53125};

    ASSERT_EQ(FILTER_RET_OK, FilterSlidingWindowCreate(&win, sizeof(int32_t), 32, NULL));

    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowReset(win));
    for (int32_t i = 0; i < 32; i++)
    {

        EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowGetTail(win, &old_data));
        accum -= old_data;
        accum += Samples_3[i];
        EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowAppend(win, (int32_t *)&Samples_3[i]));
        avg = (float)accum / 32;
        EXPECT_FLOAT_EQ(Averages[i], avg);
        vTaskDelay(10);
    }
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowDelete(&win));
}

static void TestItemPosition(size_t window_size)
{
    int32_t value;
    ASSERT_EQ(FILTER_RET_OK, FilterSlidingWindowCreate(&win, sizeof(int32_t), window_size, NULL));
    for (int32_t i = 0; i < window_size; i++)
    {
        EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowAppend(win, &i));
        vTaskDelay(10);
    }
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowGetItem(win, 15, &value));
    EXPECT_EQ(value, 15);
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowGetTail(win, &value));
    EXPECT_EQ(value, 0);
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowGetHead(win, &value));
    EXPECT_EQ(window_size - 1, value);
    value = window_size;
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowAppend(win, &value));

    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowGetTail(win, &value));
    EXPECT_EQ(1, value);
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowGetHead(win, &value));
    EXPECT_EQ(window_size, value);
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowDelete(&win));
}

static void TestResetWindow(size_t window_size)
{
    bool is_cleared;
    ASSERT_EQ(FILTER_RET_OK, FilterSlidingWindowCreate(&win, sizeof(int32_t), window_size, NULL));
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowReset(win));
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowIsCleared(win, window_size, &is_cleared));
    EXPECT_TRUE(is_cleared);
    EXPECT_EQ(FILTER_RET_OK, FilterSlidingWindowDelete(&win));
}

void TestFilterCleanup(void)
{
    FilterSlidingWindowDelete(&win);
    TestFree(Samples);
    Samples = NULL;

    // TestFree(Averages);
}

// TestFatalError