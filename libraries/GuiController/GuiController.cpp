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


#include "GuiController.h"
#include <iostream>

GuiController::GuiController()
{
}

GuiController::GuiController(Gui *gui, std::string name)
{
	setup(gui, name);
}

int GuiController::setup(Gui* gui, std::string name)
{
	_gui = gui;
	_name = name;
	_gui->setControlDataCallback(controlCallback, this);
	int ret = sendController();
	return ret;
}

int GuiController::sendController()
{
    nlohmann::json root;
    root["event"] = "set-controller";
    root["name"] = _name; // Assuming _name is a std::string
    return _gui->sendControl(root);
}


void GuiController::cleanup()
{
}

GuiController::~GuiController()
{
	cleanup();
}

bool GuiController::controlCallback(nlohmann::json &root, void* param)
{
	GuiController* controller = static_cast<GuiController*>(param);

    // Check for the 'event' field and its type
    if (root.contains("event") && root["event"].is_string())
    {
        std::string event = root["event"].get<std::string>();

        if (event == "connection-reply")
        {
            controller->sendController();
            if (controller->getNumSliders() != 0)
            {
                for (auto& slider : controller->_sliders)
                {
                    controller->sendSlider(slider);
                }
            }
        }
        else if (event == "slider")
        {
            int sliderIndex = -1;
            float sliderValue = 0.0f;

            if (root.contains("slider") && root["slider"].is_number())
            {
                sliderIndex = root["slider"].get<int>();
            }

            if (root.contains("value") && root["value"].is_number())
            {
                sliderValue = root["value"].get<float>();
                controller->_sliders.at(sliderIndex).setValue(sliderValue);
            }
        }
    }

    return true;
}

int GuiController::sendSlider(const GuiSlider& slider)
{
    nlohmann::json root = slider.getParametersAsJSON();
    root["event"] = "set-slider";
    root["controller"] = _name;
    return _gui->sendControl(root);
}

int GuiController::sendSliderValue(int sliderIndex)
{
    auto& slider = _sliders.at(sliderIndex);
    nlohmann::json root;
    root["event"] = "set-slider-value";
    root["controller"] = _name;
    root["index"] = slider.getIndex();
    root["name"] = slider.getName(); // Assuming getName() returns std::string
    root["value"] = slider.getValue();
    return _gui->sendControl(root.dump());
}


int GuiController::addSlider(std::string name, float value, float min, float max, float step)
{
	_sliders.push_back(GuiSlider(name, value, min, max, step));
	_sliders.back().setIndex(getNumSliders() - 1);
	return _sliders.back().getIndex();
}

float GuiController::getSliderValue(int sliderIndex)
{
	return _sliders.at(sliderIndex).getValue();
}

int GuiController::setSliderValue(int sliderIndex, float value)
{
	auto& s = _sliders.at(sliderIndex);
	s.setValue(value);
	return sendSliderValue(sliderIndex);
}
