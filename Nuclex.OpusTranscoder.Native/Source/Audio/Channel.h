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

#include <Nuclex/Audio/ChannelPlacement.h>

#include <vector>

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Single audio channel storing samples and placement information</summary>
  /// <typeparam name="TSample">Type of samples the audio channel is storgin</typeparam>
  template<typename TSample>
  class Channel {

    /// <summary>Initializes a new audio channel with the specified sample count</summary>
    /// <param name="sampleCount">Number of samples the channel starts out with</param>
    public: Channel(std::size_t sampleCount = 0) :
      samples(),
      sampleRate(48000.0),
      placement(Nuclex::Audio::ChannelPlacement::Unknown) {}

    /// <summary>Retrieves the spatial placement of this channel</summary>
    /// <returns>The channel's spatial placement using Nuclex.Audio's flags</returns>
    public: Nuclex::Audio::ChannelPlacement GetChannelPlacement() const {
      return this->placement;
    }

    /// <summary>Changes the spatial placement intended for the channel</summary>
    /// <param name="placement">Spatial placement</param>
    public: void SetChannelPlacement(Nuclex::Audio::ChannelPlacement placement) {
      this->placement = placement;
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
    private: Nuclex::Audio::ChannelPlacement placement;

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder

#endif // NUCLEX_OPUSTRANSCODER_AUDIO_CHANNEL_H
