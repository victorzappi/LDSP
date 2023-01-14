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

#include <iostream>
#include <unistd.h> // for usleep()

#include "sensors.h"
#include "LDSP.h"

bool sensorsVerbose = false;

LDSPsensorsContext sensorsContext;
extern LDSPinternalContext intContext;

ASensorManager *sensor_manager;
ASensorEventQueue *event_queue;

void initSensors();
void initSensorBuffers();

void LDSP_initSensors(LDSPinitSettings *settings)
{
    sensorsVerbose = settings->verbose;

    if(sensorsVerbose)
        printf("\nLDSP_initSensors()\n");
    

    initSensors();
    initSensorBuffers();
    
    // update context
    intContext.analogIn = sensorsContext.sensorBuffer;
    intContext.analogSampleRate = (int)(settings->samplerate / settings->periodSize);
    intContext.analogInChannels = chn_ain_count;
    intContext.analogInSensorState = sensorsContext.sensorsStates;
    intContext.analogInSensorDetails = sensorsContext.sensorsDetails;
    intContext.analogInNormalFactor = sensorsContext.sensorsNormalFactors;
    //VIC user context is reference of this internal one, so no need to update it

    // read a couple of times, to make sure we have some sensor data once our audio application starts
    readSensors();
    usleep(200000); // 200 ms
    readSensors();
    // if non full duplex engine, we need to read an extra time, becuase initAudio() will take less time
    if(settings->outputOnly)
    {
        usleep(200000); // 200 ms
        readSensors();
    }
}

void LDSP_cleanupSensors()
{
    if(sensorsVerbose)
        printf("LDSP_cleanupSensors()\n");

    // disable present sensors and deallocate channels
    for(int i=0; i<sensorsContext.sensorsCount; i++)
    {
        if(sensorsContext.sensors[i].present)
        {
            ASensorEventQueue_enableSensor(event_queue, sensorsContext.sensors[i].asensor);
            delete[] sensorsContext.sensors[i].channels;
        }
    }

    // deallocate queue
    ASensorManager_destroyEventQueue(sensor_manager, event_queue);

    // daallocated sensor buffers
    delete[] sensorsContext.sensorBuffer;
    delete[] sensorsContext.sensorsStates;
    delete[] sensorsContext.sensorsDetails;
    delete[] sensorsContext.sensorsNormalFactors;

}


//--------------------------------------------------------------------------------------------------

void initSensors()
{
    sensor_manager = ASensorManager_getInstance();
    ALooper *looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    event_queue = ASensorManager_createEventQueue(sensor_manager, looper, 1, NULL, NULL);

    sensorsContext.sensorsCount = 0;
    //sensorsContext.channelCount = 0;
    int channelIndex = 0;

    if(sensorsVerbose)
        printf("Sensor list:\n");

    for(int i=0; i<(int)LDSP_sensor::count; i++)
    {
        sensorsContext.sensorsCount++; // to avoid using LDSP_sensor::count anymore

        LDSP_sensor sensor_type = (LDSP_sensor::_enum)LDSP_sensor::_from_index(i);
        const ASensor *sensor = ASensorManager_getDefaultSensor(sensor_manager, sensor_type);
        sensor_struct& sens_struct = sensorsContext.sensors[i];
        
        sens_struct.numOfChannels = atoi(sensors_channelsInfo[i][0].c_str());

        if (sensor == NULL) 
        {
            // skip sensor
            sens_struct.present = false;
            channelIndex += sens_struct.numOfChannels;
            if(sensorsVerbose)
                printf("\t%s not present ):\n", sensor_type._to_string());
        }
        else 
        {
            // fill sensor struct
            sens_struct.asensor = sensor;
            sens_struct.present = true;
            sens_struct.type = ASensor_getType(sensor);
            sens_struct.channels = (analogInChannel*) new unsigned int[sens_struct.numOfChannels];
            for(int chn=0; chn<sens_struct.numOfChannels; chn++) 
                sens_struct.channels[chn] = (analogInChannel)channelIndex++;

            sensorsContext.sensorsType_index[sens_struct.type] = i; // to quickly find this sensors in array
            
            if(sensorsVerbose)
            {
                printf("\t%s present!\n", sensor_type._to_string());
                printf("\t\tvendor and name: %s, %s\n", ASensor_getVendor(sensor), ASensor_getName(sensor));
                printf("\t\tresolution: %f\n", ASensor_getResolution(sensor));
                printf("\t\tmax rate: %f\n", 1.0/ASensor_getMinDelay(sensor));
            }
        
            ASensorEventQueue_enableSensor(event_queue, sensor);
            ASensorEventQueue_setEventRate(event_queue, sensor, 1); // symbolic 1 us sampling period... to make sure we request max rate
            //VIC there is an android API function that is supposed to return the min period supported, ASensor_getMinDelay()
            // but the doc says its value is often an underestimation: https://developer.android.com/ndk/reference/group/sensor#asensoreventqueue_seteventrate
        }
        
        //sensorsContext.channelCount += sens_struct.numOfChannels;
    }
}

