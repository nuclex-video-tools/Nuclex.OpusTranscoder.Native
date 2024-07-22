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

  /// <summary>Audio track with a variable number of audio channels</summary>
  /// <typeparam name="TSample">Data type that wlll be used for the samples</typeparam>
  /// <remarks>
  ///   <para>
  ///     The hierarchy and terms used are relatively straightforward:
  ///   </para>
  ///   <code>
  ///     Media file                   (for example .wav, .opus, .mka)
  ///       -> contains tracks         (for example german stereo, english 5.1)
  ///          -> contains channels    (for example left, right, center, LFE)
  ///             -> contains samples  (one vertex of the waveform, 48000 each second)
  ///   </code>
  /// </remarks>
  template<typename TSample>
  class Track {

    /// <summary>Initializes a new track with the specified number of channels</summary>
    /// <param name="channelCount">Number of channels in the audio track</param>
    public: Track(std::size_t channelCount) :
      channels(channelCount) {}

    /// <summary>Returns the number of channels in the audio track</summary>
    /// <returns>The number of channels the audio track has</returns>
    public: std::size_t CountChannels() const {
      return this->channels.size();
    }

    /// <summary>Accesses a channel of the audio track</summary>
    /// <param name="index">Index of the channel that will be accessed</param>
    public: Channel<TSample> &GetChannel(std::size_t index) {
      return this->channels.at(index);
    }

    /// <summary>Accesses a channel of the audio track</summary>
    /// <param name="index">Index of the channel that will be accessed</param>
    public: const Channel<TSample> &GetChannel(std::size_t index) const {
      return this->channels.at(index);
    }

    /// <summary>Channels that belong to the track</summary>
    private: std::vector<Channel<TSample>> channels;

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder

#endif // NUCLEX_OPUSTRANSCODER_AUDIO_TRACK_H
