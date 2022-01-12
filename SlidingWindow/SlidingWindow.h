/**
 * @file SlidingWindow.h
 * @author Henrique Awoyama Klein
 * @author Guilherme Frick de Oliveira (guiherme.oliveira_irede@perto.com.br)
 * @brief
 * @version 0.1
 * @date 2021-11-16
 *
 * @copyright Copyright (c) 2021
 *
 */
/** @addtogroup   SlidingWindow Sliding Window
 * @{
 */
#ifndef SLIDING_WINDOW_H
#define SLIDING_WINDOW_H
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#ifdef __cplusplus
extern "C"
{
#endif
    typedef enum SlidingWindowRet_e
    {
        SLIDING_WINDOW_OK = 0,        /**<The function returned OK*/
        SLIDING_WINDOW_ERR_INV_PARAM, /**<A parameter is wrong*/
        SLIDING_WINDOW_ERR_MEM        /**<Insufficient memory*/
    } SlidingWindowRet_et;

    typedef void *SlidingWindow_t; /**<Sliding window handle*/

    SlidingWindowRet_et SlidingWindowCreate(SlidingWindow_t *window, size_t item_size, size_t num_elements, void *default_value);
    SlidingWindowRet_et SlidingWindowAppend(SlidingWindow_t window, void *item);
    SlidingWindowRet_et SlidingWindowGetLastItems(SlidingWindow_t window, size_t n, void *items);
    SlidingWindowRet_et SlidingWindowGetFloatAvg(SlidingWindow_t window, size_t n, float *const avg);
    SlidingWindowRet_et SlidingWindowDelete(SlidingWindow_t *window);
    SlidingWindowRet_et SlidingWindowIsCleared(SlidingWindow_t window, bool *is_empty);
    SlidingWindowRet_et SlidingWindowGetTail(SlidingWindow_t window, void *item);
    SlidingWindowRet_et SlidingWindowGetHead(SlidingWindow_t window, void *item);
    SlidingWindowRet_et SlidingWindowGetItem(SlidingWindow_t window, size_t n, void *items);
    SlidingWindowRet_et SlidingWindowReset(SlidingWindow_t window);
    SlidingWindowRet_et SlidingWindowGetWinSize(SlidingWindow_t window, size_t *window_size);

    // void *              SlidingWindowMalloc(size_t size);
    // void                SlidingWindowFree(void *ptr);

#ifdef __cplusplus
}
#endif
/** @}*/ // End of SlidingWindow

#endif
