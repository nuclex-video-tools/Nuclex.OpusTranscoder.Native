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

#include "./HalfwaveTucker.h"

namespace {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Amplitude a -0.001 dB, also useful as a factor to scale to -0.001 dB</summary>
  const float MinusOneThousandthDecibel = (
    0.99988487737246860830993605587529673614422529030613405900998412734419982883669222875138231966f
  );

  // ------------------------------------------------------------------------------------------- //

} // anonymous namespace

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  void HalfwaveTucker::TuckClippingHalfwaves(
    const std::shared_ptr<Track> &track,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
    Nuclex::Support::Events::Delegate<void(float)> &progressCallback
  ) {
    std::size_t channelCount = track->Channels.size();
    std::size_t frameCount = track->Samples.size() / channelCount;

    for(std::size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
      Channel &channel = track->Channels[channelIndex];
      float *samples = track->Samples.data() + channelIndex;

      std::uint64_t skipStartIndex = 0;

      std::size_t halfwaveCount = channel.ClippingHalfwaves.size();
      for(std::size_t halfwaveIndex = 0; halfwaveIndex < halfwaveCount; ++halfwaveIndex) {
        ClippingHalfwave &halfwave = channel.ClippingHalfwaves[halfwaveIndex];

        samples += (halfwave.PriorZeroCrossingIndex - skipStartIndex) * channelCount;

        // We completely rely on the measured peak here and don't update the scaling factor.
        // The reason is that, for the iterative de-clipper, the peaks will have been collected
        // from the decoded Opus stream, but we need to apply them to (a copy of) the original
        // channels, to avoid generation loss when we encode the Opus file once more.
        {
          // This is by how much we'd have to scale the amplitude down to tuck the half-wave
          // in far enough to stay below the signal ceiling
          float quotient = std::abs(halfwave.CurrentPeakAmplitude);

          // If there is a valid prior peak amplitude, it means we already did the above
          // calculation, yet it didn't bring the peak down far enough. So this time around,
          // we'll calculate how much we will have to overshoot to hit the goal.
          if(halfwave.PriorVolumeQuotient != 0.0f) {
            quotient *= halfwave.PriorVolumeQuotient;
          }

          // Record the current volume so that, after decoding the Opus file, if there is
          // still clipping, we can record the quotient we tried but which wasn't enough,
          // which will then be picked up by the compensation just above this statement.
          halfwave.CurrentVolumeQuotient = quotient;

          // Copy the data inside the clipping half-wave scaled down to the -1.0 .. +1.0 level
          for(
            std::uint64_t index = halfwave.PriorZeroCrossingIndex;
            index < halfwave.NextZeroCrossingIndex;
            ++index
          ) {
            samples[0] /= quotient;
            samples += channelCount;

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
          } // for each sample in the clipping half-wave

          skipStartIndex = halfwave.NextZeroCrossingIndex;
        }

      } // for each clipping halfwave identified
    } // for each channel
  }

  // ------------------------------------------------------------------------------------------- //

  void HalfwaveTucker::TuckClippingHalfwaves(
    const std::shared_ptr<Track> &track,
    std::vector<float> &tuckedSamples,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
    Nuclex::Support::Events::Delegate<void(float)> &progressCallback
  ) {
    std::size_t channelCount = track->Channels.size();
    std::size_t frameCount = track->Samples.size() / channelCount;

    for(std::size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
      Channel &channel = track->Channels[channelIndex];
      const float *read = track->Samples.data() + channelIndex;
      float *write = tuckedSamples.data() + channelIndex;

      std::uint64_t copyStartIndex = 0;

      std::size_t halfwaveCount = channel.ClippingHalfwaves.size();
      for(std::size_t halfwaveIndex = 0; halfwaveIndex < halfwaveCount; ++halfwaveIndex) {
        ClippingHalfwave &halfwave = channel.ClippingHalfwaves[halfwaveIndex];

        // Copy the data up to where the clipping half-wave begins
        for(
          std::size_t index = copyStartIndex;
          index < halfwave.PriorZeroCrossingIndex;
          ++index
        ) {
          *write = *read;
          read += channelCount;
          write += channelCount;

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
        } // for each sample up to the clipping half-wave

        // We completely rely on the measured peak here and don't update the scaling factor.
        // The reason is that, for the iterative de-clipper, the peaks will have been collected
        // from the decoded Opus stream, but we need to apply them to (a copy of) the original
        // channels, to avoid generation loss when we encode the Opus file once more.
        {
          // This is by how much we'd have to scale the amplitude down to tuck the half-wave
          // in far enough to stay below the signal ceiling
          float quotient = std::abs(halfwave.CurrentPeakAmplitude);

          // If there is a valid prior peak amplitude, it means we already did the above
          // calculation, yet it didn't bring the peak down far enough. So this time around,
          // we'll calculate how much we will have to overshoot to hit the goal.
          if(halfwave.PriorVolumeQuotient != 0.0f) {
            quotient *= halfwave.PriorVolumeQuotient;
          }

          // Record the current volume so that, after decoding the Opus file, if there is
          // still clipping, we can record the quotient we tried but which wasn't enough,
          // which will then be picked up by the compensation just above this statement.
          halfwave.CurrentVolumeQuotient = quotient;

          // Copy the data inside the clipping half-wave scaled down to the -1.0 .. +1.0 level
          for(
            std::uint64_t index = halfwave.PriorZeroCrossingIndex;
            index < halfwave.NextZeroCrossingIndex;
            ++index
          ) {
            *write = *read / quotient;

            read += channelCount;
            write += channelCount;

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
          } // for each sample in the clipping half-wave

          copyStartIndex = halfwave.NextZeroCrossingIndex;
        }

      } // for each clipping halfwave identified

      // Copy the data from where last clipping half-wave ended to the end of the channel
      for(std::size_t index = copyStartIndex; index < frameCount; ++index) {
        *write = *read;
        read += channelCount;
        write += channelCount;

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
      } // for each sample between the last clipping half-wave and the channel's end
    } // for each channel
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio
