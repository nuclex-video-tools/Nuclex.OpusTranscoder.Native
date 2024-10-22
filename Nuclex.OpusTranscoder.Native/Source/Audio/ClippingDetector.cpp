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

#include <algorithm>

namespace {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Value that is used to indicate an invalid index in a list</summary>
  const std::size_t InvalidIndex = std::size_t(-1);

  // ------------------------------------------------------------------------------------------- //

  /// <summary>
  ///   Checks if the first half-wave's start sample is earlier then the second one's
  /// </summary>
  /// <param name="first">First half-wave that will be compared</param>
  /// <param name="second">Second half-wave that will be compared</param>
  /// <returns>True if the first half-wave begins earlier than the second</returns?
  bool halfwaveStartIndexIsLess(
    const Nuclex::OpusTranscoder::Audio::ClippingHalfwave &first,
    const Nuclex::OpusTranscoder::Audio::ClippingHalfwave &second
  ) {
    return first.PriorZeroCrossingIndex < second.PriorZeroCrossingIndex;
  }

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Inserts a clipping halfwave into a list preserving its ordering</summary>
  /// <param name="halfwaves">Existing list of half-waves</param>
  /// <param name="halfwaveToInsert">Half-wave that wil be inserted int othe list</param>
  void insertOrdered(
    std::vector<Nuclex::OpusTranscoder::Audio::ClippingHalfwave> &halfwaves,
    const Nuclex::OpusTranscoder::Audio::ClippingHalfwave &halfwaveToInsert
  ) {
    std::vector<Nuclex::OpusTranscoder::Audio::ClippingHalfwave>::iterator insertionIt = (
      std::lower_bound(
        halfwaves.begin(), halfwaves.end(),
        halfwaveToInsert,
        &halfwaveStartIndexIsLess
      )
    );

    halfwaves.insert(insertionIt, halfwaveToInsert);
  }

  // ------------------------------------------------------------------------------------------- //

  /// <summary>
  ///   Looks for an existing clipping half-wave that intersects with the found one
  /// </summary>
  /// <param name="halfwaves">Existing half-waves</param>
  /// <param name="clippingHalfwave">New half-wave to match to an existing one</param>
  /// <returns>The index of the maching existing half-wave or -1 if none is found</returns>
  std::size_t findExistingClippingHalfwave(
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

  Nuclex::OpusTranscoder::Audio::ClippingHalfwave getHalfwaveAroundSample(
    const std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> &sourceTrack,
    std::size_t channelIndex,
    std::uint64_t sampleIndex
  ) {
    std::size_t channelCount = sourceTrack->Channels.size();

    const float *forwardRead = sourceTrack->SampleAt(channelIndex, sampleIndex);

    bool startsAboveZero = (forwardRead[0] >= 0.0f);

    // Figure out the earliest sample that is still on the same side of the zero line
    // as the sample from which the search started. As per usual conventions, The start index
    // is inclusive, so it points at the same that is already on the same side.
    std::uint64_t priorCrossingIndex = sampleIndex;
    {
      const float *backwardRead = forwardRead;
      while(0 < priorCrossingIndex) {
        backwardRead -= channelCount; // Step back to before index counter
        bool priorIsAboveZero = (backwardRead[0] >= 0.0f); // Preceding sample!

        if(priorIsAboveZero != startsAboveZero) {
          break;
        }

        // Adjust index *after* checking sample, so when loop breaks,
        // the index is still on sample that starts the half-wave
        --priorCrossingIndex;
      }
    }

    // Now scan forward until the next zero crossing. The end index is exclusive, so we want
    // the end index to be of the first sample that has crossed the zero line. We also do
    // an immediate step forward because we expect the start sample to be a valid index.
    std::uint64_t nextCrossingIndex = sampleIndex;
    {
      // We don't need to re-check the starting sample (and want the end to be at least
      // on starting sample + 1), so skip over it.
      forwardRead += channelCount;
      ++nextCrossingIndex;

      std::uint64_t endIndex = sourceTrack->Samples.size();
      while(nextCrossingIndex < endIndex) {
        bool isAboveZero = (forwardRead[0] >= 0.0f);

        if(isAboveZero != startsAboveZero) {
          break;
        }

        // In this direction, we advance both index and read pointer at the same time
        // because we want the index return to be one past the last sample that is
        // within the half-wave.
        ++nextCrossingIndex;
        forwardRead += channelCount;
      }
    }

    // Obviously, the sampleIndex will likely not be the peak and we leave the peak
    // at 0.0 because the actual peak is unknown to us (it is in the other, decoded
    // sample array). So an Update() call is needed to fix that, too.
    return Nuclex::OpusTranscoder::Audio::ClippingHalfwave(
      priorCrossingIndex, sampleIndex, nextCrossingIndex, 0.0f
    );
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

  void ClippingDetector::Integrate(
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

        std::size_t existingIndex = findExistingClippingHalfwave(
          sourceTrack->Channels[channelIndex].ClippingHalfwaves,
          decodedClippingHalfwaves[index]
        );
        if(existingIndex == InvalidIndex) {
          insertOrdered(
            sourceTrack->Channels[channelIndex].ClippingHalfwaves,
            getHalfwaveAroundSample(
              sourceTrack, channelIndex, decodedClippingHalfwaves[index].PeakIndex
            )
          );
        } else {
          ClippingHalfwave &existingClippingHalfwave = (
            sourceTrack->Channels[channelIndex].ClippingHalfwaves[existingIndex]
          );
          existingClippingHalfwave.PeakAmplitude = decodedClippingHalfwaves[index].PeakAmplitude;
        }
      }

    } // for each channel
  }

