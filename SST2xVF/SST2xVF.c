/************************************************
 * $Id$		SST2xVF.c			2021-09-07
 *//**
* \file		SST2xVF.c
* \brief	Source file of the SST2xVF flash memories
* \version	1.1
* \date		07/09/2021
* \author	Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
*************************************************/
#include <string.h>
#include "SST2xVF.h"

#ifndef __weak
#define __weak __attribute__((weak))
#endif

/** \weakgroup   SST2xVFWeak  SST2xVF Weak
 *  \ingroup SST2xVF
 * @{
 */
__weak void        SST2xVF_ChipEnable(SST2xVF_ChipEnable_e state);
__weak SST2xVF_RET SST2xVF_WriteSPI(uint8_t *data, uint32_t size);
__weak SST2xVF_RET SST2xVF_ReadSPI(uint8_t *data);
__weak uint32_t    SST2xVF_GetTick(void);
__weak void        SST2xVF_DelayUs(uint32_t delay);
__weak SST2xVF_RET SST2xVF_MutexTake(void);
__weak SST2xVF_RET SST2xVF_MutexGive(void);

/** @}*/ // End of SST2xVFWeak

/** \addtogroup  SST2xVFPrivate SST2xVF Private
 *  \ingroup SST2xVF
 * @{
 */

#define SSTxVF_DRIVER_BUSY_TIMEOUT      200 /**< 200 ms*/
#define SSTxVF_DRIVER_BYTE_PROGRAM_TIME 20  /**< 20 us*/

#define SST2xVF_SR_BUSY   1 << 0 /**< (0)1 = (No) Internal Write operation is in progress*/
#define SST2xVF_SR_WEL    1 << 1 /**< (0)1 = Device is (not) memory Write enabled*/
#define SST25VF010_SR_BP0 1 << 2 /**< Indicate current level of block write protection*/
#define SST25VF010_SR_BP1 1 << 3 /**< Indicate current level of block write protection*/
#define SST25VF010_SR_AAI 1 << 6 /**< 1 = AAI programming mode | 0 = Byte-Program mode*/
#define SST25VF010_SR_BPL 1 << 7 /**< 1 = BP1, BP0 are read-only bits | 0 = BP1, BP0 are read/writable*/

#define SST26VF064B_SR_BUSY   1 << 0 /**< (0)1 = (No) Internal Write operation is in progress*/
#define SST26VF064B_SR_WEL    1 << 1 /**< (0)1 = Device is (not) memory Write enabled*/
#define SST26VF064B_SR_WSE    1 << 2 /**< Indicate current level of block write protection*/
#define SST26VF064B_SR_WSP    1 << 3 /**< Indicate current level of block write protection*/
#define SST26VF064B_SR_WPLD   1 << 4 /**< Indicate current level of block write protection*/
#define SST26VF064B_SR_SEC    1 << 5 /**< Indicate current level of block write protection*/
#define SST26VF064B_SR_RES    1 << 6 /**< 1 = AAI programming mode | 0 = Byte-Program mode*/
#define SST26VF064B_SR_BUSY_2 1 << 7 /**< 1 = BP1, BP0 are read-only bits | 0 = BP1, BP0 are read/writable*/

#define SST25VF064C_SR_BUSY 1 << 0 /**< (0)1 = (No) Internal Write operation is in progress*/
#define SST25VF064C_SR_WEL  1 << 1 /**< (0)1 = Device is (not) memory Write enabled*/
#define SST25VF064C_SR_BP0  1 << 2 /**< Indicate current level of block write protection*/
#define SST25VF064C_SR_BP1  1 << 3 /**< Indicate current level of block write protection*/
#define SST25VF064C_SR_BP2  1 << 4 /**< Indicate current level of block write protection*/
#define SST25VF064C_SR_BP3  1 << 5 /**< Indicate current level of block write protection*/
#define SST25VF064C_SR_SEC  1 << 6 /**< 1 = AAI programming mode | 0 = Byte-Program mode*/
#define SST25VF064C_SR_BPL  1 << 7 /**< 1 = BP1, BP0 are read-only bits | 0 = BP1, BP0 are read/writable*/

#define SST26VF064B_BLOCK_MASK                0xff /**< Block bits for each of the 128/144 configurable block bits bits*/
#define SST26VF064B_BLOCK_MASK_FIRST_8_BLOCKS 0x55 /**< Block bits for the first 16/144 configurable block bits*/

static SST2xVF_INFO_t FlashInfo; /**<Variable containing the information of the flash memory installed on the board*/

/*!
 *  \brief  Command list
 */
