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
#include "tinyalsaAudio.h" // for LDSPinternalContext

#include <sstream> // for ostream
#include <dirent.h> // to search for files
#include <memory> // smart pointers
#include <regex> // replace substring

using std::ifstream;
using std::ostringstream;
using std::shared_ptr;
using std::make_shared;
using std::regex;
using std::regex_replace;

#define CTRL_BASE_PATH "/sys/class"

bool ctrlOutputsVerbose = false;

LDSPctrlOutputsContext ctrlOutputsContext;
extern LDSPinternalContext intContext;



int initCtrlOutputs(string **ctrlOutputsFiles);




int LDSP_initCtrlOutputs(LDSPinitSettings *settings, LDSPhwConfig *hwconfig)
{
    ctrlOutputsVerbose = settings->verbose;

    if(ctrlOutputsVerbose)
        printf("\nLDSP_initCtrlOutputs()\n");

    int retVal = initCtrlOutputs(hwconfig->ctrlOutputs);
    if(retVal!=0)
        return retVal;

    // update context
    // --analog outputs
    intContext.ctrlOutputs = ctrlOutputsContext.ctrlOutBuffer;
    intContext.ctrlOutChannels = chn_cout_count;
    intContext.ctrlOutputsState = ctrlOutputsContext.ctrlOutStates;
    intContext.ctrlOutputsDetails = ctrlOutputsContext.ctrlOutDetails;
    return 0;
}


void cleanupCtrlOutputs(ctrlout_struct *ctrlOutputs, int numOfOutputs);

void LDSP_cleanupCtrlOutputs()
{
    if(ctrlOutputsVerbose)
        printf("LDSP_cleanupCtrlOutputs()\n");

    // analog control outputs
    cleanupCtrlOutputs(ctrlOutputsContext.ctrlOutputs, chn_cout_count);
}




bool ctrlOutputAutoFill_ctrl(int out, shared_ptr<ctrlOutputKeywords> keywords, string &control_file)
{        
    DIR* directory;
    string path;
    
    // first try to open one of the possible container folders
    for(string folder : keywords->folders[out])
    {
        path = string(CTRL_BASE_PATH) + "/" + folder;
        directory = opendir(path.c_str());
        if (directory != nullptr)
            break;
        path = "";
    }
    // cannot find any folder ):
    if(path == "")
    {
        closedir(directory);
        return false;
    }

    // if we found folder, 
    // search for a subfolder whose name contains one of the strings
    dirent* entry;
    string content = "";
    while( (entry = readdir(directory)) != nullptr ) 
    {
        string d_name = string(entry->d_name);
        for(string str : keywords->subFolders_strings[out])
        {
            // skip if not an actual string to check
            if(str == "")
                continue;

            size_t found = d_name.find(str);
            // if found, we append subfolder to path
            if (found != string::npos) 
            {
                content = path + "/" + d_name;
                break;
            }
        }
        // we can stop at this first string that we found!
        if(content != "")
        {
            path = content;
            break;
        }
    }
    closedir(directory);
    // cannot find any string in names ):
    if(content == "")
        return false;

    // once we have folder and subfolder,
    // finally check if the control file is there
    directory = opendir(path.c_str());
    if (directory == nullptr)
    {
        closedir(directory); 
        fprintf(stderr, "Control output \"%s\", cannot open file \"%s\" during autofill!\n", LDSP_analog_ctrlOutput[out].c_str(), path.c_str());
        return false;
    }

    //TODO complete case of vibration with activate+duration
    // also in the write function!!!
    content = "";
    while( (entry = readdir(directory)) != nullptr ) 
    {
        string d_name = string(entry->d_name);
        // in most cases, there is a single possible name for the control file
        if(d_name == keywords->files[out][0])
        {
            content = "/" + d_name;
            break;
        }
        // onyl exception is vibraiton, that has two possibilities
        else if(out==chn_cout_vibration && d_name == keywords->files[out][1])
        {
            content = "/" + d_name;
            break;
        }
    }
    // cannot find file ):
    if(content == "")
    {
        closedir(directory);        
        return false;
    }

    // this is the full path to the control file!
    control_file = path + content;
    
    closedir(directory);     

    return true;
}


void ctrlOutputAutoFill_scale(int out, shared_ptr<ctrlOutputKeywords> keywords, string control_file, string &scale_file)
{
    // this will never be called for vibration, cos vibration scale is set to 1 by default

    scale_file = "255"; // default value, in case file cannot be found

    // search for max file in same dir where control file is
    // to obtain dir path, remove control file name from path!
    string path = std::regex_replace(control_file, std::regex("/"+keywords->files[out][0]), "");
    
    DIR* directory = opendir(path.c_str());
    if (directory == nullptr)
    {
        closedir(directory); 
        fprintf(stderr, "Control output \"%s\", cannot open file \"%s\" during autofill!\n", LDSP_analog_ctrlOutput[out].c_str(), path.c_str());
        return;
    }

    // then check if the max file is here too!
    dirent* entry;
    while( (entry = readdir(directory)) != nullptr ) 
    {
        string d_name = string(entry->d_name);
        if(d_name == keywords->files[out][1])
        {
            scale_file = path + "/" + d_name;
            break;
        }
    }            
    
    closedir(directory); 
}


