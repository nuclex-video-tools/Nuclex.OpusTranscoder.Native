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

    /// <summary>Opens an audio file for reading by libsndfile</summary>
    /// <param name="path">Path of the audio file in the file system</param>
    /// <param name="soundFileInformation">
    ///   Will receive additional information about the loaded audio file
    /// </param>
    /// <returns>
    ///   A shared pointer to a libsndfile SNDFILE that can be used with other functions
    ///   in the libsndfile API
    /// </returns>
    /// <remarks>
    ///   The path is assumed to be UTF-8 on all platforms. On Windows systems, it will
    ///   be converted to UTF-16 and the wide char version of the open method will be used.
    ///   The returned shared pointer has a custom deleter set up, so this is fully RAII
    ///   compatible and once the pointer goes out of scope, the SNDFILE instance is
    ///   deleted as intended by libsndfile.
    /// </remarks>
    public: static std::shared_ptr<::SNDFILE> OpenForReading(
      const std::string &path, ::SF_INFO &soundFileInformation
    );

    /// <summary>
    ///   Retrieves the sound file information structure from a SNDFILE poitner
    /// </summary>
    /// <param name="soundFile">
    ///   Sound file for which the information structure will be retrieved
    /// </param>
    /// <returns>
    ///   The sound file information structure describing some of the audio file's properties
    /// </returns>
    public: static ::SF_INFO GetSoundFileInfo(const std::shared_ptr<::SNDFILE> &soundFile);

    /// <summary>Looks up the intended spatial audio channel placements</summary>
    /// <param name="soundFile">
    ///   SNDFILE instance for whose channels the spatial placements will be returned
    /// </param>
    /// <returns>A vector containign the spatial placements of all audio channels</returns>
    public: static std::vector<int> GetChannelMapInfo(
      const std::shared_ptr<::SNDFILE> &soundFile
    );

  };

  // ------------------------------------------------------------------------------------------- //

}}} // namespace Nuclex::OpusTranscoder::Platform

#endif // NUCLEX_OPUSTRANSCODER_PLATFORM_SNDFILEAPI_H
