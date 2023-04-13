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
#include <vector>

/**
 *@brief A collection of functions for loading and storing audio files.
 *
 * A collection of functions for loading and storing audio files.
 */

namespace AudioFileUtilities {
	/**
	 * @addtogroup AudioFileUtilities
	 */
	/**
	 * Load audio frames between @p startFrame and @p endFrame from @p
	 * channel of the specified @p file into the preallocated memory
	 * location @p buf.
	 *
	 * @return 0 on success, or an error code upon failure.
	 */
	int getSamples(const std::string& file, float *buf, unsigned int channel, unsigned int startFrame, unsigned int endFrame);
	/**
	 * Get the number of audio channels in @p file.
	 *
	 * @return the number of audio channels on success, or a negative error
	 * code upon failure.
	 */
	int getNumChannels(const std::string& file);
	/**
	 * Get the number of audio frames in @p file.
	 *
	 * @return the number of frames on success, or a negative error code
	 * upon failure.
	 */
	int getNumFrames(const std::string& file);
	/**
	 * Store samples from memory into an audio file on disk.
	 *
	 * @param filename the file to write to
	 * @param buf a vector containing \p channels * \p frames of interlaved data
	 * @param channels the channels in the data and output file
	 * @param frames the frames in the data and output file
	 * @param sampleRate the sampling rate of the data
	 */
	int write(const std::string& filename, float *buf, unsigned int channels, unsigned int frames, unsigned int samplerate);
	/**
	 * Write non-interlaved samples from memory into an audio file on disk.
	 *
	 * @param filename the file to write to
	 * @param dataIn a vector containing one vector of data per channel
	 * @param sampleRate the sampling rate of the data
	 *
	 */
	int write(const std::string& filename, const std::vector<std::vector<float> >& dataIn, unsigned int sampleRate);
	/**
	 * Load audio samples from a file into memory.
	 *
	 * Loads at most \p count samples from each channel of \p filename,
	 * starting from frame \p start.
	 *
	 * @param filename the file to load
	 * @param maxCount the maximum number of samples to load from each
	 * channel. Pass a negative value for no limit.
	 * @param start the first sample to load.
	 *
	 * @return a vector containing one vector of data per channel.
	 */
	std::vector<std::vector<float> > load(const std::string& filename, int maxCount = -1, unsigned int start = 0);
	/**
	 * Load audio samples from a file into memory.
	 *
	 * Simplified version of write(), which only loads the first channel of
	 * the file.
	 */
	std::vector<float> loadMono(const std::string& file);
};
