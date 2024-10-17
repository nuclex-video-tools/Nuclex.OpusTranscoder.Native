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

#ifndef NUCLEX_OPUSTRANSCODER_AUDIO_NORMALIZER_H
#define NUCLEX_OPUSTRANSCODER_AUDIO_NORMALIZER_H

#include "../Config.h"
#include "./Track.h"

#include <Nuclex/Support/Threading/StopToken.h>
#include <Nuclex/Support/Events/Delegate.h>

#include <memory> // for std::shared_ptr

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Increases the volume so the audio track uses the full range</summary>
  /// <remarks>
  ///   <para>
  ///     For movie audio tracks, this is generally desirable - if an entire movie or episode
  ///     doesn't make use of the signal range anywhere, it's usually because it has been
  ///     mastered at a too low volume.
  ///   </para>
  ///   <para>
  ///     For music tracks, it's usually not a good idea to normalize. Some albums,
  ///     especially complex metal opera and classic pieces may embed a quieter track
  ///     between normal ones that should not be scaled to its full range. That's why
  ///     FLAC and other audio formats implemented "Album Gain" to replace "Track Gain.""
  ///   </para>
  /// </remarks>
  class Normalizer {

    /// <summary>Normalizes the volume of an audio track if it is too quiet</summary>
    /// <param name="track">Audio track that will be normalized</param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    /// <param name="progressCallback">Callback to which progress reports should be sent</param>
    /// <remarks>
    ///   This only normalizes upwards - the volume is only ever increased, never lowered.
    ///   If the track is too loud, it needs to be processed otherwise - either de-clipped if
    ///   only small 
    /// </remarks>
    public: static void Normalize(
      const std::shared_ptr<Track> &track,
      bool allowVolumeDecrease,
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
      Nuclex::Support::Events::Delegate<void(float)> &progressCallback
    );

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio

#endif // NUCLEX_OPUSTRANSCODER_AUDIO_NORMALIZER_H
