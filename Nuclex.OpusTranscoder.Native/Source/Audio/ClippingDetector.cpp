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

  /// <summary>Value that is used to indicate an invalid index in a list</summary>
  const std::size_t InvalidIndex = std::size_t(-1);

  // ------------------------------------------------------------------------------------------- //

  /// <summary>
  ///   Looks for an existing clipping half-wave that intersects with the found one
  /// </summary>
  /// <param name="halfwaves">Existing half-waves</param>
  /// <param name="clippingHalfwave">New half-wave to match to an existing one</param>
  /// <returns>The index of the maching existing half-wave or -1 if none is found</returns>
  std::size_t findExisitingClippingHalfwave(
    const std::vector<Nuclex::OpusTranscoder::Audio::ClippingHalfwave> &halfwaves,
    const Nuclex::OpusTranscoder::Audio::ClippingHalfwave &clippingHalfwave
  ) {

    // We assume that touching means it's the same half-wave.
    //
    // If an audio codec completely reinvented the waveform, we'd risk associating
    // a half-wave that isn't directly related to a newly discovered clipping peak
    // and wildly scaling that one around without effect.
    //
    // So far, Opus seems to have a high correlation between input waveform and
    // output waveform, as it should be.
    //
    std::size_t count = halfwaves.size();
    for(std::size_t index = 0; index < count; ++index) {
      bool beginsInside = (
        (clippingHalfwave.PriorZeroCrossingIndex >= halfwaves[index].PriorZeroCrossingIndex) &&
        (clippingHalfwave.PriorZeroCrossingIndex < halfwaves[index].NextZeroCrossingIndex)
      );
      bool endsInside = (
        (halfwaves[index].NextZeroCrossingIndex >= clippingHalfwave.NextZeroCrossingIndex) &&
        (halfwaves[index].PriorZeroCrossingIndex < clippingHalfwave.NextZeroCrossingIndex)
      );
      bool envelops = (
        (clippingHalfwave.PriorZeroCrossingIndex < halfwaves[index].PriorZeroCrossingIndex) &&
        (clippingHalfwave.NextZeroCrossingIndex >= halfwaves[index].NextZeroCrossingIndex)
      );
      if(beginsInside || endsInside || envelops) {
        return index;
      }
    }

    // If this point is reached, none of the existing half-waves intersected.
    return InvalidIndex;

  }

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

  void ClippingDetector::IntegrateClippingInstances(
    const std::shared_ptr<Track> &sourceTrack,
    const std::shared_ptr<Track> &decodedTrack
  ) {
    std::size_t channelCount = sourceTrack->Channels.size();
    assert(sourceTrack->Channels.size() == decodedTrack->Channels.size());

    // Process the clipping half-waves for each channel
    for(std::size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
      std::vector<ClippingHalfwave> decodedClippingHalfwaves = (
        decodedTrack->Channels[channelIndex].ClippingHalfwaves
      );
      std::size_t decodedClippingHalfwaveCount = decodedClippingHalfwaves.size();

      // Check each clipping instance in the decodedTrack
      for(std::size_t index = 0; index < decodedClippingHalfwaveCount; ++index) {

        std::size_t existingIndex = findExisitingClippingHalfwave(
          sourceTrack->Channels[channelIndex].ClippingHalfwaves,
          decodedClippingHalfwaves[index]
        );
        if(existingIndex == InvalidIndex) {
          sourceTrack->Channels[channelIndex].ClippingHalfwaves.push_back(
            decodedClippingHalfwaves[index]
          );
        } else {
          ClippingHalfwave &existingClippingHalfwave = (
            sourceTrack->Channels[channelIndex].ClippingHalfwaves[existingIndex]
          );
          if(
            existingClippingHalfwave.PeakAmplitude ==
            decodedClippingHalfwaves[index].PeakAmplitude
          ) {
            ++existingClippingHalfwave.IneffectiveIterationCount;
          } else {
            existingClippingHalfwave.PeakAmplitude = decodedClippingHalfwaves[index].PeakAmplitude;
          }
        }
      }

    } // for each channel
  }

  // ------------------------------------------------------------------------------------------- //

  std::size_t ClippingDetector::UpdateClippingHalfwaves(
    const std::shared_ptr<Track> &track,
    const std::vector<float> &samples,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
    Nuclex::Support::Events::Delegate<void(float)> &progressCallback
  ) {
    std::size_t channelCount = track->Channels.size();
    assert(samples.size() == track->Samples.size());

    std::size_t clippingPeakCount = 0;

    // We'll process each channel separately, otherwise keeping track of the start and
    // end indices of each clipping half-wave becomes just to complicated...
    for(std::size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
      std::vector<ClippingHalfwave> &clippingHalfwaves = (
        track->Channels[channelIndex].ClippingHalfwaves
      );
      std::size_t clippingHalfwaveCount = clippingHalfwaves.size();
      for(std::size_t clipIndex = 0; clipIndex < clippingHalfwaveCount; ++clipIndex) {

        float peak = 0.0f;
        std::size_t peakIndex = clippingHalfwaves[clipIndex].PriorZeroCrossingIndex;
        const float *halfwaveSamples = (
          samples.data() +
          (clippingHalfwaves[clipIndex].PriorZeroCrossingIndex * channelCount) +
          channelIndex
        );
        for(
          std::uint64_t sampleIndex = clippingHalfwaves[clipIndex].PriorZeroCrossingIndex;
          sampleIndex < clippingHalfwaves[clipIndex].NextZeroCrossingIndex;
          sampleIndex += channelCount
        ) {
          if(peak < std::abs(halfwaveSamples[0])) {
            peak = std::abs(halfwaveSamples[0]);
            peakIndex = sampleIndex;
          }
          halfwaveSamples += channelCount;
        } // for each sample in a clipping halfwave

        clippingHalfwaves[clipIndex].PeakIndex = peakIndex;
        if(peak != clippingHalfwaves[clipIndex].PeakAmplitude) {
          clippingHalfwaves[clipIndex].IneffectiveIterationCount = 0;
          clippingHalfwaves[clipIndex].PeakAmplitude = peak;
          if(1.0f < peak) {
            ++clippingPeakCount;
          }
        } else if(clippingHalfwaves[clipIndex].IneffectiveIterationCount < 10) {
          ++clippingPeakCount;
        }

      } // for each clipping halfwave
    } // for each channel

    return clippingPeakCount;
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio
