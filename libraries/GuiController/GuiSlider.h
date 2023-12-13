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

#include <string>
#include "libraries/JSON/json.hpp"

class GuiSlider {
	private:
		int _index = -1;
		float _value;
		float _range[2];
		float _step;
		bool _changed = false;
		std::string _name;
		std::wstring _wname;

	public:
		GuiSlider() {};
		GuiSlider(std::string name, float val = 0.5, float min = 0.0, float max = 0.1, float step = 0.001);
		int setup(std::string name, float val, float min, float max, float step);
		void cleanup() {};
		~GuiSlider();

		/* Getters */
		float getValue() {
			_changed = false;
			return _value;
		};
		float getStep() { return _step; };
		std::string& getName() { return _name; };
		std::wstring& getNameW() { return _wname; }
		float getMin() { return _range[0]; };
		float getMax() { return _range[1]; };
		float getIndex() { return _index; };
		bool hasChanged() { return _changed; };

		/* Setters */
		int setValue(float val);
		int setStep(float step);
		int setRange(float min, float max);
		int setIndex(int index) { return (index < 0) ? -1 : _index=index; };

		nlohmann::json getParametersAsJSON() const;
};
