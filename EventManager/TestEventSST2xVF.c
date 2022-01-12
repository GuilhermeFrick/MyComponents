/*!
 * \file       TestEventSSTxVF.c
 * \brief      Source file of the Event SSTxVF test module
 * \date       2021-09-25
 * \version    1.0
 * \author     Henrique Awoyama Klein (henrique.klein@perto.com.br)
 * \copyright  Copyright (c) 2021
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "TestEventSST2xVF.h"
#include "uTest.h"
#include "EventManager.h"
#include "EventSST2xVF.h"

#define CRC_POLYNOMIAL_32 0xEDB88320ul

const uint16_t  MAX_FAKE_EVENT_SIZE = 128;
static uint8_t *fake_event;
static uint32_t read_crc  = 0;
static uint32_t write_crc = 0;

static void TestInit(EventManagerConfig_t event_config);
static void TestDeInit(void);
static void TestConsistency(void);
static void TestTurnaround(void);
uint32_t    CalcChecksum32(uint32_t curr_crc, uint8_t value);
static bool CheckEventMemory(void);
static bool CheckTurnaround(void);
static void FillEventBuffer(void);
static void CalcReadCRC(void);

void TestEventSST2xVF(void)
{
    EventManagerConfig_t test_event_config = {0};
    SetUp();

    test_event_config.counter_init    = 0;
    test_event_config.event_size      = MAX_FAKE_EVENT_SIZE;
    test_event_config.pointer_init    = 0;
    test_event_config.counter_init    = 0;
    test_event_config.queue_size      = 5;
    test_event_config.mutex_wait_tick = 10;

    TestDeInit();
    TestInit(test_event_config);
    TestConsistency();
    TestTurnaround();
    test_event_config.first_valid_addr = EventManagerGetInfo()->SectorSize * 4;
    test_event_config.pointer_init     = test_event_config.first_valid_addr;
    test_event_config.size_used        = MAX_FAKE_EVENT_SIZE * 64;
    TestDeInit();
    TestInit(test_event_config);
    TestConsistency();
    TestTurnaround();

    TearDown();
}

static void TestInit(EventManagerConfig_t event_config)
{
    EXPECT_EQ(true, event_config.event_size <= MAX_FAKE_EVENT_SIZE);
    if (fake_event)
    {
        TestFree(fake_event);
        fake_event = NULL;
    }
    fake_event = (uint8_t *)TestMalloc(event_config.event_size);
    EXPECT_EQ(true, fake_event != NULL);
    if (fake_event)
    {
        EXPECT_EQ(EVENT_RET_OK, EventManagerInitialize(&event_config, EventSST2xVFGetInterface()));
        EXPECT_EQ(event_config.event_size, EventManagerGetInfo()->EventSize);
        EXPECT_EQ(event_config.pointer_init, EventManagerGetInfo()->Pointer);
        EXPECT_EQ(event_config.counter_init, EventManagerGetInfo()->Counter);
    }
}
static void TestDeInit(void)
{
    EXPECT_EQ(EVENT_RET_OK, EventManagerUninitialize());
    EXPECT_EQ(EVENT_NOT_INIT, EventManagerClear());
    EXPECT_EQ(EVENT_NOT_INIT, EventManagerRun(10));
    EXPECT_EQ(EVENT_NOT_INIT, EventManagerRead(0, (uint8_t *)fake_event, sizeof(fake_event)));
    EXPECT_EQ(EVENT_NOT_INIT, EventManagerSave((uint8_t *)fake_event, sizeof(fake_event)));
    EXPECT_EQ(0, EventManagerGetInfo()->ManID);
    EXPECT_EQ(0, EventManagerGetInfo()->DevID);
    EXPECT_EQ(0, EventManagerGetInfo()->EventSize);
    EXPECT_EQ(0, EventManagerGetInfo()->LogsPerSector);
    EXPECT_EQ(0, EventManagerGetInfo()->MaxLogsNumber);
    EXPECT_EQ(0, EventManagerGetInfo()->FirstPointer);
    EXPECT_EQ(0, EventManagerGetInfo()->MaxPointer);
    EXPECT_EQ(0, EventManagerGetInfo()->SectorSize);
    EXPECT_EQ(0, EventManagerGetInfo()->Pointer);
    EXPECT_EQ(0, EventManagerGetInfo()->Counter);
    EXPECT_EQ(0, EventManagerGetInfo()->ManID);
}
static void TestConsistency(void)
{
    EXPECT_EQ(true, CheckEventMemory());
    EXPECT_EQ(write_crc, read_crc);
    EXPECT_EQ(EVENT_RET_OK, EventManagerClear());
}
static void TestTurnaround(void)
{
    EventManagerConfig_t test_event_config = {0};

    test_event_config.event_size      = EventManagerGetInfo()->EventSize;
    test_event_config.pointer_init    = EventManagerGetInfo()->MaxPointer - EventManagerGetInfo()->SectorSize + 1;
    test_event_config.counter_init    = EventManagerGetInfo()->MaxLogsNumber - EventManagerGetInfo()->LogsPerSector;
    test_event_config.queue_size      = 5;
    test_event_config.mutex_wait_tick = 10;
    EXPECT_EQ(EVENT_RET_OK, EventManagerClear());
    EXPECT_EQ(EVENT_RET_OK, EventManagerUninitialize());
    EXPECT_EQ(EVENT_RET_OK, EventManagerInitialize(&test_event_config, EventSST2xVFGetInterface()));
    EXPECT_EQ(true, CheckTurnaround());
}

/*!
 *  \brief      Fills memory with numbers from 0 to MaxLogsNumber.
 *  \return     Boolean indicating succes of operation
 */
static bool CheckEventMemory(void)
{
    bool ret  = true;
    read_crc  = 0;
    write_crc = 0;

    for (uint32_t i = 0; i < EventManagerGetInfo()->MaxLogsNumber; i++)
    {
        FillEventBuffer();
        uint32_t save_timestamp = TestGetTick();
        while (EventManagerWriteThrough((uint8_t *)fake_event, EventManagerGetInfo()->EventSize) != EVENT_RET_OK)
        {
            vTaskDelay(1);
            if (TestGetElapsedTime(save_timestamp) > 100)
            {
                ret = false;
                TestPrintf("[     INFO ] EventManagerSave: [Save Failed][%d] \r\n", i);
                break;
            }
        }
        if (ret == false)
        {
            break;
        }
    }

    for (int32_t i = EventManagerGetInfo()->Counter - 1; i >= 0; i--)
    {
        uint32_t read_timestamp = TestGetTick();

        while (EventManagerRead(i, (uint8_t *)fake_event, EventManagerGetInfo()->EventSize) != EVENT_RET_OK)
        {
            vTaskDelay(1);
            if (TestGetElapsedTime(read_timestamp) > 100)
            {
                ret = false;
                TestPrintf("[     INFO ] EventManagerRead: [Read Failed][%d] \r\n", i);
                break;
            }
        }
        if (ret == false)
        {
            break;
        }
        CalcReadCRC();
    }
    vTaskDelay(50);
    return ret;
}

static bool CheckTurnaround(void)
{
    bool     ret            = true;
    uint32_t finishing_addr = EventManagerGetInfo()->FirstPointer + EventManagerGetInfo()->LogsPerSector * EventManagerGetInfo()->EventSize;
    uint32_t total_logs     = (EventManagerGetInfo()->MaxPointer + 1 - EventManagerGetInfo()->Pointer) / EventManagerGetInfo()->EventSize;
    uint32_t old_count      = EventManagerGetInfo()->Counter;

    total_logs += ((finishing_addr - EventManagerGetInfo()->FirstPointer) / EventManagerGetInfo()->EventSize);
    for (uint32_t i = 0; i < total_logs; i++)
    {
        fake_event[0]           = i & 0xff;
        fake_event[1]           = (i & 0xff00) >> 8;
        fake_event[2]           = (i & 0xff0000) >> 16;
        fake_event[3]           = (i & 0xff000000) >> 24;
        uint32_t save_timestamp = TestGetTick();
        while (EventManagerWriteThrough((uint8_t *)fake_event, EventManagerGetInfo()->EventSize) != EVENT_RET_OK)
        {
            vTaskDelay(1);
            if (TestGetElapsedTime(save_timestamp) > 20)
            {
                ret = false;
                TestPrintf("[     INFO ] EventManagerSave: [Save Failed][%d] \r\n", i);
                break;
            }
        }
        if (ret == false)
        {
            break;
        }
    }
    if (finishing_addr != EventManagerGetInfo()->Pointer)
    {
        ret = false;
    }
    else
    {
        for (uint32_t i = 0; i < total_logs; i++)
        {
            uint32_t read_timestamp = TestGetTick();

            while (EventManagerRead(i, (uint8_t *)fake_event, EventManagerGetInfo()->EventSize) != EVENT_RET_OK)
            {
                vTaskDelay(1);
                if (TestGetElapsedTime(read_timestamp) > 20)
                {
                    ret = false;
                    TestPrintf("[     INFO ] EventManagerRead: [Read Failed][%d] \r\n", i);
                    break;
                }
            }
            if (ret == false)
            {
                break;
            }
            uint32_t value_read = 0;
            value_read          = fake_event[0];
            value_read += (fake_event[1] << 8);
            value_read += (fake_event[2] << 16);
            value_read += (fake_event[3] << 24);
            if (value_read != total_logs - i - 1)
            {
                ret = false;
                break;
            }
        }
    }
    return ret;
}

uint32_t CalcChecksum32(uint32_t curr_crc, uint8_t value)
{
    uint32_t tmp;

    tmp = curr_crc ^ (0x000000FFul & (uint32_t)value);
    for (uint32_t j = 0; j < 8; j++)
    {
        if (tmp & 0x00000001ul)
        {
            tmp = (tmp >> 1) ^ CRC_POLYNOMIAL_32;
        }
        else
        {
            tmp = tmp >> 1;
        }
    }
    curr_crc = (curr_crc >> 8) ^ tmp;

    return curr_crc;
}

static void FillEventBuffer(void)
{
    srand(TestGetTick());
    for (uint16_t j = 0; j < EventManagerGetInfo()->EventSize; j++)
    {
        fake_event[j] = (uint8_t)(rand() & (0xff));
        write_crc     = CalcChecksum32(write_crc, fake_event[j]);
    }
}
static void CalcReadCRC(void)
{
    for (uint16_t j = 0; j < EventManagerGetInfo()->EventSize; j++)
    {
        read_crc = CalcChecksum32(read_crc, fake_event[j]);
    }
}
