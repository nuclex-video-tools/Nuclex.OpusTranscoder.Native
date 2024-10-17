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

#ifndef NUCLEX_OPUSTRANSCODER_AUDIO_CLIPPINGHALFWAVE_H
#define NUCLEX_OPUSTRANSCODER_AUDIO_CLIPPINGHALFWAVE_H

#include "../Config.h"

#include <vector> // for std::vector
#include <cstdint> // for std::uint64_t

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Stores informations about a clipping half-wave</summary>
  struct ClippingHalfwave {

    public: ClippingHalfwave(
      std::uint64_t startIndex, std::uint64_t peakIndex, std::uint64_t endIndex,
      float peakAmplitude
    ) :
      PriorZeroCrossingIndex(startIndex),
      PeakIndex(peakIndex),
      NextZeroCrossingIndex(endIndex),
      IneffectiveIterationCount(0),
      PeakAmplitude(peakAmplitude),
      CurrentVolumeQuotient(1.0f) {}

    /// <summary>Index of the sample at which the half-wave begins at the zero line</summary>
    public: std::uint64_t PriorZeroCrossingIndex;
    /// <summary>Sample at which the half-wave has its maximum amplitude</summary>
    public: std::uint64_t PeakIndex;
    /// <summary>Index of the sample at which the half-wave ends at the zero line</summary>
    public: std::uint64_t NextZeroCrossingIndex;

    /// <summary>In which iteration the clipping half-wave was first detected</summary>
    public: std::size_t IneffectiveIterationCount;
    /// <summary>Peak amplitude the clipping half-wave has in this iteration</summary>
    public: float PeakAmplitude;
    /// <summary>Quotient by which the volume was scaled to de-clip the halfwave</summary>
    public: float CurrentVolumeQuotient;

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio

#endif // NUCLEX_OPUSTRANSCODER_AUDIO_CLIPPINGHALFWAVE_H
