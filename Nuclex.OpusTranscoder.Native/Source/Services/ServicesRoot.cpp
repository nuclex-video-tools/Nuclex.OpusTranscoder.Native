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

#include "./ServicesRoot.h"

#include "./MetadataReader.h"
#include <Nuclex/Audio/Storage/AudioLoader.h>

//#include <QDir>

namespace {

  // ------------------------------------------------------------------------------------------- //
  // ------------------------------------------------------------------------------------------- //

} // anonymous namespace

namespace Nuclex::OpusTranscoder::Services {

  // ------------------------------------------------------------------------------------------- //

  ServicesRoot::ServicesRoot() :
    audioLoader(std::make_shared<Audio::Storage::AudioLoader>()),
    metadataReader() {

    this->metadataReader = std::make_shared<MetadataReader>(this->audioLoader);
  }

  // ------------------------------------------------------------------------------------------- //

  ServicesRoot::~ServicesRoot() {}

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::CriuGui::Services
