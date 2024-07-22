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

  template<typename TSample>
  class Track {

    public: Track(std::size_t channelCount) {}

    public: std::size_t CountChannels() const {
      return this->channels.size();
    }

    public: Channel<TSample> &GetChannel(std::size_t index) {
      return this->channels.at(index);
    }

    public: const Channel<TSample> &GetChannel(std::size_t index) const {
      return this->channels.at(index);
    }

    private: std::vector<Channel<TSample>> channels;

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder

#endif // NUCLEX_OPUSTRANSCODER_AUDIO_TRACK_H
