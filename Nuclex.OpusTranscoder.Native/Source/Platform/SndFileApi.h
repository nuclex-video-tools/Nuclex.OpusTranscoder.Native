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

#ifndef NUCLEX_OPUSTRANSCODER_PLATFORM_SNDFILEAPI_H
#define NUCLEX_OPUSTRANSCODER_PLATFORM_SNDFILEAPI_H

#include "../Config.h"

#include <string> // for std::string
#include <memory> // for std::shared_ptr
#include <vector> // for std::vector

#include <sndfile.h> // for SNDFILE and its functions

namespace Nuclex { namespace OpusTranscoder { namespace Platform {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Wraps the libsndfile API with error checking</summary>
  /// <remarks>
  ///   <para>
  ///     Included with libsndfile is a C++ wrapper, but it intentionally avoids exceptions
  ///     and is designed in a way that makes it as much of a foreign element in C++ code
  ///     as using libsndfile's C API directly would be, so we use the C API with some
  ///     helpers to aid in thorough error checking and resource cleanup.
  ///   </para>
  /// </remarks>
  class SndFileApi {

    public: static std::shared_ptr<::SNDFILE> OpenForReading(
      const std::string &path, ::SF_INFO &soundFileInformation
    );

    public: static ::SF_INFO GetSoundFileInfo(const std::shared_ptr<::SNDFILE> &soundFile);

    public: static std::vector<int> GetChannelMapInfo(
      const std::shared_ptr<::SNDFILE> &soundFile
    );

  };

  // ------------------------------------------------------------------------------------------- //

}}} // namespace Nuclex::OpusTranscoder::Platform

#endif // NUCLEX_OPUSTRANSCODER_PLATFORM_SNDFILEAPI_H
