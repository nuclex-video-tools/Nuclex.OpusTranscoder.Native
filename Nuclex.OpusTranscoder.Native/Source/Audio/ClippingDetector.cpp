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

#include "./ClippingDetector.h"

namespace {

  // ------------------------------------------------------------------------------------------- //
  // ------------------------------------------------------------------------------------------- //

} // anonymous namespace

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  void ClippingDetector::FindClippingHalfwaves(
    const std::shared_ptr<Track> &track,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
    Nuclex::Support::Events::Delegate<void(float)> &progressCallback
  ) {
    std::size_t channelCount = track->Channels.size();
    std::size_t frameCount = track->Samples.size() / channelCount;
    for(std::size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
      Channel &channel = track->Channels[channelIndex];
      const float *read = track->Samples.data() + channelIndex;

      channel.ClippingHalfwaves.clear();

      float clippingPeak = *read;
      std::size_t clippingPeakIndex = 0;
      std::size_t wasClipping = (1.0f < std::abs(clippingPeak));

      bool wasBelowZero = (clippingPeak < 0.0f);
      std::size_t zeroCrossingIndex = 0;

      for(std::size_t index = 1; index < frameCount; ++index) {
        float sample = *read;

        bool isBelowZero = (sample < 0.0f);
        if(wasBelowZero != isBelowZero) {
          if(wasClipping) {
            channel.ClippingHalfwaves.emplace_back(
              zeroCrossingIndex, clippingPeakIndex, index, clippingPeak
            );

            wasClipping = false;
            clippingPeak = 0.0f;
          }

          zeroCrossingIndex = index;
          wasBelowZero = isBelowZero;
        } // is current sample cross zero line

        bool isClipping = (1.0f < std::abs(sample));
        if(isClipping) {
          wasClipping = true;
          if(clippingPeak < std::abs(sample)) {
            clippingPeak = std::abs(sample);
            clippingPeakIndex = index;
          }
        } // if current sample is clipping

        if((index & 0x2fff) == 0) {
          canceler->ThrowIfCanceled();
          progressCallback(
            (static_cast<float>(channelIndex) / static_cast<float>(channelCount)) +
            (static_cast<float>(index) / static_cast<float>(frameCount) / channelCount)
          );
        }
      } // for each sample in the channel

      if(wasClipping) {
        channel.ClippingHalfwaves.emplace_back(
          zeroCrossingIndex, clippingPeakIndex, frameCount - 1, clippingPeak
        );
      }
    } // for each channel
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio
