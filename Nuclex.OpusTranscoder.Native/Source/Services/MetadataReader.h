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

#ifndef NUCLEX_OPUSTRANSCODER_SERVICES_METADATAREADER_H
#define NUCLEX_OPUSTRANSCODER_SERVICES_METADATAREADER_H

#include "../Config.h"

#include <Nuclex/Support/Threading/ConcurrentJob.h> // for ConcurrentJob
#include <Nuclex/Support/Events/ConcurrentEvent.h> // for ConcurrentEvent
#include <Nuclex/Audio/TrackInfo.h> // for TrackInfo

#include <memory> // for std::shared_ptr
#include <mutex> // for std::mutex

namespace Nuclex::Audio::Storage {

  // ------------------------------------------------------------------------------------------- //

  class AudioLoader;

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::Audio::Storage

namespace Nuclex::OpusTranscoder::Services {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Reads the metadata of audio files in the background</summary>
  class MetadataReader : public Nuclex::Support::Threading::ConcurrentJob {

    /// <summary>Fired when the recorded metadata is cleared or updated</summary>
    public: Nuclex::Support::Events::ConcurrentEvent<void()> Updated;

    // ----------------------------------------------------------------------------------------- //

    /// <summary>Initializes a new audio metadata reader</summary>
    /// <remarks>
    ///   A fresh live data updater will not do any updating until it has been given
    ///   a list of adopted processes to check on.
    /// </remarks>
    public: MetadataReader(const std::shared_ptr<Nuclex::Audio::Storage::AudioLoader> &loader);
    /// <summary>Stops the checking thread and frees all resources</summary>
    public: ~MetadataReader() override;

    // ----------------------------------------------------------------------------------------- //

    /// <summary>Analyzes the specified audio file</summary>
    /// <param name="path">Path to the audio file that will be analyzed</param>
    /// <remarks>
    ///   Immediately clears the current analyzed file and begins analyzing
    ///   the specified file, aborting any running analysis.
    /// </remarks>
    public: void AnalyzeAudioFile(const std::string &path);

    /// <summary>Retrieves the metadata of the most recently analyzed file</summary>
    /// <returns>The most recently analyzed file's metadata</returns>
    public: std::optional<Nuclex::Audio::TrackInfo> GetMetadata() const;

    // ----------------------------------------------------------------------------------------- //

    /// <summary>Called in the background thread to perform the actual work</summary>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    protected: void DoWork(
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
    ) override;

    // ----------------------------------------------------------------------------------------- //

    /// <summary>Handles loading and decoding audio files</summary>
    private: std::shared_ptr<Nuclex::Audio::Storage::AudioLoader> loader;
    /// <summary>Must be held when copying or updating the metadata</summary>
    private: mutable std::mutex metadataAccessMutex;
    /// <summary>Path of the file being examines, empty when started/done</summary>
    private: std::string path;
    /// <summary>Metadata for the current audio file, if any</summary>
    private: std::optional<Nuclex::Audio::TrackInfo> metadata;
  
  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Services

#endif // NUCLEX_OPUSTRANSCODER_SERVICES_METADATAREADER_H
