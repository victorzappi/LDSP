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

int initOutDevices(string **outDevicesFiles, unsigned int chCount, outdev_struct *outDevices, outDeviceState *outDevicesStates, string *outDevicesDetails, bool isAnalog=true);



int LDSP_initOutDevices(LDSPinitSettings *settings, LDSPhwConfig *hwconfig)
{
    outDevicesVerbose = settings->verbose;

    if(outDevicesVerbose)
        printf("\nLDSP_initOutDevices()\n");

    // analog devices
    if(outDevicesVerbose)
        printf("Analog output device list:\n");

    int retVal = initOutDevices(hwconfig->analogOutDevices, chn_aout_count, outDevicesContext.analogOutDevices, 
                                outDevicesContext.analogDevicesStates, outDevicesContext.analogDevicesDetails);
    if(retVal!=0)
        return retVal;


    /*ifstream readFile;
    for (int dev=0; dev<chn_aout_count; dev++)
    {
        outdev_struct &device = outDevicesContext.analogOutDevices[dev];

        // if empty, device not configured
        if(hwconfig->analogOutDevices[DEVICE_CTRL_FILE][dev].compare("") == 0)
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

            string fileName = hwconfig->analogOutDevices[DEVICE_CTRL_FILE][dev];

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
            device.prevVal = 0; // the write buffer is filled with 0s and new values are only written if different from prev one
            // hence the initial value is preserved until we explicitly make a change
            
            // control file
            device.file.open(fileName);

            // max value
            int scaleVal;
            string content = hwconfig->analogOutDevices[DEVICE_SCALE][dev];
            int isNumber = isdigit(content.c_str()[0]);
            // if this is a number, then use it as custom max value
            if(isNumber)
                 scaleVal = stoi(content);
            else // otherwise this is the path to the file that contains the max value
            {
                readFile.open(content);
                if(!readFile.is_open())
                {
                    fprintf(stderr, "Output device \"%s\", cannot open associated max file \"%s\"\n", LDSP_analog_outDevices[dev].c_str(), hwconfig->analogOutDevices[DEVICE_SCALE][dev].c_str());
                    LDSP_cleanupOutDevices();
                    return -1;
                }
                readFile >> val;
                scaleVal = atoi(val);  
                readFile.close();
            }
            // assign max               
            device.scaleVal = scaleVal;

            if(outDevicesVerbose)
            {
                printf("\t%s configured!\n", LDSP_analog_outDevices[dev].c_str());
                printf("\t\tcontrol file \"%s\"\n",  hwconfig->analogOutDevices[DEVICE_CTRL_FILE][dev].c_str());
                if(isNumber)
                    printf("\t\tmanually set max value: %d\n", device.scaleVal);
                else
                    printf("\t\tmax file \"%s\", containing value: %d\n", hwconfig->analogOutDevices[DEVICE_SCALE][dev].c_str(), device.scaleVal);
                printf("\t\tinitial value: %d\n", device.initialVal);
            }

            //-------------------------------------------------------------------
            // context update
            outDevicesContext.analogDevicesStates[dev] = device_configured;
            ostringstream stream;
            stream << LDSP_analog_outDevices[dev] << ", with max value: " << device.scaleVal;
            outDevicesContext.analogDevicesDetails[dev] =  stream.str();
        }
    }*/
    

    // digital devices
    if(outDevicesVerbose)
        printf("Digital output device list:\n");

    retVal = initOutDevices(hwconfig->digitalOutDevices, chn_dout_count, outDevicesContext.digitalOutDevices, 
                            outDevicesContext.digitalDevicesStates, outDevicesContext.digitalDevicesDetails, false);
    if(retVal!=0)
        return retVal;

    /*for (int dev=0; dev<chn_dout_count; dev++)
    {
        outdev_struct &device = outDevicesContext.digitalOutDevices[dev];

        // if empty, device not configured
        if(hwconfig->digitalOutDevices[dev].compare("") == 0)
        {
            device.configured = false;
            
            if(outDevicesVerbose)
                printf("\t%s not configured\n", LDSP_digital_outDevices[dev].c_str());

            //-------------------------------------------------------------------
            // context update
            outDevicesContext.digitalDevicesStates[dev] = device_not_configured;
            outDevicesContext.digitalDevicesDetails[dev] =  "Not configured";
        }
        else
        {

            string fileName = hwconfig->digitalOutDevices[dev];

            // configured
            device.configured = true;
            
            // initial value
            readFile.open(fileName);
            if(!readFile.is_open())
            {
                fprintf(stderr, "Output device \"%s\", cannot open associated control file \"%s\"\n", LDSP_digital_outDevices[dev].c_str(), fileName.c_str());
                LDSP_cleanupOutDevices();
		        return -1;
            }
            char val[6];
	        readFile >> val;
            device.initialVal = atoi(val);  
            readFile.close();

            // prev value
            device.prevVal = 0; // the write buffer is filled with 0s and new values are only written if different from prev one
            // hence the initial value is preserved until we explicitly make a change

            // control file
            device.file.open(fileName);
            
            if(outDevicesVerbose)
            {
                printf("\t%s configured!\n", LDSP_digital_outDevices[dev].c_str());
                printf("\t\tcontrol file \"%s\"\n", hwconfig->digitalOutDevices[dev].c_str());
                printf("\t\tinitial value: %d\n", device.initialVal);
            }

            //-------------------------------------------------------------------
            // context update
            outDevicesContext.digitalDevicesStates[dev] = device_configured;
            ostringstream stream;
            stream << LDSP_digital_outDevices[dev];
            outDevicesContext.digitalDevicesDetails[dev] =  stream.str();
        }
    }*/

    // update context
    // --analog outputs
    intContext.analogOut = outDevicesContext.analogOutDevBuffer;
    intContext.analogOutChannels = chn_aout_count;
    intContext.analogOutDeviceState = outDevicesContext.analogDevicesStates;
    intContext.analogOutDeviceDetails = outDevicesContext.analogDevicesDetails;
    // --digital outputs
    intContext.digitalOut = outDevicesContext.digitalOutDevBuffer;
    intContext.digitalOutChannels = chn_dout_count;
    intContext.digitalOutDeviceState = outDevicesContext.digitalDevicesStates;
    intContext.digitalOutDeviceDetails = outDevicesContext.digitalDevicesDetails;

    return 0;
}


