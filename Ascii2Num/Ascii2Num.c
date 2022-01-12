/*************************************************
* $Id$		Ascii2Num.c			2020-06-06	
*//**
* \file		Ascii2Num.c
* \brief	Some useful functions
* \version	1.1
* \date		13/06/2020
* \author	Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
*************************************************/
#include "Ascii2Num.h"

/*!
 * \brief 		Convert ASCII characters to a 8-bit value
 * \param[in]   data: Buffer containing the data to be converted
 * \param[in]   size: Size of the data
 * \param[out]  value: The converted value
 * \retval		true Success
 * \retval		false Buffer contain non numeric characters
 */
bool Ascii2Num_8(uint8_t *data, uint32_t size, /*@out@*/uint8_t *value)
{
	uint32_t val;
    
    *value = 0;
	
	if (Ascii2Num_32(data, size, &val) == false)
		return false;

	*value = (uint8_t) val;
	return true;
}
/*!
 * \brief 		Convert ASCII characters to a 16-bit value
 * \param[in]   data: Buffer containing the data to be converted
 * \param[in]   size: Size of the data
 * \param[out]  value: The converted value
 * \retval		true Success
 * \retval		false Buffer contain non numeric characters
 */
bool Ascii2Num_16(uint8_t *data, uint32_t size, /*@out@*/uint16_t *value)
{
	uint32_t val;
    
    *value = 0;
	
	if (Ascii2Num_32(data, size, &val) == false)
		return false;

	*value = (uint16_t) val;
	return true;
}
/*!
 * \brief 		Convert ASCII characters to a 32-bit value
 * \param[in]   data: Buffer containing the data to be converted
 * \param[in]   size: Size of the data
 * \param[out]  value: The converted value
 * \retval		true Success
 * \retval		false Buffer contain non numeric characters
 */
bool Ascii2Num_32(uint8_t *data, uint32_t size, /*@out@*/uint32_t *value)
{
	uint32_t i;
	uint32_t val = 0;

	*value = 0;

	for (i = 0; i < size; i++, data++) {
		if (*data < (uint8_t)'0' || *data > (uint8_t)'9')
			return false;

		val = 10*val + *data - (uint8_t)'0';
	}

	*value = val;
	return true;
}
/*!
 * \brief 		Convert ASCII characters to a float value
 * \param[in]   data: Buffer containing the data to be converted
 * \param[in]   size: Size of the data
 * \param[out]  value: The converted value
 * \retval		true Success
 * \retval		false Buffer contain non numeric characters
 */
bool Ascii2Num_float(uint8_t *data, uint32_t size, /*@out@*/float *value)
{
	uint32_t val;

	*value = 0;
	if (Ascii2Num_32(data, size, &val) == false)
		return false;


	*value = (float) val;
	return true;
}
/*!
 * \brief 		Convert HEX-ASCII characters to a 32-bit value
 * \param[in]   data: Buffer containing the data to be converted
 * \param[in]   size: Size of the data
 * \param[out]  value: The converted value
 * \retval		true Success
 * \retval		false Buffer contain non numeric characters
 */
bool HexAscii2Num_8(uint8_t *data, uint32_t size, /*@out@*/uint8_t *value)
{
	uint32_t val;
	
	*value = 0;
	if (HexAscii2Num_32(data, size, &val) == false)
		return false;

	*value = (uint8_t) val;
	return true;
}
/*!
 * \brief 		Convert HEX-ASCII characters to a 32-bit value
 * \param[in]   data: Buffer containing the data to be converted
 * \param[in]   size: Size of the data
 * \param[out]  value: The converted value
 * \retval		true Success
 * \retval		false Buffer contain non numeric characters
 */
bool HexAscii2Num_32(uint8_t *data, uint32_t size, /*@out@*/uint32_t *value)
{
	uint32_t i;
	uint32_t val = 0;

	*value = 0;

	for (i = 0; i < size; i++) {
        if((data[i] >= (uint8_t)'0') && (data[i] <= (uint8_t)'9'))
            val = 16*val + data[i] - (uint8_t)'0';
        else if((data[i] >= (uint8_t)'A') && (data[i] <= (uint8_t)'F'))
            val = 16*val + ((data[i] - (uint8_t)'A') + 0x0A);
        else if((data[i] >= (uint8_t)'a') && (data[i] <= (uint8_t)'f'))
            val = 16*val + ((data[i] - (uint8_t)'a') + 0x0A);
        else
            return false;
	}

	*value = val;
	return true;
}