typedef enum
{
    /* Configuration */
    SST2xVF_NOP         = 0x00, /**< No Operation*/
    SST2xVF_RSTEN       = 0x66, /**< Reset Enable*/
    SST2xVF_RST         = 0x99, /**< Reset Memory*/
    SST2xVF_EQIO        = 0x38, /**< Enable Quad I/O*/
    SST2xVF_RDSR        = 0x05, /**< Read Status Register*/
    SST2xVF_WRSR        = 0x01, /**< Write Status Register*/
    SST2xVF_ENABLE_WRSR = 0x50, /**< Enable write Status Register*/
    SST2xVF_RDCR        = 0x35, /**< Read Configuration Register*/
                                /* Read */
    SST2xVF_READ    = 0x03,     /**< Read Memory (20\40MHz)*/
    SST2xVF_HS_READ = 0x0B,     /**< Read Memory at Higher Speed (33/80/104MHz)*/
    SST2xVF_SQOR    = 0x6B,     /**< SPI Quad Output Read*/
    SST2xVF_SQIOR   = 0xEB,     /**< SPI Quad I/O Read*/
    SST2xVF_SDOR    = 0x3B,     /**< SPI Dual Output Read (same as "Fast-Read Dual")*/
    SST2xVF_SDIOR   = 0xBB,     /**< SPI Dual I/O Read (same as "Fast-Read Dual")*/
    SST2xVF_SB      = 0xC0,     /**< Set Burst Length*/
    SST2xVF_RBSQI   = 0x0C,     /**< SQI Read Burst with Wrap*/
    SST2xVF_RBSPI   = 0xEC,     /**< SPI Read Burst with Wrap*/
                                /* Identification */
    SST2xVF_JEDEC_ID  = 0x9F,   /**< JEDEC ID Read */
    SST2xVF_QUAD_J_ID = 0xAF,   /**< Quad I/O JEDEC ID Read */
    SST2xVF_READ_ID   = 0x90,   /**< Manufacturer-ID Read (same as SST2xVF_READ_ID_2)*/
    SST2xVF_READ_ID_2 = 0xAB,   /**< Manufacturer-ID Read (same as SST2xVF_READ_ID)*/
    SST2xVF_SFDP      = 0x5A,   /**< Serial Flash Discoverable Parameters*/
                                /* Write */
    SST2xVF_WREN        = 0x06, /**< Write Enable*/
    SST2xVF_WRDI        = 0x04, /**< Write Disable*/
    SST2xVF_SE          = 0x20, /**< Sector erase - Erase 4 KBytes of Memory Array*/
    SST2xVF_BE          = 0xD8, /**< Block erase - Erase 64, 32 or 8 KBytes of Memory Array */
    SST2xVF_BE_64       = 0x52, /**< Block erase - Erase 64 KBytes of Memory Array (064C) */
    SST2xVF_CE          = 0xC7, /**< Erase Full Array (same as SST2xVF_CE_2)*/
    SST2xVF_CE_2        = 0x60, /**< Erase Full Array (same as SST2xVF_CE)*/
    SST2xVF_PP          = 0x02, /**< Byte Program or Page Program (mem dependent)*/
    SST2xVF_SPI_QUAD_PP = 0x32, /**< SQI Quad Page Program*/
    SST2xVF_WRSU        = 0xB0, /**< Suspends Program/Erase*/
    SST2xVF_WRRE        = 0x30, /**< Resumes Program/Erase*/
    SST2xVF_DI_PP       = 0xA2, /**< Dual Input Page Program (064C)*/
    SST2xVF_AAIP        = 0xAF, /**< Auto Address Increment (AAI) Program	(010A))*/
                                /* Protection */
    SST2xVF_RBPR        = 0x72, /**< Read Block Protection Register*/
    SST2xVF_WBPR        = 0x42, /**< Write Block Protection Register*/
    SST2xVF_LBPR        = 0x8D, /**< Lock Down Block Protection Register*/
    SST2xVF_nVWLDR      = 0xE8, /**< non-Volatile Write Lock Down Register*/
    SST2xVF_ULBPR       = 0x98, /**< Global Block Protection Unlock*/
    SST2xVF_READ_SID    = 0x88, /**< Read Security ID*/
    SST2xVF_PROGRAM_SID = 0xA5, /**< Program User Security ID area */
    SST2xVF_LOCKOUT_SID = 0x85, /**< Lockout Security ID Programming */
    SST2xVF_EHLD        = 0xAA  /**< Enable HOLD ping of the RST HOLD pin */
} SST2xVF_INSTRUCTION;

static uint32_t    SST2xVF_GetElapsedTime(uint32_t InitialTime);
static SST2xVF_RET SST2xVF_SendInstruction(SST2xVF_INSTRUCTION instr);
static SST2xVF_RET SST2xVF_ReadStatusRegister(uint8_t *value);
static SST2xVF_RET SST25VF_WriteStatusDriver(uint8_t value);
static SST2xVF_RET SST26VF_WriteStatusDriver(uint8_t value);
static uint8_t     SST2xVF_StatusBusy(void);
static SST2xVF_RET SST2xVF_WaitBusy(uint16_t timeout);
static SST2xVF_RET SST2xVF_WriteEnable(void);
static SST2xVF_RET SST2xVF_WriteDisable(void);
static SST2xVF_RET SST26VF064B_ReadBlockProtectionRegister(uint8_t *value);
static SST2xVF_RET SST26VF064B_WriteBlockProtectionRegister(uint8_t *value);
static SST2xVF_RET SST25VF010A_FullMemoryLock(void);
static SST2xVF_RET SST25VF064C_FullMemoryLock(void);
static SST2xVF_RET SST26VF064B_FullMemoryLock(void);
static SST2xVF_RET SST25VF010A_FullMemoryUnlock(void);
static SST2xVF_RET SST25VF064C_FullMemoryUnlock(void);
static SST2xVF_RET SST26VF064B_FullMemoryUnlock(void);
static SST2xVF_RET SST2xVF_ReadID(uint8_t *man_id, uint8_t *dev_id);
static SST2xVF_RET SST2xVF_ReadJEDEC_ID(uint8_t *man_id, uint8_t *dev_id);

/**
 * \brief   	Function pointer to write memory status register
 * \param[in]   value - Status for write
 * \return      Result of operation \ref SST2xVF_RET
 */
static SST2xVF_RET (*SST2xVF_WriteStatusDriver)(uint8_t value);
/**
 * \brief   Function pointer to lock all SST2x memory
 * \return  Result of operation \ref SST2xVF_RET
 */
static __attribute__((used)) SST2xVF_RET (*SST2xVF_FullMemoryLock)(void);
/**
 * \brief   Function pointer to unlock all SST2x memory
 * \return  Result of operation \ref SST2xVF_RET
 */
static SST2xVF_RET (*SST2xVF_FullMemoryUnlock)(void);

/** @}*/ // End of SST2xVFPrivate

SST2xVF_RET           SST2xVF_Initialize(void);
SST2xVF_RET           SST2xVF_Uninitialize(void);
SST2xVF_RET           SST2xVF_EraseSector(uint32_t address);
SST2xVF_RET           SST2xVF_EraseBlock(uint32_t address);
SST2xVF_RET           SST2xVF_EraseChip(void);
SST2xVF_RET           SST2xVF_ReadData(uint32_t address, uint8_t *buffer, uint16_t length);
SST2xVF_RET           SST2xVF_ProgramByte(uint32_t address, uint8_t byte);
SST2xVF_RET           SST2xVF_ProgramData(uint32_t address, uint8_t *buffer, uint16_t length);
const SST2xVF_INFO_t *SST2xVF_GetInfo(void);

/*!
 *  \brief Access structure of the SST2xVF Driver
 */
SST2xVF_DRIVER_t SST2xVF_DRIVER = { //
    SST2xVF_Initialize,             //
    SST2xVF_Uninitialize,           //
    SST2xVF_ReadData,               //
    SST2xVF_ProgramData,            //
    SST2xVF_ProgramByte,            //
    SST2xVF_EraseSector,            //
    SST2xVF_EraseBlock,             //
    SST2xVF_EraseChip,              //
    SST2xVF_GetInfo};
