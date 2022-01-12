/*************************************************************************
 * $Id$     logger_manager.h     08 de oct de 2020
 */
/**
 * \file    logger_manager.h
 * \brief   Source file of the logger_manager.h
 * \version	1.0
 * \date    08 de oct de 2020
 * \author  Guilherme Frick
 * \verbatim
 *   ** Making this component functional **
 *   =======================================
 *   1- Create one or more LoggerHandle_t to hold your loggers.
 *   2- Initialize your handles with the ItemInfo_t table, informing how you want to print each item.
 *   3- Call the LoggerInit function with the above info.
 *   4- Define the static buffer to print on each handle using the LoggerDefineBuffer function.
 *   5- Override LoggerManagerSendBuffer and/or LoggerManagerPrint functions to send the data according to your liking.
 *   If you want to redirect to a debug printf function, you can only overwrite the LoggerManagerPrint function.
 *   If you want to send the data through a interface, you can only overwrite the LoggerManagerSendBuffer function.
 *   6- Trigger the LoggerRun function when desired.
 *
 *   ** Memory allocation notes **
 *   ========================================
 *   1(Optional)- Override LoggerManagerMalloc and LoggerManagerFree functions to use your own memory control functions.
 *   - If the application doesn't have a memory management module, the weak functions will use the native calls.
 *
 * \endverbatim
 *************************************************************************/
/** \addtogroup   LoggerManager LoggerManager
 * @{
 */

#ifndef LOGGER_MANAGER_H_
#define LOGGER_MANAGER_H_
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum FormatType_e
    {
        FORMAT_ERROR = -1,
        FORMAT_INT   = 0,      ///<
        FORMAT_UINT,           ///<
        FORMAT_FLOAT,          ///<
        FORMAT_LONG_INT,       ///<
        FORMAT_LONG_UINT,      ///<
        FORMAT_LONG_LONG_INT,  ///<
        FORMAT_LONG_LONG_UINT, ///<
        FORMAT_STRING,
    } FormatType_et;

    /*!
     *  \brief Return values of the LoggerManager component
     */
    typedef enum LoggerManagerRetDefinition
    {
        LOGGER_RET_OK          = 0, /*!<The LoggerManager was successfully initialized*/
        LOGGER_RET_ERROR       = 1, /*!<The Logger was not initialized*/
        LOGGER_FAULT_ERR_MEM   = 2, /*!<Insufficient memory*/
        LOGGER_RET_OUT_OF_DATA = 3, /*!<Data was sent*/
        LOGGER_RET_INVALID     = 4  /*!<Invalid parameter*/
    } LoggerManagerRet_e;

    /*!
     * \brief       Item format information, required to print
     */
    typedef struct ItemInfoDefinition
    {
        const char *label;
        const char *format_str; /*!<Format string for one item*/
        size_t      item_size;  /*!<Size of the item in bytes*/
    } ItemInfo_t;

    /*!
     *  \brief Logger operating states
     */
    typedef enum
    {
        LOGGER_IDLE    = 0, /*!<Puts the logger in an idle state>*/
        LOGGER_RUNNING = 1  /*!<Puts the logger in a running state>*/
    } LoggerState_e;

    /*!
     * \brief       Handle of a logger instance
     */
    typedef void *LoggerHandle_t;

    LoggerManagerRet_e LoggerInit(LoggerHandle_t *inst, ItemInfo_t *info, size_t qty);
    LoggerManagerRet_e LoggerDeInit(LoggerHandle_t *inst);
    LoggerManagerRet_e LoggerDefineBuffer(LoggerHandle_t inst, void *buff, size_t buffer_size);
    LoggerManagerRet_e LoggerStart(LoggerHandle_t inst);
    LoggerManagerRet_e LoggerStop(LoggerHandle_t inst);
    LoggerManagerRet_e LoggerRun(LoggerHandle_t inst, uint32_t max_iter);
    LoggerManagerRet_e LoggerGetState(LoggerHandle_t inst, LoggerState_e *logger_state);
    FormatType_et      GetFormatType(const char *format_str);

#ifdef __cplusplus
}
#endif   /* LOGGER_MANAGER_H_ */
/** @}*/ // End of Logger Manager
#endif
