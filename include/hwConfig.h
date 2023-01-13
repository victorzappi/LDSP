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
#ifndef HW_CONFIG_H_
#define HW_CONFIG_H_

#include <unordered_map> // unordered_map
#include <vector> // vector

#include "LDSP.h"

using std::vector;
using std::unordered_map;

#define DEVICE_CTRL_FILE 0
#define DEVICE_SCALE 1

struct LDSPhwConfig {
    string hw_confg_file;
    string device_id_placeholder;
    string xml_paths_file;
    string xml_volumes_file;
    unordered_map<string, string> paths_p;
    vector<string> paths_p_order;
    unordered_map<string, string> paths_c;
    vector<string> paths_c_order;
    int default_dev_p;
    int default_dev_c;
    string deviceActivationCtl_p;
    string deviceActivationCtl_c;
    string *analogCtrlOutputs[2]; // control file and max value (or max file)
};

LDSPhwConfig* LDSP_HwConfig_alloc();

void LDSP_HwConfig_free(LDSPhwConfig* hwconfig);

int LDSP_parseHwConfigFile(LDSPinitSettings *settings, LDSPhwConfig *hwconfig);

#endif /* HW_CONFIG_H_ */
