#include <stdint.h>
#include <Arduino.h>
#include <HID-Project.h>
#include <EEPROM.h>
#include "raw_hid.hpp"


    namespace raw_hid 
    {

        uint8_t rawBuffer[255];
        uint8_t outBuffer[64];
        uint8_t inBuffer[64];

        output_data_t *pOutputData = reinterpret_cast<output_data_t *>(outBuffer);
        input_data_t *pInputData = reinterpret_cast<input_data_t *>(inBuffer);

        void start() {
            RawHID.begin(rawBuffer, sizeof(rawBuffer));
        }

        void update() {
            RawHID.write(outBuffer, 64);
            if (RawHID.available()) {
                RawHID.readBytes(inBuffer, 64);
            }
        }

        input_data_t* getI()
        {
            return pInputData;
        }
        output_data_t* getO()
        {
            return pOutputData;
        }
    }

