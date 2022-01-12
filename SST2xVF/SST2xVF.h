/*************************************************
 * $Id$		SST2xVF.h			2021-09-07
 *//**
* \file		SST2xVF.h
* \brief	Header file of the SST2xVF flash memories
* \version	1.1
* \date		07/09/2021
* \author	Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
*************************************************/
/** @addtogroup  SST2xVF SST2xVF
 * @{
 */
#ifndef __SST2xVF_DRIVER_H
#define __SST2xVF_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/*!
 *  \brief Return codes of the functions
 */
typedef enum
{
    SST2xVF_RET_OK                = 0,  /**< The function executed succesfully*/
    SST2xVF_RET_ERROR             = -1, /**< Error occurred*/
    SST2xVF_RET_INVALID_ADDRESS   = -2, /**< Invalid input address*/
    SST2xVF_RET_WARNING_READ_SIZE = -3, /**< If read less than requested*/
    SST2xVF_RET_NOT_INIT          = -4, /**< The component is not initialized*/
    SST2xVF_RET_MUTEX_TAKE_ERR    = -5, /**< Mutex take failed*/
    SST2xVF_RET_MUTEX_GIVE_ERR    = -6  /**< Mutex give failed*/
} SST2xVF_RET;
/*!
 *  \brief Return codes of the functions
 */
typedef enum
{
    CE_DISABLE = 0,          /**<Chip select disable*/
    CE_ENABLE  = !CE_DISABLE /**<Chip select enable*/
} SST2xVF_ChipEnable_e;
/*!
 *  \brief List of supported models
 */
typedef enum
{
    SST25VF010A, /**<1Mbit memory (sector: 256 x 4096)*/
    SST26VF016B, /**<Not implemented*/
    SST26VF032B, /**<Not implemented*/
    SST26VF064B, /**<64Mbit memory (sector: 16384 x 4096)*/
    SST25VF064C  /**<64Mbit memory DEPRACATED (sector: 16384 x 4096)*/
} SST2xVF_Model_e;
/*!
 *  \brief SST25VF flash information
 */
typedef struct
{
    SST2xVF_Model_e model;        /**<Used memory model*/
    uint32_t        size;         /**<Total flash size, in bytes*/
    uint32_t        sector_count; /**<Number of sectors*/
    uint32_t        sector_size;  /**<Uniform sector size in bytes (0=sector_info used)*/
    uint32_t        page_size;    /**<Optimal programming page size in bytes*/
    uint32_t        block_count;  /**<Number of blocks*/
    uint32_t        block_size;   /**<Block size in bytes*/
    uint32_t        program_unit; /**<Smallest programmable unit in bytes*/
    uint8_t         erased_value; /**<Contents of erased memory (usually 0xFF)*/
    uint8_t         man_id;       /**<Manufacturer ID*/
    uint8_t         dev_type;     /**<Device type (only on JEDEC compatible memories)*/
    uint8_t         dev_id;       /**<Device ID*/
    bool            initialized;  /**<If the component is initialized*/
} SST2xVF_INFO_t;
/*!
 *  \brief SST25VF Driver control block
 */
typedef struct
{
    SST2xVF_RET (*Initialize)(void);                                        /**<Pointer to \ref SST2xVF_Initialize : initialize driver*/
    SST2xVF_RET (*Unitialize)(void);                                        /**<Pointer to \ref SST2xVF_Uninitialize :*/
    SST2xVF_RET (*ReadData)(uint32_t addr, uint8_t *data, uint16_t cnt);    /**<Pointer to \ref SST2xVF_ReadData : reads a buffer*/
    SST2xVF_RET (*ProgramData)(uint32_t addr, uint8_t *data, uint16_t cnt); /**<Pointer to \ref SST2xVF_ProgramData : writes a buffer*/
    SST2xVF_RET (*ProgramByte)(uint32_t addr, uint8_t data);                /**<Pointer to \ref SST2xVF_ProgramByte : writes a single byte*/
    SST2xVF_RET (*EraseSector)(uint32_t addr);                              /**<Pointer to \ref SST2xVF_EraseSector : erase a sector*/
    SST2xVF_RET (*EraseBlock)(uint32_t addr);                               /**<Pointer to \ref SST2xVF_EraseBlock : erase a block*/
    SST2xVF_RET (*EraseChip)(void);                                         /**<Pointer to \ref SST2xVF_EraseChip : erase full chip*/
    const SST2xVF_INFO_t *(*GetInfo)(void);                                 /**<Pointer to \ref SST2xVF_GetInfo : get flash information*/
} SST2xVF_DRIVER_t;

#endif /* __SST2xVF_DRIVER_H */

/** @} */ // End of SST2xVF