/*!
 *  \brief      Function that calculates the elapsed time from an initial time
 *	\param[in]  InitialTime: Initial time for calculation
 *  \return     Elapsed time from initial time in milliseconds
 *  \note       This function corrects the error caused by overflow
 */
static uint32_t SST2xVF_GetElapsedTime(uint32_t InitialTime)
{
    uint32_t actualTime;
    actualTime = SST2xVF_GetTick();

    if (InitialTime <= actualTime)
        return (actualTime - InitialTime);
    else
        return (((0xFFFFFFFFUL) - InitialTime) + actualTime);
}
/*!
 * \brief       Function to send an instruction to memory
 * \param[in]   instr: instruction to sendo (SST2xVF_INSTRUCTION)
 * \return      Result of operation
 * \retval      SST2xVF_RET_OK: success
 * \retval      SST2xVF_RET_ERROR: error
 */
static SST2xVF_RET SST2xVF_SendInstruction(SST2xVF_INSTRUCTION instr)
{
    return SST2xVF_WriteSPI((uint8_t *)&instr, 1);
}
/**
 * \brief   	Read status register
 * \param[out]  *value - Pointer to the return value
 * \return      Result of operation
 * \retval      SST2xVF_RET_OK: success
 * \retval      SST2xVF_RET_ERROR: error
 */
static SST2xVF_RET SST2xVF_ReadStatusRegister(uint8_t *value)
{
    SST2xVF_RET ret = SST2xVF_RET_NOT_INIT;

    do
    {
        SST2xVF_ChipEnable(CE_ENABLE);

        ret = SST2xVF_SendInstruction(SST2xVF_RDSR);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        ret = SST2xVF_ReadSPI(value);

        /* code */
    } while (0);
    SST2xVF_ChipEnable(CE_DISABLE);

    return ret;
}
/**
 * \brief   	Write status register for SST25 family
 * \param[in]   value - Status for write
 * \return      Result of operation
 * \retval      SST2xVF_RET_OK: success
 * \retval      SST2xVF_RET_ERROR: error
 */
static SST2xVF_RET SST25VF_WriteStatusDriver(uint8_t value)
{
    SST2xVF_RET ret = SST2xVF_RET_ERROR;

    do
    {
        SST2xVF_ChipEnable(CE_ENABLE);
        ret = SST2xVF_SendInstruction(SST2xVF_ENABLE_WRSR);
        SST2xVF_ChipEnable(CE_DISABLE);

        if (ret != SST2xVF_RET_OK)
        {
            break;
        }

        SST2xVF_ChipEnable(CE_ENABLE);
        ret = SST2xVF_SendInstruction(SST2xVF_WRSR);

        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        ret = SST2xVF_WriteSPI(&value, 1);

    } while (0);

    SST2xVF_ChipEnable(CE_DISABLE);
    return ret;
}
/**
 * \brief   	Write status register for SST26 family
 * \param[in]   value - Status for write
 * \return      Result of operation
 * \retval      SST2xVF_RET_OK: success
 * \retval      SST2xVF_RET_ERROR: error
 */
static SST2xVF_RET SST26VF_WriteStatusDriver(uint8_t value)
{
    SST2xVF_RET ret = SST2xVF_RET_ERROR;

    do
    {
        uint8_t data_temp[2] = {0};

        data_temp[1] = value;
        SST2xVF_ChipEnable(CE_ENABLE);
        ret = SST2xVF_SendInstruction(SST2xVF_ENABLE_WRSR);
        SST2xVF_ChipEnable(CE_DISABLE);

        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        SST2xVF_ChipEnable(CE_ENABLE);
        ret = SST2xVF_SendInstruction(SST2xVF_WRSR);

        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        ret = SST2xVF_WriteSPI(data_temp, 2);

    } while (0);
    SST2xVF_ChipEnable(CE_DISABLE);

    return ret;
}
/**
 * \brief   Read status register and verify if memory is busy
 * \retval  0x01 : memory busy
 * \retval  0x00 : memory idle
 */
static uint8_t SST2xVF_StatusBusy(void)
{
    uint8_t status_register;
    uint8_t retval;

    // Read status register
    if (SST2xVF_ReadStatusRegister(&status_register) == SST2xVF_RET_OK)
    {
        retval = status_register & SST2xVF_SR_BUSY;
    }
    else
    {
        retval = 1;
    }

    return (retval);
}
/**
 * \brief   	Wait memory busy
 * \param[in]   timeout - Timeout in ms
 * \return      Result of operation
 * \retval      SST2xVF_RET_OK: success
 * \retval      SST2xVF_RET_ERROR: error, timeout while waiting
 */
static SST2xVF_RET SST2xVF_WaitBusy(uint16_t timeout)
{
    uint8_t  busy_status;
    uint32_t Timestamp;

    Timestamp = SST2xVF_GetTick();

    do
    {
        busy_status = SST2xVF_StatusBusy();
    } while (busy_status && (SST2xVF_GetElapsedTime(Timestamp) < timeout));

    if (busy_status == 0)
    {
        return SST2xVF_RET_OK;
    }
    else
    {
        return SST2xVF_RET_ERROR;
    }
}

/**
 * \brief   Enable write in memory
 * \return      Result of operation
 * \retval      SST2xVF_RET_OK: success
 * \retval      SST2xVF_RET_ERROR: error
 */
static SST2xVF_RET SST2xVF_WriteEnable(void)
{
    SST2xVF_RET ret = SST2xVF_RET_ERROR;
    do
    {
        ret = SST2xVF_WaitBusy(SSTxVF_DRIVER_BUSY_TIMEOUT);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        SST2xVF_ChipEnable(CE_ENABLE);
        ret = SST2xVF_SendInstruction(SST2xVF_WREN);
        SST2xVF_ChipEnable(CE_DISABLE);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        ret = SST2xVF_WaitBusy(SSTxVF_DRIVER_BUSY_TIMEOUT);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        uint8_t status_register;
        ret = SST2xVF_ReadStatusRegister(&status_register);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        if ((status_register & SST2xVF_SR_WEL) != SST2xVF_SR_WEL)
        {
            ret = SST2xVF_RET_ERROR;
        }
    } while (0);

    return (ret);
}
/**
 * \brief       Disable write in memory
 * \return      Result of operation
 * \retval      SST2xVF_RET_OK: success
 * \retval      SST2xVF_RET_ERROR: error
 */
