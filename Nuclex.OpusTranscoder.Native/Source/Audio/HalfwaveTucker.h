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

#ifndef NUCLEX_OPUSTRANSCODER_AUDIO_HALFWAVETUCKER_H
#define NUCLEX_OPUSTRANSCODER_AUDIO_HALFWAVETUCKER_H

#include "../Config.h"
#include "./Track.h"

#include <Nuclex/Support/Threading/StopToken.h>
#include <Nuclex/Support/Events/Delegate.h>

#include <memory> // for std::shared_ptr

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Scales clipping half-waves down so they don't clip anymore</summary>
  class HalfwaveTucker {

    /// <summary>Tucks any half-waves that extend above the signal ceiling in</summary>
    /// <param name-"track">Track in which half-waves will be tucked</param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    /// <param name="progressCallback">Callback to which progress reports should be sent</param>
    public: static void TuckClippingHalfwaves(
      const std::shared_ptr<Track> &track,
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
      Nuclex::Support::Events::Delegate<void(float)> &progressCallback
    );

    /// <summary>Tucks any half-waves that extend above the signal ceiling in</summary>
    /// <param name-"track">Track in which half-waves will be tucked</param>
    /// <param name-"tuckedSamples">
    ///   Will receive a full copy of all samples with de-clipping applied
    /// </param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    /// <param name="progressCallback">Callback to which progress reports should be sent</param>
    public: static void TuckClippingHalfwaves(
      const std::shared_ptr<Track> &track,
      std::vector<float> &tuckedSamples,
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
      Nuclex::Support::Events::Delegate<void(float)> &progressCallback
    );

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio

#endif // NUCLEX_OPUSTRANSCODER_AUDIO_HALFWAVETUCKER_H
