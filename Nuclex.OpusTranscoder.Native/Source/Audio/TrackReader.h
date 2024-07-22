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

#ifndef NUCLEX_OPUSTRANSCODER_AUDIO_TRACKREADER_H
#define NUCLEX_OPUSTRANSCODER_AUDIO_TRACKREADER_H

#include "../Config.h"

#include <cstdint> // for std::int16_t
#include <memory> // for std::shared_ptr

namespace Nuclex::Support::Threading {

  // ------------------------------------------------------------------------------------------- //

  class StopToken;

  // ------------------------------------------------------------------------------------------- //

}

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  template<typename TSample>
  class Track;

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Reads audio tracks from a file using libsndfile</summary>
  /// <typeparam name="TSample">Type of samples the tracks should be loaded as</typeparam>
  template<typename TSample>
  class TrackReader {

    /// <summary>Reads an audio track from the specified audio file</summary>
    /// <param name="path">Path of the audio file that will be read</param>
    /// <param name="stopToken">Token by which the loading operation can be cancelled</param>
    /// <returns>The audio track loaded from the specified file</returns>
    /// <remarks>
    ///   Assumes that the media file contains exactly one track. Otherwise, it is up
    ///   to libsndfile which track will be loaded, probably the first, but who knows?
    /// </remarks>
    public: static std::shared_ptr<Track<TSample>> Read(
      const std::string &path,
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &stopToken = (
        std::shared_ptr<const Nuclex::Support::Threading::StopToken>()
      )
    );

  };

  // ------------------------------------------------------------------------------------------- //

  template<> std::shared_ptr<Track<double>> TrackReader<double>::Read(
    const std::string &path,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &stopToken /* = (
      std::shared_ptr<const Nuclex::Support::Threading::StopToken>()
    ) */
  );

  // ------------------------------------------------------------------------------------------- //

  template<> std::shared_ptr<Track<float>> TrackReader<float>::Read(
    const std::string &path,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &stopToken /* = (
      std::shared_ptr<const Nuclex::Support::Threading::StopToken>()
    ) */
  );

  // ------------------------------------------------------------------------------------------- //

  template<> std::shared_ptr<Track<std::int16_t>> TrackReader<std::int16_t>::Read(
    const std::string &path,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &stopToken /* = (
      std::shared_ptr<const Nuclex::Support::Threading::StopToken>()
    ) */
  );

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio

#endif // NUCLEX_OPUSTRANSCODER_AUDIO_TRACKREADER_H