static SST2xVF_RET SST2xVF_WriteDisable(void)
{
    SST2xVF_RET ret = SST2xVF_RET_ERROR;

    do
    {
        ret = SST2xVF_WaitBusy(SSTxVF_DRIVER_BUSY_TIMEOUT);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        SST2xVF_ChipEnable(CE_ENABLE);
        ret = SST2xVF_SendInstruction(SST2xVF_WRDI);
        SST2xVF_ChipEnable(CE_DISABLE);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        ret = SST2xVF_WaitBusy(SSTxVF_DRIVER_BUSY_TIMEOUT);

    } while (0);

    return ret;
}
/**
 * \brief   	Read block protection registers
 * \param[out]  *value - Pointer to the return value, must have size of 18 bytes at least
 * \return      Result of operation
 * \retval      SST2xVF_RET_OK: success
 * \retval      SST2xVF_RET_ERROR: error
 */
static SST2xVF_RET SST26VF064B_ReadBlockProtectionRegister(uint8_t *value)
{
    SST2xVF_RET ret = SST2xVF_RET_ERROR;

    do
    {
        SST2xVF_ChipEnable(CE_ENABLE);
        ret = SST2xVF_SendInstruction(SST2xVF_RBPR);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        for (uint8_t i = 0; i < 18; i++)
        {
            ret = SST2xVF_ReadSPI(value + i);
            if (ret != SST2xVF_RET_OK)
                break;
        }

    } while (0);

    SST2xVF_ChipEnable(CE_DISABLE);

    return ret;
}
/**
 * \brief   	Read block protection registers
 * \param[out]  *value - Pointer to the return value, must have size of 18 bytes at least
 * \return      Result of operation
 * \retval      SST2xVF_RET_OK: success
 * \retval      SST2xVF_RET_ERROR: error
 * \warning     This function might be wrong due to be copied and pasted from read function
 */
static SST2xVF_RET SST26VF064B_WriteBlockProtectionRegister(uint8_t *value)
{
    SST2xVF_RET ret = SST2xVF_RET_ERROR;

    do
    {
        ret = SST2xVF_WriteEnable();
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        SST2xVF_ChipEnable(CE_ENABLE);
        ret = SST2xVF_SendInstruction(SST2xVF_WBPR);

        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        for (uint8_t i = 0; i < 18; i++)
        {
            ret = SST2xVF_ReadSPI(value + i);
            if (ret != SST2xVF_RET_OK)
                break;
        }

    } while (0);

    SST2xVF_ChipEnable(CE_DISABLE);

    return ret;
}
/**
 * \brief   Global block protection lock
 * \return  Result of operation
 * \retval  SST2xVF_RET_OK: success
 * \retval  SST2xVF_RET_ERROR: error
 */
static SST2xVF_RET SST25VF010A_FullMemoryLock(void)
{
    SST2xVF_RET ret = SST2xVF_RET_ERROR;

    do
    {
        uint8_t status_register;
        ret = SST2xVF_ReadStatusRegister(&status_register);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }

        status_register |= (SST25VF010_SR_BP0 | SST25VF010_SR_BP1); // BP0 = 1 |  BP1 = 1
        ret = SST2xVF_WriteStatusDriver(status_register);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        ret = SST2xVF_ReadStatusRegister(&status_register);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        if (!(status_register & (SST25VF010_SR_BP0 | SST25VF010_SR_BP1)))
        {
            ret = SST2xVF_RET_ERROR;
        }

    } while (0);

    return ret;
}
/**
 * \brief   Global block protection lock
 * \return  Result of operation
 * \retval  SST2xVF_RET_OK: success
 * \retval  SST2xVF_RET_ERROR: error
 */
static SST2xVF_RET SST25VF064C_FullMemoryLock(void)
{
    SST2xVF_RET ret = SST2xVF_RET_ERROR;

    do
    {
        uint8_t status_register;

        ret = SST2xVF_ReadStatusRegister(&status_register);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        status_register |= (SST25VF064C_SR_BP0 | SST25VF064C_SR_BP1 | SST25VF064C_SR_BP2 | SST25VF064C_SR_BP3);
        ret = SST2xVF_WriteStatusDriver(status_register);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        ret = SST2xVF_ReadStatusRegister(&status_register);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        if (!(status_register & (SST25VF064C_SR_BP0 | SST25VF064C_SR_BP1 | SST25VF064C_SR_BP2 | SST25VF064C_SR_BP3)))
        {
            ret = SST2xVF_RET_ERROR;
        }
    } while (0);

    return ret;
}
/**
 * \brief   Global block protection lock
 * \return  Result of operation
 * \retval  SST2xVF_RET_OK: success
 * \retval  SST2xVF_RET_ERROR: error
 */
static SST2xVF_RET SST26VF064B_FullMemoryLock(void)
{
    SST2xVF_RET ret = SST2xVF_RET_ERROR;
    do
    {
        uint8_t block_registers[18]      = {SST26VF064B_BLOCK_MASK_FIRST_8_BLOCKS, SST26VF064B_BLOCK_MASK_FIRST_8_BLOCKS};
        uint8_t block_regs_reference[18] = {SST26VF064B_BLOCK_MASK_FIRST_8_BLOCKS, SST26VF064B_BLOCK_MASK_FIRST_8_BLOCKS};

        ret = SST26VF064B_ReadBlockProtectionRegister(block_registers);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        memset(&block_registers[2], SST26VF064B_BLOCK_MASK, (sizeof(block_registers) - 2));
        memset(&block_regs_reference[2], SST26VF064B_BLOCK_MASK, (sizeof(block_regs_reference) - 2));
        ret = SST26VF064B_WriteBlockProtectionRegister(block_registers);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        ret = SST26VF064B_ReadBlockProtectionRegister(block_registers);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        if (memcmp(block_registers, block_regs_reference, sizeof(block_registers)))
        {
            ret = SST2xVF_RET_ERROR;
        }
    } while (0);

    return ret;
}
/**
 * \brief   Block protection unlock for the 010A
 * \return  Result of operation
 * \retval  SST2xVF_RET_OK: success
 * \retval  SST2xVF_RET_ERROR: error
 */
static SST2xVF_RET SST25VF010A_FullMemoryUnlock(void)
{
    SST2xVF_RET ret = SST2xVF_RET_ERROR;

    do
    {
        uint8_t status_register;

        ret = SST2xVF_ReadStatusRegister(&status_register);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        status_register &= ~(SST25VF010_SR_BP0 | SST25VF010_SR_BP1);
        ret = SST2xVF_WriteStatusDriver(status_register);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        ret = SST2xVF_ReadStatusRegister(&status_register);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        if (status_register & (SST25VF010_SR_BP0 | SST25VF010_SR_BP1))
        {
            ret = SST2xVF_RET_ERROR;
        }

    } while (0);

    return ret;
}

