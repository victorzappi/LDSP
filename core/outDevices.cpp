/*
 * [2-Clause BSD License]
 *
 * Copyright 2022 Victor Zappi
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "outDevices.h"
#include "LDSP.h"

#include <sstream> // for ostream

bool outDevicesVerbose = false;

LDSPoutDevContext outDevicesContext;
extern LDSPinternalContext intContext;


int LDSP_initOutDevices(LDSPinitSettings *settings, LDSPhwConfig *hwconfig)
{
    outDevicesVerbose = settings->verbose;

    if(outDevicesVerbose)
        printf("\nLDSP_initOutDevices()\n");



    //TODO turn into function and add digital! 

    // analog devices
    if(outDevicesVerbose)
        printf("Analog output device list:\n");

    ifstream readFile;
    for (int dev=0; dev<chn_aout_count; dev++)
    {
        outdev_struct &device = outDevicesContext.analogOutDevices[dev];

        if(hwconfig->analogOutDevices[ANALOG_CTRL_FILE][dev].compare("") == 0)
        {
            device.configured = false;
            
            if(outDevicesVerbose)
                printf("\t%s not configured\n", LDSP_analog_outDevices[dev].c_str());

            //-------------------------------------------------------------------
            // context update
            outDevicesContext.analogDevicesStates[dev] = device_not_configured;
            outDevicesContext.analogDevicesDetails[dev] =  "Not configured";
        }
        else
        {

            string fileName = hwconfig->analogOutDevices[ANALOG_CTRL_FILE][dev];

            // configured
            device.configured = true;
            
            // initial value
            readFile.open(fileName);
            if(!readFile.is_open())
            {
                fprintf(stderr, "Output device \"%s\", cannot open associated control file \"%s\"\n", LDSP_analog_outDevices[dev].c_str(), fileName.c_str());
                LDSP_cleanupOutDevices();
		        return -1;
            }
            char val[6];
	        readFile >> val;
            device.initialVal = atoi(val);  
            readFile.close();

            // prev value
            device.prevVal = device.initialVal;  
            
            // control file
            device.file.open(fileName);

            // max value
            readFile.open(hwconfig->analogOutDevices[ANALOG_MAX_FILE][dev]);
            if(!readFile.is_open())
            {
                fprintf(stderr, "Output device \"%s\", cannot open associated max file \"%s\"\n", LDSP_analog_outDevices[dev].c_str(), hwconfig->analogOutDevices[ANALOG_MAX_FILE][dev].c_str());
                LDSP_cleanupOutDevices();
		        return -1;
            }
	        readFile >> val;
            device.maxVal = atoi(val);  
            readFile.close();

            if(outDevicesVerbose)
            {
                printf("\t%s configured!\n", LDSP_analog_outDevices[dev].c_str());
                printf("\t\tcontrol file \"%s\"\n",  hwconfig->analogOutDevices[ANALOG_CTRL_FILE][dev].c_str());
                printf("\t\tmax file \"%s\", containing value: %d\n", hwconfig->analogOutDevices[ANALOG_MAX_FILE][dev].c_str(), device.maxVal);
                printf("\t\tinitial value: %d\n", device.initialVal);
            }

            //-------------------------------------------------------------------
            // context update
            outDevicesContext.analogDevicesStates[dev] = device_configured;
            ostringstream stream;
            stream << LDSP_analog_outDevices[dev] << ", with max value: " << device.maxVal;
            outDevicesContext.analogDevicesDetails[dev] =  stream.str();
        }
    }
    

    // update context
    intContext.analogOut = outDevicesContext.analogOutDevBuffer;
    intContext.analogOutChannels = chn_aout_count;
    intContext.analogOutDeviceState = outDevicesContext.analogDevicesStates;
    intContext.analogInSensorDetails = outDevicesContext.analogDevicesDetails;

    return 0;
}


void LDSP_cleanupOutDevices()
{
    if(outDevicesVerbose)
        printf("LDSP_cleanupOutDevices()\n");

    for (int dev=0; dev<chn_aout_count; dev++)
    {
        //TODO turn this into a function and call for both analog and digital

        // reset device to its initial state
        outdev_struct *device = &outDevicesContext.analogOutDevices[dev];
        // nothing to do on non-configured devices
        if(!device->configured)
            continue;
        write(&device->file, device->initialVal);

        // close file
        if(!device->configured)
            continue;
        if(!device->file.is_open())
            continue;
        device->file.close();
    }
}

void outputDevWrite()
{
    for(int dev=0; dev<chn_aout_count; dev++)
    {
        outdev_struct *device = &outDevicesContext.analogOutDevices[dev];
        
        // nothing to do on non-configured devices
        if(!device->configured)
            continue;

        unsigned int outInt = outDevicesContext.analogOutDevBuffer[dev]*device->maxVal;
        // if de-normalized value is same as previous one, no need to update
        if(outInt == device->prevVal)
            continue;

        // // write
        write(&device->file, outInt);
        // device->file << to_string(outInt);
        // device->file.flush();
        // update prev val for next call

        device->prevVal = outInt;
    }

    //VIC output values are persistent! we don't reset the output buffers once written to the devices
}