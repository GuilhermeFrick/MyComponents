/**
 * @file FastMeanWindow.c
 * @author Guilherme Frick de Oliveira (guiherme.oliveira_irede@perto.com.br)
 * @brief
 * @version 0.1
 * @date 2021-11-16
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "SlidingWindow.h"
#include "FastMeanWindow.h"

extern void *SlidingWindowMalloc(size_t size);
extern void  SlidingWindowFree(void *ptr);

typedef struct FastMeanWinCtl_s
{
    SlidingWindow_t window;
    int64_t         accumm;
} FastMeanWinCtl_st;

FastMeanWindowRet_et FastMeanWindowCreate(FastMeanWindow_t *win, size_t win_size, void *default_value)
{
    FastMeanWindowRet_et ret = FAST_MEAN_WINDOW_ERR_INV_PARAM;

    do
    {
        if ((*win != NULL) || (win_size <= 0))
        {
            break;
        }

        FastMeanWinCtl_st *win_ctrl = (FastMeanWinCtl_st *)SlidingWindowMalloc(sizeof(FastMeanWinCtl_st));
        if (win_ctrl == NULL)
        {
            ret = FAST_MEAN_WINDOW_ERR_MEM;
            break;
        }
        win_ctrl->window = NULL;
        win_ctrl->accumm = 0;
        if (SlidingWindowCreate(&win_ctrl->window, sizeof(int32_t), win_size, default_value) != SLIDING_WINDOW_OK)
        {
            SlidingWindowFree(win_ctrl);
            ret = FAST_MEAN_WINDOW_ERR_INIT;
            break;
        }
        *win = win_ctrl;
        ret  = FAST_MEAN_WINDOW_OK;

    } while (0);

    return ret;
}

FastMeanWindowRet_et FastMeanWindowAppend(FastMeanWindow_t win, int32_t new_data)
{
    FastMeanWindowRet_et ret = FAST_MEAN_WINDOW_ERR_INV_PARAM;

    do
    {
        if (win == NULL)
        {
            break;
        }
        FastMeanWinCtl_st *this_window = (FastMeanWinCtl_st *)win;

        int32_t old_data = 0;
        if (SlidingWindowGetTail(this_window->window, &old_data) != SLIDING_WINDOW_OK)
        {
            ret = FAST_MEAN_WINDOW_ERR_TAIL;
            break;
        }
        this_window->accumm -= old_data;
        this_window->accumm += new_data;
        if (SlidingWindowAppend(this_window->window, &new_data) != SLIDING_WINDOW_OK)
        {
            ret = FAST_MEAN_WINDOW_ERR_APPEND;
            break;
        }

        ret = FAST_MEAN_WINDOW_OK;

    } while (0);

    return ret;
}

FastMeanWindowRet_et FastMeanWindowGetAverage(FastMeanWindow_t win, float *const moving_average)
{
    FastMeanWindowRet_et ret         = FAST_MEAN_WINDOW_ERR_INV_PARAM;
    FastMeanWinCtl_st *  this_window = win;

    do
    {
        if (win == NULL)
        {
            break;
        }
        FastMeanWinCtl_st *this_window = (FastMeanWinCtl_st *)win;
        size_t             win_size;
        if (SlidingWindowGetWinSize(this_window->window, &win_size) != SLIDING_WINDOW_OK)
        {
            ret = FAST_MEAN_WINDOW_ERR;
            break;
        }
        if (win_size >= 0)
        {
            *moving_average = (float)this_window->accumm / (float)win_size;
            ret             = FAST_MEAN_WINDOW_OK;
        }

    } while (0);

    return ret;
}

FastMeanWindowRet_et FastMeanWindowReset(FastMeanWindow_t win)
{
    FastMeanWindowRet_et ret = FAST_MEAN_WINDOW_ERR_INV_PARAM;

    do
    {
        if (win == NULL)
        {
            break;
        }
        FastMeanWinCtl_st *this_window = (FastMeanWinCtl_st *)win;
        this_window->accumm            = 0;
        if (SlidingWindowReset(this_window->window) != SLIDING_WINDOW_OK)
        {
            ret = FAST_MEAN_WINDOW_ERR;
            break;
        }
        ret = FAST_MEAN_WINDOW_OK;

    } while (0);

    return ret;
}

FastMeanWindowRet_et FastMeanWindowDelete(FastMeanWindow_t *win)
{

    FastMeanWindowRet_et ret = FAST_MEAN_WINDOW_ERR_INV_PARAM;

    FastMeanWinCtl_st *this_window = (FastMeanWinCtl_st *)win;

    if (*win != NULL)
    {

        if (SlidingWindowDelete(this_window->window) != SLIDING_WINDOW_OK)
        {
            ret = FAST_MEAN_WINDOW_ERR;
        }

        SlidingWindowFree(*win);

        *win = NULL;

        ret = FAST_MEAN_WINDOW_OK;
    }

    return ret;
}