  // ------------------------------------------------------------------------------------------- //

  std::size_t ClippingDetector::Update(
    const std::shared_ptr<Track> &track,
    const std::vector<float> &samples,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
    Nuclex::Support::Events::Delegate<void(float)> &progressCallback
  ) {
    assert(samples.size() == track->Samples.size());

    std::size_t clippingPeakCount = 0;

    // We'll process each channel separately, otherwise keeping track of the start and
    // end indices of each clipping half-wave becomes just to complicated...
    std::size_t channelCount = track->Channels.size();
    for(std::size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
      std::vector<ClippingHalfwave> &clippingHalfwaves = (
        track->Channels[channelIndex].ClippingHalfwaves
      );

      std::size_t clippingHalfwaveCount = clippingHalfwaves.size();
      for(std::size_t clipIndex = 0; clipIndex < clippingHalfwaveCount; ++clipIndex) {

        // Re-scan the samples in this half-wave and determine their current
        // peak amplitude and the index of the peak amplitude sample.
        float peak = 0.0f;
        std::size_t peakIndex = clippingHalfwaves[clipIndex].PriorZeroCrossingIndex;
        {
          const float *halfwaveSamples = (
            samples.data() +
            (clippingHalfwaves[clipIndex].PriorZeroCrossingIndex * channelCount) +
            channelIndex
          );
          for(
            std::uint64_t sampleIndex = clippingHalfwaves[clipIndex].PriorZeroCrossingIndex;
            sampleIndex < clippingHalfwaves[clipIndex].NextZeroCrossingIndex;
            ++sampleIndex
          ) {
            if(peak < std::abs(halfwaveSamples[0])) {
              peak = std::abs(halfwaveSamples[0]);
              peakIndex = sampleIndex;
            }
            halfwaveSamples += channelCount;
          } // for each sample in a clipping halfwave
        } // beauty scope to determine peak and peak index

        // Now we know the new peak, update the recorded peak in the half-wave with it.
        // At this point, we also count up the 'IneffectiveIterationCount' for any
        // peaks that remain unchanged compared to the previous iteration to allow us
        // to give up on those that we can't get moving.
        //clippingHalfwaves[clipIndex].PeakIndex = peakIndex;
        if(peak != clippingHalfwaves[clipIndex].PeakAmplitude) {
          clippingHalfwaves[clipIndex].IneffectiveIterationCount = 0;
          clippingHalfwaves[clipIndex].PeakAmplitude = peak;
        } else {
          ++clippingHalfwaves[clipIndex].IneffectiveIterationCount;
        }

        // Count the total number of half-waves that are still clipping. This count is
        // also the exit condition in the rectifying loop, so if a half-wave has not
        // improved after 10 (increasingly drastic) attempts, don't count it anymore,
        // as it is now considered a lost cause.
        if(1.0f < peak) {
          if(clippingHalfwaves[clipIndex].IneffectiveIterationCount < 10) {
            ++clippingPeakCount;
          }
        }

      } // for each clipping halfwave
    } // for each channel

    return clippingPeakCount;
  }

  // ------------------------------------------------------------------------------------------- //

  void ClippingDetector::DebugVerifyConsistency(const std::shared_ptr<Track> &track) {
    std::size_t channelCount = track->Channels.size();
    for(std::size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
      std::vector<ClippingHalfwave> &clippingHalfwaves = (
        track->Channels[channelIndex].ClippingHalfwaves
      );

      std::uint64_t previousHalfwaveEnd = 0;

      std::size_t clippingHalfwaveCount = clippingHalfwaves.size();
      for(std::size_t clipIndex = 0; clipIndex < clippingHalfwaveCount; ++clipIndex) {
        assert(
          (clippingHalfwaves[clipIndex].PriorZeroCrossingIndex >= previousHalfwaveEnd) &&
          u8"Clipping half-waves must not intersect each other or be duplicated"
        );
        previousHalfwaveEnd = clippingHalfwaves[clipIndex].NextZeroCrossingIndex;
      }
    }
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio
