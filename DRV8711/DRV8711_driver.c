/***********************************************************************
 * $Id$     DRV8711_driver.c         28 de mar de 2021
 *//**
 * \file    DRV8711_driver.c 
 * \brief   Source file of the DRV8711_driver.c
 * \version	1.0
 * \date    28 de mar de 2021
 * \author  Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 *************************************************************************/
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "DRV8711_driver.h"
#include "DRV8711_regs.h"

#warning "TO DO: Create check status and clear error functions. Test static allocation. Also deinit one day...probably...maybe."
#define DRV8711_DINAMIC_MEM

/** @addtogroup DRV8711DriverWeak DRV8711 Driver Weak
 * @{
 */

__attribute__((weak)) DRV8711Ret_e drv8711_transfer_spi(drv8711_instance_t driver_ins, uint8_t *buffer_write, uint8_t *buffer_read,
                                                        uint32_t write_size, uint32_t read_size);
__attribute__((weak)) void         drv8711_chip_enable(drv8711_instance_t driver_ins, DRV8711ChipEnable_e state);
__attribute__((weak)) DRV8711Ret_e drv8711_low_lvl_config(drv8711_instance_t driver_ins);
__attribute__((weak)) void *       drv8711_malloc(uint32_t size);
__attribute__((weak)) void         drv8711_free(void *ptr);
__attribute__((weak)) DRV8711Ret_e drv8711_mutex_take(void);
__attribute__((weak)) DRV8711Ret_e drv8711_mutex_give(void);

/** @}*/ // end of Weak group

/** @addtogroup DRV8711DriverPrivate DRV8711 Driver Private
 * @{
 */

#define DRV8711_READ_OP  (1 << 15) /**<Read bit in 16bit SPI*/
#define DRV8711_WRITE_OP (0 << 15) /**<Write bit in 16bit SPI*/

/** Addresses of the DRV8711 registers*/
typedef enum
{
    DRV8711_CTRL_REG   = 0x00,
    DRV8711_TORQUE_REG = 0x01,
    DRV8711_OFF_REG    = 0x02,
    DRV8711_BLANK_REG  = 0x03,
    DRV8711_DECAY_REG  = 0x04,
    DRV8711_STALL_REG  = 0x05,
    DRV8711_DRIVE_REG  = 0x06,
    DRV8711_STAT_REG   = 0x07
} drv8711_register_e;

/** Structure with all registers on the DRV8711*/
typedef struct
{
    CtrlReg_u   ctrl_reg;
    TorqueReg_u torque_reg;
    OffReg_u    off_reg;
    BlankReg_u  blank_reg;
    DecayReg_u  decay_reg;
    StallReg_u  stall_reg;
    DriveReg_u  drive_reg;
    StatusReg_u status_reg;

} drv8711_registers_t;

/** DRV8711 instance control structure, that also serves as a list node*/
typedef struct drv8711
{
    struct drv8711 *    self;     /*!<Pointer to self for sanity checking*/
    drv8711_registers_t regs;     /*!<Driver registers \ref drv8711_registers_t*/
    DRV8711Config_t     config;   /*!<Configuration \ref DRV8711Config_t*/
    struct drv8711 *    next_drv; /**<Pointer to next instance*/
} DRV8711_LIST_T;

DRV8711_LIST_T *DRV_INSTANCES = NULL; /**<Head of the DRV8711 instance list*/

#ifndef DRV8711_DINAMIC_MEM
#define NUM_INSTANCES 4
uint8_t DRV8711_STATIC_MEM[NUM_INSTANCES * sizeof(DRV8711_LIST_T)]; /**<Static memory dedicated to creation of DRV8711 instances */
uint8   drv8711_instaces_created = 0;                               /**<Counter of static DRV8711 instances created so far*/
#endif

static DRV8711Ret_e drv8711_create_instance(drv8711_instance_t *driver_ins, DRV8711Config_t *default_config);
static DRV8711Ret_e drv8711_read_register(drv8711_instance_t driver_ins, drv8711_register_e reg, uint16_t *value);
static DRV8711Ret_e drv8711_write_register(drv8711_instance_t driver_ins, drv8711_register_e reg, uint16_t value);

/** @}*/

/*!
 * \brief       Function that initializes a driver instance
 * \param[out]  driver_ins: Pointer to a instance to be allocated and initialized
 * \retval      DRV8711_RET_OK: Successfully created
 * \retval      DRV8711_RET_MEM_ERR: Shortage of memory
 * \retval      DRV8711_RET_SPI_ERR: Low level init error
 * \note        This function also returns true if driver is already initialized.
 */
DRV8711Ret_e DRV8711Init(drv8711_instance_t *p_driver_ins, DRV8711Config_t *default_config)
{
    DRV8711Ret_e ret = DRV8711_RET_OK;

    do
    {
        if (drv8711_create_instance(p_driver_ins, default_config) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_MEM_ERR;
            break;
        }
        if (drv8711_low_lvl_config(*p_driver_ins) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_CFG_ERR;
            break;
        }
        if (drv8711_load_default_config(*p_driver_ins) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_LOAD_ERR;
            break;
        }

    } while (0);

    return ret;
}

/*!
 * \brief       Function that enables the driver
 * \param[in]   driver_ins: Driver to be enabled
 * \retval      DRV8711_RET_OK: Successfully enabled
 * \retval      DRV8711_RET_NOT_INIT_ERR: Driver not initialized
 * \retval      DRV8711_RET_SPI_ERR: SPI write error
 */
DRV8711Ret_e drv8711_en_motor(drv8711_instance_t driver_ins, bool verify)
{
    DRV8711Ret_e    ret         = DRV8711_RET_OK;
    DRV8711_LIST_T *this_driver = driver_ins;

    if ((this_driver) && (this_driver->self == this_driver))
    {
        this_driver->regs.ctrl_reg.bit_val.ENBL = true;

        ret = drv8711_write_register(driver_ins, DRV8711_CTRL_REG, this_driver->regs.ctrl_reg.val);
        if (verify)
        {
            uint16_t reg_read = 0;
            ret               = drv8711_read_register(driver_ins, DRV8711_CTRL_REG, &reg_read);
            if (ret == DRV8711_RET_OK)
            {
                if (reg_read != this_driver->regs.ctrl_reg.val)
                {
                    ret = DRV8711_RET_SPI_ERR;
                }
            }
        }
    }
    else
        ret = DRV8711_RET_NOT_INIT_ERR;

    return ret;
}

/*!
 * \brief       Function that disables the driver
 * \param[in]   driver_ins: Driver to be disabled
 * \retval      DRV8711_RET_OK: Successfully disabled
 * \retval      DRV8711_RET_NOT_INIT_ERR: Driver not initialized
 * \retval      DRV8711_RET_SPI_ERR: SPI write error
 */
DRV8711Ret_e drv8711_disable_motor(drv8711_instance_t driver_ins, bool verify)
{
    DRV8711Ret_e    ret         = DRV8711_RET_OK;
    DRV8711_LIST_T *this_driver = driver_ins;

    if ((this_driver) && (this_driver->self == this_driver))
    {
        this_driver->regs.ctrl_reg.bit_val.ENBL = false;

        ret = drv8711_write_register(driver_ins, DRV8711_CTRL_REG, this_driver->regs.ctrl_reg.val);
        if (verify)
        {
            uint16_t reg_read = 0;
            ret               = drv8711_read_register(this_driver, DRV8711_CTRL_REG, &reg_read);
            if (ret == DRV8711_RET_OK)
            {
                if (reg_read != this_driver->regs.ctrl_reg.val)
                {
                    ret = DRV8711_RET_SPI_ERR;
                }
            }
        }
    }
    else
        ret = DRV8711_RET_NOT_INIT_ERR;

    return ret;
}

/*!
 * \brief       Function that sets the mode of the driver
 * \param[in]   driver_ins: Driver to be disabled
 * \retval      DRV8711_RET_OK: Successfully disabled
 * \retval      DRV8711_RET_NOT_INIT_ERR: Driver not initialized
 * \retval      DRV8711_RET_SPI_ERR: SPI write error
 */
DRV8711Ret_e drv8711_set_mode(drv8711_instance_t driver_ins, drv8711_mode_e motor_mode)
{
    DRV8711Ret_e    ret         = DRV8711_RET_OK;
    DRV8711_LIST_T *this_driver = driver_ins;

    if ((this_driver) && (this_driver->self == this_driver))
    {
        if (motor_mode != DRV8711_MODE_INDEF)
        {
            this_driver->regs.ctrl_reg.bit_val.MODE = motor_mode;

            ret = drv8711_write_register(driver_ins, DRV8711_CTRL_REG, this_driver->regs.ctrl_reg.val);
        }
    }
    else
        ret = DRV8711_RET_NOT_INIT_ERR;

    return ret;
}

/*!
 * \brief       Function that checks if the driver is enabled
 * \param[in]   driver_ins: Driver to check
 * \retval      true: Driver enabled
 * \retval      false: Driver not enabled or inexistent
 */
bool drv8711_check_enabled(drv8711_instance_t driver_ins)
{
    DRV8711_LIST_T *this_driver = driver_ins;
    if ((this_driver) && (this_driver->self == this_driver))
    {
        return ((DRV8711_LIST_T *)driver_ins)->regs.ctrl_reg.bit_val.ENBL;
    }
    else
        return false;
}

/*!
 * \brief       Function that returns the driver mode
 * \param[in]   driver_ins: Driver to check
 * \return      motor_mode: \ref drv8711_mode_e
 */
drv8711_mode_e drv8711_get_mode(drv8711_instance_t driver_ins)
{
    DRV8711_LIST_T *this_driver = driver_ins;
    if ((this_driver) && (this_driver->self == this_driver))
    {
        return (drv8711_mode_e)((DRV8711_LIST_T *)driver_ins)->regs.ctrl_reg.bit_val.MODE;
    }
    else
        return DRV8711_MODE_INDEF;
}

/*!
 * \brief       Function that configures the TORQUE value
 * \param[in]   driver_ins: Driver to be configured
 * \param[in]   torque: Desired TORQUE value of the torque register
 * \retval      DRV8711_RET_OK: Successfully configures
 * \retval      DRV8711_RET_NOT_INIT_ERR: Driver not initialized
 * \retval      DRV8711_RET_SPI_ERR: SPI write error
 * \retval      DRV8711_RET_INVALID_PARAM: Invalid parameter
 */
DRV8711Ret_e drv8711_torque_config(drv8711_instance_t driver_ins, uint8_t torque)
{
    DRV8711Ret_e    ret         = DRV8711_RET_OK;
    DRV8711_LIST_T *this_driver = driver_ins;

    if ((this_driver) && (this_driver->self == this_driver))
    {
        if (torque)
        {
            this_driver->regs.torque_reg.bit_val.TORQUE = torque;

            if (drv8711_write_register(this_driver, DRV8711_TORQUE_REG, this_driver->regs.torque_reg.val) != DRV8711_RET_OK)
            {
                ret = DRV8711_RET_SPI_ERR;
            }
        }
        else
            ret = DRV8711_RET_INVALID_PARAM;
    }
    else
        ret = DRV8711_RET_NOT_INIT_ERR;

    return ret;
}

/*!
 * \brief       Function that configures the ISGAIN value
 * \param[in]   driver_ins: Driver to be configured
 * \param[in]   isgain: Desired ISGAIN value of the control register
 * \retval      DRV8711_RET_OK: Successfully configures
 * \retval      DRV8711_RET_NOT_INIT_ERR: Driver not initialized
 * \retval      DRV8711_RET_SPI_ERR: SPI write error
 * \retval      DRV8711_RET_INVALID_PARAM: Invalid parameter
 */
DRV8711Ret_e drv8711_isgain_config(drv8711_instance_t driver_ins, uint8_t isgain)
{
    DRV8711Ret_e    ret         = DRV8711_RET_OK;
    DRV8711_LIST_T *this_driver = driver_ins;

    if ((this_driver) && (this_driver->self == this_driver))
    {
        if (isgain < 4)
        {
            this_driver->regs.ctrl_reg.bit_val.ISGAIN = isgain;

            if (drv8711_write_register(this_driver, DRV8711_CTRL_REG, this_driver->regs.ctrl_reg.val) != DRV8711_RET_OK)
            {
                ret = DRV8711_RET_SPI_ERR;
            }
        }
        else
            ret = DRV8711_RET_INVALID_PARAM;
    }
    else
        ret = DRV8711_RET_NOT_INIT_ERR;

    return ret;
}

/*!
 * \brief       Function that configures the output current of the DRV8711
 * \param[in]   driver_ins: Driver to be configured
 * \param[in]   current_mA: Desired current in mA
 * \retval      DRV8711_RET_OK: Successfully configures
 * \retval      DRV8711_RET_NOT_INIT_ERR: Driver not initialized or Risense not informed
 * \retval      DRV8711_RET_INVALID_PARAM: Invalid parameter
 */
DRV8711Ret_e drv8711_current_config(drv8711_instance_t driver_ins, uint16_t current_mA)
{
    DRV8711Ret_e    ret         = DRV8711_RET_OK;
    DRV8711_LIST_T *this_driver = driver_ins;

    if ((this_driver) && (this_driver->self == this_driver))
    {
        do
        {
            if (this_driver->config.risense_m_ohm == 0)
            {
                ret = DRV8711_RET_NOT_INIT_ERR;
                break;
            }
            uint32_t max_current = (2750000 * 255) / (256 * 5 * this_driver->config.risense_m_ohm);
            uint32_t min_current = (2750000 * 1) / (256 * 40 * this_driver->config.risense_m_ohm);
            if ((current_mA > max_current) || (current_mA < min_current))
            {
                ret = DRV8711_RET_INVALID_PARAM;
                break;
            }
            bool     isgain_found = false;
            uint8_t  isgain_value = 40;
            uint32_t this_current; // = (2750*255)/(256*40*this_driver->risense_m_ohm);
            uint8_t  best_torque = 1;
            uint8_t  best_isgain = 5;
            while (!isgain_found)
            {
                this_current = (2750000 * 255) / (256 * isgain_value * this_driver->config.risense_m_ohm);
                if (isgain_value < 5)
                {
                    isgain_found = true;
                    ret          = DRV8711_RET_INVALID_PARAM;
                }
                else if (current_mA < this_current)
                {
                    best_isgain  = isgain_value;
                    isgain_found = true;
                    best_torque  = (uint8_t)((uint64_t)(isgain_value * this_driver->config.risense_m_ohm * current_mA * 256) / (2750000));
                }
                isgain_value /= 2;
            }

            if (ret == DRV8711_RET_INVALID_PARAM)
            {
                break;
            }
            uint8_t isgain_bit = (best_isgain == 5 ? 0 : (best_isgain == 10 ? 1 : (best_isgain == 20 ? 2 : 3)));
            if (drv8711_isgain_config(driver_ins, isgain_bit) != DRV8711_RET_OK)
            {
                ret = DRV8711_RET_SPI_ERR;
                break;
            }

            if (drv8711_torque_config(driver_ins, best_torque) != DRV8711_RET_OK)
            {
                ret = DRV8711_RET_SPI_ERR;
                break;
            }

        } while (0);
    }
    else
        ret = DRV8711_RET_NOT_INIT_ERR;

    return ret;
}

/*!
 * \brief       Function that informs the Risense resistance used
 * \param[in]   driver_ins: Driver instance
 * \param[in]   risense_m_ohm: Resistor in mOhm used on the driver
 * \retval      DRV8711_RET_OK: Successfully informed
 * \retval      DRV8711_RET_NOT_INIT_ERR: Driver not initialized
 * \retval      DRV8711_RET_INVALID_PARAM: Invalid parameter
 */
DRV8711Ret_e drv8711_inform_risense(drv8711_instance_t driver_ins, uint16_t risense_m_ohm)
{
    DRV8711Ret_e    ret         = DRV8711_RET_OK;
    DRV8711_LIST_T *this_driver = driver_ins;

    if ((this_driver) && (this_driver->self == this_driver))
    {
        if (risense_m_ohm)
        {
            this_driver->config.risense_m_ohm = risense_m_ohm;
        }
        else
            ret = DRV8711_RET_INVALID_PARAM;
    }
    else
        ret = DRV8711_RET_NOT_INIT_ERR;

    return ret;
}

/*!
 * \brief       Load default configs on the DRV8711, clearing errors as well
 * \param[in]   driver_ins: Driver to be written
 * \retval      DRV8711_RET_OK: Successfully read
 * \retval      DRV8711_RET_SPI_ERR: SPI write error
 */
DRV8711Ret_e drv8711_load_default_config(drv8711_instance_t driver_ins)
{
    DRV8711Ret_e    ret         = DRV8711_RET_OK;
    DRV8711_LIST_T *this_driver = driver_ins;

    do
    {
        if ((this_driver == NULL) || (this_driver->self != this_driver))
        {
            break;
        }
        this_driver->regs.ctrl_reg.val             = 0;
        this_driver->regs.ctrl_reg.bit_val.ENBL    = 0;
        this_driver->regs.ctrl_reg.bit_val.RDIR    = 0;
        this_driver->regs.ctrl_reg.bit_val.RSTEP   = 0;
        this_driver->regs.ctrl_reg.bit_val.MODE    = this_driver->config.mode;
        this_driver->regs.ctrl_reg.bit_val.EXSTALL = 0;
        this_driver->regs.ctrl_reg.bit_val.ISGAIN  = 3;
        this_driver->regs.ctrl_reg.bit_val.DTIME   = 3;
        this_driver->regs.ctrl_reg.bit_val.UNUSED  = 0;
        if (drv8711_write_register(driver_ins, DRV8711_CTRL_REG, this_driver->regs.ctrl_reg.val) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }
        uint16_t read_reg = 0;
        if (drv8711_read_register(this_driver, DRV8711_CTRL_REG, &read_reg) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }
        if (read_reg != this_driver->regs.ctrl_reg.val)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        this_driver->regs.torque_reg.val            = 0;
        this_driver->regs.torque_reg.bit_val.TORQUE = 120;
        this_driver->regs.torque_reg.bit_val.SMPLTH = 1;
        this_driver->regs.torque_reg.bit_val.UNUSED = 0;

        if (drv8711_write_register(this_driver, DRV8711_TORQUE_REG, this_driver->regs.torque_reg.val) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        if (drv8711_read_register(this_driver, DRV8711_TORQUE_REG, &read_reg) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        if (read_reg != this_driver->regs.torque_reg.val)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        this_driver->regs.off_reg.val             = 0;
        this_driver->regs.off_reg.bit_val.TOFF    = 0x32;
        this_driver->regs.off_reg.bit_val.PWMMODE = 0;
        this_driver->regs.off_reg.bit_val.UNUSED  = 0;

        if (drv8711_write_register(this_driver, DRV8711_OFF_REG, this_driver->regs.off_reg.val) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        if (drv8711_read_register(this_driver, DRV8711_OFF_REG, &read_reg) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        if (read_reg != this_driver->regs.off_reg.val)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        this_driver->regs.blank_reg.val            = 0;
        this_driver->regs.blank_reg.bit_val.TBLANK = 0x00;
        this_driver->regs.blank_reg.bit_val.ABT    = 1;
        this_driver->regs.blank_reg.bit_val.UNUSED = 0;

        if (drv8711_write_register(this_driver, DRV8711_BLANK_REG, this_driver->regs.blank_reg.val) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        if (drv8711_read_register(this_driver, DRV8711_BLANK_REG, &read_reg) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        if (read_reg != this_driver->regs.blank_reg.val)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        this_driver->regs.decay_reg.val            = 0;
        this_driver->regs.decay_reg.bit_val.TDECAY = 0x10;
        this_driver->regs.decay_reg.bit_val.DECMOD = 5;
        this_driver->regs.decay_reg.bit_val.UNUSED = 0;

        if (drv8711_write_register(this_driver, DRV8711_DECAY_REG, this_driver->regs.decay_reg.val) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        if (drv8711_read_register(this_driver, DRV8711_DECAY_REG, &read_reg) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        if (read_reg != this_driver->regs.decay_reg.val)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        this_driver->regs.stall_reg.val            = 0;
        this_driver->regs.stall_reg.bit_val.SDTHR  = 0x14;
        this_driver->regs.stall_reg.bit_val.SDCNT  = 1;
        this_driver->regs.stall_reg.bit_val.VDIV   = 2;
        this_driver->regs.stall_reg.bit_val.UNUSED = 0;

        if (drv8711_write_register(this_driver, DRV8711_STALL_REG, this_driver->regs.stall_reg.val) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        if (drv8711_read_register(this_driver, DRV8711_STALL_REG, &read_reg) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        if (read_reg != this_driver->regs.stall_reg.val)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        this_driver->regs.drive_reg.val             = 0;
        this_driver->regs.drive_reg.bit_val.OCPTH   = 0;
        this_driver->regs.drive_reg.bit_val.OCPDEG  = 0;
        this_driver->regs.drive_reg.bit_val.TDRIVEN = 0;
        this_driver->regs.drive_reg.bit_val.TDRIVEP = 0;
        this_driver->regs.drive_reg.bit_val.IDRIVEN = 0;
        this_driver->regs.drive_reg.bit_val.IDRIVEP = 0;
        this_driver->regs.drive_reg.bit_val.UNUSED  = 0;

        if (drv8711_write_register(this_driver, DRV8711_DRIVE_REG, this_driver->regs.drive_reg.val) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        if (drv8711_read_register(this_driver, DRV8711_DRIVE_REG, &read_reg) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        if (read_reg != this_driver->regs.drive_reg.val)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        this_driver->regs.status_reg.val            = 0;
        this_driver->regs.status_reg.bit_val.OTS    = false;
        this_driver->regs.status_reg.bit_val.AOCP   = false;
        this_driver->regs.status_reg.bit_val.BOCP   = false;
        this_driver->regs.status_reg.bit_val.APDF   = false;
        this_driver->regs.status_reg.bit_val.BPDF   = false;
        this_driver->regs.status_reg.bit_val.UVLO   = false;
        this_driver->regs.status_reg.bit_val.STD    = false;
        this_driver->regs.status_reg.bit_val.STDLAT = false;
        this_driver->regs.status_reg.bit_val.UNUSED = 0;

        if (drv8711_write_register(this_driver, DRV8711_STAT_REG, this_driver->regs.status_reg.val) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        if (drv8711_read_register(this_driver, DRV8711_STAT_REG, &read_reg) != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }

        if (read_reg != this_driver->regs.status_reg.val)
        {
            ret = DRV8711_RET_SPI_ERR;
            break;
        }
        if (this_driver->config.risense_m_ohm)
        {
            drv8711_current_config(this_driver, this_driver->config.current_mA);
        }

    } while (0);

    return ret;
}

/*!
 * \brief       Reads a DRV8711 register
 * \param[in]   driver_ins: Driver to be read
 * \param[in]   reg: Register of the DRV8711 \ref drv8711_register_e
 * \param[out]  *value: value read from reg
 * \retval      DRV8711_RET_OK: Successfully read
 * \retval      DRV8711_RET_SPI_ERR: SPI read error
 */
static DRV8711Ret_e drv8711_read_register(drv8711_instance_t driver_ins, drv8711_register_e reg, uint16_t *value)
{
    uint16_t     read_instr;
    uint8_t      read_instr_buff[2];
    uint8_t      read_data[2];
    DRV8711Ret_e ret = DRV8711_RET_OK;
    do
    {
        if (drv8711_mutex_take() != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_MUTEX_TAKE_ERR;
            break;
        }
        drv8711_chip_enable(driver_ins, DRV8711_CE_ENABLE);
        read_instr         = (DRV8711_READ_OP | (reg << 12));
        read_instr_buff[0] = (read_instr >> 8) & 0xff;
        read_instr_buff[1] = (read_instr >> 0) & 0xff;
        ret                = drv8711_transfer_spi(driver_ins, read_instr_buff, read_data, 2, 2);
        drv8711_chip_enable(driver_ins, DRV8711_CE_DISABLE);
        if (drv8711_mutex_give() != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_MUTEX_GIVE;
            break;
        }
        *value = ((read_data[0] << 8) | read_data[1]);
        *value &= 0xfff;

    } while (0);

    return ret;
}

/*!
 * \brief       Write a value into a DRV8711 register
 * \param[in]   driver_ins: Driver to be written
 * \param[in]   reg: Register of the DRV8711 \ref drv8711_register_e
 * \param[in]   value: value to be written in reg
 * \retval      DRV8711_RET_OK: Successfully read
 * \retval      DRV8711_RET_SPI_ERR: SPI write error
 */
static DRV8711Ret_e drv8711_write_register(drv8711_instance_t driver_ins, drv8711_register_e reg, uint16_t value)
{
    uint16_t     read_instr;
    uint8_t      write_buff[2];
    uint8_t      dummy_data[2];
    DRV8711Ret_e ret = DRV8711_RET_OK;

    do
    {

        if (drv8711_mutex_take() != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_MUTEX_TAKE_ERR;
            break;
        }
        drv8711_chip_enable(driver_ins, DRV8711_CE_ENABLE);
        read_instr = (DRV8711_WRITE_OP | (reg << 12));
        read_instr |= value;

        write_buff[0] = (read_instr >> 8) & 0xff;
        write_buff[1] = (read_instr >> 0) & 0xff;
        ret           = drv8711_transfer_spi(driver_ins, write_buff, dummy_data, 2, 0);
        drv8711_chip_enable(driver_ins, DRV8711_CE_DISABLE);
        if (drv8711_mutex_give() != DRV8711_RET_OK)
        {
            ret = DRV8711_RET_MUTEX_GIVE;
            break;
        }
    } while (0);

    return ret;
}

/*!
 * \brief       Create a driver instance
 * \param[out]   *driver_ins: Pointer to future driver instance
 * \retval      true: Successfully created or already existent
 * \retval      false: Can't create due to memory shortage
 */
static DRV8711Ret_e drv8711_create_instance(drv8711_instance_t *p_driver_ins, DRV8711Config_t *default_config)
{
    DRV8711Ret_e ret = DRV8711_RET_MEM_ERR;

    if (DRV_INSTANCES == NULL)
    {
#ifndef DRV8711_DINAMIC_MEM
        DRV_INSTANCES = &DRV8711_STATIC_MEM[0];
        drv8711_instaces_created++;
#else
        DRV_INSTANCES = (DRV8711_LIST_T *)drv8711_malloc(sizeof(DRV8711_LIST_T));
#endif
        if (DRV_INSTANCES)
        {
            *p_driver_ins           = (drv8711_instance_t)DRV_INSTANCES;
            DRV_INSTANCES->self     = (*p_driver_ins);
            DRV_INSTANCES->next_drv = NULL;
            ret                     = DRV8711_RET_OK;
            if (default_config)
            {
                DRV_INSTANCES->config = *default_config;
            }
            else
            {
                DRV_INSTANCES->config.current_mA    = 0;
                DRV_INSTANCES->config.risense_m_ohm = 0;
                DRV_INSTANCES->config.mode          = DRV8711_FULL_STEP;
                DRV_INSTANCES->config.type          = DRV8711_STEP_MOTOR;
            }
        }
    }
    else
    {
        DRV8711_LIST_T *list_head       = DRV_INSTANCES;
        DRV8711_LIST_T *desired_drv_ins = (DRV8711_LIST_T *)(*p_driver_ins);

        while ((list_head->next_drv != NULL) && (list_head != desired_drv_ins)) list_head = list_head->next_drv;

        if (list_head != desired_drv_ins)
        {
#ifndef DRV8711_DINAMIC_MEM
            if (drv8711_instaces_created < NUM_INSTANCES)
            {
                list_head->next_drv = &DRV8711_STATIC_MEM[drv8711_instaces_created * sizeof(DRV8711_LIST_T)];
                drv8711_instaces_created++;
            }
#else
            list_head->next_drv = (DRV8711_LIST_T *)drv8711_malloc(sizeof(DRV8711_LIST_T));
#endif
            if (list_head->next_drv)
            {
                *p_driver_ins                 = (drv8711_instance_t)list_head->next_drv;
                list_head->next_drv->self     = (*p_driver_ins);
                list_head->next_drv->next_drv = NULL;
                ret                           = DRV8711_RET_OK;
                if (default_config)
                {
                    list_head->next_drv->config = *default_config;
                }
                else
                {
                    list_head->next_drv->config.current_mA    = 0;
                    list_head->next_drv->config.risense_m_ohm = 0;
                    list_head->next_drv->config.mode          = DRV8711_FULL_STEP;
                    list_head->next_drv->config.type          = DRV8711_STEP_MOTOR;
                }
            }
        }
        else
        {
            ret = DRV8711_RET_OK;
        }
    }
    return ret;
}

/*!
 * \brief       Weakly defined transfer SPI function
 * \param[in]   driver_ins: Driver instance
 * \param[in]   *buffer_write: send buffer
 * \param[out]  *buffer_read: receive buffer
 * \param[in]   write_size: size of buffer_write
 * \param[in]   read_size: size of buffer_read
 * \retval      DRV8711_RET_OK: Succesfuly transfered
 * \retval      DRV8711_RET_SPI_ERR: Transfer failed
 */
__attribute__((weak)) DRV8711Ret_e drv8711_transfer_spi(drv8711_instance_t driver_ins, uint8_t *buffer_write, uint8_t *buffer_read,
                                                        uint32_t write_size, uint32_t read_size)
{
    // implement your own function in high level layer
    return DRV8711_RET_SPI_ERR;
}

/*!
 * \brief       User defined low level configuration (optional)
 * \param[in]   driver_ins: Driver instance
 * \retval      DRV8711_RET_OK: Configured
 * \retval      DRV8711_RET_CFG_ERR: Not configured
 */
__attribute__((weak)) DRV8711Ret_e drv8711_low_lvl_config(drv8711_instance_t driver_ins)
{
    // implement your own function in high level layer if low level initializing haven't been done already
    return DRV8711_RET_OK;
}

/*!
 * \brief       User defined malloc function (optional)
 * \param[in]   size: Number of bytes to allocate
 * \return      Valid pointer or NULL
 */
__attribute__((weak)) void *drv8711_malloc(uint32_t size)
{
    // implement your own function if you have a memory management module
    return malloc(size);
}

/*!
 * \brief       User defined free function (optional)
 * \param[out]  *ptr: Pointer to free
 */
__attribute__((weak)) void drv8711_free(void *ptr)
{
    // implement you own function
    free(ptr);
}

/*!
 *  \brief      Obtain a mutex
 *  \return     Result of operation
 *  \retval     DRV8711_RET_OK: mutex successfully obtained
 *  \retval     DRV8711_RET_MUTEX_TAKE_ERR: error in obtaining the mutex
 *  \warning    Stronger function must be declared externally
 */
__attribute__((weak)) DRV8711Ret_e drv8711_mutex_take(void)
{
    return DRV8711_RET_OK;
}
/*!
 *  \brief      Release a mutex
 *  \return     Result of operation
 *  \retval     DRV8711_RET_OK: mutex successfully released
 *  \retval     DRV8711_RET_MUTEX_GIVE: mutex release error
 *  \warning    Stronger function must be declared externally
 */
__attribute__((weak)) DRV8711Ret_e drv8711_mutex_give(void)
{
    return DRV8711_RET_OK;
}
/**
 *\brief {Source file of}
 *
 */
__attribute__((weak)) void drv8711_chip_enable(drv8711_instance_t driver_ins, DRV8711ChipEnable_e state)
{
   
}