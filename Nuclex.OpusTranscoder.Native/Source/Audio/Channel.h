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

#include "../Platform/SndFileApi.h"

#include <vector>

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Audio channel 
  template<typename TSample>
  class Channel {

    /// <summary>Initializes a new audio channel with the specified sample count</summary>
    /// <param name="sampleCount">Number of samples the channel starts out with</param>
    public: Channel(std::size_t sampleCount = 0) :
      samples(),
      sampleRate(48000.0),
      placement(SF_CHANNEL_MAP_INVALID) {}

    /// <summary>Retrieves the spatial placement of this channel</summary>
    /// <returns>The channel's spatial placement using libsndfile's channel map</returns>
    /// <remarks>
    ///   SF_CHANNEL_MAP_INVALID
    ///   SF_CHANNEL_MAP_MONO
    ///   SF_CHANNEL_MAP_LEFT
    ///   SF_CHANNEL_MAP_RIGHT
    ///   SF_CHANNEL_MAP_CENTER
    ///   SF_CHANNEL_MAP_FRONT_LEFT
    ///   SF_CHANNEL_MAP_FRONT_RIGHT
    ///   SF_CHANNEL_MAP_FRONT_CENTER
    ///   SF_CHANNEL_MAP_REAR_CENTER
    ///   SF_CHANNEL_MAP_REAR_LEFT
    ///   SF_CHANNEL_MAP_REAR_RIGHT
    ///   SF_CHANNEL_MAP_LFE
    ///   SF_CHANNEL_MAP_FRONT_LEFT_OF_CENTER
    ///   SF_CHANNEL_MAP_FRONT_RIGHT_OF_CENTER
    ///   SF_CHANNEL_MAP_SIDE_LEFT
    ///   SF_CHANNEL_MAP_SIDE_RIGHT
    ///   SF_CHANNEL_MAP_TOP_CENTER
    ///   SF_CHANNEL_MAP_TOP_FRONT_LEFT
    ///   SF_CHANNEL_MAP_TOP_FRONT_RIGHT
    ///   SF_CHANNEL_MAP_TOP_FRONT_CENTER
    ///   SF_CHANNEL_MAP_TOP_REAR_LEFT
    ///   SF_CHANNEL_MAP_TOP_REAR_RIGHT
    ///   SF_CHANNEL_MAP_TOP_REAR_CENTER
    /// </remarks>
    public: int GetChannelPlacement() const { return this->placement; }

    /// <summary>Changes the spatial placement intended for the channel</summary>
    /// <param name="placementFromLibSndFile">Spatial placement using the libsndfile map</param>
    public: void SetChannelPlacement(int placementFromLibSndFile) {
      this->placement = placementFromLibSndFile;
    }

    /// <summary>Retrieves the sample rate the channel is played back at</summary>
    /// <returns>The channel's playback sample rate</returns>
    public: double GetSampleRate() const { return this->sampleRate; }

    /// <summary>Changes the sample rate the channel is intended to be played back at</summary>
    /// <param name="sampleRate">New sample rate the channel should be played at</param>
    public: void SetSampleRate(double sampleRate) {
      this->sampleRate = sampleRate;
    }

    /// <summary>Appends samples to the channel</summary>
    /// <param name="samples">Pointer to the first sample that will be appended</param>
    /// <param name="count">Number of samples that will be appended in total</param>
    /// <param name="skipCount">Number of samples to skip after each appended sample</param>
    public: void AppendSamples(
      const TSample *samples, std::size_t count, std::size_t skipCount = 0
    ) {
      std::size_t appendIndex = this->samples.size();
      this->samples.resize(appendIndex + count);

      TSample *target = this->samples.data();
      for(std::size_t index = 0; index < count; ++index) {
        *target = *samples;
        ++target;
        samples += skipCount;
      }
    }

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
