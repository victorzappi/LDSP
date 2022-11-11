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
#include <fstream> // files
#include <sstream> // ostringstream

#include <tinyalsa/asoundlib.h>

#include "mixer.h"
#include "libraries/XML/pugixml.hpp"


using namespace std;
using namespace pugi;

bool mixerVerbose = false;

mixer *mix = nullptr;

int mixerCtl_setInt(mixer *mx, const char *name, int val, int id=0, bool verbose=true);
int mixerCtl_setStr(mixer *mx, const char *name, const char *val, bool verbose=true);
int setDefaultMixerPath(mixer *mx, xml_document *xml);
int getDeviceId(int card, int device, string &pcmid, bool is_playback);
int activateDevice(mixer *mx, string &deviceActivationCtl, string device_id, LDSPhwConfig *hwconfig);
void deactivateDevice(mixer *mx, string deviceActivationCtl);
int loadPath(mixer *mx, xml_document *xml, unordered_map<string, string> *paths, vector<string> *paths_order, string &pathAlias, LDSPhwConfig *hwconfig);



int LDSP_setMixerPaths(LDSPinitSettings *settings, LDSPhwConfig *hwconfig)
{
	if(!settings)
		return -1;
    
    mixerVerbose = settings->verbose;

	if(mixerVerbose)
		printf("\nLDSP_setMixerPaths()\n");

	// set default devices, using values from config file
	if(settings->deviceOut == -1)
		settings->deviceOut = hwconfig->default_dev_p ;
	if(settings->deviceIn == -1)
		settings->deviceIn = hwconfig->default_dev_c ;

	// open mixer
	mix = mixer_open(settings->card);
    if (!mix || mix == nullptr) 
	{
        fprintf(stderr, "Failed to open mixer\n");
		LDSP_resetMixerPaths(hwconfig);
        return -2;
    }


	// use XML to load paths
	xml_document mixer_docs[2]; // [0] -> mixer paths, [1] -> mixer volumes (optional)
   
    // load the mixer paths XML file
    if(!mixer_docs[0].load_file(hwconfig->xml_paths_file.c_str()))
	{
		LDSP_resetMixerPaths(hwconfig);
		return -3;
	}
	// load the mixer volumes XML file, if present
	if(!hwconfig->xml_volumes_file.empty())
	{
		if(!mixer_docs[1].load_file(hwconfig->xml_volumes_file.c_str()))
		{
			LDSP_resetMixerPaths(hwconfig);
			return -3;
		}
	} 
		

	// always start from loading default paths
	setDefaultMixerPath(mix, &mixer_docs[0]);


	// get id of both playback and capture device
	getDeviceId(settings->card, settings->deviceOut, settings->deviceOutId, true);
	getDeviceId(settings->card, settings->deviceIn, settings->deviceInId, false);
	
	// if necessary, activate devices
	if(!hwconfig->deviceActivationCtl_p.empty())
	{
		if(activateDevice(mix, hwconfig->deviceActivationCtl_p , settings->deviceOutId, hwconfig) < 0)
		{
			LDSP_resetMixerPaths(hwconfig);
			return -4;
		}
	}
	if(!settings->outputOnly)
	{
		if(!hwconfig->deviceActivationCtl_c.empty())
		{
			if(activateDevice(mix, hwconfig->deviceActivationCtl_c , settings->deviceInId, hwconfig) < 0)
			{
				LDSP_resetMixerPaths(hwconfig);
				return -4;
			}
		}
	}

	// set actual playback and capture paths
	string pathAlias = settings->pathOut;
	unordered_map<string, string> *paths = &hwconfig->paths_p;
	vector<string> *paths_order = &hwconfig->paths_p_order;
	if(loadPath(mix, mixer_docs, paths, paths_order, pathAlias, hwconfig)!=0)
	{
		fprintf(stderr, "Playback path error\n");
		LDSP_resetMixerPaths(hwconfig);
		return -5;
	}
	if(mixerVerbose)
		printf("Playback path loaded: \"%s\"\n", pathAlias.c_str());

	if(!settings->outputOnly)
	{
		pathAlias = settings->pathIn;
		paths = &hwconfig->paths_c;
		paths_order = &hwconfig->paths_c_order;
		if(loadPath(mix, mixer_docs, paths, paths_order, pathAlias, hwconfig)!=0)
		{
			fprintf(stderr, "Capture path error\n");
			LDSP_resetMixerPaths(hwconfig);
			return -5;
		}
		if(mixerVerbose)
			printf("Capture path loaded: \"%s\"\n", pathAlias.c_str());
	}
	
	return 0;
}


