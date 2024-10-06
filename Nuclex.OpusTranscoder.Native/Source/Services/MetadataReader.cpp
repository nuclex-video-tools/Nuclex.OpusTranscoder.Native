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

// If the application is compiled as a DLL, this ensures symbols are exported
#define NUCLEX_CRIUGUI_SOURCE 1

#include "./MetadataReader.h"

#include <Nuclex/Support/Threading/StopToken.h>
#include <Nuclex/Support/Threading/Thread.h>

#include <Nuclex/Audio/Storage/AudioLoader.h>

#include <QDir>

namespace {

  // ------------------------------------------------------------------------------------------- //
  // ------------------------------------------------------------------------------------------- //

} // anonymous namespace

namespace Nuclex::OpusTranscoder::Services {

  // ------------------------------------------------------------------------------------------- //

  MetadataReader::MetadataReader(
    const std::shared_ptr<Nuclex::Audio::Storage::AudioLoader> &loader
  ) :
    loader(loader),
    metadataAccessMutex(),
    path(),
    metadata() {}

  // ------------------------------------------------------------------------------------------- //

  MetadataReader::~MetadataReader() {
    Cancel();
    Join();
  }

  // ------------------------------------------------------------------------------------------- //

  void MetadataReader::AnalyzeAudioFile(const std::string &path) {
    {
      std::unique_lock<std::mutex> metadataAccessScope(this->metadataAccessMutex);

      this->path = path;
      this->metadata.reset();

      StartOrRestart();
    }

    this->Updated.Emit();
  }

  // ------------------------------------------------------------------------------------------- //

  std::optional<Nuclex::Audio::TrackInfo> MetadataReader::GetMetadata() const {
    std::unique_lock<std::mutex> metadataAccessScope(this->metadataAccessMutex);
    return this->metadata;
  }

  // ------------------------------------------------------------------------------------------- //

  void MetadataReader::DoWork(
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {
    std::string path;
    {
      std::unique_lock<std::mutex> metadataAccessScope(this->metadataAccessMutex);
      path.swap(this->path);
    }

    std::optional<Audio::ContainerInfo> metadata = this->loader->TryReadInfo(path);
    {
      std::unique_lock<std::mutex> metadataAccessScope(this->metadataAccessMutex);

      // Only write the metadata if the path is still empty. If another file was selected
      // by the user in the meantime, we don't want to bring up the wrong file's metadata.
      if(this->path.empty()) {
        if(metadata.has_value()) {
          this->metadata = metadata.value().Tracks.at(0);
        } else {
          this->metadata.reset();
        }
      }
    }

    this->Updated.Emit();
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::CriuGui::Services
