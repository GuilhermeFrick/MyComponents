/**
 * @file uTest.hpp
 * @author Guilherme Frick de Oliveira
 * @brief  Unit tests for c++ code
 * @version 0.1
 * @date 2022-01-14
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef _UTEST_HPP_
#define _UTEST_HPP_
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

extern void TestFatalError(void);

/**
 * @brief       Unity test Class
 */
class uTest
{
  public:
    uTest(const char *name);
    ~uTest();
    template <typename T>
    bool BinaryEqual(T expected, T actual, bool fatal, const char *file, int32_t line);
    template <typename T>
    bool BinaryNotEqual(T expected, T actual, bool fatal, const char *file, int32_t line);

    void TestRunning(const char *func);
    void TestSetUp(const char *test_case_name);
    void TestTearDown(void);

    uint32_t error_count;       ///< Number of failures occurred
    uint32_t test_count;        ///< Number of tests performed
    uint32_t TestTimestamp;     ///< Time stamp of the start of the test
    uint32_t TestCaseTimestamp; ///< Time stamp of the start of the test case execution
    uint8_t  TestCaseName[64];  ///< Name of Test Case, function that performs SetUp

  private:
    uint32_t GetElapsedTime(uint32_t InitialTime);
    template <typename T>
    void Success(T res);
    template <typename T>
    void Failed(T expected, T actual);
    void FailedWhere(const char *file, int32_t line);
    void print(bool val)
    {
        if (val)
        {
            printf("true");
        }
        else
        {
            printf("false");
        }
    }
    void print(int32_t val)
    {
        printf("%d", val);
    }
    void print(uint32_t val)
    {
        printf("%u", val);
    }
    void print(float val)
    {
        printf("%f", val);
    }
    void print(void *val)
    {
        printf("%p", val);
    }
};

template <typename T>
void uTest::Success(T res)
{
    printf("[       OK ] [%d] Result: ", this->test_count);
    print(res);
    printf(" (%d ms)\r\n", GetElapsedTime(TestTimestamp));
}
template <typename T>
void uTest::Failed(T expected, T actual)
{
    printf("  Actual: ");
    print(actual);
    printf("\r\n");
    printf("  Expected: ");
    print(expected);
    printf("\r\n");
}
template <typename T>
bool uTest::BinaryEqual(T expected, T actual, bool fatal, const char *file, int32_t line)
{
    bool ret = false;

    if (expected == actual)
    {
        this->Success(actual);
        ret = true;
    }
    else
    {
        this->FailedWhere(file, line);
        this->Failed(expected, actual);
        if (fatal)
        {
            printf("[    FATAL ] [%d]\r\n", test_count);
            // TestFatalError();
        }
    }

    return ret;
}
template <typename T>
bool uTest::BinaryNotEqual(T expected, T actual, bool fatal, const char *file, int32_t line)
{
    bool ret = false;

    if (expected != actual)
    {
        this->Success(actual);
        ret = true;
    }
    else
    {
        this->FailedWhere(file, line);
        this->Failed(expected, actual);
        if (fatal)
        {
            printf("[    FATAL ] [%d]\r\n", test_count);
            // TestFatalError();
        }
    }

    return ret;
}

// #define TEST(title)        \
//     void Test##title(void) \
//     {                      \
//         std::unique_ptr<uTest> ptr##title(new uTest());
// #define TEST_CASE(title, subtitle) void Test##title##subtitle(uTest *ptr)
// #define EXECUTE(title, subtitle)   Test##title##subtitle(ptr##title.get())

#define EXPECT_TRUE(actual)      \
    Test->TestRunning(__func__); \
    Test->BinaryEqual(true, actual, false, __FILE__, __LINE__)
#define EXPECT_FALSE(actual)     \
    Test->TestRunning(__func__); \
    Test->BinaryEqual(false, actual, false, __FILE__, __LINE__)

#define EXPECT_EQ(expected, actual) \
    Test->TestRunning(__func__);    \
    Test->BinaryEqual(expected, actual, false, __FILE__, __LINE__)
#define ASSERT_EQ(expected, actual) \
    Test->TestRunning(__func__);    \
    Test->BinaryEqual(expected, actual, true, __FILE__, __LINE__)
#define EXPECT_NE(expected, actual) \
    Test->TestRunning(__func__);    \
    Test->BinaryNotEqual(expected, actual, false, __FILE__, __LINE__)
#define ASSERT_NE(expected, actual) \
    Test->TestRunning(__func__);    \
    Test->BinaryNotEqual(expected, actual, true, __FILE__, __LINE__)

#define EXPECT_GT(expected, actual)
#define EXPECT_FLOAT_EQ(expected, actual)
#define EXPECT_STREQ(expected, actual)
#define ASSERT_STREQ(expected, actual)
#define SetUp()    Test = new uTest(__func__);
#define TearDown() delete Test;

#endif //_TEST_H_