/**
 * \brief   Block protection unlock for the 064C
 * \return  Result of operation
 * \retval  SST2xVF_RET_OK: success
 * \retval  SST2xVF_RET_ERROR: error
 */
static SST2xVF_RET SST25VF064C_FullMemoryUnlock(void)
{
    SST2xVF_RET ret = SST2xVF_RET_ERROR;

    do
    {
        uint8_t status_register;

        ret = SST2xVF_ReadStatusRegister(&status_register);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        status_register &= ~(SST25VF064C_SR_BP0 | SST25VF064C_SR_BP1 | SST25VF064C_SR_BP2 | SST25VF064C_SR_BP3);
        ret = SST2xVF_WriteStatusDriver(status_register);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        ret = SST2xVF_ReadStatusRegister(&status_register);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        if (status_register & (SST25VF064C_SR_BP0 | SST25VF064C_SR_BP1 | SST25VF064C_SR_BP2 | SST25VF064C_SR_BP3))
        {
            ret = SST2xVF_RET_ERROR;
        }
    } while (0);

    return ret;
}
/**
 * \brief   Block protection unlock for the 064B
 * \return  Result of operation
 * \retval  SST2xVF_RET_OK: success
 * \retval  SST2xVF_RET_ERROR: error
 */
static SST2xVF_RET SST26VF064B_FullMemoryUnlock(void)
{
    SST2xVF_RET ret = SST2xVF_RET_ERROR;

    do
    {
        uint8_t block_registers[18] = {0};

        ret = SST26VF064B_ReadBlockProtectionRegister(block_registers);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        ret = SST26VF064B_WriteBlockProtectionRegister(block_registers);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        ret = SST26VF064B_ReadBlockProtectionRegister(block_registers);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        uint8_t block_regs_reference[18] = {0};
        if (memcmp(block_registers, block_regs_reference, sizeof(block_registers)))
        {
            ret = SST2xVF_RET_ERROR;
        }
    } while (0);

    return ret;
}

/**
 * \brief   	Read configuration register
 * \param[out]  *man_id - Pointer to the manufacturer id return value
 * \param[out]  *dev_id - Pointer to the device id return value
 * \return      Result of operation
 * \retval      SST2xVF_RET_OK: success
 * \retval      SST2xVF_RET_ERROR: error
 */
static SST2xVF_RET SST2xVF_ReadID(uint8_t *man_id, uint8_t *dev_id)
{
    SST2xVF_RET ret = SST2xVF_RET_ERROR;

    do
    {
        ret = SST2xVF_WaitBusy(SSTxVF_DRIVER_BUSY_TIMEOUT);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        uint8_t data[4] = {SST2xVF_READ_ID, 0, 0, 0};
        SST2xVF_ChipEnable(CE_ENABLE);
        ret = SST2xVF_WriteSPI(data, 4);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        ret = SST2xVF_ReadSPI(man_id);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        ret = SST2xVF_ReadSPI(dev_id);

    } while (0);

    SST2xVF_ChipEnable(CE_DISABLE);

    return ret;
}
/**
 * \brief   	Read configuration register
 * \param[out]  *man_id - Pointer to the manufacturer id return value
 * \param[out]  *dev_id - Pointer to the device id return value
 * \return      Result of operation
 * \retval      SST2xVF_RET_OK: success
 * \retval      SST2xVF_RET_ERROR: error
 */
static SST2xVF_RET SST2xVF_ReadJEDEC_ID(uint8_t *man_id, uint8_t *dev_id)
{
    SST2xVF_RET ret = SST2xVF_RET_ERROR;

    do
    {
        ret = SST2xVF_WaitBusy(SSTxVF_DRIVER_BUSY_TIMEOUT);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }

        SST2xVF_ChipEnable(CE_ENABLE);
        ret = SST2xVF_SendInstruction(SST2xVF_JEDEC_ID);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        ret = SST2xVF_ReadSPI(man_id);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        uint8_t dev_type;
        ret = SST2xVF_ReadSPI(&dev_type);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        ret = SST2xVF_ReadSPI(dev_id);

    } while (0);

    SST2xVF_ChipEnable(CE_DISABLE);

    return ret;
}
/**
 * \brief   Initialize the device and unluck all blocks
 * \return  Result of operation
 * \retval  SST2xVF_RET_OK : Successful initialization
 * \retval  SST2xVF_RET_ERROR : error on initialization
 */
SST2xVF_RET SST2xVF_Initialize(void)
{
    SST2xVF_RET ret         = SST2xVF_RET_NOT_INIT;
    bool        mutex_taken = false;

    do
    {
        if (FlashInfo.initialized == true)
        {
            ret = SST2xVF_RET_OK;
            break;
        }
        ret = SST2xVF_MutexTake();
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        mutex_taken = true;

        uint8_t dev_id = 0xFF;
        uint8_t man_id = 0xFF;
        ret            = SST2xVF_ReadID(&man_id, &dev_id);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        if (man_id != 0xBF)
        {
            ret = SST2xVF_ReadJEDEC_ID(&man_id, &dev_id);
        }
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }

        if (dev_id == 0x49)
        {
            FlashInfo.model           = SST25VF010A; /**<model*/
            FlashInfo.size            = 0x20000;     /**<size: 128 Kbytes - 131072 */
            FlashInfo.sector_count    = 32;          /**<sector_count*/
            FlashInfo.sector_size     = 0x1000;      /**<sector_size: 4 kBytes*/
            FlashInfo.page_size       = 1;           /**<page_size: no page programming, byte only*/
            FlashInfo.block_count     = 4;           /**<block_count*/
            FlashInfo.block_size      = 0x8000;      /**<block_size: 32 kBytes*/
            FlashInfo.program_unit    = 1;           /**<program_unit*/
            FlashInfo.erased_value    = 0xFF;        /**<erased_value*/
            FlashInfo.man_id          = man_id;      /**<man_id: SST ID*/
            FlashInfo.dev_type        = 0;           /**<dev_type: no dev_type on this chip*/
            FlashInfo.dev_id          = dev_id;      /**<dev_id: SST25VF010A ID*/
            FlashInfo.initialized     = true;
            SST2xVF_WriteStatusDriver = SST25VF_WriteStatusDriver;
            SST2xVF_FullMemoryLock    = SST25VF010A_FullMemoryLock;
            SST2xVF_FullMemoryUnlock  = SST25VF010A_FullMemoryUnlock;
        }
        else if (dev_id == 0x4B)
        {
            FlashInfo.model           = SST25VF064C; /**<model*/
            FlashInfo.size            = 0x800000;    /**<size: 8 Mbytes - 8388608 */
            FlashInfo.sector_count    = 2048;        /**<sector_count*/
            FlashInfo.sector_size     = 0x1000;      /**<sector_size: 4 kBytes*/
            FlashInfo.page_size       = 256;         /**<page_size: 256 bytes*/
            FlashInfo.block_count     = 256;         /**<block_count*/
            FlashInfo.block_size      = 0x8000;      /**<block_size: 32 kBytes*/
            FlashInfo.program_unit    = 256;         /**<program_unit*/
            FlashInfo.erased_value    = 0xFF;        /**<erased_value*/
            FlashInfo.man_id          = man_id;      /**<man_id: SST ID*/
            FlashInfo.dev_type        = 0x25;        /**<dev_type: 0x25*/
            FlashInfo.dev_id          = dev_id;      /**<dev_id: SST25VF064C ID*/
            FlashInfo.initialized     = true;
            SST2xVF_WriteStatusDriver = SST25VF_WriteStatusDriver;
            SST2xVF_FullMemoryLock    = SST25VF064C_FullMemoryLock;
            SST2xVF_FullMemoryUnlock  = SST25VF064C_FullMemoryUnlock;
        }
        else if (dev_id == 0x43)
        {
            FlashInfo.model           = SST26VF064B; /**<model*/
            FlashInfo.size            = 0x800000;    /**<size: 8 Mbytes - 8388608 */
            FlashInfo.sector_count    = 2048;        /**<sector_count*/
            FlashInfo.sector_size     = 0x1000;      /**<sector_size: 4 kBytes*/
            FlashInfo.page_size       = 256;         /**<page_size: 256 bytes*/
            FlashInfo.block_count     = 256;         /**<block_count*/
            FlashInfo.block_size      = 0x8000;      /**<block_size: 32 kBytes*/
            FlashInfo.program_unit    = 256;         /**<program_unit*/
            FlashInfo.erased_value    = 0xFF;        /**<erased_value*/
            FlashInfo.man_id          = man_id;      /**<man_id: SST ID*/
            FlashInfo.dev_type        = 0x26;        /**<dev_type: 0x26*/
            FlashInfo.dev_id          = dev_id;      /**<dev_id: SST26VF064B ID*/
            FlashInfo.initialized     = true;
            SST2xVF_WriteStatusDriver = SST26VF_WriteStatusDriver;
            SST2xVF_FullMemoryLock    = SST26VF064B_FullMemoryLock;
            SST2xVF_FullMemoryUnlock  = SST26VF064B_FullMemoryUnlock;
        }
        else
        {
            ret = SST2xVF_RET_ERROR;
            break;
        }
        ret = SST2xVF_FullMemoryUnlock();
    } while (0);

    if (mutex_taken)
    {
        ret = SST2xVF_MutexGive();
    }

    return ret;
}
/**
 * \brief   Unitialize the device
 * \return  Result of operation
 * \retval  SST2xVF_RET_OK : Successful uninitialization
 * \retval  SST2xVF_RET_ERROR : error on uninitialization
 */
SST2xVF_RET SST2xVF_Uninitialize(void)
{
    SST2xVF_RET ret         = SST2xVF_RET_NOT_INIT;
    bool        mutex_taken = false;

    do
    {
        if (FlashInfo.initialized == false)
        {
            break;
        }
        ret = SST2xVF_MutexTake();
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        mutex_taken = true;
        memset(&FlashInfo, 0, sizeof(SST2xVF_INFO_t));
        SST2xVF_WriteStatusDriver = NULL;
        SST2xVF_FullMemoryLock    = NULL;
        SST2xVF_FullMemoryUnlock  = NULL;

        ret = SST2xVF_RET_OK;

    } while (0);

    if (mutex_taken)
    {
        ret = SST2xVF_MutexGive();
    }

    return ret;
}
/**
 * \brief   	Erase sector address 4KBytes
 * \param[in]   address - Sector address (0 ~ (FlashInfo.size - 1))
 * \retval  	SST2xVF_RET_OK: success
 * \retval  	SST2xVF_RET_ERROR: error
 * \retval  	SST2xVF_RET_INVALID_ADDRESS: invalid address
 */
SST2xVF_RET SST2xVF_EraseSector(uint32_t address)
{
    SST2xVF_RET ret         = SST2xVF_RET_NOT_INIT;
    bool        mutex_taken = false;

    do
    {
        if (FlashInfo.initialized == false)
        {
            break;
        }
        if (address > (FlashInfo.size - 1))
        {
            ret = SST2xVF_RET_INVALID_ADDRESS;
            break;
        }
        ret = SST2xVF_MutexTake();
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        mutex_taken = true;
        ret         = SST2xVF_WriteEnable();
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        SST2xVF_ChipEnable(CE_ENABLE);
        ret = SST2xVF_SendInstruction(SST2xVF_SE);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }

        uint8_t addr[3];
        // Address value
        addr[0] = (uint8_t)((address >> 16) & 0x000000FF);
        addr[1] = (uint8_t)((address >> 8) & 0x000000FF);
        addr[2] = (uint8_t)(address & 0x000000FF);
        ret     = SST2xVF_WriteSPI(addr, 3);

    } while (0);
    SST2xVF_ChipEnable(CE_DISABLE);

    if (mutex_taken)
    {
        ret = SST2xVF_MutexGive();
    }

    return ret;
}
/**
 * \brief   	Erase block sizes can be 32 KByte
 * \param[in]   address - Block address (0 ~ (FlashInfo.size - 1))
 * \return      Result of operation
 * \retval  	SST2xVF_RET_OK: success
 * \retval  	SST2xVF_RET_ERROR: error
 * \retval  	SST2xVF_RET_INVALID_ADDRESS: invalid address
 */