void cleanupOutDevices(outdev_struct *outDevices, int numOfDev);

void LDSP_cleanupOutDevices()
{
    if(outDevicesVerbose)
        printf("LDSP_cleanupOutDevices()\n");

    // analog devices
    cleanupOutDevices(outDevicesContext.analogOutDevices, chn_aout_count);
    // digital devices
    cleanupOutDevices(outDevicesContext.digitalOutDevices, chn_dout_count);
}




int initOutDevices(string **outDevicesFiles, unsigned int chCount, outdev_struct *outDevices, outDeviceState *outDevicesStates, string *outDevicesDetails, bool isAnalog)
{
    ifstream readFile;
    for (int dev=0; dev<chCount; dev++)
    {
        outdev_struct &device = outDevices[dev];

        // if empty, device not configured
        if(outDevicesFiles[DEVICE_CTRL_FILE][dev].compare("") == 0)
        {
            device.configured = false;
            
            if(outDevicesVerbose)
            {
                if(isAnalog)
                    printf("\t%s not configured\n", LDSP_analog_outDevices[dev].c_str());
                else
                    printf("\t%s not configured\n", LDSP_digital_outDevices[dev].c_str());
            }

            //-------------------------------------------------------------------
            // context update
            outDevicesStates[dev] = device_not_configured;
            outDevicesDetails[dev] =  "Not configured";
        }
        else
        {
            string fileName = outDevicesFiles[DEVICE_CTRL_FILE][dev];

            // configured
            device.configured = true;
            
            // initial value
            readFile.open(fileName);
            if(!readFile.is_open())
            {
                if(isAnalog)
                    fprintf(stderr, "Output device \"%s\", cannot open associated control file \"%s\"\n", LDSP_analog_outDevices[dev].c_str(), fileName.c_str());
                else
                    fprintf(stderr, "Output device \"%s\", cannot open associated control file \"%s\"\n", LDSP_digital_outDevices[dev].c_str(), fileName.c_str());

                LDSP_cleanupOutDevices();
		        return -1;
            }
            char val[6];
	        readFile >> val;
            device.initialVal = atoi(val);  
            readFile.close();

            // prev value
            device.prevVal = 0; // the write buffer is filled with 0s and new values are only written if different from prev one
            // hence the initial value is preserved until we explicitly make a change
            
            // control file
            device.file.open(fileName);

            // scale value
            int scaleVal;
            string content = outDevicesFiles[DEVICE_SCALE][dev];
            int isNumber = isdigit(content.c_str()[0]);
            // if this is a number, then use it as custom max value
            if(isNumber)
                 scaleVal = stoi(content);
            else // otherwise this is the path to the file that contains the max value
            {
                readFile.open(content);
                if(!readFile.is_open())
                {
                    if(isAnalog)
                        fprintf(stderr, "Output device \"%s\", cannot open associated max file \"%s\"\n", LDSP_analog_outDevices[dev].c_str(), outDevicesFiles[DEVICE_SCALE][dev].c_str());
                    else
                        fprintf(stderr, "Output device \"%s\", cannot open associated on value file \"%s\"\n", LDSP_digital_outDevices[dev].c_str(), fileName.c_str());

                    LDSP_cleanupOutDevices();
                    return -1;
                }
                readFile >> val;
                scaleVal = atoi(val);  
                readFile.close();
            }
            // assign max               
            device.scaleVal = scaleVal;

            if(outDevicesVerbose)
            {

                if(isAnalog)
                    printf("\t%s configured!\n", LDSP_analog_outDevices[dev].c_str());
                else
                    printf("\t%s configured!\n", LDSP_digital_outDevices[dev].c_str());
                printf("\t\tcontrol file \"%s\"\n", outDevicesFiles[DEVICE_CTRL_FILE][dev].c_str());
                
                // we skip this info if vibration, as reading about the default scale value may be confusing  
                if(isAnalog || dev<chCount-1)
                {
                    string valueName = "on";
                    if(isAnalog)
                        valueName = "max";    
                    if(isNumber)
                        printf("\t\tmanually set %s value: %d\n", valueName.c_str(), device.scaleVal);
                    else
                        printf("\t\t%s value file \"%s\", containing value: %d\n", valueName.c_str(), outDevicesFiles[DEVICE_SCALE][dev].c_str(), device.scaleVal);
                    printf("\t\tinitial value: %d\n", device.initialVal);
                }
            }

            //-------------------------------------------------------------------
            // context update
            outDevicesStates[dev] = device_configured;
            ostringstream stream;
            if(isAnalog)
                stream << LDSP_analog_outDevices[dev] << ", with max value: " << device.scaleVal;
            else
                stream << LDSP_digital_outDevices[dev] << ", with on value value: " << device.scaleVal;
            outDevicesDetails[dev] =  stream.str();
        }
    }

    return 0;
}


