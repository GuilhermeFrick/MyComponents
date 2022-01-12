/***********************************************************************
* $Id$		logger_manager_FreeRTOS.c         2020-10-14
*//**
* \file		logger_manager_FreeRTOS.c 
* \brief	Template file for defining strong functions for component
* \version	1.0
* \date		14. Oct. 2020
* \author	Guilherme Frick de Oliveira
*************************************************************************/

#include "logger_manager.h"
#include "FreeRTOS.h"
#include "task.h"
/*!
 *  \brief      Allocate memory block with control
 *  \param[in]  size: size of the memory block, in bytes
 *  \return 	On success, a pointer to the memory block allocated by the function
 *              If the function failed to allocate the requested block of memory, a null pointer is returned
 *  \warning    Stronger function must be declared externally
 */
void * LoggerManagerMalloc(uint32_t size)
{
    uint8_t *buffer = NULL;
    buffer = pvPortMalloc( size );
    while(buffer == NULL)
    {
        vTaskDelay(10);
        buffer = pvPortMalloc( size );
    }
    return buffer;
}
/*!
 *  \brief      Deallocate memory block
 *  \param[in]  buffer: pointer to a memory block previously allocated with malloc
 *  \warning    Stronger function must be declared externally
 */
void LoggerManagerFree(void * buffer)
{
    vPortFree(buffer);
}





