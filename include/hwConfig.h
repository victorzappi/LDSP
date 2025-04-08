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
#include <string>
#include <vector> // vector

using std::string;
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
    unordered_map<string, int> paths_p_order;
    unordered_map<int, string> paths_p_rank;
    unordered_map<string, string> paths_c;
    unordered_map<string, int> paths_c_order;
    unordered_map<int, string> paths_c_rank;
    int default_dev_num_p;
    int default_dev_num_c;
    string default_dev_id_p;
    string default_dev_id_c;
    int default_period_size;
    int default_chn_num_p;
    int default_chn_num_c;
    string dev_activation_ctl_p;
    string dev_activation_ctl_c;
    
    //string dev_activation_ctl2_p; // secondary playback device activation control, mostly used for line-out
    //string dev_activation_ctl2_c; // secondary capture device activation control, mostly used for line-in
    
    vector<string> dev_activation_ctl2_p;
    vector<string> dev_activation_ctl2_c;
    
    string *ctrl_outputs[2]; // control file and max value (or max file)
};


#endif /* HW_CONFIG_H_ */