SST2xVF_RET SST2xVF_EraseBlock(uint32_t address)
{
    SST2xVF_RET ret         = SST2xVF_RET_NOT_INIT;
    bool        mutex_taken = false;

    do
    {
        if (FlashInfo.initialized == false)
        {
            break;
        }
        if (address > (FlashInfo.size - 1))
        {
            ret = SST2xVF_RET_INVALID_ADDRESS;
            break;
        }
        ret = SST2xVF_MutexTake();
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        mutex_taken = true;

        ret = SST2xVF_WriteEnable();
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        SST2xVF_ChipEnable(CE_ENABLE);
        ret = SST2xVF_SendInstruction(SST2xVF_BE);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        uint8_t addr[3];
        // Address value
        addr[0] = (uint8_t)((address >> 16) & 0x000000FF);
        addr[1] = (uint8_t)((address >> 8) & 0x000000FF);
        addr[2] = (uint8_t)(address & 0x000000FF);
        ret     = SST2xVF_WriteSPI(addr, 3);

    } while (0);

    SST2xVF_ChipEnable(CE_DISABLE);
    if (mutex_taken)
    {
        ret = SST2xVF_MutexGive();
    }

    return ret;
}
/**
 * \brief   Erase full chip
 * \return  Result of operation
 * \retval  SST2xVF_RET_OK: success
 * \retval  SST2xVF_RET_ERROR: error
 */
SST2xVF_RET SST2xVF_EraseChip(void)
{
    SST2xVF_RET ret         = SST2xVF_RET_NOT_INIT;
    bool        mutex_taken = false;

    do
    {
        if (FlashInfo.initialized == false)
        {
            break;
        }
        ret = SST2xVF_MutexTake();
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        mutex_taken = true;
        ret         = SST2xVF_WriteEnable();
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        SST2xVF_ChipEnable(CE_ENABLE);
        ret = SST2xVF_SendInstruction(SST2xVF_CE);
        SST2xVF_ChipEnable(CE_DISABLE);

    } while (0);

    if (mutex_taken)
    {
        ret = SST2xVF_MutexGive();
    }

    return ret;
}

/**
 * \brief   	Read N bytes from memory address
 * \param[in]   address - Address to read (0 ~ (FlashInfo.size - 1))
 * \param[out]  *buffer - Pointer to buffer to write data bytes read from memory
 * \param[in]   length - Address to read (1 ~ (FlashInfo.size - address))
 * \return      Result of operation
 * \retval  	SST2xVF_RET_OK: success
 * \retval  	SST2xVF_RET_ERROR: error
 * \retval  	SST2xVF_RET_INVALID_ADDRESS: invalid address
 */
SST2xVF_RET SST2xVF_ReadData(uint32_t address, uint8_t *buffer, uint16_t length)
{
    SST2xVF_RET ret         = SST2xVF_RET_NOT_INIT;
    bool        mutex_taken = false;

    do
    {
        if (FlashInfo.initialized == false)
        {
            break;
        }
        if ((length == 0) || (length > (FlashInfo.size - address)))
        {
            ret = SST2xVF_RET_ERROR;
            break;
        }
        if (address > (FlashInfo.size - 1))
        {
            ret = SST2xVF_RET_INVALID_ADDRESS;
            break;
        }
        ret = SST2xVF_MutexTake();
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        mutex_taken = true;
        ret         = SST2xVF_WaitBusy(SSTxVF_DRIVER_BUSY_TIMEOUT);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        SST2xVF_ChipEnable(CE_ENABLE);
        ret = SST2xVF_SendInstruction(SST2xVF_READ);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        uint8_t addr[3];
        // Address value
        addr[0] = (uint8_t)((address >> 16) & 0x000000FF);
        addr[1] = (uint8_t)((address >> 8) & 0x000000FF);
        addr[2] = (uint8_t)(address & 0x000000FF);
        ret     = SST2xVF_WriteSPI(addr, 3);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        uint16_t i;
        for (i = 0; i < length; i++)
        {
            ret = SST2xVF_ReadSPI(&buffer[i]);
            if (ret != SST2xVF_RET_OK)
                break;
        }
        if (i != length)
        {
            ret = SST2xVF_RET_WARNING_READ_SIZE;
        }

    } while (0);

    SST2xVF_ChipEnable(CE_DISABLE);
    if (mutex_taken)
    {
        ret = SST2xVF_MutexGive();
    }

    return ret;
}
/**
 * \brief   	Write 1 byte in memory address
 * \param[in]   address - Address to write (0 ~ (FlashInfo.size - 1))
 * \param[in]   byte - Byte to write in memory
 * \return      Result of operation
 * \retval  	SST2xVF_RET_OK: success
 * \retval  	SST2xVF_RET_ERROR: error
 * \retval  	SST2xVF_RET_INVALID_ADDRESS: invalid address
 */
SST2xVF_RET SST2xVF_ProgramByte(uint32_t address, uint8_t byte)
{
    SST2xVF_RET ret         = SST2xVF_RET_NOT_INIT;
    bool        mutex_taken = false;

    do
    {
        if (FlashInfo.initialized == false)
        {
            break;
        }
        if (address > (FlashInfo.size - 1))
        {
            ret = SST2xVF_RET_INVALID_ADDRESS;
            break;
        }
        ret = SST2xVF_MutexTake();
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        mutex_taken = true;
        ret         = SST2xVF_WriteEnable();
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        SST2xVF_ChipEnable(CE_ENABLE);
        ret = SST2xVF_SendInstruction(SST2xVF_PP);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        uint8_t addr[3];
        // Address value
        addr[0] = (uint8_t)((address >> 16) & 0x000000FF);
        addr[1] = (uint8_t)((address >> 8) & 0x000000FF);
        addr[2] = (uint8_t)(address & 0x000000FF);
        ret     = SST2xVF_WriteSPI(addr, 3);
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        ret = SST2xVF_WriteSPI(&byte, 1);
    } while (0);

    SST2xVF_ChipEnable(CE_DISABLE);
    if (mutex_taken)
    {
        ret = SST2xVF_MutexGive();
    }

    return ret;
}
/**
 * \brief   	Write N bytes from memory address
 * \param[in]   address - Address to write (0 ~ (FlashInfo.size - 1))
 * \param[in]   *buffer - Pointer to buffer to write in memory
 * \param[in]   length - Address to write (1 ~ (FlashInfo.size - address))
 * \return      Result of operation
 * \retval  	SST2xVF_RET_OK: success
 * \retval  	SST2xVF_RET_ERROR: error
 * \retval  	SST2xVF_RET_INVALID_ADDRESS: invalid address
 */
