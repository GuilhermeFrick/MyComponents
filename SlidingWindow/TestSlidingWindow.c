/**
 * @file test_filter.c
 * @author Guilherme Frick de Oliveira (guiherme.oliveira_irede@perto.com.br)
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
#include "SlidingWindow.h"
#include "TestSlidingWindow.h"
#include "FastMeanWindow.h"
#include <stdbool.h>
#include <stdio.h>

static SlidingWindow_t  win;
static FastMeanWindow_t fast_win;
static int32_t *        Samples = NULL;
static void             TestCreation(size_t window_size);
static void             TestAppend(size_t window_size);
static void             TestGetLastItemsAppended(size_t window_size);
static void             TestCalcAverage_1(void);
static void             TestCalcAverage_2(void);
static void             TestItemPosition(size_t window_size);
static void             TestResetWindow(size_t window_size);

static void TestFastMeanWindowCreation(void);
static void TestFastMeanWindow_MovingAverage(void);

void TestSlidingWindow(void)
{
    SetUp();
    TestCreation(32);
    TestAppend(32);
    TestGetLastItemsAppended(32);
    TestCalcAverage_1();
    TestCalcAverage_2();
    TestItemPosition(32);
    TestResetWindow(32);

    TestFastMeanWindowCreation();
    TestFastMeanWindow_MovingAverage();

    TestSlidingWindowCleanup();

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
    
    ASSERT_EQ(SLIDING_WINDOW_OK, SlidingWindowCreate(&win, sizeof(int32_t), window_size, NULL));
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowIsCleared(win, &is_cleared));
    EXPECT_TRUE(is_cleared);
    size_t size;
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowGetWinSize(win, &size));
    EXPECT_EQ(window_size, size);
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowDelete(&win));

    EXPECT_EQ(SLIDING_WINDOW_ERR_INV_PARAM, SlidingWindowCreate(NULL, sizeof(int32_t), 32, NULL));
    EXPECT_EQ(SLIDING_WINDOW_ERR_INV_PARAM, SlidingWindowDelete(&win));

    EXPECT_EQ(SLIDING_WINDOW_ERR_INV_PARAM, SlidingWindowCreate(&win, sizeof(int32_t), 0, NULL));
    EXPECT_EQ(SLIDING_WINDOW_ERR_INV_PARAM, SlidingWindowDelete(&win));

    EXPECT_EQ(SLIDING_WINDOW_ERR_INV_PARAM, SlidingWindowCreate(&win, 0, 32, NULL));
    EXPECT_EQ(SLIDING_WINDOW_ERR_INV_PARAM, SlidingWindowDelete(&win));
}
/**
 * @brief
 *
 * @param window_size
 */
static void TestAppend(size_t window_size)
{
    bool is_cleared = false;
    ASSERT_EQ(SLIDING_WINDOW_OK, SlidingWindowCreate(&win, sizeof(int32_t), window_size, NULL));
    for (int32_t i = 0; i < window_size; i++)
    {
        EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowAppend(win, &i));
        vTaskDelay(10);
    }
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowIsCleared(win, &is_cleared));
    ASSERT_EQ(false, is_cleared);
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowDelete(&win));
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
    ASSERT_EQ(SLIDING_WINDOW_OK, SlidingWindowCreate(&win, sizeof(int32_t), window_size, NULL));
    for (int32_t i = 0; i < window_size; i++)
    {
        EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowAppend(win, &i));
        vTaskDelay(10);
    }
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowGetLastItems(win, window_size, Samples));

    for (int32_t i = 0; i < window_size; i++)
    {
        EXPECT_EQ(((window_size - 1) - i), Samples[i]);
    }

    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowDelete(&win));
    TestFree(Samples);
    Samples = NULL;
}

static void TestCalcAverage_1(void)
{
    static const int32_t Samples_1[]  = {8, 42, 56, 58, 98, 65, 235, 54, 78, 96, 54, 52, 33, 22, 55, 66};
    const float          avg_expected = 67.0f;
    ASSERT_EQ(SLIDING_WINDOW_OK, SlidingWindowCreate(&win, sizeof(int32_t), 16, NULL));

    for (int32_t i = 0; i < 16; i++)
    {
        EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowAppend(win, (int32_t *)&Samples_1[i]));
        vTaskDelay(10);
    }
    float avg = 0.0f;
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowGetFloatAvg(win, 16, &avg));
    EXPECT_FLOAT_EQ(avg_expected, avg);
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowDelete(&win));
}

static void TestCalcAverage_2(void)
{
    static const int32_t Samples[]    = {-8, -1, -7, 22, 2, 13, -1, 54, -78, -96, -54, 52, 330, 22, -55, 66};
    const float          avg_expected = 16.3125f;
    ASSERT_EQ(SLIDING_WINDOW_OK, SlidingWindowCreate(&win, sizeof(int32_t), 16, NULL));

    for (int32_t i = 0; i < 16; i++)
    {
        EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowAppend(win, (int32_t *)&Samples[i]));
        vTaskDelay(10);
    }
    float avg = 0.0f;
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowGetFloatAvg(win, 16, &avg));
    EXPECT_FLOAT_EQ(avg_expected, avg);
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowDelete(&win));
}