void LDSP_resetMixerPaths(LDSPhwConfig *hwconfig)
{
	if(mixerVerbose)
		printf("LDSP_resetMixerPaths()\n");

	xml_document mixer_docs;
    if(mixer_docs.load_file(hwconfig->xml_paths_file.c_str()))
		setDefaultMixerPath(mix, &mixer_docs); // deactivates devices too, if necessary on phone

	if (mix != nullptr) 
		mixer_close(mix);
}

//----------------------------------------------------------------------------------


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

int getDeviceId(int card, int device, string &pcmid, bool is_playback)
{
	// assemble file name
	// the id of the device/pcm will be in "/proc/asound/cardX/pcmY_/info" where _ = p or c
	std::ostringstream namestream;
	char dir;
	if(is_playback)
		dir = 'p'; // playback
	else
		dir = 'c';  // capture
	namestream << "/proc/asound/card" << card <<"/pcm" << device << dir << "/info"; 
	string pcmfilename = namestream.str();

	// open file
	std::ifstream pcmfile_;
	pcmfile_.open(pcmfilename);
	if ( !pcmfile_.is_open() )
	{
		printf("Warning! Could not open \"%s\" to retrieve id\n", pcmfilename.c_str());
		return -1;
	}

	// the id is preceeded by this substring
	string check_str = "id: ";
	string line;
	// let's look for it
	while(pcmfile_) 
	{
		getline(pcmfile_, line);
		size_t pos = line.find(check_str);
		if(pos!= string::npos)
		{
			pcmid = line.substr(pos+check_str.length()); // remove substring
			return 0;
		}
	}

	return -2;
}


// first completes device activation control string and then activates device
int activateDevice(mixer *mx, string &deviceActivationCtl, string device_id, LDSPhwConfig *hwconfig)
{
	// make sure that there no spaces in the name
	// in case, get rid of everything after the first space
	size_t pos = device_id.find(" ");
	string dev_id = device_id.substr(0, pos);
	
	// swap placeholder string with actual device id
	string placeholder = hwconfig->device_id_placeholder;
	pos = deviceActivationCtl.find(placeholder);
	if(pos== string::npos)
	{
		fprintf(stderr, "Device activation control \"%s\" in hw config file does not contain placeholder \"%s\"\n", deviceActivationCtl.c_str(), placeholder.c_str());
		return -1;
	}
	// update activation control
	deviceActivationCtl.replace(pos, placeholder.length(), dev_id);

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


int loadPath(mixer *mx, xml_document *xml, unordered_map<string, string> *paths, vector<string> *paths_order, string &pathAlias, LDSPhwConfig *hwconfig)
{
	string pathName;
	if(!pathAlias.empty())
	{
		// search id
		unordered_map<string, string>::iterator iter = paths->find(pathAlias);
		if(iter == paths->end())
		{
			fprintf(stderr, "Cannot find path alias \"%s\" in config file %s\n", pathAlias.c_str(), hwconfig->hw_confg_file.c_str());
			return -1;
		}
		pathName = iter->second;
	}
	else
	{
		pathName = (*paths)[(*paths_order)[0]]; // get id of first element put in map
		pathAlias = (*paths_order)[0]; // get also alias of first element put in map, for printouts where outside of function
	}

	// set the chosen mixer path
	xml_document *xml_paths = &xml[0];
	if(setPath(mx, xml_paths, pathName, hwconfig)!=0)
		return -2;

	if(!xml[1].empty())
	{
		// set the associated volume path
		xml_document *xml_volumes = &xml[1];
		setPath(mx, xml_volumes, pathName, hwconfig, true); // no check here, because the chosen mixer path may not be associated with any volume settings
	}

	return 0;
}