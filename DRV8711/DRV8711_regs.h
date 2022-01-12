/*!
 * \file       DRV8711_regs.h
 * \brief      Register definitions for the DRV8711 driver
 * \date       2021-03-18
 * \version    1.0
 * \author     Guilherme Frick de Oliveira (frickoliveira.ee@gmail.com)
 * \copyright  Copyright (c) 2021
 */
#ifndef _DRV8711_REGS_H_
#define _DRV8711_REGS_H_

/*!
 * \brief       Control register definition
 */
typedef union CtrlRegisterDefinition
{
    struct
    {
        uint16_t ENBL : 1;    /*!<Enable or disable motor*/
        uint16_t RDIR : 1;    /*!<The logic of the Dir pin (0 for direct and 1 for inverse)*/
        uint16_t RSTEP : 1;   /*!<If 1, Indexer will advance 1 step. Clears after write*/
        uint16_t MODE : 4;    /*!<Mode of the stepping \ref drv8711_mode_e*/
        uint16_t EXSTALL : 1; /*!<External stall detect if 1, else internal detection*/
        uint16_t ISGAIN : 2;  /*!<ISENSE amplifier gain \ref DRV8711IsGain_e */
        uint16_t DTIME : 2;   /*!<Dead time set*/
        uint16_t UNUSED : 4;  /*!<Unused bits*/
    } bit_val;
    uint16_t val; /*!<Unified control register value*/
} CtrlReg_u;

/*!
 * \brief       Torque register definition
 */
typedef union TorqueRegisterDefinition
{
    struct
    {
        uint16_t TORQUE : 8;   /*!<Sets full-scale output current for both H-bridges*/
        uint16_t SMPLTH : 3;   /*!<Back EMF sample threshold*/
        uint16_t RESERVED : 1; /*!<Reserved bit*/
        uint16_t UNUSED : 4;   /*!<Unused bits*/
    } bit_val;
    uint16_t val; /*!<Unified torque register value*/
} TorqueReg_u;

/*!
 * \brief       OFF register definition
 */
typedef union OffRegisterDefinition
{
    struct
    {
        uint16_t TOFF : 8;     /*!<The fixed off time, in increment of 500ns*/
        uint16_t PWMMODE : 1;  /*!<If 1, bypass the internal indexer and use INx directly*/
        uint16_t RESERVED : 3; /*!<Reserved bits*/
        uint16_t UNUSED : 4;   /*!<Unused bits*/
    } bit_val;
    uint16_t val; /*!<Unified OFF register value*/
} OffReg_u;

/*!
 * \brief       Blank register defintion
 */
typedef union BlankRegisterDefinition
{
    struct
    {
        uint16_t TBLANK : 8;   /*!<Trip blanking time and minimum PWM, in increment of 20ns*/
        uint16_t ABT : 1;      /*!<If 1 enable adaptive blanking time*/
        uint16_t RESERVED : 3; /*!<Reserved bits*/
        uint16_t UNUSED : 4;   /*!<Unused bits*/
    } bit_val;
    uint16_t val; /*!<Unified Blank register value*/
} BlankReg_u;

/*!
 * \brief       Decay register definition
 */
typedef union DecayRegisterDefinition
{
    struct
    {
        uint16_t TDECAY : 8;   /*!<Mixed decay transition time, in increments of 500 ns*/
        uint16_t DECMOD : 3;   /*!<Decay mode \ref DRV8711DecayMode_e*/
        uint16_t RESERVED : 1; /*!<Reserved bits*/
        uint16_t UNUSED : 4;   /*!<Unused bits*/
    } bit_val;
    uint16_t val; /*!<Unified Decay register value*/
} DecayReg_u;

/*!
 * \brief       Stall register definition
 */
typedef union StallRegisterDefinition
{
    struct
    {
        uint16_t SDTHR : 8;  /*!<Stall detection threshold */
        uint16_t SDCNT : 2;  /*!<Number of steps until stall detection*/
        uint16_t VDIV : 2;   /*!<Back EMF division*/
        uint16_t UNUSED : 4; /*!<Unused bits*/
    } bit_val;
    uint16_t val; /*!<Unified Stall register value*/
} StallReg_u;

/*!
 * \brief       Drive register definition
 */
typedef union DriveRegisterDefinition
{
    struct
    {
        uint16_t OCPTH : 2;   /*!<OCP threshold*/
        uint16_t OCPDEG : 2;  /*!<OCP deglitch time*/
        uint16_t TDRIVEN : 2; /*!<Low-side gate drive time*/
        uint16_t TDRIVEP : 2; /*!<High-side gate drive time*/
        uint16_t IDRIVEN : 2; /*!<Low-side gate drive peak current*/
        uint16_t IDRIVEP : 2; /*!<High-side gate drive peak current*/
        uint16_t UNUSED : 4;  /*!<Unused bits*/
    } bit_val;
    uint16_t val; /*!<Unified Drive register value*/
} DriveReg_u;

/*!
 * \brief       Status register definition
 */
typedef union StatusRegisterDefinition
{
    struct
    {
        uint16_t OTS : 1;      /*!<If 1, Overtemperature shutdown occurred*/
        uint16_t AOCP : 1;     /*!<If 1, Channel A overcurrent shutdown occurred*/
        uint16_t BOCP : 1;     /*!<If 1, Channel B overcurrent shutdown occurred*/
        uint16_t APDF : 1;     /*!<If 1, Channel A predriver fault occurred*/
        uint16_t BPDF : 1;     /*!<If 1, Channel B predriver fault occurred*/
        uint16_t UVLO : 1;     /*!<If 1, Undervoltage lockout occurred*/
        uint16_t STD : 1;      /*!<If 1, Stall detected*/
        uint16_t STDLAT : 1;   /*!<If 1, Latched stall detected*/
        uint16_t RESERVED : 4; /*!<Reserved bits*/
        uint16_t UNUSED : 4;   /*!<Unused*/
    } bit_val;
    uint16_t val; /*!<Unified Status register value*/
} StatusReg_u;

#endif
