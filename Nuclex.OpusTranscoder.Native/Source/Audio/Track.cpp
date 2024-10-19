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

// If the application is compiled as a DLL, this ensures symbols are exported
#define NUCLEX_OPUSTRANSCODER_SOURCE 1

#include "./Track.h"

#if !defined(NDEBUG)
#include <Nuclex/Audio/Processing/DecibelConverter.h>
#include <iostream>
#endif

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  void Track::CopyClippingHalfwavesInto(
    const std::shared_ptr<Track> &otherTrack
  ) const {
    if(this->Channels.size() != otherTrack->Channels.size()) {
      throw std::logic_error(
        u8"Unable to copy, other audio track has mismatching channel layout"
      );
    }

    for(std::size_t index = 0; index < this->Channels.size(); ++index) {
      otherTrack->Channels[index].ClippingHalfwaves = this->Channels[index].ClippingHalfwaves;
    }
  }

  // ------------------------------------------------------------------------------------------- //

  void Track::DebugOutputAllClippingHalfwaves() {
#if !defined(NDEBUG)
    using Nuclex::Audio::Processing::DecibelConverter;

    std::size_t channelCount = this->Channels.size();
    for(std::size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
      std::string channelName = Nuclex::Audio::StringFromChannelPlacement(
        this->Channels[channelIndex].Placement
      );
      std::cout << "Channel " << channelName << std::endl;

      std::size_t clipCount = this->Channels[channelIndex].ClippingHalfwaves.size();
      for(std::size_t clipIndex = 0; clipIndex < clipCount; ++clipIndex) {
        ClippingHalfwave &halfwave = this->Channels[channelIndex].ClippingHalfwaves[clipIndex];

        float decibels = DecibelConverter::FromLinearAmplitude(halfwave.PeakAmplitude);

        std::cout << "\tClipping Halfwave " << (clipIndex + 1)
                  << " [" << std::hex << halfwave.PeakIndex << std::dec << "] -> "
                  << (1.0f < halfwave.PeakAmplitude ? "open" : "fixed") << std::endl;

        std::cout << "\t\tPeak: " << halfwave.PeakAmplitude
                  << " (" << decibels << " dB) at "
                  << halfwave.PeakIndex << std::endl;

        std::cout << "\t\tVolume quotient: " << halfwave.VolumeQuotient << std::endl;

        std::cout << "\t\tIneffective adjustment count: " << halfwave.IneffectiveIterationCount << std::endl;
      }

    }
#endif
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio

