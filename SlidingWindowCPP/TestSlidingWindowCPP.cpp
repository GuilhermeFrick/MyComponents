/**
 * @file TestSlidingWindowCPP.cpp
 * @author Guilherme Frick de Oliveira 
 * @brief  
 * @version 0.1
 * @date 2022-01-14
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "uTest.hpp"
#include "TestSlidingWindowCPP.h"
#include "SlidingWindow.hpp"

using namespace std;

static uTest *Test = nullptr;

template <typename T>
void TestWinCreation(const size_t n, T default_val);
template <typename T>
void TestWinAppend(const size_t n, T default_val, T new_val);
template <typename T>
void TestWinGetItems(const size_t n, T default_val);

void TestSlidingWindowCPP(void)
{
    SetUp();
    TestWinCreation<int32_t>(10, 3);
    TestWinCreation<int32_t>(10, -2);
    TestWinCreation<uint32_t>(10, 0);
    TestWinCreation<float>(10, 2.3F);

    TestWinAppend<int>(10, 3, 4);
    TestWinGetItems<int>(10, -12);

    TestWinAppend<float>(10, 3.2F, 4.5F);
    TestWinGetItems<float>(10, 3.2F);

    // TestWinFloat(10, 1.3F);
    TestSlidingWindowCPPCleanup();
    TearDown();
}
template <typename T>
void TestWinCreation(const size_t n, T default_val)
{
    SlidingWindow<T> *win = new SlidingWindow<T>(n, default_val);

    ASSERT_NE(static_cast<void *>(nullptr), static_cast<void *>(win));

    EXPECT_EQ(n, win->size());

    EXPECT_EQ(default_val, win->head());
    EXPECT_EQ(default_val, win->tail());
    for (size_t i = 0; i < n; i++)
    {
        EXPECT_EQ(default_val, win->at(i));
    }

    delete win;
}
void TestSlidingWindowCPPCleanup(void)
{
}
template <typename T>
void TestWinAppend(const size_t n, T default_val, T new_val)
{
    SlidingWindow<T> *win = new SlidingWindow<T>(n, default_val);

    ASSERT_NE(static_cast<void *>(nullptr), static_cast<void *>(win));

    for (size_t i = 0; i < n; i++)
    {
        EXPECT_EQ(default_val, win->tail());
        EXPECT_EQ(true, win->append(new_val));
        EXPECT_EQ(new_val, win->head());
    }
    for (size_t i = 0; i < n; i++)
    {
        EXPECT_EQ(new_val, win->at(i));
    }

    delete win;
}
template <typename T>
void TestWinGetItems(const size_t n, T default_val)
{
    SlidingWindow<T> *win = new SlidingWindow<T>(n, default_val);
    T                *arr = new T[n];

    ASSERT_NE(static_cast<void *>(nullptr), static_cast<void *>(win));
    ASSERT_NE(static_cast<void *>(nullptr), static_cast<void *>(arr));

    EXPECT_EQ(true, win->GetItems(n, arr));

    EXPECT_EQ(true, win->GetItems(0, arr));
    EXPECT_EQ(false, win->GetItems(n + 1, arr));
    EXPECT_EQ(false, win->GetItems(n, nullptr));

    for (size_t i = 0; i < n; i++)
    {
        EXPECT_EQ(default_val, arr[i]);
    }

    delete win;
    delete[] arr;
}
