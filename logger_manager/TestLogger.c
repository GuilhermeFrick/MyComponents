/**
 * @file TestLogger.c
 * @author Guilherme Frick de Oliveira (guiherme.oliveira_irede@perto.com.br)
 * @brief
 * @version 0.1
 * @date 2021-11-24
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "uTest.h"
#include "FreeRTOS.h"
#include "task.h"
#include "TestLogger.h"
#include "logger_manager.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

static void TestStringType(void);
static void TestLogger_1(void);

typedef struct TestLogger_s
{
    uint32_t sample;
    int32_t  val_1;
    uint32_t val_2;
    float    val_3;
} TestLogger_st;

static TestLogger_st TestInfo[50];
uint32_t             log_idx;
LoggerHandle_t       TestLoggerHandle = NULL;

static const ItemInfo_t TestDescriptInfo[] = {
    {.label = "sample", .format_str = "%d,", .item_size = 4},
    {.label = "val_1", .format_str = "%d,", .item_size = 4},
    {.label = "val_2", .format_str = "%d", .item_size = 4},
    {.label = "val_3", .format_str = "%.01f,\n", .item_size = 4},
};

void TestLogger(void)
{
    SetUp();
    TestLogger_1();
    TestLoggerCleanup();
    TearDown();
}

static void TestLogger_1(void)
{

    log_idx = 0;
    for (size_t i = 0; i < 30; i++)
    {
        TestInfo[log_idx].sample = i;
        TestInfo[log_idx].val_1  = i + 3;
        TestInfo[log_idx].val_3  = i + 7;
        TestInfo[log_idx].val_3  = i + 0.234f;
        log_idx++;
    }
    vTaskDelay(3000);

    ASSERT_EQ(LOGGER_RET_OK, LoggerInit(&TestLoggerHandle, (ItemInfo_t *)TestDescriptInfo, 4));
    EXPECT_EQ(LOGGER_RET_OK, LoggerDefineBuffer(TestLoggerHandle, TestInfo, log_idx * sizeof(TestLogger_st)));
    LoggerState_e logger_state;
    EXPECT_EQ(LOGGER_RET_OK, LoggerGetState(TestLoggerHandle, &logger_state));
    EXPECT_EQ(LOGGER_IDLE, logger_state);
    EXPECT_EQ(LOGGER_RET_OK, LoggerStart(TestLoggerHandle));
    EXPECT_EQ(LOGGER_RET_OK, LoggerGetState(TestLoggerHandle, &logger_state));
    EXPECT_EQ(LOGGER_RUNNING, logger_state);

    for (size_t i = 0; i < 30; i++)
    {
        LoggerRun(TestLoggerHandle, 10);
    }
    EXPECT_EQ(LOGGER_RET_OK, LoggerStop(TestLoggerHandle));
    EXPECT_EQ(LOGGER_RET_OK, LoggerGetState(TestLoggerHandle, &logger_state));
    EXPECT_EQ(LOGGER_IDLE, logger_state);
    EXPECT_EQ(LOGGER_RET_OK, LoggerDeInit(&TestLoggerHandle));
}

void TestLoggerRun(void)
{
    // LoggerRun(TestLoggerHandle, 10);
}

void TestLoggerCleanup(void)
{
    LoggerDeInit(&TestLoggerHandle);
}

static void TestStringType(void)
{

    EXPECT_EQ(FORMAT_INT, GetFormatType("%i"));
    EXPECT_EQ(FORMAT_INT, GetFormatType("%d"));
    EXPECT_EQ(FORMAT_UINT, GetFormatType("%u"));
    EXPECT_EQ(FORMAT_UINT, GetFormatType("Battery %%: %u"));
    EXPECT_EQ(FORMAT_FLOAT, GetFormatType("%f"));
    EXPECT_EQ(FORMAT_FLOAT, GetFormatType("%.01f"));
    EXPECT_EQ(FORMAT_FLOAT, GetFormatType("%F"));
    EXPECT_EQ(FORMAT_LONG_INT, GetFormatType("%li"));
    EXPECT_EQ(FORMAT_LONG_UINT, GetFormatType("%lu"));
    EXPECT_EQ(FORMAT_LONG_LONG_INT, GetFormatType("%lli"));
    EXPECT_EQ(FORMAT_LONG_LONG_INT, GetFormatType("%lld"));
    EXPECT_EQ(FORMAT_LONG_LONG_UINT, GetFormatType("%llu"));
    EXPECT_EQ(FORMAT_ERROR, GetFormatType("%a"));
    EXPECT_EQ(FORMAT_STRING, GetFormatType("%s"));
}
