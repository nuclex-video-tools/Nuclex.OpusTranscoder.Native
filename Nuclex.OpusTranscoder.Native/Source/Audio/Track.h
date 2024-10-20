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

#ifndef NUCLEX_OPUSTRANSCODER_AUDIO_TRACK_H
#define NUCLEX_OPUSTRANSCODER_AUDIO_TRACK_H

#include "../Config.h"

#include "./Channel.h"

#include <vector>
#include <memory> // for std::shared_ptr

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Audio track with all data uncompressed in memory</summary>
  class Track {

    ///<summary>Initializes a new audio track</summary>
    public: Track() :
      Samples(),
      Channels(),
      Iteration(0) {}

    /// <summary>Copies the clipping data into another track</summary>
    /// <param name="otherTrack">Track the clipping data will be copied into</param>
    public: void CopyClippingHalfwavesInto(
      const std::shared_ptr<Track> &otherTrack
    ) const;

    /// <summary>Provides a pointer to the specified interleaved sample</summary>
    /// <param name="channelIndex">Index of the channel the sample is in</param>
    /// <param name="sampleIndex">Index of the sample inside the channel</param>
    /// <returns>A pointer to the requested sample</returns>
    public: inline float *SampleAt(
      std::size_t channelIndex, std::uint64_t sampleIndex
    );

    /// <summary>Provides a pointer to the specified interleaved sample</summary>
    /// <param name="channelIndex">Index of the channel the sample is in</param>
    /// <param name="sampleIndex">Index of the sample inside the channel</param>
    /// <returns>A pointer to the requested sample</returns>
    public: inline const float *SampleAt(
      std::size_t channelIndex, std::uint64_t sampleIndex
    ) const;

    /// <summary>Prints a summary of all clipping half-waves to stdout</summary>
    public: void DebugOutputAllClippingHalfwaves();

    /// <summary>Stores the decoded samples of all channels</summary>
    public: std::vector<float> Samples;
    /// <summary>Samples per second the track plays at</summary>
    public: std::size_t SampleRate;
    /// <summary>Data about the channels and clipping found in each of them</summary>
    public: std::vector<Channel> Channels;

    /// <summary>Current iteration the iterative declipper is processing</summary>
    public: std::size_t Iteration;

  };

  // ------------------------------------------------------------------------------------------- //

  inline float *Track::SampleAt(std::size_t channelIndex, std::uint64_t sampleIndex) {
    return (
      this->Samples.data() + // Pointer to start of interleaved sample array
      channelIndex + // Start at first sample of selected channel
      (sampleIndex * this->Channels.size()) // Go forward in interleaved samples
    );
  }

  // ------------------------------------------------------------------------------------------- //

  inline const float *Track::SampleAt(std::size_t channelIndex, std::uint64_t sampleIndex) const {
    return (
      this->Samples.data() + // Pointer to start of interleaved sample array
      channelIndex + // Start at first sample of selected channel
      (sampleIndex * this->Channels.size()) // Go forward in interleaved samples
    );
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio

#endif // NUCLEX_OPUSTRANSCODER_AUDIO_TRACK_H
