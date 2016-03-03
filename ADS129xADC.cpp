/* Teensy ADS129xADC library
 * Copyright (C) 2014 by Valentin Goverdovsky
 *
 * This file is part of the Teensy ADS129xADC Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Teensy ADS129xADC Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "ADS129xInfo.h"
#include "ADS129xADC.h"

#if USE_SOFT_SPI
SoftSPI<ADS_SOFT_SPI_MISO_PIN, ADS_SOFT_SPI_MOSI_PIN, ADS_SOFT_SPI_SCK_PIN, SPI_MODE> SPI;
#endif

// Initialise ADC interface
void ADS129xADC::init(const uint8_t& pwdnPin = ADS_PWDN_PIN, \
                      const uint8_t& resetPin = ADS_RESET_PIN, \
                      const uint8_t& startPin = ADS_START_PIN, \
                      const uint8_t& clkSelPin = ADS_CLKSEL_PIN, \
                      const uint8_t& dRdyPin = ADS_DRDY_PIN, \
                      const uint8_t& chipSelectPin = ADS_CS_PIN)
{
    m_pwdnPin = pwdnPin;
    m_resetPin = resetPin;
    m_startPin = startPin;
    m_clkSelPin = clkSelPin;
    m_dRdyPin = dRdyPin;
    m_chipSelectPin = chipSelectPin;
    
    pinMode(m_pwdnPin, OUTPUT);
    pinMode(m_resetPin, OUTPUT);
    pinMode(m_startPin, OUTPUT);
    pinMode(m_clkSelPin, OUTPUT);
    pinMode(m_dRdyPin, INPUT);
    pinMode(m_chipSelectPin, OUTPUT);
}

// Power down the ADC
void ADS129xADC::pwrDown()
{
    digitalWriteFast(m_pwdnPin, LOW);
}

// Power up ADC and disable read data continuous mode
void ADS129xADC::pwrUp(const bool& init)
{
    digitalWriteFast(m_pwdnPin, LOW);
    digitalWriteFast(m_startPin, LOW);
    digitalWriteFast(m_clkSelPin, HIGH);
    digitalWriteFast(m_pwdnPin, HIGH);
    digitalWriteFast(m_resetPin, HIGH);
    
    if (init)
        delay(200);                // No need to wait for VCAP1 to charge if we are powering up after sleep
    
    digitalWriteFast(m_resetPin, LOW);
    delayMicroseconds(1);
    digitalWriteFast(m_resetPin, HIGH);
    delayMicroseconds(9);
    SPI.begin();
    sendCmd(SDATAC);
}

// Put ADC into standby mode
void ADS129xADC::standby()
{
    sendCmd(STANDBY);
}

// Wake up ADC from standby mode
void ADS129xADC::wakeup()
{
    sendCmd(WAKEUP);
    delayMicroseconds(2);
}

// Get ADC ID
uint8_t ADS129xADC::getID()
{
    switch (readRegister(ID)) {
        case ID_ADS1294:
            return ID_ADS1294;
            break;
        case ID_ADS1296:
            return ID_ADS1296;
            break;
        case ID_ADS1298:
            return ID_ADS1298;
            break;
        case ID_ADS1294R:
            return ID_ADS1294R;
            break;
        case ID_ADS1296R:
            return ID_ADS1296R;
            break;
        case ID_ADS1298R:
            return ID_ADS1298R;
            break;
        default:
            return 0;
    }
}

// Setup signal acquisition
void ADS129xADC::setup(const uint8_t& numChs, const uint8_t& maxChs, const uint8_t& res_speed, const bool& rld, const bool& intTest, const bool& resp)
{
    uint8_t RLD_bits = 0;
    // All GPIO set to output (floating CMOS inputs can flicker, creating noise)
    writeRegister(GPIO, 0x00);
    
    // Set ADC to work at high resolution 1 KS/s sampling rate
    writeRegister(CONFIG1, res_speed);
    
    if (intTest) {
        // Generate AC internal test signal
        writeRegister(CONFIG2, CONFIG2_const | INT_TEST_2HZ | TEST_AMP);
        
        // Setup channels required to acquire test signal
        for (uint8_t iii = 0; iii < numChs; ++iii) {
            writeRegister(CH1SET + iii, CHnSET_const | TEST_SIGNAL | GAIN_X12); //create square wave
        }
        
        // Power down channels that are not required
        for (uint8_t iii = numChs; iii < maxChs; ++iii) {
            writeRegister(CH1SET + iii, PD_CH | SHORTED);
        }
    }
    else {
        // Generate DC internal test signal
        writeRegister(CONFIG2, CONFIG2_const);
        
        if (resp) {
            writeRegister(RESP, RESP_DEMOD_EN1 | RESP_MOD_EN1 | RESP_PH_135 | RESP_const | RESP_INT_SIG_INT);
            writeRegister(CONFIG4, CONFIG4_const | RESP_FREQ_32k_Hz);
            
            // Setup channel 1 for respiration acquisition
            writeRegister(CH1SET, CHnSET_const | ELECTRODE_INPUT | GAIN_X4);
            
            // Setup channels other channels
            for (uint8_t iii = 1; iii < numChs; ++iii) {
                writeRegister(CH1SET + iii, CHnSET_const | ELECTRODE_INPUT | GAIN_X12);
                RLD_bits |= (1 << iii);
            }
        }
        else {
            // Setup channels
            for (uint8_t iii = 0; iii < numChs; ++iii) {
                writeRegister(CH1SET + iii, CHnSET_const | ELECTRODE_INPUT | GAIN_X12);
                RLD_bits |= (1 << iii);
            }
        }
        
        // Power down channels that are not required
        for (uint8_t iii = numChs; iii < maxChs; ++iii) {
            writeRegister(CH1SET + iii, PD_CH | SHORTED);
        }
    }
    
    if (rld) {
        // FOR RLD: Power up the internal reference
        writeRegister(CONFIG3, RLDREF_INT | PD_RLD | PD_REFBUF | CONFIG3_const);
        // Only use channels IN2P and IN2N for the RLD Measurement
        writeRegister(RLD_SENSP, 0x02);
        writeRegister(RLD_SENSN, 0x02);
//            writeRegister(RLD_SENSP, RLD_bits);
//            writeRegister(RLD_SENSN, RLD_bits);
    }
    else {
        writeRegister(CONFIG3, PD_REFBUF | CONFIG3_const);
    }
}

// Start continuous data acquisition
void ADS129xADC::startC()
{
    digitalWriteFast(m_startPin, HIGH);
    sendCmd(RDATAC);
}

// If required reconfigure SPI interface for ADC and pull ADC CS pin LOW
void ADS129xADC::chipSelectLow()
{
#if !USE_SOFT_SPI
    SPI.setBitOrder(MSBFIRST);
    SPI.setClockDivider(SPI_CLOCK_DIV2);
    SPI.setDataMode(SPI_MODE1);
#endif
    digitalWriteFast(m_chipSelectPin, LOW);
    nop;
}

// Pull ADC CS pin HIGH
void ADS129xADC::chipSelectHigh()
{
    delayMicroseconds(3);       // Wait for at least 4 tclk before pulling CS high (tclk = 2.048 MHz)
    digitalWriteFast(m_chipSelectPin, HIGH);
}

void ADS129xADC::sendCmd(const uint8_t& cmd)
{
    chipSelectLow();            // Chip select needs to be pulled low to communicate with the device
    SPI.transfer(cmd);
    chipSelectHigh();
}

// Write one ADC register
void ADS129xADC::writeRegister(const uint8_t& reg, const uint8_t& arg)
{
    chipSelectLow();
    SPI.transfer(WREG | reg);
    SPI.transfer(0x00);         // Number of registers to be read/written minus 1
    SPI.transfer(arg);
    chipSelectHigh();
}

// Read one ADC register
uint8_t ADS129xADC::readRegister(const uint8_t& reg)
{
    uint8_t reg_val = 0;
    chipSelectLow();
    SPI.transfer(RREG | reg);
    SPI.transfer(0x00);         // Number of registers to be read/written minus 1
    reg_val = SPI.transfer(0);
    chipSelectHigh();
    return reg_val;
}

// Fetch samples writing data directly to the buffer
void ADS129xADC::fetchData(uint8_t* chData, const uint8_t& numChs, const bool& gpio)
{
    chipSelectLow();
    if (gpio) {
        // Fetch sample data
        for (uint8_t iii = 0; iii < (numChs + gpio) * 3; iii++) {
            chData[iii]     = SPI.transfer(0);
        }
    }
    else {
        // Skip GPIO data
        SPI.transfer(0);
        SPI.transfer(0);
        SPI.transfer(0);
        // Fetch sample data
        for (uint8_t iii = 0; iii < numChs * 3; iii++) {
            chData[iii] = SPI.transfer(0);
        }
    }
    chipSelectHigh();
}
