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

#include <fstream> // files
#include "hwConfig.h"
#include "ctrlOutputs.h"
#include "libraries/JSON/json.hpp"
#include "libraries/JSON/fifo_map.hpp"

#define HW_CONFIG_FILE "ldsp_hw_config.json"
#define DEV_ID_PLCHLDR_STR "DEVICE_ID"

using std::ifstream;
using json = nlohmann::json;
// workaround to have json parser maintain order of objects
// adapted from here: https://github.com/nlohmann/json/issues/485#issuecomment-333652309
template<class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using ordered_json = nlohmann::basic_json<my_workaround_fifo_map>;

bool hwconfigVerbose = false;


void parseMixerSettings(ordered_json *config, LDSPhwConfig *hwconfig);
void parseCtrlOutputs(ordered_json *config, LDSPhwConfig *hwconfig);


LDSPhwConfig* LDSP_HwConfig_alloc()
{
    LDSPhwConfig* hwconfig = new LDSPhwConfig();
    
    // these are the only parts of the config that need defaults
    hwconfig->hw_confg_file = HW_CONFIG_FILE;
    hwconfig->device_id_placeholder = DEV_ID_PLCHLDR_STR;
    hwconfig->default_dev_p = 0;
    hwconfig->default_dev_c = 0;
	hwconfig->deviceActivationCtl_p = "";
    hwconfig->deviceActivationCtl_c = "";
    hwconfig->ctrlOutputs[DEVICE_CTRL_FILE] = new string[chn_cout_count];
	hwconfig->ctrlOutputs[DEVICE_SCALE] = new string[chn_cout_count];

    return hwconfig;
}


void LDSP_HwConfig_free(LDSPhwConfig* hwconfig)
{
	delete[] hwconfig->ctrlOutputs[DEVICE_CTRL_FILE];
	delete[] hwconfig->ctrlOutputs[DEVICE_SCALE];
	delete hwconfig;
}


// retrieves hw config data from json file
int LDSP_parseHwConfigFile(LDSPinitSettings *settings, LDSPhwConfig *hwconfig)
{
    if(!settings)
		return -1;
    
    hwconfigVerbose = settings->verbose;

    if(hwconfigVerbose)
    	printf("LDSP_parseHwConfigFile()\n");

	// open json config file
	ifstream f_config(hwconfig->hw_confg_file.c_str());
	if (!f_config.is_open())
	{
		fprintf(stderr, "Cannot open hardware configuration file %s\n", hwconfig->hw_confg_file.c_str());
	 	return -1;
	}

	// parse file
	ordered_json config = ordered_json::parse(f_config);

    // parse specific settings
	parseMixerSettings(&config, hwconfig);
	parseCtrlOutputs(&config, hwconfig);
	
    // done parsing config file
	f_config.close();

	return 0;
}



//-----------------------------------------------------------------------------


void parseMixerSettings(ordered_json *config, LDSPhwConfig *hwconfig)
{
    if(hwconfigVerbose)
        printf("Parsing mixer settings...\n");

    // parse mixer settings container
    ordered_json config_ = *(config);
	ordered_json mixer = config_["mixer settings"];

	// mixer paths file
	hwconfig->xml_paths_file = mixer["mixer paths xml file"];
	// optional mixer volumes file
	json optional = mixer["mixer volumes xml file"];
	if(optional.is_string())
    {
        string s = optional;
        if(s.compare("") != 0)
		    hwconfig->xml_volumes_file = optional;
    }
	
	// playback parse
	ordered_json paths = mixer["playback path names"];
	
	// populate paths names and aliases
	for (auto &it : paths.items()) 
	{
		hwconfig->paths_p[it.key()] = it.value();
		hwconfig->paths_p_order.push_back(it.key()); // to keep track of order of insertion in map
	}


	// capture parse
	paths = mixer["capture path names"];
	// populate paths names and aliases
	for(auto &it : paths.items()) 
	{
		hwconfig->paths_c[it.key()] = it.value();
		hwconfig->paths_c_order.push_back(it.key()); // to keep track of order of insertion in map
	}

	// optional entries
    optional = mixer["default playback device number"];
	if(optional.is_number_integer())
		hwconfig->default_dev_p = optional;
	
	optional = mixer["default capture device number"];
	if(optional.is_number_integer())
		hwconfig->default_dev_c = optional;

	optional = mixer["mixer playback device activation"];
	if(optional.is_string())
    {
        string s = optional;
        if(s.compare("") != 0)
		    hwconfig->deviceActivationCtl_p = optional;
    }

	optional = mixer["mixer capture device activation"];
	if(optional.is_string())
    {
        string s = optional;
        if(s.compare("") != 0)
		    hwconfig->deviceActivationCtl_c = optional;
    }
}

void parseCtrlOutputs(ordered_json *config, LDSPhwConfig *hwconfig)
{
    if(hwconfigVerbose)
        printf("Parsing output devices...\n");


	// temporary map for quick retrieval of ctrlOutput index from name
	unordered_map<string, int> ctrlOut_indices;
	for (unsigned int i=0; i<chn_cout_count; i++)
		ctrlOut_indices[LDSP_ctrlOutput[i]] = i;

    // parse control outputs container
    ordered_json config_ = *(config);

	// these are all optional
	// parse control outputs
	ordered_json ctrlOutputs = config_["control outputs"];
	for(auto &it : ctrlOutputs.items()) 
	{
		int ctrlOut;
		string key = it.key();
		// check if ctrlOutput name is in list
		if(ctrlOut_indices.find(key)!=ctrlOut_indices.end())
		{
			ordered_json ctrlOutput = it.value();
			
			// control file
			// retrieve control file name
			json val = ctrlOutput["control file"];
			
			string ctrl;
			if(val.is_string())
				ctrl = val;
			// remains empty otherwise and auto fill will be tried later on
				
			// assign control file 
			ctrlOut = ctrlOut_indices[key];
			hwconfig->ctrlOutputs[DEVICE_CTRL_FILE][ctrlOut] = ctrl;


			// max file/max value
			// vibration does not have this entry in hw config file!
			if(ctrlOut==chn_cout_vibration)
			{
				// alway assigne default max value!
				hwconfig->ctrlOutputs[DEVICE_SCALE][ctrlOut_indices[key]] = "1"; // default is no scale!
				continue;
			}
			// other control outputs
			// retrieve max file name/max value
			json max = ctrlOutput["max value"];
			// assign max file
			if(max.is_string())
				hwconfig->ctrlOutputs[DEVICE_SCALE][ctrlOut_indices[key]] = max;
			else if(max.is_number_integer())
				hwconfig->ctrlOutputs[DEVICE_SCALE][ctrlOut_indices[key]] = to_string(max); // can be a number, but still needs to be stringified
			else 
				hwconfig->ctrlOutputs[DEVICE_SCALE][ctrlOut_indices[key]] = ""; // default! will be tenatively auto filled
		}
	}
}
