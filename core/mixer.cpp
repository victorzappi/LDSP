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

/*
 * mixer.c
 *
 *  Created on: 2022-08-14
 *      Author: Victor Zappi
 */

#include <iostream>
#include <cstdio> // fprintf

#include <tinyalsa/asoundlib.h>

#include "mixer.h"
#include "audioDeviceInfo.h"
#include "libraries/XML/pugixml.hpp"

using std::string;
using std::to_string;
using namespace pugi;

bool mixerVerbose = false;

bool skipMixerPaths = false;

bool preserveMixerPaths = false;

mixer *mix = nullptr;


int setupDevicesNumAndId(LDSPinitSettings *settings, LDSPhwConfig *hwconfig);
int mixerCtl_setInt(mixer *mx, const char *name, int val, int id=0, bool verbose=true);
int mixerCtl_setStr(mixer *mx, const char *name, const char *val, bool verbose=true);
int setDefaultMixerPath(mixer *mx, xml_document *xml);
int activateDevice(mixer *mx, LDSPhwConfig *hwconfig, string &deviceActivationCtl, string device_id);
void deactivateDevice(mixer *mx, string deviceActivationCtl);
int loadPath(mixer *mx, xml_document *xml, LDSPhwConfig *hwconfig, LDSPinitSettings *settings, string &pathAlias, bool isCapture=false);



int LDSP_setMixerPaths(LDSPinitSettings *settings, LDSPhwConfig *hwconfig)
{
	if(!settings)
		return -1;
    
	// if not using embedded card, we can skip mixer paths
	skipMixerPaths = (settings->card != 0);
	
    mixerVerbose = settings->verbose;

	preserveMixerPaths = settings->preserveMixer;

	
	if(mixerVerbose && !skipMixerPaths)
		printf("\nLDSP_setMixerPaths()\n");


	// populate devices' numbers and ids
	if(setupDevicesNumAndId(settings, hwconfig)!=0)
		return -2;

	// populate devices' main audio settings 
	if(settings->periodSize == -1)
		settings->periodSize = hwconfig->default_period_size;
	
	if(settings->numAudioOutChannels == -1)
		settings->numAudioOutChannels = hwconfig->default_chn_num_p;
	
	if(!settings->captureOff)
	{
		if(settings->numAudioInChannels == -1)
			settings->numAudioInChannels = hwconfig->default_chn_num_c;
	}


	if(skipMixerPaths)
		return 0;



	// open mixer
	mix = mixer_open(settings->card);
    if (!mix || mix == nullptr) 
	{
        fprintf(stderr, "Failed to open mixer\n");
		LDSP_resetMixerPaths(hwconfig);
        return -3;
    }
	

	// use XML to load paths
	xml_document mixer_docs[2]; // [0] -> mixer paths, [1] -> mixer volumes (optional)
   
    // load the mixer paths XML file
    if(!mixer_docs[0].load_file(hwconfig->xml_paths_file.c_str()))
	{
		LDSP_resetMixerPaths(hwconfig);
		return -4;
	}
	// load the mixer volumes XML file, if present
	if(!hwconfig->xml_volumes_file.empty())
	{
		if(!mixer_docs[1].load_file(hwconfig->xml_volumes_file.c_str()))
		{
			LDSP_resetMixerPaths(hwconfig);
			return -4;
		}
	} 
		

	// start from loading default paths, unless requested to preserve the current paths
	// if paths are preservered, more than one alsa device can be routed to the codec at once
	if(!preserveMixerPaths)
		setDefaultMixerPath(mix, &mixer_docs[0]);


	
	// if necessary, activate devices, i.e., when in config file device activation path is given
	// secondary activation is checked after path is set, in loadPath()
	if(!hwconfig->dev_activation_ctl_p.empty())
	{
		if(activateDevice(mix, hwconfig, hwconfig->dev_activation_ctl_p , settings->deviceOutId) < 0)
		{
			LDSP_resetMixerPaths(hwconfig);
			return -5;
		}
	}
	if(!settings->captureOff)
	{
		if(!hwconfig->dev_activation_ctl_c.empty())
		{
			if(activateDevice(mix, hwconfig, hwconfig->dev_activation_ctl_c , settings->deviceInId) < 0)
			{
				LDSP_resetMixerPaths(hwconfig);
				return -5;
			}
		}
	}

	// set actual playback and capture paths
	string pathAlias = settings->pathOut;
	if(loadPath(mix, mixer_docs, hwconfig, settings, pathAlias)!=0)
	{
		fprintf(stderr, "Playback path error\n");
		LDSP_resetMixerPaths(hwconfig);
		return -6;
	}
	if(mixerVerbose)
		printf("Playback path loaded: \"%s\"\n", pathAlias.c_str());

	if(!settings->captureOff)
	{
		pathAlias = settings->pathIn;
		if(loadPath(mix, mixer_docs, hwconfig, settings, pathAlias, true)!=0)
		{
			fprintf(stderr, "Capture path error\n");
			LDSP_resetMixerPaths(hwconfig);
			return -6;
		}
		if(mixerVerbose)
			printf("Capture path loaded: \"%s\"\n", pathAlias.c_str());
	}
	
	return 0;
}


