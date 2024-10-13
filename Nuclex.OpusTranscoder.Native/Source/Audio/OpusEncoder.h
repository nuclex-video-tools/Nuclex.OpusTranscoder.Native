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

#ifndef NUCLEX_OPUSTRANSCODER_AUDIO_OPUSENCODER_H
#define NUCLEX_OPUSTRANSCODER_AUDIO_OPUSENCODER_H

#include "../Config.h"
#include "./Track.h"

#include <Nuclex/Support/Threading/StopToken.h>
#include <Nuclex/Support/Events/Delegate.h>
#include <Nuclex/Audio/Storage/VirtualFile.h>

#include <memory> // for std::shared_ptr

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Encodes interleaved audio samples to an in-memory Opus file</summary>
  struct OpusEncoder {

    /// <summary>Encodes the specified audio track into an Opus file</summary>
    /// <param name-"track">Track in which half-waves will be tucked</param>
    /// <param name-"target">Virtual file to write the Opus audio file contents into</param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    /// <param name="progressCallback">Callback to which progress reports should be sent</param>
    public: static void Encode(
      const std::shared_ptr<Track> &track,
      int sampleRate, float bitRate,
      const std::shared_ptr<Nuclex::Audio::Storage::VirtualFile> &target,
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
      Nuclex::Support::Events::Delegate<void(float)> &progressCallback
    );

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio

#endif // NUCLEX_OPUSTRANSCODER_AUDIO_OPUSENCODER_H