//TODO move redundant code to inline function[s]?
void outputDevWrite()
{
    //VIC output values are persistent! 
    // we don't reset the output buffers once written to the devices
    // but we keep track of last value set and do not write again if same as requested value
    // NOTE: the only exception is vibration control, that is timed, so its value is reset to 0 once written

    // write to analog devices
    for(int dev=0; dev<chn_aout_count; dev++)
    {
        outdev_struct *device = &outDevicesContext.analogOutDevices[dev];
        
        // nothing to do on non-configured devices
        if(!device->configured)
            continue;

        unsigned int outInt = outDevicesContext.analogOutDevBuffer[dev]*device->scaleVal;
        // if scaled (de-normalized) value is same as previous one, no need to update
        if(outInt == device->prevVal)
            continue;

        // write
        write(&device->file, outInt);

        // update prev val for next call
        device->prevVal = outInt;
    }

    // write to digital devices
    for(int dev=0; dev<chn_dout_count; dev++)
    {
        outdev_struct *device = &outDevicesContext.digitalOutDevices[dev];

        // nothing to do on non-configured devices
        if(!device->configured)
            continue;

        unsigned int outInt = outDevicesContext.digitalOutDevBuffer[dev]*device->scaleVal;
        // if scaled value is same as previous one, no need to update
        if(outInt == device->prevVal)
            continue;

        // write
        write(&device->file, outInt);
        
        // vibration is timed based and the value that we write is the duration of vibration
        // once vibration ends, the written file goes automatically to zero 
        // hence, we reset the value in our buffer once written, to align with the file's behavior
        // this allows us to set vibration repeatedly regardless of previous value
        if(dev == chn_dout_vibration)
        {
            outDevicesContext.digitalOutDevBuffer[dev] = 0; // reset buffer value
            outInt = 0; // set prev value to 0, so that same value can be set in subsequent calls
        }

        // update prev val for next call
        device->prevVal = outInt;
    }
}


void cleanupOutDevices(outdev_struct *outDevices, int numOfDev)
{
    for (int dev=0; dev<numOfDev; dev++)
    {
        // reset device to its initial state
        outdev_struct *device = &outDevices[dev];
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