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
#ifndef ADS129xADC_h
#define ADS129xADC_h

/** Nop for timing. */
#ifndef nop
#define nop __asm__ __volatile__ ("nop\n\t")
#endif  // nop

#include "Arduino.h"

#define USE_SOFT_SPI    1
#define MAX_CH_NUM      8
#define BYTES_P_CH      3

// Select bit-bang (Soft) SPI
#if USE_SOFT_SPI
#include "SoftSPI.h"
#define ADS_SOFT_SPI_MISO_PIN   16
#define ADS_SOFT_SPI_MOSI_PIN   15
#define ADS_SOFT_SPI_SCK_PIN    14
#define SPI_MODE                1
extern SoftSPI<ADS_SOFT_SPI_MISO_PIN, ADS_SOFT_SPI_MOSI_PIN, ADS_SOFT_SPI_SCK_PIN, SPI_MODE> SPI;
#else
#include "SPI.h"
#endif  // USE_SOFT_SPI

/** Define ADC control pins */
#define ADS_PWDN_PIN     2
#define ADS_RESET_PIN    3
#define ADS_START_PIN    4
#define ADS_CLKSEL_PIN   6
#define ADS_DRDY_PIN     7
#define ADS_CS_PIN       10

// Define channel types
enum chType
{
    NC = 0,     // Nothing connected
    PHY,        // Physiological input (EEG, ECG, EMG, etc.) used for RLD
    SEN,        // External sensor (Breathing belt, MM sensor, etc.) not used for RLD
    RES         // Use impedance pneumography with R series device (valid type for channel 1 only!) not used for RLD
};

class ADS129xADC
{
private:
    // Private functions
    void chipSelectLow();
    void chipSelectHigh();
    // ADC interface pins
    int m_pwdnPin;
    int m_resetPin;
    int m_startPin;
    int m_clkSelPin;
    int m_dRdyPin;
    int m_chipSelectPin;
    // ADC params
    int m_adcID     = 0;
    bool m_getGPIO  = false;
    bool m_respEN   = false;
    chType m_chSpec[MAX_CH_NUM];
    // Initialise ADC interface pins
    void initPins();
    void setRecInfo(const chType chSpec[]);
public:
    int numChAv     = 0;
    int numChCon    = 0;
    int recSize     = 0;
    // Bring interface pin numbers into private vars at construction
    ADS129xADC(const int& pwdnPin = ADS_PWDN_PIN, \
               const int& resetPin = ADS_RESET_PIN, \
               const int& startPin = ADS_START_PIN, \
               const int& clkSelPin = ADS_CLKSEL_PIN, \
               const int& dRdyPin = ADS_DRDY_PIN, \
               const int& chipSelectPin = ADS_CS_PIN);
    // Power down the ADCs
    void pwrDown();
    // Power up the ADC
    void pwrUp(const bool& first);
    // Put ADC into standby mode
    void standby();
    // Wake ADC from standby mode
    void wakeup();
    // Get ADC ID
    void getID();
    // Start continuous data acquisition
    void streamC(const chType chSpec[], const uint8_t& res_speed, \
                 const bool& intTest, const bool& useGPIO = false);
    void setAqParams(const chType chSpec[], const uint8_t& res_speed, \
                     const bool& intTest, const bool& useGPIO);
    // Initialize ADC pins, power it up and test comms by fetching and saving ID
    void startUp();
    // Start continuous data stream
    void sendCmd(const uint8_t& cmd);
    // Write single ADC register
    void writeRegister(const uint8_t& reg, const uint8_t& arg);
    // Read single ADC register
    uint8_t readRegister(const uint8_t& reg);
    // Fetch data from ADC, specify number of channels to fetch and if GPIO data is required
    void fetchData(uint8_t* chData);
};
#endif /* ADS129xADC_h */
