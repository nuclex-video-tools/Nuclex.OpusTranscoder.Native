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

// If the application is compiled as a DLL, this ensures symbols are exported
#define NUCLEX_OPUSTRANSCODER_SOURCE 1

#include "./TrackReader.h"

#include "./Track.h"
#include "../Platform/SndFileApi.h"

#include <Nuclex/Support/Threading/StopToken.h>

namespace {

  // ------------------------------------------------------------------------------------------- //

  template<typename TSample>
  std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track<TSample>> readAudioTrack(
    const std::string &path,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &stopToken = (
      std::shared_ptr<const Nuclex::Support::Threading::StopToken>()
    )
  ) {

    // Open the audio file for reading
    ::SF_INFO soundFileInformation = {};
    std::shared_ptr<::SNDFILE> soundFile = (
      Nuclex::OpusTranscoder::Platform::SndFileApi::OpenForReading(path, soundFileInformation)
    );

    // Make sure the load has not been cancelled in the meantime
    if(static_cast<bool>(stopToken)) {
      stopToken->ThrowIfCanceled();
    }

    // Now create the audio track, which serves as the container for the audio channels
    std::size_t channelCount = static_cast<std::size_t>(soundFileInformation.channels);
    std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track<TSample>> track = (
      std::make_shared<Nuclex::OpusTranscoder::Audio::Track<TSample>>(channelCount)
    );
    
    std::vector<int> channelPlacements = (
      Nuclex::OpusTranscoder::Platform::SndFileApi::GetChannelMapInfo(soundFile)
    );

    return track;
  }

  // ------------------------------------------------------------------------------------------- //

} // anonymous namespace

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  template<> std::shared_ptr<Track<double>> TrackReader<double>::Read(
    const std::string &path,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &stopToken /* = (
      std::shared_ptr<const Nuclex::Support::Threading::StopToken>()
    ) */
  ) {
    return readAudioTrack<double>(path, stopToken);
  }

  // ------------------------------------------------------------------------------------------- //

  template<> std::shared_ptr<Track<float>> TrackReader<float>::Read(
    const std::string &path,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &stopToken /* = (
      std::shared_ptr<const Nuclex::Support::Threading::StopToken>()
    ) */
  ) {
    return readAudioTrack<float>(path, stopToken);
  }

  // ------------------------------------------------------------------------------------------- //

  template<> std::shared_ptr<Track<std::int16_t>> TrackReader<std::int16_t>::Read(
    const std::string &path,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &stopToken /* = (
      std::shared_ptr<const Nuclex::Support::Threading::StopToken>()
    ) */
  ) {
    return readAudioTrack<std::int16_t>(path, stopToken);
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio
