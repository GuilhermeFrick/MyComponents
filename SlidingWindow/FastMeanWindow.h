/**
 * @file FastSlidingWindow.h
 * @author Guilherme Frick de Oliveira (guiherme.oliveira_irede@perto.com.br)
 * @brief
 * @version 0.1
 * @date 2021-11-16
 *
 * @copyright Copyright (c) 2021
 *
 */
/** @addtogroup   FastSlidingWindow Sliding Window
 * @{
 */
#ifndef _FAST_MEAN_WINDOW_
#define _FAST_MEAN_WINDOW_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum FastMeanWindowRet_et
    {
        FAST_MEAN_WINDOW_OK = 0,        /**<The function returned OK*/
        FAST_MEAN_WINDOW_ERR_INV_PARAM, /**<A parameter is wrong*/
        FAST_MEAN_WINDOW_ERR,
        FAST_MEAN_WINDOW_ERR_TAIL,
        FAST_MEAN_WINDOW_ERR_APPEND,
        FAST_MEAN_WINDOW_ERR_INIT,
        FAST_MEAN_WINDOW_ERR_MEM /**<Insufficient memory*/
    } FastMeanWindowRet_et;

    typedef void *FastMeanWindow_t; /**<Sliding window handle*/

    FastMeanWindowRet_et FastMeanWindowCreate(FastMeanWindow_t *win, size_t win_size, void *default_value);
    FastMeanWindowRet_et FastMeanWindowAppend(FastMeanWindow_t win, int32_t new_data);
    FastMeanWindowRet_et FastMeanWindowGetAverage(FastMeanWindow_t win, float *const moving_average);
    FastMeanWindowRet_et FastMeanWindowReset(FastMeanWindow_t win);
    FastMeanWindowRet_et FastMeanWindowDelete(FastMeanWindow_t *win);

#ifdef __cplusplus
}
#endif

#endif