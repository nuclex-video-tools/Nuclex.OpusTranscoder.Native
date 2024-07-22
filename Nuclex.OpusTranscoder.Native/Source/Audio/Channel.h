#pragma region Apache License 2.0
/*
Nuclex Opus Transcoder
Copyright (C) 2024 Markus Ewald / Nuclex Development Labs

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#pragma endregion // Apache License 2.0

#ifndef NUCLEX_OPUSTRANSCODER_AUDIO_CHANNEL_H
#define NUCLEX_OPUSTRANSCODER_AUDIO_CHANNEL_H

#include "../Config.h"

#include <vector>

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Audio channel 
  template<typename TSample>
  class Channel {

    public: Channel(std::size_t sampleCount = 0);

/*
  SF_CHANNEL_MAP_INVALID = 0,
	SF_CHANNEL_MAP_MONO = 1,
	SF_CHANNEL_MAP_LEFT,					// Apple calls this 'Left'
	SF_CHANNEL_MAP_RIGHT,					// Apple calls this 'Right'
	SF_CHANNEL_MAP_CENTER,					// Apple calls this 'Center'
	SF_CHANNEL_MAP_FRONT_LEFT,
	SF_CHANNEL_MAP_FRONT_RIGHT,
	SF_CHANNEL_MAP_FRONT_CENTER,
	SF_CHANNEL_MAP_REAR_CENTER,				// Apple calls this 'Center Surround', Msft calls this 'Back Center'
	SF_CHANNEL_MAP_REAR_LEFT,				// Apple calls this 'Left Surround', Msft calls this 'Back Left'
	SF_CHANNEL_MAP_REAR_RIGHT,				// Apple calls this 'Right Surround', Msft calls this 'Back Right'
	SF_CHANNEL_MAP_LFE,						// Apple calls this 'LFEScreen', Msft calls this 'Low Frequency' 
	SF_CHANNEL_MAP_FRONT_LEFT_OF_CENTER,	// Apple calls this 'Left Center'
	SF_CHANNEL_MAP_FRONT_RIGHT_OF_CENTER,	// Apple calls this 'Right Center
	SF_CHANNEL_MAP_SIDE_LEFT,				// Apple calls this 'Left Surround Direct'
	SF_CHANNEL_MAP_SIDE_RIGHT,				// Apple calls this 'Right Surround Direct'
	SF_CHANNEL_MAP_TOP_CENTER,				// Apple calls this 'Top Center Surround'
	SF_CHANNEL_MAP_TOP_FRONT_LEFT,			// Apple calls this 'Vertical Height Left'
	SF_CHANNEL_MAP_TOP_FRONT_RIGHT,			// Apple calls this 'Vertical Height Right'
	SF_CHANNEL_MAP_TOP_FRONT_CENTER,		// Apple calls this 'Vertical Height Center'
	SF_CHANNEL_MAP_TOP_REAR_LEFT,			// Apple and MS call this 'Top Back Left'
	SF_CHANNEL_MAP_TOP_REAR_RIGHT,			// Apple and MS call this 'Top Back Right'
	SF_CHANNEL_MAP_TOP_REAR_CENTER,			// Apple and MS call this 'Top Back Center'

*/
    public: int GetChannelPlacement() const;
    public: void SetChannelPlacement(int placement);
    
    public: void AppendSamples(const TSample *samples, std::size_t count);
    public: void AppendSamples(const std::vector<TSample> &samples);

    //public: void ReadSamples(


    /// <summary>Audio samples that describe the waveform to play back</summary>
    private: std::vector<TSample> samples;
    /// <summary>Intended playback speed in samples per second</summary>
    private: double sampleRate;
    /// <summary>Where this channel should be audible in relation to the listener</summary>
    private: int placement;

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder

#endif // NUCLEX_OPUSTRANSCODER_AUDIO_CHANNEL_H
