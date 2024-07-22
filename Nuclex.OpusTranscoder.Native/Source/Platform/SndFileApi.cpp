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

// If the library is compiled as a DLL, this ensures symbols are exported
#define NUCLEX_OPUSTRANSCODER_SOURCE 1

#include "SndFileApi.h"

#include <Nuclex/Support/Text/StringConverter.h>

#include <stdexcept>

namespace {

  // ------------------------------------------------------------------------------------------- //
  // ------------------------------------------------------------------------------------------- //

} // anonymous namespace

namespace Nuclex { namespace OpusTranscoder { namespace Platform {

  // ------------------------------------------------------------------------------------------- //

  std::shared_ptr<::SNDFILE> SndFileApi::OpenForReading(
    const std::string &path, ::SF_INFO &soundFileInformation
  ) {

    // Try to load it. Because of Microsoft's unicode mess, there's a completely
    // different sf_open() method for Windows systems to placate the strange
    // UTF16 choice and avoid the use of ANSI code pages for Windows users...
    ::SNDFILE *newSoundFile;
#if defined(NUCLEX_OPUSTRANSCODER_WINDOWS)
    {
      std::wstring utf16Path = Nuclex::Support::Text::StringConverter::WideFromUtf8(path);
      newSoundFile = ::sf_wchar_open(utf16Path.c_str(), SFM_READ, &soundFileInformation);
    }
#else
    newSoundFile = ::sf_open(path.c_str(), SFM_READ, &soundFileInformation);
#endif

    // Now check if audio file was opened successfully, or if we encountered an error
    if(unlikely(newSoundFile == nullptr)) {
      const char *messageAsCString = ::sf_strerror(nullptr);

      std::string message(u8"Error opening audio file '", 26);
      message.append(path);
      message.append(u8"' for reading", 13);

      if(messageAsCString != nullptr) {
        message.append(u8": ", 2);
        message.append(messageAsCString);
      }

      throw std::runtime_error(message);
    }

    return std::shared_ptr<::SNDFILE>(newSoundFile, ::sf_close);
  }

  // ------------------------------------------------------------------------------------------- //

  ::SF_INFO SndFileApi::GetSoundFileInfo(const std::shared_ptr<::SNDFILE> &soundFile) {
    ::SF_INFO soundFileInfo = {0};

    int result = ::sf_command(
      soundFile.get(),
      SFC_GET_CURRENT_SF_INFO,
      reinterpret_cast<void *>(&soundFileInfo), sizeof(soundFileInfo)
    );
    if(unlikely(result != 0)) {
      const char *messageAsCString = ::sf_strerror(nullptr);

      std::string message(u8"Error retrieving sound file info", 32);
      if(messageAsCString != nullptr) {
        message.append(u8": ", 2);
        message.append(messageAsCString);
      }

      throw std::runtime_error(message);
    }
    
    return soundFileInfo;
  }

  // ------------------------------------------------------------------------------------------- //

  std::vector<int> SndFileApi::GetChannelMapInfo(const std::shared_ptr<::SNDFILE> &soundFile) {
    std::vector<int> channelPlacements;
    {
      ::SF_INFO soundFileInfo = GetSoundFileInfo(soundFile);
      channelPlacements.resize(soundFileInfo.channels);
    }

    int result = ::sf_command(
      soundFile.get(),
      SFC_GET_CHANNEL_MAP_INFO,
      channelPlacements.data(), channelPlacements.size() * sizeof(int)
    );
    if(unlikely(result != SF_TRUE)) { // Yep, this one returns 1 for good, 0 for bad O_o
      const char *messageAsCString = ::sf_strerror(soundFile.get());

      std::string message(u8"Error retrieving channel mappings", 33);
      if(messageAsCString != nullptr) {
        message.append(u8": ", 2);
        message.append(messageAsCString);
      }

      throw std::runtime_error(message);
    }
    
    return channelPlacements;
  }

  // ------------------------------------------------------------------------------------------- //

}}} // namespace Nuclex::OpusTranscoder::Platform
