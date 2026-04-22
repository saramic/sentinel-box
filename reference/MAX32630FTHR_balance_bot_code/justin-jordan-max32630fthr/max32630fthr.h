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
 

#ifndef _MAX32630FTHR_H_
#define _MAX32630FTHR_H_


#include "mbed.h"
#include "MAX14690.h"


/**
 * @brief MAX32630FTHR Board Support Library
 *
 * @details The MAX32630FTHR is a rapid development application board for
 * ultra low power wearable applications.  It includes common peripherals and
 * expansion connectors all power optimized for getting the longest life from
 * the battery.  This library configures the power and I/O for the board.
 * <br>https://www.maximintegrated.com/max32630fthr
 *
 * @code
 * #include "mbed.h"
 * #include "max32630fthr.h"
 *
 * DigitalOut led1(LED1);
 * MAX32630FTHR pegasus(MAX32630FTHR::VIO_3V3);
 *
 * // main() runs in its own thread in the OS
 * // (note the calls to Thread::wait below for delays)
 * int main()
 * {
 *     // initialize power and I/O on MAX32630FTHR board
 *     pegasus.init();
 *
 *     while (true) {
 *         led1 = !led1;
 *         Thread::wait(500);
 *     }
 * }
 * @endcode
 */
class MAX32630FTHR
{
public:

    // max32630fthr configuration utilities
    
    ///@brief   IoVoltage
    ///@details Enumerated options for operating voltage
    enum IoVoltage
    {
        VIO_1V8 = 0x00,    ///< 1.8V IO voltage at headers (from BUCK2)
        VIO_3V3 = 0x01    ///< 3.3V IO voltage at headers (from LDO2)
    };


    ///@brief MAX32630FTHR constructor..\n
    ///
    ///On Entry:
    ///@param[in] vio - I/O voltage for header pins 
    ///
    ///On Exit:
    ///@param[out] none
    ///
    ///@returns none
    MAX32630FTHR();
    
    
    ///@brief MAX32630FTHR destructor..\n
    ///
    ///On Entry:
    ///@param[in] none 
    ///
    ///On Exit:
    ///@param[out] none
    ///
    ///@returns none
    ~MAX32630FTHR(){ };

  
    ///@brief Initialize MAX32630FTHR board.\n
    ///@details Initializes PMIC and I/O on MAX32630FTHR board.\n
    ///Configures PMIC to enable LDO2 and LDO3 at 3.3V.
    ///Disables resisitive pulldown on MON(AIN_0).\n
    ///Sets default I/O voltages to 3V3 for micro SD card.\n
    ///Sets I/O voltage for header pins to hdrVio specified.\n
    ///
    ///On Entry:
    ///@param[in] none 
    ///
    ///On Exit:
    ///@param[out] none
    ///
    ///@returns 0 if no errors, -1 if error.
    int32_t init(IoVoltage vio);
    

    ///@brief Sets I/O Voltage.\n
    ///@details Sets the voltage rail to be used for a given pin.\n
    ///VIO_1V8 selects VDDIO which is supplied by Buck2, which is set at 1.8V,\n
    ///VIO_3V3 selects VDDIOH which is supplied by LDO2, which is typically 3.3V.
    ///On Entry:
    ///@param[in] pin - Pin whose voltage supply is being assigned.
    ///@param[in] vio - Voltage rail to be used for specified pin.
    ///
    ///On Exit:
    ///@param[out] none
    ///
    ///@returns 0 if no errors, -1 if error.
    int32_t vddioh(PinName pin, IoVoltage vio);
    
    
    ///@brief Gets battery voltage as float.\n
    ///
    ///On Entry:
    ///@param[in] battVolts - pointer to float for storing battery voltage 
    ///
    ///On Exit:
    ///@param[out] battVolts - holds battery voltage on success
    ///
    ///@returns 0 if no errors, -1 if error.
    int32_t getBatteryVoltage(float *battVolts);
    
    
    ///@brief Gets system voltage voltage as float.\n
    ///
    ///On Entry:
    ///@param[in] sysVolts - pointer to float for storing system voltage 
    ///
    ///On Exit:
    ///@param[out] sysVolts - holds system voltage on success
    ///
    ///@returns 0 if no errors, -1 if error.
    int32_t getSysVoltage(float *sysVolts);
    
    
    ///@brief Gets buck 1 voltage as float.\n
    ///
    ///On Entry:
    ///@param[in] buckVolts - pointer to float for storing buck 1 voltage 
    ///
    ///On Exit:
    ///@param[out] buckVolts - holds buck 1 voltage on success
    ///
    ///@returns 0 if no errors, -1 if error.
    int32_t getBuck1Voltage(float *buckVolts);
    
    
    ///@brief Gets buck 2 voltage as float.\n
    ///
    ///On Entry:
    ///@param[in] buckVolts - pointer to float for storing buck 2 voltage 
    ///
    ///On Exit:
    ///@param[out] buckVolts - holds buck 2 voltage on success
    ///
    ///@returns 0 if no errors, -1 if error.
    int32_t getBuck2Voltage(float *buckVolts);
    
    
    ///@brief Gets LDO1 voltage as float.\n
    ///
    ///On Entry:
    ///@param[in] ldoVolts - pointer to float for storing LDO1 voltage 
    ///
    ///On Exit:
    ///@param[out] ldoVolts - holds LDO1 voltage on success
    ///
    ///@returns 0 if no errors, -1 if error.
    int32_t getLDO1Voltage(float *ldoVolts);
    
    
    ///@brief Gets LDO2 voltage as float.\n
    ///
    ///On Entry:
    ///@param[in] ldoVolts - pointer to float for storing LDO2 voltage 
    ///
    ///On Exit:
    ///@param[out] ldoVolts - holds LDO2 voltage on success
    ///
    ///@returns 0 if no errors, -1 if error.
    int32_t getLDO2Voltage(float *ldoVolts);
    
    
    ///@brief Gets LDO3 voltage as float.\n
    ///
    ///On Entry:
    ///@param[in] ldoVolts - pointer to float for storing LDO3 voltage 
    ///
    ///On Exit:
    ///@param[out] ldoVolts - holds LDO3 voltage on success
    ///
    ///@returns 0 if no errors, -1 if error.
    int32_t getLDO3Voltage(float *ldoVolts);
    
    
private:

    /// I2C bus for configuring PMIC
    I2C m_i2c;

    /// MAX14690 PMIC Instance
    MAX14690 m_max14690;
    
    /// The default I/O voltage to be used for header pins.
    IoVoltage m_hdrVio;
    
    int32_t readMonVoltage(MAX14690::monCfg_t monCfg, float *volts);
};

#endif /* _MAX32630FTHR_H_ */

///@brief fx documentation template.\n
///
///On Entry:
///@param[in] none 
///
///On Exit:
///@param[out] none
///
///@returns none

