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

#include "LDSP.h"
#include "RTNeuralLSTM.h"
#include "MonoFilePlayer.h"
#include <libraries/Gui/Gui.h>
#include <libraries/GuiController/GuiController.h>

// This code example uses this sound from freesound:
// Clean Electric Guitar by guitarman213 -- https://freesound.org/s/715794/ -- License: Creative Commons 0
// the original file has been normalized, exported as a mono track and resampled at 48 kHz


string filename = "715794__guitarman213__clean-electric-guitar_mono_norm_48k.wav";
float guitarVol = 0.5;
float drive = 0.5;
float ampVol = 0.5;

//------------------------------------

MonoFilePlayer player;
RT_LSTM model;
Gui gui;
GuiController controller;

bool setup(LDSPcontext *context, void *userData)
{
    // load the audio file
    if( !player.setup(filename, true, true) ) 
    {
        printf("Error loading audio file '%s'\n", filename.c_str());
        return false;
    }

    model.load_json("TS9.json"); // model's dictionary sourced from: https://github.com/GuitarML/NeuralPi
    model.reset();

    gui.setup(context->projectName);
    controller.setup(&gui, "Control parameters");
    controller.addSlider("Guitar Volume", guitarVol, 0, 1, 0);
    controller.addSlider("Drive (conditioning)", map(drive, 0, 1, 0, 11), 0, 11, 0);
    controller.addSlider("Amp Volume", ampVol, 0, 1, 0);

    return true;
}

void render(LDSPcontext *context, void *userData)
{
    guitarVol = controller.getSliderValue(0);
    drive = controller.getSliderValue(1);
    drive = map(drive, 0, 11, 0, 1);
    ampVol = controller.getSliderValue(2);

    float output[] = {0};
    for(int n=0; n<context->audioFrames; n++)
    {
        float original = player.process()*guitarVol;

        const float input[] = {original};
        model.process(input, drive, output, 1);

        output[0] *= ampVol;

        audioWrite(context, n, 0, output[0]);
        audioWrite(context, n, 1, output[0]);
    }
}

void cleanup(LDSPcontext *context, void *userData)
{

}
