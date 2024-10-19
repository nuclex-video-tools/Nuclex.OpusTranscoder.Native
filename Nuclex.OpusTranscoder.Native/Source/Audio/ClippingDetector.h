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

#ifndef NUCLEX_OPUSTRANSCODER_AUDIO_CLIPPINGDETECTOR_H
#define NUCLEX_OPUSTRANSCODER_AUDIO_CLIPPINGDETECTOR_H

#include "../Config.h"
#include "./Track.h"

#include <Nuclex/Support/Threading/StopToken.h>
#include <Nuclex/Support/Events/Delegate.h>

#include <memory> // for std::shared_ptr

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Discovers instances of clipping in the audio signal</summary>
  class ClippingDetector {

    /// <summary>Scans all channels in the track and records instances of clipping</summary>
    /// <param name-"track">Track that will be scanned for clipping half-waves</param>
    /// <param name="samples">Samples in which the clipping half-waves will be checked</param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    /// <param name="progressCallback">Callback to which progress reports should be sent</param>
    public: static void FindClippingHalfwaves(
      const std::shared_ptr<Track> &track,
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
      Nuclex::Support::Events::Delegate<void(float)> &progressCallback
    );

    /// <summary>
    ///   Integrates a new scan into an existing scan, copying the new peaks where existing
    ///   half-waves have been rediscovered and inserting entirely new clipping instances
    /// </summary>
    /// <param name="sourceTrack">Source audio track to integrate into</param>
    /// <param name="decodedTrack">
    ///   Decoded audio track whose clipping halfwaves will be integrated
    /// </param>
    public: static void Integrate(
      const std::shared_ptr<Track> &sourceTrack,
      const std::shared_ptr<Track> &decodedTrack
    );

    /// <summary>Updates the existing clipping half-waves in the track</summary>
    /// <param name-"track">
    ///   Track in which the existing clipping half-waves will be re-examined
    /// </param>
    /// <param name="samples">Sample array that will be scanned</param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    /// <param name="progressCallback">Callback to which progress reports should be sent</param>
    /// <returns>The number of half-ways that are still suffering from clipping</returns>
    public: static std::size_t Update(
      const std::shared_ptr<Track> &track,
      const std::vector<float> &samples,
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
      Nuclex::Support::Events::Delegate<void(float)> &progressCallback
    );

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio

#endif // NUCLEX_OPUSTRANSCODER_AUDIO_CLIPPINGDETECTOR_H
