#pragma region Apache License 2.0
/*
Nuclex CriuGui
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

#ifndef NUCLEX_OPUSTRANSCODER_SERVICES_SERVICESROOT_H
#define NUCLEX_OPUSTRANSCODER_SERVICES_SERVICESROOT_H

#include "../Config.h"

//#include <Nuclex/Platform/Locations/StandardDirectoryResolver.h> // for directory lookup

#include <memory> // for std::unique_ptr

namespace Nuclex::Audio::Storage {

  // ------------------------------------------------------------------------------------------- //

  class AudioLoader;

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::Audio::Storage

namespace Nuclex::OpusTranscoder::Services {

  // ------------------------------------------------------------------------------------------- //

  class MetadataReader;
  class Transcoder;

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Services

namespace Nuclex::OpusTranscoder::Services {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>
  ///   Stand-in for a small service locator, aka a bunch of app-global variables
  /// </summary>
  class ServicesRoot {

    /// <summary>Initializes a new service container</summary>
    public: ServicesRoot();
    /// <summary>Shuts down all services 
    public: ~ServicesRoot();

    // ----------------------------------------------------------------------------------------- //

    /// <summary>Accesses the background audio file metadata reader</summary>
    /// <returns>The background metadata reader</summary>
    public: const std::shared_ptr<MetadataReader> &GetMetadataReader() const {
      return this->metadataReader;
    }

    /// <summary>Accesses the background opus transcoder</summary>
    /// <returns>The background opus transcoder</summary>
    public: const std::shared_ptr<Transcoder> &GetOpusTranscoder() const {
      return this->opusTranscoder;
    }

    // ----------------------------------------------------------------------------------------- //

    /// <summary>Detects audio file formats and decodes audio from them</summary>
    private: std::shared_ptr<Audio::Storage::AudioLoader> audioLoader;
    /// <summary>Handles loading metadata in the background</summary>
    private: std::shared_ptr<MetadataReader> metadataReader;
    /// <summary>Runs the transcoding process in the background</summary>
    private: std::shared_ptr<Transcoder> opusTranscoder;

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Services

#endif // NUCLEX_OPUSTRANSCODER_SERVICES_SERVICESROOT_H
