// This code is based on the code credited below, but it has been modified
// further by Victor Zappi

 /*
 ___  _____ _        _
| __ )| ____| |      / \
|  _ \|  _| | |     / _ \
| |_) | |___| |___ / ___ \
|____/|_____|_____/_/   \_\

The platform for ultra-low latency audio and sensor processing

http://bela.io

A project of the Augmented Instruments Laboratory within the Centre for Digital Music at Queen Mary University of London. http://instrumentslab.org

(c) 2016-2020 Augmented Instruments Laboratory: Andrew McPherson, Astrid Bin, Liam Donovan, Christian Heinrichs, Robert Jack, Giulio Moro, Laurel Pardue, Victor Zappi. All rights reserved.

The Bela software is distributed under the GNU Lesser General Public License (LGPL 3.0), available here: https://www.gnu.org/licenses/lgpl-3.0.txt */


#pragma once

#include <vector>
#include <string>
#include <libraries/Gui/Gui.h>
#include "GuiSlider.h"
#include "libraries/JSON/json.hpp"

// Forward declarations
class Gui;

class GuiController {
	private:
		std::vector<GuiSlider> _sliders;
		Gui *_gui;
		std::string _name;

		int sendController();
		int sendSlider(const GuiSlider& slider);
		int sendSliderValue(int sliderIndex);

	public:
		GuiController();
		~GuiController();

		GuiController(Gui *gui, std::string name);

		int setup(Gui *gui, std::string name);

		int setup();
		void cleanup();
		int addSlider(std::string name, float value = 0.5f, float min = 0.0f, float max = 1.0, float step = 0.001f);

		float getSliderValue(int sliderIndex);
		int setSliderValue(int sliderIndex, float value);
		GuiSlider& getSlider(int sliderIndex) { return _sliders.at(sliderIndex); };

		std::string getName() { return _name; };

		int getNumSliders() { return _sliders.size(); };

		static bool controlCallback(nlohmann::json &root, void* param);
};
