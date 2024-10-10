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

#ifndef NUCLEX_OPUSTRANSCODER_AUDIO_CHANNEL_H
#define NUCLEX_OPUSTRANSCODER_AUDIO_CHANNEL_H

#include "../Config.h"

#include <Nuclex/Audio/ChannelPlacement.h>

#include "./ClippingHalfwave.h"

#include <vector>

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Single audio channel storing samples and placement information</summary>
  class Channel {

    /// <summary>Order in which this channel will appear in the input file</summary>
    public: std::size_t InputOrder;
    /// <summary>Order in which this channel will appear in the Opus file</summary>
    public: std::size_t OpusOrder;

    /// <summary>Placement from which this channel is supposed to play</summary>
    public: Nuclex::Audio::ChannelPlacement Placement;
    /// <summary>Detected clipping instances</summary>
    public: std::vector<ClippingHalfwave> ClippingHalfwaves;

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder

#endif // NUCLEX_OPUSTRANSCODER_AUDIO_CHANNEL_H