void LDSP_resetMixerPaths(LDSPhwConfig *hwconfig)
{
	if(skipMixerPaths)
		return;

	if(mixerVerbose)
		printf("LDSP_resetMixerPaths()\n");

	xml_document mixer_docs;
    if(!preserveMixerPaths && mixer_docs.load_file(hwconfig->xml_paths_file.c_str()))
		setDefaultMixerPath(mix, &mixer_docs); // deactivates devices too, if necessary on phone

	if (mix != nullptr) 
		mixer_close(mix);
}

//----------------------------------------------------------------------------------

int findDeviceInfo(vector<string> deviceInfoPath, string match_a, string name, string match_b, string &value)
{
	// iterate all info files of all devices, cos we don't know which one belongs to current device
	for(auto path : deviceInfoPath)
	{
		string info;
		// first, we need to locate the info file that belongs to current device
		// so we extract first piece of info, that we will compare with passed name
		// if they match, this is the right info file!
		// and we will extract the actual piece of info we are looking for [value]
		int ret=getDeviceInfo(path, match_a, info);

		if(ret==-1)
		{
			printf("Warning! Could not open \"%s\" to retrieve device %s's \"%s\"\n", path.c_str(), name.c_str(), match_b.c_str());
			continue;
		}
		if(ret==-2)
		{
			printf("Warning! Could not find \"%s\" in \"%s\" while looking for %s's \"%s\"\n", match_a.c_str(), path.c_str(), name.c_str(), match_b.c_str());
			continue;
		}

		// check if passed name is contained in piece of info we retrieved
		size_t found = info.find(name);
		// if it's not, this is not the info file of the current device
		if(found==std::string::npos)
			continue;
		// if the searched value is a number, we go for an exact match!
		if(isdigit(name.c_str()[0]))
		{
			if(stoi(info)!=stoi(name))
				continue;
		}
		// in case we are serching for an id, the actual string may have an extra (*) termination
		// so we have to avoid an exact match search
		
		// if we get here, this is the right info file!
		// let's extract the piece of info we are looking for and store it in value
		ret=getDeviceInfo(path, match_b, value);
		if(ret==-2)
		{
			fprintf(stderr, "Could not find %s's \"%s\" in \"%s\"\n", name.c_str(), match_b.c_str(), path.c_str());
			return -1;
		}
		return 0;
	}
	return -2;
}

