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
#include "ctrlOutputs.h"
#include "LDSP.h"

#include <sstream> // for ostream

bool ctrlOutputsVerbose = false;

LDSPctrlOutContext ctrlOutputsContext;
extern LDSPinternalContext intContext;

int initCtrlOutputs(string **ctrlOutputsFiles, unsigned int chCount, ctrlout_struct *ctrlOutputs, ctrlOutState *ctrlOutputsStates, string *ctrlOutputsDetails, bool isAnalog=true);



int LDSP_initCtrlOutputs(LDSPinitSettings *settings, LDSPhwConfig *hwconfig)
{
    ctrlOutputsVerbose = settings->verbose;

    if(ctrlOutputsVerbose)
        printf("\nLDSP_initCtrlOutputs()\n");

    // analog control outputs
    if(ctrlOutputsVerbose)
        printf("Analog output ctrlOutput list:\n");

    int retVal = initCtrlOutputs(hwconfig->analogCtrlOutputs, chn_aout_count, ctrlOutputsContext.analogCtrlOutputs, 
                                ctrlOutputsContext.analogCtrlOutStates, ctrlOutputsContext.analogCtrlOutDetails);
    if(retVal!=0)
        return retVal;


    // digital control outputs
    if(ctrlOutputsVerbose)
        printf("Digital output ctrlOutput list:\n");

    retVal = initCtrlOutputs(hwconfig->digitalCtrlOutputs, chn_dout_count, ctrlOutputsContext.digitalCtrlOutputs, 
                            ctrlOutputsContext.digitalCtrlOutStates, ctrlOutputsContext.digitalCtrlOutDetails, false);
    if(retVal!=0)
        return retVal;


    // update context
    // --analog outputs
    intContext.analogOut = ctrlOutputsContext.analogCtrlOutBuffer;
    intContext.analogOutChannels = chn_aout_count;
    intContext.analogCtrlOutputState = ctrlOutputsContext.analogCtrlOutStates;
    intContext.analogCtrlOutputDetails = ctrlOutputsContext.analogCtrlOutDetails;
    // --digital outputs
    intContext.digitalOut = ctrlOutputsContext.digitalCtrlOutBuffer;
    intContext.digitalOutChannels = chn_dout_count;
    intContext.digitalCtrlOutputState = ctrlOutputsContext.digitalCtrlOutStates;
    intContext.digitalCtrlOutputDetails = ctrlOutputsContext.digitalCtrlOutDetails;

    return 0;
}


void cleanupCtrlOutputs(ctrlout_struct *ctrlOutputs, int numOfOutputs);

void LDSP_cleanupCtrlOutputs()
{
    if(ctrlOutputsVerbose)
        printf("LDSP_cleanupCtrlOutputs()\n");

    // analog devices
    cleanupCtrlOutputs(ctrlOutputsContext.analogCtrlOutputs, chn_aout_count);
    // digital devices
    cleanupCtrlOutputs(ctrlOutputsContext.digitalCtrlOutputs, chn_dout_count);
}




