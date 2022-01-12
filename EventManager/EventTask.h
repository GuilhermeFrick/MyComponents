/*!
 * \file       EventTask.c
 * \brief      Header file of event manager task
 * \date       2021-09-15
 * \version    1.0
 * \author     Luiz Guilherme Rizzatto Zucchi (luiz.zucchi@perto.com.br)
 * \copyright  Copyright (c) 2021
 */
#ifndef EVENT_TASK_H
#define EVENT_TASK_H
#include <stdbool.h>

/*! \addtogroup  EventTask Event Task
 *  \ingroup EventManager
 * @{
 */
void EventTaskCreate(void);
void EventTaskTerminate(bool blocking);

/*! @}*/ // End of EventTask
#endif   // EVENT_TASK_H
