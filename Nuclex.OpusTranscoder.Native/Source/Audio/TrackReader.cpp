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

#include <type_traits> // for std::is_same

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

    // Pick up some channel properties while we got them in front of us...
    {    
      std::vector<int> channelPlacements = (
        Nuclex::OpusTranscoder::Platform::SndFileApi::GetChannelMapInfo(soundFile)
      );
      for(std::size_t index = 0; index < channelCount; ++index) {
        track->GetChannel(index).SetChannelPlacement(channelPlacements[index]);
        track->GetChannel(index).SetSampleRate(soundFileInformation.samplerate);
      }
    }

    // Make sure the load has not been cancelled in the meantime
    if(static_cast<bool>(stopToken)) {
      stopToken->ThrowIfCanceled();
    }

    // Read the actual audio samples and split them into non-interleaved channels.
    // The libsndfile examples deinterleave by linearly reading the source buffer,
    // we instead do it channel-by-channel (the third argument to AppendSamples()
    // is the number of samples to skip, we use it to skip over the other channels).
    const std::size_t ChunkSize = 16384;
    if constexpr(std::is_same<TSample, double>::value) {
      std::vector<double> buffer;
      buffer.resize(ChunkSize);
      for(;;) {
        int readFrameCount = ::sf_readf_double(
          soundFile.get(),
          buffer.data(), ChunkSize / channelCount
        );
        if(readFrameCount == 0) {
          break; // There's no error return for sf_readf_float(), only EOF.
        }
        for(std::size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
          track->GetChannel(channelIndex).AppendSamples(
            buffer.data() + channelIndex, readFrameCount, channelCount - 1
          );
        }
      }
    } else if constexpr(std::is_same<TSample, float>::value) {
      std::vector<float> buffer;
      buffer.resize(ChunkSize);
      for(;;) {
        int readFrameCount = ::sf_readf_float(
          soundFile.get(),
          buffer.data(), ChunkSize / channelCount
        );
        if(readFrameCount == 0) {
          break; // There's no error return for sf_readf_float(), only EOF.
        }
        for(std::size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
          track->GetChannel(channelIndex).AppendSamples(
            buffer.data() + channelIndex, readFrameCount, channelCount - 1
          );
        }
      }
    } else if constexpr(std::is_same<TSample, std::uint16_t>::value) {
      std::vector<std::int16_t> buffer;
      buffer.resize(ChunkSize);
      for(;;) {
        int readFrameCount = ::sf_readf_short(
          soundFile.get(),
          buffer.data(), ChunkSize / channelCount
        );
        if(readFrameCount == 0) {
          break; // There's no error return for sf_readf_float(), only EOF.
        }
        for(std::size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
          track->GetChannel(channelIndex).AppendSamples(
            buffer.data() + channelIndex, readFrameCount, channelCount - 1
          );
        }
      }
    } else {
      throw std::logic_error(u8"This data type is not supported by the TrackReader");
    }

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