int setupDeviceNumAndId(vector<string> deviceInfoPath, int &deviceNum, int defaultNum, string &deviceId, string defaultId)
{
	// device id has priority!
	// but device number is always set, at least as a default value

	if(deviceId.empty())
		deviceId = defaultId; // the config file may not include a default id, in which case the value remains empty
	
	if(deviceNum == -1)
		deviceNum = defaultNum; // the config file may not include a default id, in which case the value is set to 0 [check LDSP_HwConfig_alloc()]

	// if device is set by id, adjust number accordingly
	if(deviceId!="")
	{
		// look for device number in info files 
		string num;
		int ret = findDeviceInfo(deviceInfoPath, "id: ", deviceId, "device: ", num);
		if(ret!=0)
			return -1;
		deviceNum = stoi(num);
	}
	else // otherwise, adjust id from number, which always have a value
	{
		// look for device id in info files 
		string id;
		int ret = findDeviceInfo(deviceInfoPath, "device: ", to_string(deviceNum), "id: ", id);
		if(ret!=0)
			return -2;
		// sometimes device id ends with (*)... let's remove it
		int len = id.length();
		if(id.compare(len-3,4,"(*)") == 0)
			id = id.substr(0, len-3);
		deviceId = id;
	}
	
	return 0;
}


int setupDevicesNumAndId(LDSPinitSettings *settings, LDSPhwConfig *hwconfig)
{
	vector<string> deviceInfoPath_p;
	vector<string> deviceInfoPath_c;

	// get the paths to the info files of all devices 
	getDeviceInfoPaths(settings->card, "info", deviceInfoPath_p, deviceInfoPath_c);

	// if using an external card, we force the default device numbers to 0
	// because what we read from the hw config file applies to the embedded card only
	int defaultDevNum_p;
	int defaultDevNum_c;
	if(settings->card == 0)
	{
		defaultDevNum_p = hwconfig->default_dev_num_p;
		defaultDevNum_c = hwconfig->default_dev_num_c;
	}
	else 
	{
		defaultDevNum_p = 0;
		defaultDevNum_c = 0;
	}

	// playback
	if(setupDeviceNumAndId(deviceInfoPath_p, settings->deviceOutNum, defaultDevNum_p, settings->deviceOutId, hwconfig->default_dev_id_p)!=0)
		return -1;

	if(!settings->captureOff)
	{
		// capture
		if(setupDeviceNumAndId(deviceInfoPath_c, settings->deviceInNum, defaultDevNum_c, settings->deviceInId, hwconfig->default_dev_id_c)!=0)
			return -1;
	}

	return 0;
}

// these are 2 helper functions to quickly set mixer controls via their name
int mixerCtl_setInt(mixer *mx, const char *name, int val, int id, bool verbose) // input is number
{
	//printf("MIXER i----> %s %d\n", name, val);
	int ret = 0;
    struct mixer_ctl *ctl = mixer_get_ctl_by_name(mx, name);
    ret = mixer_ctl_set_value(ctl, id, val);
    if (verbose && ret != 0)
		fprintf(stderr, "Mixer: Failed to set %s to %d\n", name, val);
	return ret;
}
int mixerCtl_setStr(mixer *mx, const char *name, const char *val, bool verbose) // input is enum, passed as string
{
	//printf("MIXER s----> %s %s\n", name, val);
	int ret = 0;
    struct mixer_ctl *ctl = mixer_get_ctl_by_name(mx, name);
	ret = mixer_ctl_set_enum_by_string(ctl, val);
    if (verbose && ret != 0)
		fprintf(stderr, "Mixer: Failed to set %s to %s\n", name, val);
	return ret;
}

int setCtlFromXmlNode(mixer *mx, xml_node ctl_node, bool verbose)
{
	int ret = 0;

	string name = ctl_node.attribute("name").value();

	struct mixer_ctl *ctl = mixer_get_ctl_by_name(mx, name.c_str());
	mixer_ctl_type ctlType  = mixer_ctl_get_type(ctl);
	
	string value = ctl_node.attribute("value").value();
	
	//VIC not sure why, but these are not recognized as mixer enums
	if(value.compare("On") == 0)
		value = "1";
	else if(value.compare("Off") == 0)
		value = "0";

	if(isdigit(value.c_str()[0]))
	{
		// set everything else as int
		int id = 0;
		// not all nodes have a declared id
		if(!ctl_node.attribute("id").empty()) 
			id = ctl_node.attribute("id").as_int();
		int intValue = atoi(value.c_str());//ctl_node.attribute("value").as_int();
		ret += mixerCtl_setInt(mx, name.c_str(), intValue, id, verbose);
	}
	else //if(ctlType==MIXER_CTL_TYPE_ENUM)
		ret += mixerCtl_setStr(mx, name.c_str(), value.c_str(), verbose);
		
	return ret;
}

int setDefaultMixerPath(mixer *mx, xml_document *xml)
{
	// default path is defined as a series of controls laid out at the beginning of the mixer file
	// as 'ctl' nodes inside the 'mixer' node
	int ret = 0;
	for(xml_node ctl_node: xml->child("mixer").children("ctl"))
	{
		//VIC some of these controls will not work and the mixer will output some errors
		// but we don't care, this is vendor's problem and is harmless to us!
		// and this is why error messages from mixerCtl_set functions are disabled here [false argument]!
		ret += setCtlFromXmlNode(mx, ctl_node, false);
	}

	return ret;	
}

// first completes device activation control string and then activates device
int activateDevice(mixer *mx, LDSPhwConfig *hwconfig, string &deviceActivationCtl, string device_id)
{
	// make sure that there are no spaces in the name
	// in case, get rid of everything after the first space
	// size_t pos = device_id.find(" ");
	// string dev_id = device_id.substr(0, pos);
	size_t pos = trimDeviceInfo(device_id);
	
	// swap placeholder string with actual device id
	string placeholder = hwconfig->device_id_placeholder;
	pos = deviceActivationCtl.find(placeholder);
	if(pos== string::npos)
	{
		fprintf(stderr, "Device activation control \"%s\" in hw config file does not contain placeholder \"%s\"\n", deviceActivationCtl.c_str(), placeholder.c_str());
		return -1;
	}
	// update activation control
	deviceActivationCtl.replace(pos, placeholder.length(), /* dev_id */device_id);

	// use it to activate device!
	mixerCtl_setInt(mx, deviceActivationCtl.c_str(), 1);

	return 0;
}

void deactivateDevice(mixer *mx, string deviceActivationCtl)
{
	// much simpler, we just se the control that we have already!
	mixerCtl_setInt(mx, deviceActivationCtl.c_str(), 0);
}

int setPath(mixer *mx, xml_document *xml, string pathName, LDSPhwConfig *hwconfig, bool volumesPath=false) 
{
	int ret = 0;

	bool path_found =false;
	xml_node path;
	// first search for the requested path 
	for(xml_node path_node: xml->child("mixer").children("path"))
	{
		string pathNode_name = path_node.attribute("name").as_string();
		if(pathName.compare(pathNode_name) == 0)
		{
			path_found = true;
			path = path_node;
			break;
		}		
	}

	if(!path_found)
	{
		// if we are trying to set a volume path, it is possible that the current mixer path is not associated to any volume settings
		if(volumesPath)
			return 0;
		
		fprintf(stderr, "Cannot find path \"%s\" in xml file %s\n", pathName.c_str(), hwconfig->xml_paths_file.c_str());
        return -1;
	}

	// then go through all its children nodes
	for(xml_node inner_node = path.first_child(); inner_node; inner_node = inner_node.next_sibling())
	{
		string nodeName = inner_node.name();
		// if it's a ctl, set it!
		if(nodeName.compare("ctl") == 0)
			ret += setCtlFromXmlNode(mx, inner_node, true);
		
		// if it's a path, search for its definiteion and set all ctls in it!
		else if(nodeName.compare("path") == 0)
			ret += setPath(mx, xml, inner_node.attribute("name").value(), hwconfig, volumesPath); 
		else
		{
			string file = hwconfig->xml_paths_file;
			if(volumesPath)
				file = hwconfig->xml_volumes_file;
			fprintf(stderr, "XML file \"%s\" not well formatted! Unknown node %s\n", file.c_str(), nodeName.c_str());
        	return -2;
		}
	}
		
	return ret;
}


int secondaryDeviceActivation(mixer *mx, string pathName, string secondPath, string device, string devActCtl, string devActCtl2, LDSPhwConfig *hwconfig)
{
	// if there is a secondary device activation control, we should activate the device that matches the mixer path
	// if it was not supplied in the config file, then this is not necessary...
	if(!devActCtl2.empty())
	{
		// if mixer path is line-out or line_in
		if (pathName == secondPath)
		{
			// we call the secondary device activation control
			if(activateDevice(mix, hwconfig, devActCtl2, device) < 0)
			{
				LDSP_resetMixerPaths(hwconfig);
				return -1;
			}
		}
		else
		{
			// otherwise the main activation control
			if(activateDevice(mix, hwconfig, devActCtl, device) < 0)
			{
				LDSP_resetMixerPaths(hwconfig);
				return -1;
			}
		}
	}
	//BE CAREFUL! here we hardcode two important details:
	//_1 'line-out' and 'line-in' are the second paths in the 'path names' sections of the json config file
	//_2 the scondary device activation control is for line-out/line-in [a.k.a., heapdhones and headset mic]
	
	return 0;
}

int loadPath(mixer *mx, xml_document *xml, LDSPhwConfig *hwconfig, LDSPinitSettings *settings, string &pathAlias, bool isCapture)
{
	unordered_map<string, string> paths;
	unordered_map<string, int> paths_order;
	unordered_map<int, string> paths_rank;
	
	if(!isCapture) 
	{
		paths = hwconfig->paths_p;
		paths_order = hwconfig->paths_p_order;
		paths_rank = hwconfig->paths_p_rank;
	}
	else
	{
		paths = hwconfig->paths_c;
		paths_order = hwconfig->paths_c_order;
		paths_rank = hwconfig->paths_c_rank;
	}

	int pathIndex;
	string pathName;
	if(!pathAlias.empty())
	{
		// search name to which this alias is associated
		unordered_map<string, string>::iterator iter = paths.find(pathAlias);
		if(iter == paths.end())
		{
			fprintf(stderr, "Cannot find path alias \"%s\" in config file %s\n", pathAlias.c_str(), hwconfig->hw_confg_file.c_str());
			return -1;
		}
		pathName = iter->second;
		pathIndex = paths_order[iter->first]; // also store index for secondary activation
	}
	else
	{
		// use first path put in map!
		pathIndex = 0;
		pathAlias = paths_rank[pathIndex]; // alias of first element put in map, also useful for printouts where outside of function
		pathName = paths[pathAlias]; // name of first element put in map
	}

	// in some phones, paths are not needed, e.g., phones with no line-out/line-in socket that work via speaker by default
	// this means that there can be config files that have empty path names
	if(pathName=="")
	{
		if(mixerVerbose)
			printf("Setting empty path, with alias: %s\n", pathAlias.c_str());
		return 0;
	}

	// on some phones, a further activation is needed to trace the path from the chosen playback device to the physical destination [e.g., speaker]
	// or from the chosne capture device and the physical source [e.g., mic]
	// it may supplement or totally override the initial actication, depending on the phone
	string devActCtl2, deviceId;
	if(!isCapture)
	{
		deviceId = settings->deviceOutId;
		devActCtl2 = hwconfig->dev_activation_ctl2_p[pathIndex];
	}
	else
	{
		deviceId = settings->deviceInId;
		devActCtl2 = hwconfig->dev_activation_ctl2_c[pathIndex];
	}
	if(devActCtl2 != "") 
	{
		if(activateDevice(mx, hwconfig, devActCtl2, deviceId) < 0)
			return -5;
	}

	// set the chosen mixer path
	xml_document *xml_paths = &xml[0];
	if(setPath(mx, xml_paths, pathName, hwconfig)!=0)
		printf("Trying to move forward despite mixer issues...\n");// 	return -2;
		//VIC some mixer errors are not catastrophic, we can try going forward...

	if(!xml[1].empty())
	{
		// set the associated volume path
		xml_document *xml_volumes = &xml[1];
		setPath(mx, xml_volumes, pathName, hwconfig, true); // no check here, because the chosen mixer path may not be associated with any volume settings
	}

	return 0;
}