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

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Audio track with all data uncompressed in memory</summary>
  struct Track {

    /// <summary>Stores the decoded samples of all channels</summary>
    public: std::vector<float> Samples;
    /// <summary>Data about the channels and clipping found in each of them</summary>
    public: std::vector<Channel> Channels;

    /// <summary>Index of the sample at which the half-wave begins at the zero line</summary>
    public: std::size_t PriorZeroCrossingIndex;
    /// <summary>Sample at which the half-wave has its maximum amplitude</summary>
    public: std::size_t PeakIndex;
    /// <summary>Index of the sample at which the half-wave ends at the zero line</summary>
    public: std::size_t NextZeroCrossingIndex;

    /// <summary>In which iteration the clipping half-wave was first detected</summary>
    public: std::size_t IterationFirstSeen;
    /// <summary>Peak amplitude the clipping half-wave had in the previous iteration</summary>
    public: float PriorPeakAmplitude;
    /// <summary>Peak amplitude the clipping half-wave has in this iteration</summary>
    public: float CurrentPeakAmplitude;

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio

#endif // NUCLEX_OPUSTRANSCODER_AUDIO_TRACK_H
