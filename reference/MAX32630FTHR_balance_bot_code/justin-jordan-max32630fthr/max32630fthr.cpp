/*******************************************************************************
 * Copyright (C) 2016 Maxim Integrated Products, Inc., All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated
 * Products, Inc. shall not be used except as stated in the Maxim Integrated
 * Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all
 * ownership rights.
 *******************************************************************************
 */


#include "mbed.h"
#include "PinNames.h"
#include "max32630fthr.h"
#include "max3263x.h"
#include "ioman_regs.h"
#include "adc.h"
#include "mxc_errors.h"


//******************************************************************************
MAX32630FTHR::MAX32630FTHR() 
: m_i2c(P5_7, P6_0), m_max14690(&m_i2c)
{
}


//******************************************************************************
int32_t MAX32630FTHR::init(IoVoltage vio)
{
    int32_t rtnVal = -1;
    m_hdrVio = vio;
    
    // Override the default values
    m_max14690.ldo2Millivolts = 3300;
    m_max14690.ldo3Millivolts = 3300;
    m_max14690.ldo2Mode = MAX14690::LDO_ENABLED;
    m_max14690.ldo3Mode = MAX14690::LDO_ENABLED;
    m_max14690.monCfg = MAX14690::MON_HI_Z;
    // Note that writing the local value does directly affect the part
    // The buck-boost regulator will remain off until init is called
 
    // Call init to apply all settings to the PMIC
    if (m_max14690.init() != MAX14690_ERROR) 
    {
        // Set micro SD card pins to 3.3V
        vddioh(P0_4, VIO_3V3);
        vddioh(P0_5, VIO_3V3);
        vddioh(P0_6, VIO_3V3);
        vddioh(P0_7, VIO_3V3);
        // Set LED pins to 3.3V
        vddioh(P2_4, VIO_3V3);
        vddioh(P2_5, VIO_3V3);
        vddioh(P2_6, VIO_3V3);
        // Set header pins to hdrVio
        vddioh(P3_0, m_hdrVio);
        vddioh(P3_1, m_hdrVio);
        vddioh(P3_2, m_hdrVio);
        vddioh(P3_3, m_hdrVio);
        vddioh(P3_4, m_hdrVio);
        vddioh(P3_5, m_hdrVio);
        vddioh(P4_0, m_hdrVio);
        vddioh(P4_1, m_hdrVio);
        vddioh(P4_2, m_hdrVio);
        vddioh(P4_3, m_hdrVio);
        vddioh(P4_4, m_hdrVio);
        vddioh(P4_5, m_hdrVio);
        vddioh(P4_6, m_hdrVio);
        vddioh(P4_7, m_hdrVio);
        vddioh(P5_0, m_hdrVio);
        vddioh(P5_1, m_hdrVio);
        vddioh(P5_2, m_hdrVio);
        vddioh(P5_3, m_hdrVio);
        vddioh(P5_4, m_hdrVio);
        vddioh(P5_5, m_hdrVio);
        vddioh(P5_6, m_hdrVio);
    }
    
    return rtnVal;
}


//******************************************************************************
int32_t MAX32630FTHR::vddioh(PinName pin, IoVoltage vio)
{
    __IO uint32_t *use_vddioh = &((mxc_ioman_regs_t *)MXC_IOMAN)->use_vddioh_0;

    if (pin == NOT_CONNECTED) {
        return -1;
    }

    use_vddioh += PINNAME_TO_PORT(pin) >> 2;
    if (vio) {
        *use_vddioh |= (1 << (PINNAME_TO_PIN(pin) + ((PINNAME_TO_PORT(pin) & 0x3) << 3)));
    } else {
        *use_vddioh &= ~(1 << (PINNAME_TO_PIN(pin) + ((PINNAME_TO_PORT(pin) & 0x3) << 3)));
    }

    return 0;
}


//******************************************************************************
int32_t MAX32630FTHR::getBatteryVoltage(float *battVolts)
{
    return readMonVoltage(MAX14690::MON_BAT, battVolts);
}


//******************************************************************************    
int32_t MAX32630FTHR::getSysVoltage(float *sysVolts)
{
    return readMonVoltage(MAX14690::MON_SYS, sysVolts);
}


//******************************************************************************
int32_t MAX32630FTHR::getBuck1Voltage(float *buckVolts)
{
    return readMonVoltage(MAX14690::MON_BUCK1, buckVolts);
}


//******************************************************************************
int32_t MAX32630FTHR::getBuck2Voltage(float *buckVolts)
{
    return readMonVoltage(MAX14690::MON_BUCK2, buckVolts);
}


//******************************************************************************
int32_t MAX32630FTHR::getLDO1Voltage(float *ldoVolts)
{
    return readMonVoltage(MAX14690::MON_LDO1, ldoVolts);
}


//******************************************************************************
int32_t MAX32630FTHR::getLDO2Voltage(float *ldoVolts)
{
    return readMonVoltage(MAX14690::MON_LDO2, ldoVolts);
}


//******************************************************************************
int32_t MAX32630FTHR::getLDO3Voltage(float *ldoVolts)
{
    return readMonVoltage(MAX14690::MON_LDO3, ldoVolts);
}


//******************************************************************************
int32_t MAX32630FTHR::readMonVoltage(MAX14690::monCfg_t monCfg, float *volts)
{
    int32_t rtnVal = -1;
   
    if(m_max14690.monSet(monCfg, MAX14690::MON_DIV4) == 0)
    {
        wait_ms(1);
        AnalogIn a0(AIN_0);
        *volts = (a0.read() * 4.8F);
        rtnVal = m_max14690.monSet(MAX14690::MON_HI_Z, MAX14690::MON_DIV1);
        wait_ms(1);
    } 
    
    return rtnVal;
}