int initCtrlOutputs(string **ctrlOutputsFiles, unsigned int chCount, ctrlout_struct *ctrlOutputs, ctrlOutState *ctrlOutputsStates, string *ctrlOutputsDetails, bool isAnalog)
{
    ifstream readFile;
    for (int out=0; out<chCount; out++)
    {
        ctrlout_struct &ctrlOutput = ctrlOutputs[out];

        // if empty, ctrlOutput not configured
        if(ctrlOutputsFiles[DEVICE_CTRL_FILE][out].compare("") == 0)
        {
            ctrlOutput.configured = false;
            
            if(ctrlOutputsVerbose)
            {
                if(isAnalog)
                    printf("\t%s not configured\n", LDSP_analog_ctrlOutput[out].c_str());
                else
                    printf("\t%s not configured\n", LDSP_digital_ctrlOutput[out].c_str());
            }

            //-------------------------------------------------------------------
            // context update
            ctrlOutputsStates[out] = device_not_configured;
            ctrlOutputsDetails[out] =  "Not configured";
        }
        else
        {
            string fileName = ctrlOutputsFiles[DEVICE_CTRL_FILE][out];

            // configured
            ctrlOutput.configured = true;
            
            // initial value
            readFile.open(fileName);
            if(!readFile.is_open())
            {
                if(isAnalog)
                    fprintf(stderr, "Control output \"%s\", cannot open associated control file \"%s\"\n", LDSP_analog_ctrlOutput[out].c_str(), fileName.c_str());
                else
                    fprintf(stderr, "Control output \"%s\", cannot open associated control file \"%s\"\n", LDSP_digital_ctrlOutput[out].c_str(), fileName.c_str());

                LDSP_cleanupCtrlOutputs();
		        return -1;
            }
            char val[6];
	        readFile >> val;
            ctrlOutput.initialVal = atoi(val);  
            readFile.close();

            // prev value
            ctrlOutput.prevVal = 0; // the write buffer is filled with 0s and new values are only written if different from prev one
            // hence the initial value is preserved until we explicitly make a change
            
            // control file
            ctrlOutput.file.open(fileName);

            // scale value
            int scaleVal;
            string content = ctrlOutputsFiles[DEVICE_SCALE][out];
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
                        fprintf(stderr, "Control output \"%s\", cannot open associated max file \"%s\"\n", LDSP_analog_ctrlOutput[out].c_str(), ctrlOutputsFiles[DEVICE_SCALE][out].c_str());
                    else
                        fprintf(stderr, "Control output \"%s\", cannot open associated on value file \"%s\"\n", LDSP_digital_ctrlOutput[out].c_str(), fileName.c_str());

                    LDSP_cleanupCtrlOutputs();
                    return -1;
                }
                readFile >> val;
                scaleVal = atoi(val);  
                readFile.close();
            }
            // assign max               
            ctrlOutput.scaleVal = scaleVal;

            if(ctrlOutputsVerbose)
            {

                if(isAnalog)
                    printf("\t%s configured!\n", LDSP_analog_ctrlOutput[out].c_str());
                else
                    printf("\t%s configured!\n", LDSP_digital_ctrlOutput[out].c_str());
                printf("\t\tcontrol file \"%s\"\n", ctrlOutputsFiles[DEVICE_CTRL_FILE][out].c_str());
                
                // we skip this info if vibration, as reading about the default scale value may be confusing  
                if(isAnalog || out<chCount-1)
                {
                    string valueName = "on";
                    if(isAnalog)
                        valueName = "max";    
                    if(isNumber)
                        printf("\t\tmanually set %s value: %d\n", valueName.c_str(), ctrlOutput.scaleVal);
                    else
                        printf("\t\t%s value file \"%s\", containing value: %d\n", valueName.c_str(), ctrlOutputsFiles[DEVICE_SCALE][out].c_str(), ctrlOutput.scaleVal);
                    printf("\t\tinitial value: %d\n", ctrlOutput.initialVal);
                }
            }

            //-------------------------------------------------------------------
            // context update
            ctrlOutputsStates[out] = device_configured;
            ostringstream stream;
            if(isAnalog)
                stream << LDSP_analog_ctrlOutput[out] << ", with max value: " << ctrlOutput.scaleVal;
            else
                stream << LDSP_digital_ctrlOutput[out] << ", with on value value: " << ctrlOutput.scaleVal;
            ctrlOutputsDetails[out] =  stream.str();
        }
    }

    return 0;
}


//TODO move redundant code to inline function[s]?
void ctrlOutputsWrite()
{
    //VIC output values are persistent! 
    // we don't reset the output buffers once written to the devices
    // but we keep track of last value set and do not write again if same as requested value
    // NOTE: the only exception is vibration control, that is timed, so its value is reset to 0 once written

    // write to analog devices
    for(int out=0; out<chn_aout_count; out++)
    {
        ctrlout_struct *ctrlOutput = &ctrlOutputsContext.analogCtrlOutputs[out];
        
        // nothing to do on non-configured devices
        if(!ctrlOutput->configured)
            continue;

        unsigned int outInt = ctrlOutputsContext.analogCtrlOutBuffer[out]*ctrlOutput->scaleVal;
        // if scaled (de-normalized) value is same as previous one, no need to update
        if(outInt == ctrlOutput->prevVal)
            continue;

        // write
        write(&ctrlOutput->file, outInt);

        // update prev val for next call
        ctrlOutput->prevVal = outInt;
    }

    // write to digital devices
    for(int out=0; out<chn_dout_count; out++)
    {
        ctrlout_struct *ctrlOutput = &ctrlOutputsContext.digitalCtrlOutputs[out];

        // nothing to do on non-configured devices
        if(!ctrlOutput->configured)
            continue;

        unsigned int outInt = ctrlOutputsContext.digitalCtrlOutBuffer[out]*ctrlOutput->scaleVal;
        // if scaled value is same as previous one, no need to update
        if(outInt == ctrlOutput->prevVal)
            continue;

        // write
        write(&ctrlOutput->file, outInt);
        
        // vibration is timed based and the value that we write is the duration of vibration
        // once vibration ends, the written file goes automatically to zero 
        // hence, we reset the value in our buffer once written, to align with the file's behavior
        // this allows us to set vibration repeatedly regardless of previous value
        if(out == chn_dout_vibration)
        {
            ctrlOutputsContext.digitalCtrlOutBuffer[out] = 0; // reset buffer value
            outInt = 0; // set prev value to 0, so that same value can be set in subsequent calls
        }

        // update prev val for next call
        ctrlOutput->prevVal = outInt;
    }
}


void cleanupCtrlOutputs(ctrlout_struct *ctrlOutputs, int numOfOutputs)
{
    for (int out=0; out<numOfOutputs; out++)
    {
        // reset ctrlOutput to its initial state
        ctrlout_struct *ctrlOutput = &ctrlOutputs[out];
        // nothing to do on non-configured devices
        if(!ctrlOutput->configured)
            continue;
        write(&ctrlOutput->file, ctrlOutput->initialVal);

        // close file
        if(!ctrlOutput->configured)
            continue;
        if(!ctrlOutput->file.is_open())
            continue;
        ctrlOutput->file.close();
    }
}