/*!
 * \file       EventSST2xVF.h
 * \brief      Header file of the Event Manager using the STT2xVF memory
 * \date       2021-09-14
 * \version    1.0
 * \author     Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * \copyright  Copyright (c) 2021
 */
/*! \addtogroup EventSST2xVF SST2xVF
 *  \ingroup EventManager
 * @{
 */
#ifndef _EVENT_SST2XVF_H_
#define _EVENT_SST2XVF_H_
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "EventManager.h"

const EventMemoryInterface_t *EventSST2xVFGetInterface(void);

/*! @}*/ // End of EventSST2xVF
#endif   //_EVENT_SST2XVF_H_
