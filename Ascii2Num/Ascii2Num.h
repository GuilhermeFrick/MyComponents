/*************************************************
* $Id$		Ascii2Num.h			2020-06-06	
*//**
* \file		Ascii2Num.h
* \brief	Some useful functions
* \version	1.0
* \date		13/06/2020
* \author	Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
*************************************************/
/** \addtogroup  Ascii2Num Ascii2Num 
 * @{
 */
#ifndef ASCII_2_NUM_H
#define ASCII_2_NUM_H
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STR(s) #s           /**<Macro to get the string of a define*/
#define XSTR(s) STR(s)      /**<Stringizing macro*/

bool Ascii2Num_32(uint8_t *data, uint32_t size, /*@out@*/uint32_t *value);
bool Ascii2Num_float(uint8_t *data, uint32_t size, /*@out@*/float *value);
bool Ascii2Num_16(uint8_t *data, uint32_t size, /*@out@*/uint16_t *value);
bool Ascii2Num_8(uint8_t *data, uint32_t size, /*@out@*/uint8_t *value);
bool HexAscii2Num_32(uint8_t *data, uint32_t size, /*@out@*/uint32_t *value);
bool HexAscii2Num_8(uint8_t *data, uint32_t size, /*@out@*/uint8_t *value);

#ifdef __cplusplus
}
#endif
#endif // ASCII_2_NUM_H
 /**
 * @}
 */
