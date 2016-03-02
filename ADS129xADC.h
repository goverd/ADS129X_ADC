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

#define USE_SOFT_SPI 1

/** Nop for timing. */
#ifndef nop
#define nop __asm__ __volatile__ ("nop\n\t")
#endif  // nop

#include "Arduino.h"

#if USE_SOFT_SPI
#include "SoftSPI.h"
uint8_t const ADS_SOFT_SPI_MISO_PIN = 16;
uint8_t const ADS_SOFT_SPI_MOSI_PIN = 15;
uint8_t const ADS_SOFT_SPI_SCK_PIN  = 14;
uint8_t const SPI_MODE = 1;
extern SoftSPI<ADS_SOFT_SPI_MISO_PIN, ADS_SOFT_SPI_MOSI_PIN, ADS_SOFT_SPI_SCK_PIN, SPI_MODE> SPI;
#else
#include "SPI.h"
#endif

/** Define ADC control pins */
uint8_t const ADS_PWDN_PIN     = 2;
uint8_t const ADS_RESET_PIN    = 3;
uint8_t const ADS_START_PIN    = 4;
uint8_t const ADS_CLKSEL_PIN   = 6;
uint8_t const ADS_DRDY_PIN     = 7;
uint8_t const ADS_CS_PIN       = 10;

class ADS129xADC
{
private:
    // Private functions
    void chipSelectLow();
    void chipSelectHigh();
    // Private data
    uint8_t m_chipSelectPin;
public:
    // Construct an instance of ADS129xADC
    ADS129xADC() {}
    // Initialise ADC interface
    void initInt(const uint8_t& chipSelectPin);
    // Power down the ADCs
    void pwrDown();
    // Power up the ADC
    void pwrUp(const bool& init);
    // Put ADC into standby mode
    void standby();
    // Wake ADC from standby mode
    void wakeup();
    // Get ADC ID
    uint8_t getID();
    // Setup signal acquisition at high resolution and 1 KS/s rate
    void setup(const uint8_t& numChs, const uint8_t& maxChs, const uint8_t& res_speed, const bool& rld, const bool& intTest, const bool& resp);
    // Send single byte command to the ADC
    void sendCmd(const uint8_t& cmd);
    // Write single ADC register
    void writeRegister(const uint8_t& reg, const uint8_t& arg);
    // Read single ADC register
    uint8_t readRegister(const uint8_t& reg);
    // Fetch data from ADC, specify number of channels to fetch and if GPIO data is required
    void fetchData(uint8_t* chData, const uint8_t& numChs, const bool& gpio);
};
#endif /* ADS129xADC_h */
