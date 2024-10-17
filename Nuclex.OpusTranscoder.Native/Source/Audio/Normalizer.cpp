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

#include "./Normalizer.h"

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

  void Normalizer::Normalize(
    const std::shared_ptr<Track> &track,
    bool allowVolumeDecrease,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
    Nuclex::Support::Events::Delegate<void(float)> &progressCallback
  ) {
    using Nuclex::Audio::ChannelPlacement;

    std::size_t channelCount = track->Channels.size();
    std::size_t frameCount = track->Samples.size() / channelCount;

    float maximumAmplitude = 0.0f;
    float maximumBassAmplitude = 0.0f;

    // Stage 1: scan all channels to find their peak amplitudes. Keep bass and
    // normal peak separate, we'll normalize the bass independently.
    for(std::size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
      Channel &channel = track->Channels[channelIndex];
      const float *samples = track->Samples.data() + channelIndex;

      if(channel.Placement == ChannelPlacement::LowFrequencyEffects) {
        for(std::size_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
          float amplitude = samples[0];
          if(maximumBassAmplitude < amplitude) {
            maximumBassAmplitude = amplitude;
          }
          samples += channelCount;

          if((frameIndex & 0x2fff) == 0) {
            canceler->ThrowIfCanceled();
            progressCallback(
              static_cast<float>(frameIndex) / static_cast<float>(frameCount) / 2.0f
            );
          } // if progress interval reached
        } // for each frame
      } else { // If channel ^^ is bass ^^ / vv is not bass vv
        for(std::size_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
          float amplitude = samples[0];
          if(maximumAmplitude < amplitude) {
            maximumAmplitude = amplitude;
          }
          samples += channelCount;

          if((frameIndex & 0x2fff) == 0) {
            canceler->ThrowIfCanceled();
            progressCallback(
              static_cast<float>(frameIndex) / static_cast<float>(frameCount) / 2.0f
            );
          } // if progress interval reached
        } // for each frame
      } // if channel is bass or not bass
    } // for each channel

    // Stay 0.001 dB below the signal ceiling
    maximumAmplitude *= MinusOneThousandthDecibel;
    maximumBassAmplitude *= MinusOneThousandthDecibel;

    // Stage 2: increase the volume of all tracks by the same amount to make them
    // use the full available volume range
    for(std::size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
      Channel &channel = track->Channels[channelIndex];
      float *samples = track->Samples.data() + channelIndex;

      if(channel.Placement == ChannelPlacement::LowFrequencyEffects) {
        if(allowVolumeDecrease || (maximumBassAmplitude < 1.0f)) {
          for(std::size_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
            samples[0] /= maximumBassAmplitude;
            samples += channelCount;

            if((frameIndex & 0x2fff) == 0) {
              canceler->ThrowIfCanceled();
              progressCallback(
                static_cast<float>(frameIndex) / static_cast<float>(frameCount) / 2.0f + 0.5f
              );
            } // if progress interval reached
          } // for each frame
        } // if volume decrease allowed or channel is below maximum amplitude
      } else { // If channel ^^ is bass ^^ / vv is not bass vv
        if(allowVolumeDecrease || (maximumAmplitude < 1.0f)) {
          for(std::size_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
            samples[0] /= maximumAmplitude;
            samples += channelCount;

            if((frameIndex & 0x2fff) == 0) {
              canceler->ThrowIfCanceled();
              progressCallback(
                static_cast<float>(frameIndex) / static_cast<float>(frameCount) / 2.0f + 0.5f
              );
            } // if progress interval reached
          } // for each frame
        } // if volume decrease allowed or channel is below maximum amplitude
      } // if channel is bass or not bass
    } // for each channel
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio
