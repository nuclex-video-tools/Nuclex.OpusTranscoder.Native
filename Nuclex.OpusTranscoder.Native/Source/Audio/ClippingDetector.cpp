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

    // We'll process each channel separately, otherwise keeping track of the start and
    // end indices of each clipping half-wave becomes just to complicated...
    for(std::size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
      Channel &channel = track->Channels[channelIndex];
      const float *read = track->Samples.data() + channelIndex;

      // In case this method is run repeatedly, clear the previous clipping instances
      channel.ClippingHalfwaves.clear();

      float clippingPeak = *read;
      std::size_t clippingPeakIndex = 0;
      std::size_t wasClipping = (1.0f < std::abs(clippingPeak));

      bool wasBelowZero = (clippingPeak < 0.0f);
      std::size_t zeroCrossingIndex = 0;

      read += channelCount;

      // Now look for clipping samples. We'll keep track of when the signal crossed
      // over the zero line so that we can identify the half wave in which clipping
      // is occurring (our partner class will scale the whole half-wave down then)
      for(std::size_t index = 1; index < frameCount; ++index) {
        float sample = *read;

        // Check if this sample is on the other side of the zero line. If so,
        // either just update the indices or, if the previous half-wave had one or
        // more clipping samples, record the clipping half-wave in the list.
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

        // Check if the current sample is clipping. If so, update the maximum amplitude and
        // flip the flag so that we know to record this half-wave at the next zero crossing.
        bool isClipping = (1.0f < std::abs(sample));
        if(isClipping) {
          wasClipping = true;
          if(clippingPeak < std::abs(sample)) {
            clippingPeak = std::abs(sample);
            clippingPeakIndex = index;
          }
        } // if current sample is clipping

        // Give a progress update all 12287 samples. That's roughly every 0.25 seconds
        // of audio data being processed, or 14400 progress updates per hour of data.
        if((index & 0x2fff) == 0) {
          canceler->ThrowIfCanceled();
          progressCallback(
            (static_cast<float>(channelIndex) / static_cast<float>(channelCount)) +
            (
              static_cast<float>(index) /
              static_cast<float>(frameCount) /
              static_cast<float>(channelCount)
            )
          );
        } // if progress report interval hit

        read += channelCount;
      } // for each sample in the channel

      // If the clipping flag is still set after reaching the end of the channel,
      // record the ongoing half-wave up to the final sample as a clipping half-wave.
      if(wasClipping) {
        channel.ClippingHalfwaves.emplace_back(
          zeroCrossingIndex, clippingPeakIndex, frameCount, clippingPeak
        );
      }
    } // for each channel
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio
