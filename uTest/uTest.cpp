/**
 * @file uTest.cpp
 * @author Guilherme Frick de Oliveira
 * @brief Source file for c++ unity test
 * @version 0.1
 * @date 2022-01-14
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "uTest.hpp"

/**
 *  @brief      Returns the current value of the RTOS tick timer in ms
 *  @return     Tick value
 *  @warning    Stronger function must be declared externally
 */
__attribute__((weak)) uint32_t TestGetTick(void)
{
    return 0;
}
uTest::uTest(const char *name)
{
    printf("\r\n[==========] Running test.\r\n");
    printf("[----------] Global test environment set-up.\r\n");

    snprintf((char *)this->TestCaseName, sizeof(this->TestCaseName), "%s", name);
    this->TestCaseName[sizeof(this->TestCaseName) - 1] = '\0';
    printf("[----------] Test Case: %s\r\n", this->TestCaseName);

    this->test_count        = 0;
    this->error_count       = 0;
    this->TestCaseTimestamp = TestGetTick();
    this->TestTimestamp     = TestGetTick();
}
uTest::~uTest(void)
{
    printf("[----------] Global test environment tear-down.\r\n");
    printf("[==========] %d tests ran (%d ms total).\r\n", test_count, this->GetElapsedTime(TestTimestamp));

    if (this->error_count)
    {
        printf("[  FAILED  ] Test Case: %s : %d / %d tests.\r\n", this->TestCaseName, this->error_count, this->test_count);
    }
    else
    {
        printf("[  PASSED  ] Test Case: %s : %d tests.\r\n", this->TestCaseName, this->test_count);
    }
}
void uTest::FailedWhere(const char *file, int32_t line)
{
    this->error_count++;
    printf("[     FAIL ] [%d] (%d ms)\r\n", this->test_count, GetElapsedTime(TestCaseTimestamp));
    printf("  File: %s Line: %d\r\n", file, line);
}
void uTest::TestRunning(const char *func)
{
    this->test_count++;
    this->TestCaseTimestamp = TestGetTick();
    printf("[ RUN      ] [%d] %s\r\n", test_count, func);
}
uint32_t uTest::GetElapsedTime(uint32_t InitialTime)
{
    uint32_t actualTime;
    actualTime = TestGetTick();

    if (InitialTime <= actualTime)
        return (actualTime - InitialTime);
    else
        return ((UINT32_MAX - InitialTime) + actualTime);
}