static void TestItemPosition(size_t window_size)
{
    int32_t value;
    ASSERT_EQ(SLIDING_WINDOW_OK, SlidingWindowCreate(&win, sizeof(int32_t), window_size, NULL));
    for (int32_t i = 0; i < window_size; i++)
    {
        EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowAppend(win, &i));
        vTaskDelay(10);
    }
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowGetItem(win, 15, &value));
    EXPECT_EQ(value, 15);
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowGetTail(win, &value));
    EXPECT_EQ(value, 0);
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowGetHead(win, &value));
    EXPECT_EQ(window_size - 1, value);
    value = window_size;
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowAppend(win, &value));

    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowGetTail(win, &value));
    EXPECT_EQ(1, value);
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowGetHead(win, &value));
    EXPECT_EQ(window_size, value);
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowDelete(&win));
}

static void TestResetWindow(size_t window_size)
{
    bool is_cleared;
    ASSERT_EQ(SLIDING_WINDOW_OK, SlidingWindowCreate(&win, sizeof(int32_t), window_size, NULL));
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowReset(win));
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowIsCleared(win, &is_cleared));
    EXPECT_TRUE(is_cleared);
    EXPECT_EQ(SLIDING_WINDOW_OK, SlidingWindowDelete(&win));
}

static void TestFastMeanWindowCreation(void)
{
    ASSERT_EQ(FAST_MEAN_WINDOW_OK, FastMeanWindowCreate(&fast_win, 32, NULL));
    EXPECT_EQ(FAST_MEAN_WINDOW_OK, FastMeanWindowDelete(&fast_win));

    ASSERT_EQ(FAST_MEAN_WINDOW_ERR_INV_PARAM, FastMeanWindowCreate(NULL, 32, NULL));
    EXPECT_EQ(FAST_MEAN_WINDOW_ERR_INV_PARAM, FastMeanWindowDelete(&fast_win));

    ASSERT_EQ(FAST_MEAN_WINDOW_ERR_INV_PARAM, FastMeanWindowCreate(&fast_win, 0, NULL));
    EXPECT_EQ(FAST_MEAN_WINDOW_ERR_INV_PARAM, FastMeanWindowDelete(&fast_win));
}

static void TestFastMeanWindow_MovingAverage(void)
{
    static const int32_t Samples[]  = {-8, -1, -7, 22, 2,  13,  -1, 54, -78, -96, -54, 52, 330, 22, -55, 66,  8,
                                      42, 56, 58, 98, 65, 235, 54, 78, 96,  54,  52,  33, 22,  55, 66,  -25, 12};
    static const float   Averages[] = {-0.25,    -0.28125, -0.5,     0.1875,   0.25,     0.65625,  0.625,    2.3125,   -0.125,
                                     -3.125,   -4.8125,  -3.1875,  7.125,    7.8125,   6.09375,  8.15625,  8.40625,  9.71875,
                                     11.46875, 13.28125, 16.34375, 18.375,   25.71875, 27.40625, 29.84375, 32.84375, 34.53125,
                                     36.15625, 37.1875,  37.875,   39.59375, 41.65625, 41.125,   41.53125};
    float                avg        = 0.0f;
    ASSERT_EQ(FAST_MEAN_WINDOW_OK, FastMeanWindowCreate(&fast_win, 32, NULL));
    EXPECT_EQ(FAST_MEAN_WINDOW_OK, FastMeanWindowReset(fast_win));
    EXPECT_EQ(FAST_MEAN_WINDOW_OK, FastMeanWindowGetAverage(fast_win, &avg));
    EXPECT_FLOAT_EQ(0.0f, avg);

    for (int32_t i = 0; i < 32; i++)
    {
        EXPECT_EQ(FAST_MEAN_WINDOW_OK, FastMeanWindowAppend(fast_win, Samples[i]));
        EXPECT_EQ(FAST_MEAN_WINDOW_OK, FastMeanWindowGetAverage(fast_win, &avg));
        EXPECT_FLOAT_EQ(Averages[i], avg);
        vTaskDelay(10);
    }
    EXPECT_EQ(FAST_MEAN_WINDOW_OK, FastMeanWindowReset(fast_win));
    EXPECT_EQ(FAST_MEAN_WINDOW_OK, FastMeanWindowGetAverage(fast_win, &avg));
    EXPECT_FLOAT_EQ(0.0f, avg);

    EXPECT_EQ(FAST_MEAN_WINDOW_OK, FastMeanWindowDelete(&fast_win));
}
void TestSlidingWindowCleanup(void)
{
    FastMeanWindowDelete(&fast_win);
    SlidingWindowDelete(&win);
    TestFree(Samples);

    Samples = NULL;
}