int initCtrlOutputs(string **ctrlOutputsFiles)
{
    shared_ptr<ctrlOutputKeywords> keywords = make_shared<ctrlOutputKeywords>(); // will be useful in auto config

    if(ctrlOutputsVerbose)
        printf("Control output list:\n");

    ifstream readFile;
    for (int out=0; out<chn_cout_count; out++)
    {
        int autoConfig_ctrl = false;
        int autoConfig_scale = false;
        ctrlout_struct &ctrlOutput = ctrlOutputsContext.ctrlOutputs[out];

        // control file first

        // if empty, try auto fill
        if(ctrlOutputsFiles[DEVICE_CTRL_FILE][out].compare("") == 0)
        {
            // and if still empty, ctrlOutput not configured
            if( !ctrlOutputAutoFill_ctrl(out, keywords, ctrlOutputsFiles[DEVICE_CTRL_FILE][out]) )
            {
                ctrlOutput.configured = false;
                
                if(ctrlOutputsVerbose)
                    printf("\t%s not configured\n", LDSP_analog_ctrlOutput[out].c_str());

                //-------------------------------------------------------------------
                // context update
                ctrlOutputsContext.ctrlOutStates[out] = ctrlOutput_not_configured;
                ctrlOutputsContext.ctrlOutDetails[out] =  "Not configured";
                continue;
            }
            autoConfig_ctrl = true;
        }
      
        string fileName = ctrlOutputsFiles[DEVICE_CTRL_FILE][out];

        // configured
        ctrlOutput.configured = true;
        
        // initial value
        readFile.open(fileName);
        if(!readFile.is_open())
        {
            fprintf(stderr, "Control output \"%s\", cannot open associated control file \"%s\"\n", LDSP_analog_ctrlOutput[out].c_str(), fileName.c_str());
            
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


        // max file/max value then

        // if empty, autofill!
        // worst case scenario, autofill will put default "255"
        // note that in the case of vibration hwConfig parser always puts "1", so no autofill is ever invoked
        if(ctrlOutputsFiles[DEVICE_SCALE][out].compare("") == 0)
        {
            ctrlOutputAutoFill_scale(out, keywords, ctrlOutputsFiles[DEVICE_CTRL_FILE][out], ctrlOutputsFiles[DEVICE_SCALE][out]);
            autoConfig_scale = true;
        }
        
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
                fprintf(stderr, "Control output \"%s\", cannot open associated max file \"%s\"\n", LDSP_analog_ctrlOutput[out].c_str(), ctrlOutputsFiles[DEVICE_SCALE][out].c_str());
                
                LDSP_cleanupCtrlOutputs();
                return -1;
            }
            readFile >> val;
            scaleVal = atoi(val);  
            readFile.close();
        }
        
        // assign scale value               
        ctrlOutput.scaleVal = scaleVal;

        if(ctrlOutputsVerbose)
        {
            printf("\t%s configured!\n", LDSP_analog_ctrlOutput[out].c_str());
            
            string automatic = "";
            if(autoConfig_ctrl)
                automatic = " (set automatically)";
            printf("\t\tcontrol file \"%s\"%s\n", ctrlOutputsFiles[DEVICE_CTRL_FILE][out].c_str(), automatic.c_str());
            
            // we skip this info if vibration, as reading about the default scale value may be confusing  
            if(out!=chn_cout_vibration)
            {
                if(isNumber)
                {
                    automatic = "automatically";
                    if(!autoConfig_scale)
                        automatic = "manually";

                    printf("\t\t%s set max value: %d\n", automatic.c_str(), ctrlOutput.scaleVal);
                }
                else
                {
                    automatic = "";
                    if(autoConfig_scale)
                        automatic = " (set automatically)";
                    printf("\t\tmax value file \"%s\"%s, containing value: %d\n", ctrlOutputsFiles[DEVICE_SCALE][out].c_str(), automatic.c_str(), ctrlOutput.scaleVal);
                }
                printf("\t\tinitial value: %d\n", ctrlOutput.initialVal);
            }
        }

        //-------------------------------------------------------------------
        // context update
        ctrlOutputsContext.ctrlOutStates[out] = ctrlOutput_configured;
        ostringstream stream;
        stream << LDSP_analog_ctrlOutput[out] << ", with max value: " << ctrlOutput.scaleVal;
        ctrlOutputsContext.ctrlOutDetails[out] =  stream.str();
    }

    return 0;
}


void writeCtrlOutputs()
{
    //VIC output values are persistent! 
    // we don't reset the output buffers once written to the devices
    // but we keep track of last value set and do not write again if same as requested value
    // NOTE: the only exception is vibration control, that is timed, so its value is reset to 0 once written

    // write to analog devices
    for(int out=0; out<chn_cout_count; out++)
    {
        ctrlout_struct *ctrlOutput = &ctrlOutputsContext.ctrlOutputs[out];
        
        // nothing to do on non-configured devices
        if(!ctrlOutput->configured)
            continue;

        unsigned int outInt = ctrlOutputsContext.ctrlOutBuffer[out]*ctrlOutput->scaleVal;
        // if scaled (de-normalized) value is same as previous one, no need to update
        if(outInt == ctrlOutput->prevVal)
            continue;

        // write
        write(&ctrlOutput->file, outInt);

        // vibration is timed based and the value that we write is the duration of vibration
        // once vibration ends, the written file goes automatically to zero 
        // hence, we reset the value in our buffer once written, to align with the file's behavior
        // this allows us to set vibration repeatedly regardless of previous value
        if(out == chn_cout_vibration)
        {
            ctrlOutputsContext.ctrlOutBuffer[out] = 0; // reset buffer value
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