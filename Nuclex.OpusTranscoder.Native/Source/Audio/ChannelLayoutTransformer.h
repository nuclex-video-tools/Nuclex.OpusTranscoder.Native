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

#ifndef NUCLEX_OPUSTRANSCODER_AUDIO_CHANNELLAYOUTTRANSFORMER_H
#define NUCLEX_OPUSTRANSCODER_AUDIO_CHANNELLAYOUTTRANSFORMER_H

#include "../Config.h"
#include "./Track.h"

#include <Nuclex/Support/Threading/StopToken.h>
#include <Nuclex/Support/Events/Delegate.h>

#include <memory> // for std::shared_ptr

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Changes channel layouts, upmixing, downmixing or re-weaving accordingly</summary>
  class ChannelLayoutTransformer {

    /// <summary>Performs a downmix of the track's channels to stereo</summary>
    /// <param name-"track">Track that will be downmixed</param>
    /// <param name-"nightmodeLevel">
    ///   How much the alternative Nightmode downmix formula will be used
    /// </param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    /// <param name="progressCallback">Callback to which progress reports should be sent</param>
    public: static void DownmixToStereo(
      const std::shared_ptr<Track> &track, // can be 5.1 or 7.1
      float nightmodeLevel,
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
      Nuclex::Support::Events::Delegate<void(float)> &progressCallback
    );

    /// <summary>Performs a downmix of the track's channels to 5.1 surround</summary>
    /// <param name-"track">Track that will be downmixed</param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    /// <param name="progressCallback">Callback to which progress reports should be sent</param>
    public: static void DownmixToFiveDotOne(
      const std::shared_ptr<Track> &track, // must be 7.1
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
      Nuclex::Support::Events::Delegate<void(float)> &progressCallback
    );

    /// <summary>Performs an upmix of a mono input channel to stereo</summary>
    /// <param name-"track">Track that will be upmixed</param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    /// <param name="progressCallback">Callback to which progress reports should be sent</param>
    public: static void UpmixToStereo(
      const std::shared_ptr<Track> &track, // must be mono
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
      Nuclex::Support::Events::Delegate<void(float)> &progressCallback
    );

    /// <summary>Reorders the channels of the input file to the Vorbis order</summary>
    /// <param name-"track">Track in which the channels will be reordered</param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    /// <param name="progressCallback">Callback to which progress reports should be sent</param>
    public: static void ReweaveToVorbisLayout(
      const std::shared_ptr<Track> &track, // must be 5.1
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
      Nuclex::Support::Events::Delegate<void(float)> &progressCallback
    );

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio

#endif // NUCLEX_OPUSTRANSCODER_AUDIO_CHANNELLAYOUTTRANSFORMER_H