void initSensorBuffers()
{
        // allocate buffers for sensor input samples
    sensorsContext.sensorBuffer  = new float[chn_ain_count]; // we allocate elements also for supported but non present sensors
    sensorsContext.sensorsStates = new sensorState[chn_ain_count]; 
    sensorsContext.sensorsDetails  = new string[chn_ain_count];
    sensorsContext.sensorsNormalFactors  = new float[chn_ain_count];

    // initialize 
    int chnCnt = 0;
    // for each supported sensor...
    for(int sens=0; sens<sensorsContext.sensorsCount; sens++)
    {
        // extract it along with its name and info
        sensor_struct *sens_struct = &sensorsContext.sensors[sens];
        LDSP_sensor sensor_type_name = (LDSP_sensor::_enum)LDSP_sensor::_from_index(sens);
        string name = sensor_type_name._to_string();
        string descr = sensors_channelsInfo[sens][1];
        string delimiter = ", ";
        size_t pos = 0;
        
        // for each channel in the sensor...
        for(int chn=0; chn<sens_struct->numOfChannels; chn++)
        {
            // init input buffer element
            sensorsContext.sensorBuffer[chnCnt] = 0; // this value will never be updated for sensors that are not present

            // init state
            if(sens_struct->present)
                sensorsContext.sensorsStates[chnCnt] = sensor_present;
            else
                sensorsContext.sensorsStates[chnCnt] = sensor_not_present;

            // init type
            // in case sensors has more channels, pick the description of the current channel
            pos = descr.find(delimiter); // channel decsriptions are separated by ", "
            string chnDescr = descr.substr(0, pos);
            descr = descr.substr(pos + delimiter.length());
    
            sensorsContext.sensorsDetails[chnCnt] = name + ", sensing " + chnDescr;

            // init normalization factor
            float normFact = sensors_max[sens];
            if(normFact == -1)
                normFact = 1;
            sensorsContext.sensorsNormalFactors[chnCnt] = normFact;

            chnCnt++;
        }
    }
    // unsupported sensors/channels
    for(; chnCnt<chn_ain_count; chnCnt++)
    {
        sensorsContext.sensorBuffer[chnCnt] = 0; // this value will never be update
        sensorsContext.sensorsStates[chnCnt] = sensor_not_supported;
        sensorsContext.sensorsDetails[chnCnt] = "Not supported";
        sensorsContext.sensorsNormalFactors[chnCnt] = 1;
    }
}

void readSensors()
{
    ASensorEvent event;
    
    // get most current events, if any
    while(ASensorEventQueue_getEvents(event_queue, &event, 1) > 0) 
    {
        int idx = sensorsContext.sensorsType_index[event.type]; // get index of sensors of this type
        sensor_struct& sensor = sensorsContext.sensors[idx]; // get sensor of this type
        // fill sensorBuffer with sensor data, in the channels reserved to this type of sensor
        // normalized sensor
        if(sensors_max[idx] > 0)
        {
            for(int chn=0; chn<sensor.numOfChannels; chn++)
            {
                float val = constrain(event.data[chn], -sensors_max[idx], sensors_max[idx]);
                val /= sensors_max[idx];
                sensorsContext.sensorBuffer[sensor.channels[chn]] = val;
            }
        }
        else // non-normalized sensor
        {
            for(int chn=0; chn<sensor.numOfChannels; chn++)
                sensorsContext.sensorBuffer[sensor.channels[chn]] = event.data[chn];
        }

        // if(event.type == ASENSOR_TYPE_ACCELEROMETER)
        // {
        //     float ar = event.data[0];
        //     float br = event.data[1];
        //     float cr = event.data[2];
        //     printf("sensor raw: %f, %f, %f\n", ar, br, cr);

        //     float a = sensorsContext.sensorBuffer[sensor.channels[0]];
        //     float b = sensorsContext.sensorBuffer[sensor.channels[1]];
        //     float c = sensorsContext.sensorBuffer[sensor.channels[2]];
        //     printf("sensor: %f, %f, %f\n", a, b, c);
        // }  

        // if(event.type == ASENSOR_TYPE_LIGHT)
        // {
        //     float ar = event.data[0];
        //     printf("sensor light: %f\n", ar);
        // }
        // if(event.type == ASENSOR_TYPE_PROXIMITY)
        // {
        //     float ar = event.data[0];
        //     printf("sensor prox: %f\n", ar);
        // }
    }
}