SST2xVF_RET SST2xVF_ProgramData(uint32_t address, uint8_t *buffer, uint16_t length)
{
    SST2xVF_RET ret         = SST2xVF_RET_NOT_INIT;
    bool        mutex_taken = false;

    do
    {
        if (FlashInfo.initialized == false)
        {
            break;
        }
        if ((length == 0) || (length > (FlashInfo.size - address)))
        {
            ret = SST2xVF_RET_ERROR;
            break;
        }
        if (address > (FlashInfo.size - 1))
        {
            ret = SST2xVF_RET_INVALID_ADDRESS;
            break;
        }
        ret = SST2xVF_MutexTake();
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        mutex_taken = true;
        ret         = SST2xVF_WriteEnable();
        if (ret != SST2xVF_RET_OK)
        {
            break;
        }
        if (FlashInfo.program_unit == 1)
        {
            uint16_t i;
            SST2xVF_ChipEnable(CE_ENABLE);
            ret = SST2xVF_SendInstruction(SST2xVF_AAIP);
            if (ret != SST2xVF_RET_OK)
            {
                break;
            }
            uint8_t addr[3];
            // Address value
            addr[0] = (uint8_t)((address >> 16) & 0x000000FF);
            addr[1] = (uint8_t)((address >> 8) & 0x000000FF);
            addr[2] = (uint8_t)(address & 0x000000FF);

            SST2xVF_WriteSPI(addr, 3);
            SST2xVF_WriteSPI(&buffer[0], 1);
            SST2xVF_ChipEnable(CE_DISABLE);

            for (i = 1; i < length; i++)
            {
                SST2xVF_DelayUs(SSTxVF_DRIVER_BYTE_PROGRAM_TIME);
                SST2xVF_ChipEnable(CE_ENABLE);

                ret = SST2xVF_SendInstruction(SST2xVF_AAIP);
                if (ret != SST2xVF_RET_OK)
                    break;

                ret = SST2xVF_WriteSPI(&buffer[i], 1);
                if (ret != SST2xVF_RET_OK)
                    break;

                SST2xVF_ChipEnable(CE_DISABLE);
            }
        }
        else
        {
            uint32_t current_address = address;
            uint16_t number_of_cmds  = length / (FlashInfo.program_unit);
            if (length % (FlashInfo.program_unit))
                number_of_cmds++;
            uint16_t remaining_length = length;
            uint16_t page_length;
            uint8_t *current_buffer = buffer;
            for (uint16_t i = 0; i < number_of_cmds; i++)
            {
                if (remaining_length < FlashInfo.program_unit)
                {
                    page_length = remaining_length;
                }
                else
                {
                    page_length = FlashInfo.program_unit;
                }
                SST2xVF_ChipEnable(CE_ENABLE);
                ret = SST2xVF_SendInstruction(SST2xVF_PP);
                if (ret != SST2xVF_RET_OK)
                {
                    break;
                }
                uint8_t addr[3];
                // Address value
                addr[0] = (uint8_t)((current_address >> 16) & 0x000000FF);
                addr[1] = (uint8_t)((current_address >> 8) & 0x000000FF);
                addr[2] = (uint8_t)(current_address & 0x000000FF);

                ret = SST2xVF_WriteSPI(addr, 3);
                if (ret != SST2xVF_RET_OK)
                {
                    break;
                }

                ret = SST2xVF_WriteSPI(current_buffer, page_length);
                if (ret != SST2xVF_RET_OK)
                {
                    break;
                }
                SST2xVF_ChipEnable(CE_DISABLE);
                current_buffer += page_length;
                remaining_length -= page_length;
                current_address += page_length;
                if (remaining_length)
                    SST2xVF_WriteEnable();
            }
        }
    } while (0);

    SST2xVF_ChipEnable(CE_DISABLE);
    SST2xVF_WriteDisable();
    if (mutex_taken)
    {
        ret = SST2xVF_MutexGive();
    }

    return ret;
}

/**
 * \brief   	Returns information about flash
 * \return      Pointer to SST2xVF_INFO_t structure with flash information
 */
const SST2xVF_INFO_t *SST2xVF_GetInfo(void)
{
    return (const SST2xVF_INFO_t *)&FlashInfo;
}

/*!
 *  \brief      Function to get the current value of a tick variable (uint32_t) that is incremented every 1 ms
 *  \return     Tick value
 *  \warning    Stronger function must be declared externally
 */
__weak uint32_t SST2xVF_GetTick(void)
{
    return 1;
}
/**
 * \brief   	Chip select control pin (EC) SPI
 * \param[in]   state: CE_ENABLE | CE_DISABLE
 * \warning     Stronger function must be declared externally
 */
__weak void SST2xVF_ChipEnable(SST2xVF_ChipEnable_e state)
{
}
/**
 * \brief   	Write data in SPI
 * \param[in]   data: data to write
 * \param[in]   size: data size
 * \return      Result of operation
 * \retval  	SST2xVF_RET_OK: success
 * \retval  	SST2xVF_RET_ERROR: error
 * \warning     Stronger function must be declared externally
 */
__weak SST2xVF_RET SST2xVF_WriteSPI(uint8_t *data, uint32_t size)
{
    return SST2xVF_RET_ERROR;
}
/**
 * \brief       Read data in SPI
 * \param[in]   data - data read
 * \return      Result of operation
 * \retval  	SST2xVF_RET_OK: success
 * \retval      SST2xVF_RET_ERROR: error
 * \warning     Stronger function must be declared externally
 */
__weak SST2xVF_RET SST2xVF_ReadSPI(uint8_t *data)
{
    return SST2xVF_RET_ERROR;
}
/**
 * \brief       Delay for a time (in us)
 * \param[in]   delay: time to wait in us
 * \warning     Stronger function must be declared externally
 */
__weak void SST2xVF_DelayUs(uint32_t delay)
{
}

/*!
 * \brief       Weakly defined function to take the mutex
 * \return      Result of the operation \ref SST2xVF_RET
 */
__weak SST2xVF_RET SST2xVF_MutexTake(void)
{
    return SST2xVF_RET_OK;
}

/*!
 * \brief       Weakly defined function to give the mutex
 * \return      Result of the operation \ref SST2xVF_RET
 */
__weak SST2xVF_RET SST2xVF_MutexGive(void)
{
    return SST2xVF_RET_OK;
}
