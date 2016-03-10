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

// Bring interface pin numbers into private vars
ADS129xADC::ADS129xADC (const int& pwdnPin, const int& resetPin, \
                        const int& startPin, const int& clkSelPin, \
                        const int& dRdyPin, const int& chipSelectPin)
{
    m_pwdnPin = pwdnPin;
    m_resetPin = resetPin;
    m_startPin = startPin;
    m_clkSelPin = clkSelPin;
    m_dRdyPin = dRdyPin;
    m_chipSelectPin = chipSelectPin;
}

// Initialise ADC interface pins
void ADS129xADC::initPins()
{
    pinMode(m_pwdnPin, OUTPUT);
    pinMode(m_resetPin, OUTPUT);
    pinMode(m_startPin, OUTPUT);
    pinMode(m_clkSelPin, OUTPUT);
    pinMode(m_dRdyPin, INPUT);
    pinMode(m_chipSelectPin, OUTPUT);
    
    SPI.begin();
}

// Power down the ADC
void ADS129xADC::pwrDown()
{
    digitalWriteFast(m_pwdnPin, LOW);
}

// Power up ADC and disable read data continuous mode
void ADS129xADC::pwrUp(const bool& first)
{
    digitalWriteFast(m_pwdnPin, LOW);
    digitalWriteFast(m_startPin, LOW);
    digitalWriteFast(m_clkSelPin, HIGH);
    digitalWriteFast(m_pwdnPin, HIGH);
    digitalWriteFast(m_resetPin, HIGH);
    
    if (first)
        delay(200);     // No need to wait for VCAP1 to charge if we are powering up after sleep
    
    digitalWriteFast(m_resetPin, LOW);
    delayMicroseconds(1);
    digitalWriteFast(m_resetPin, HIGH);
    delayMicroseconds(9);
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
void ADS129xADC::getID()
{
    m_adcID = readRegister(ID);
    
    switch (m_adcID & B00000111) { //3 least significant bits reports channels
        case B000:
            numChAv = 4; //ads1294
            break;
        case B001:
            numChAv = 6; //ads1296
            break;
        case B010:
            numChAv = 8; //ads1298
            break;
        case B110:
            numChAv = 8; //ads1299
            break;
        default:
            numChAv = 0; //indicates ADC comms error
    }
    
    m_respEN = ((m_adcID >> 5) && B110)? true : false;
}

// Startup the ADC, initialize interface and setup ID information
void ADS129xADC::startUp(const chType chSpec[])
{
    // Initialize ADC interface pins
    initPins();
    
    // Power up the ADC
    pwrUp(true);
    
    // Get ADC information
    getID();
    
    setRecInfo(chSpec);
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

// Set # of channels connected and recSize
void ADS129xADC::setRecInfo(const chType chSpec[])
{
    for (int i = 0; i < numChAv; i++) {
        (chSpec[i] == NC)? numChCon : numChCon++;
        m_chSpec[i] = chSpec[i];
    }
    
    recSize = (numChCon + m_getGPIO)  * BYTES_P_CH;
}

// Start ADC conversion and read data continuous mode
void ADS129xADC::startStream()
{
    digitalWriteFast(m_startPin, HIGH);
    sendCmd(RDATAC);
}

// Stop ADC conversion and read data continuous mode
void ADS129xADC::stopStream()
{
    digitalWriteFast(m_startPin, LOW);
    sendCmd(SDATAC);
}

// Setup signal acquisition
void ADS129xADC::setAqParams(const uint8_t& res_speed, const bool& intTest, const bool& useGPIO)
{
    uint8_t RLD_bits2set = 0x00;
    
    m_getGPIO = useGPIO;
    
    // All GPIO set to output (floating CMOS inputs can flicker, creating noise)
    writeRegister(GPIO, 0x00);
    
    // Set ADC to work at high resolution 1 KS/s sampling rate
    writeRegister(CONFIG1, res_speed);
    
    if (intTest) {
        // Generate AC internal test signal at smallest amplitude, but highest freq.
        writeRegister(CONFIG2, CONFIG2_const | INT_TEST_2HZ);
        
        // Setup all available channel to acquire test signal
        for (int i = 0; i < numChAv; i++)
            writeRegister(CH1SET + i, CHnSET_const | TEST_SIGNAL | GAIN_X12);
    }
    else {
        // Generate DC internal test signal
        writeRegister(CONFIG2, CONFIG2_const | INT_TEST_DC);
        
        // Setup all available channel to acquire test signal
        for (int i = 0; i < numChAv; i++) {
            switch (m_chSpec[i]) {
                case RES:
                    writeRegister(RESP, RESP_DEMOD_EN1 | RESP_MOD_EN1 | RESP_PH_135 | RESP_const | RESP_INT_SIG_INT);
                    writeRegister(CONFIG4, CONFIG4_const | RESP_FREQ_32k_Hz);
                    writeRegister(CH1SET, CHnSET_const | ELECTRODE_INPUT | GAIN_X4);
                    break;
                case SEN:
                    writeRegister(CH1SET + i, CHnSET_const | ELECTRODE_INPUT | GAIN_X12);
                    break;
                case PHY:
                    writeRegister(CH1SET + i, CHnSET_const | ELECTRODE_INPUT | GAIN_X12);
                    RLD_bits2set |= (1<<i);
                    break;
                default:
                    writeRegister(CH1SET + i, PD_CH | SHORTED);
                    break;
            }
        }
    }
    
    if (RLD_bits2set) {
        writeRegister(CONFIG3, RLDREF_INT | PD_RLD | PD_REFBUF | CONFIG3_const);
        writeRegister(RLD_SENSP, RLD_bits2set);
        writeRegister(RLD_SENSN, RLD_bits2set);
    }
    else {
        writeRegister(CONFIG3, PD_REFBUF | CONFIG3_const);
    }
}


// Fetch samples writing data to the buffer supplied
void ADS129xADC::fetchData(uint8_t* chData)
{
    int dataIdx = 0;
    
    chipSelectLow();
    
    if (m_getGPIO) {
        chData[dataIdx++] = SPI.transfer(0);
        chData[dataIdx++] = SPI.transfer(0);
        chData[dataIdx++] = SPI.transfer(0);
    }
    else {
        SPI.transfer(0);
        SPI.transfer(0);
        SPI.transfer(0);
    }
    
    for (int i = 0; i < numChAv; i++) {
        switch (m_chSpec[i]) {
            case RES:
            case SEN:
            case PHY:
                chData[dataIdx++] = SPI.transfer(0);
                chData[dataIdx++] = SPI.transfer(0);
                chData[dataIdx++] = SPI.transfer(0);
                break;
            default:
                SPI.transfer(0);
                SPI.transfer(0);
                SPI.transfer(0);
                break;
        }
    }
    chipSelectHigh();
}

