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
#include "../Platform/WavPackApi.h"

#include <Nuclex/Support/Threading/StopToken.h>

#include <type_traits> // for std::is_same

namespace {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Checks whether a path ends with the specified characters</summary>
  /// <param name="path">Path whose ending will be checked</param>
  /// <param name="ending">Characters the path's ending will be checked against</param>
  /// <returns>True if the path ended with the specified characters</returns>
  bool pathEndsWith(const std::string &path, const std::string &ending) {
    std::string::size_type pathLength = path.length();
    std::string::size_type endingLength = ending.length();
    if(pathLength < endingLength) {
      return false;
    }

    const char *pathCharacter = path.data() + pathLength - endingLength;
    for(std::string::size_type index = 0; index < endingLength; ++index) {
      if(*pathCharacter != ending[index]) {
        return false;
      }
      ++pathCharacter;
    }

    return true;
  }

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Reads a whole audio track using libsndfile</summary>
  /// <typeparam name="TSample">Data type as which the samples should be returned</typeparam>
  /// <param name="path">Path of the audio file that will be loaded</param>
  /// <param name="stopToken">Token by which the load can be canceled</param>
  /// <returns>An audio track containing the audio data from the specifid file</returns>
  template<typename TSample>
  std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track<TSample>> readAudioTrackViaLibSndFile(
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

  /// <summary>Reads a whole audio track using libsndfile</summary>
  /// <typeparam name="TSample">Data type as which the samples should be returned</typeparam>
  /// <param name="path">Path of the audio file that will be loaded</param>
  /// <param name="stopToken">Token by which the load can be canceled</param>
  /// <returns>An audio track containing the audio data from the specifid file</returns>
  template<typename TSample>
  std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track<TSample>> readAudioTrackViaWavPack(
    const std::string &path,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &stopToken = (
      std::shared_ptr<const Nuclex::Support::Threading::StopToken>()
    )
  ) {
    using Nuclex::OpusTranscoder::Platform::WavPackApi;

    std::shared_ptr<::WavpackContext> wavpackFile = WavPackApi::OpenInputFile(path);

    int channelCount = WavPackApi::GetNumChannels(wavpackFile);

    std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track<TSample>> track = (
      std::make_shared<Nuclex::OpusTranscoder::Audio::Track<TSample>>(channelCount)
    );

    // Initialize the sample rate and set all channel placements to invalid
    {
      std::uint32_t sampleRate = WavPackApi::GetSampleRate(wavpackFile);
      for(std::size_t index = 0; index < channelCount; ++index) {
        track->GetChannel(index).SetSampleRate(sampleRate);
        track->GetChannel(index).SetChannelPlacement(SF_CHANNEL_MAP_INVALID);
      }
    }

    // Assign the channel placements. I started this with a for loop, but the switch
    // was bigger than just dumping a number of if statements like this:
    {
      int channelMask = WavPackApi::GetChannelMask(wavpackFile);

      std::size_t channelIndex = 0;
      if((channelMask & 1) != 0) {
        track->GetChannel(channelIndex).SetChannelPlacement(SF_CHANNEL_MAP_FRONT_LEFT);
        ++channelIndex;
      }
      if((channelMask & 2) != 0) {
        track->GetChannel(channelIndex).SetChannelPlacement(SF_CHANNEL_MAP_FRONT_RIGHT);
        ++channelIndex;
      }
      if((channelMask & 4) != 0) {
        track->GetChannel(channelIndex).SetChannelPlacement(SF_CHANNEL_MAP_FRONT_CENTER);
        ++channelIndex;
      }
      if((channelMask & 8) != 0) {
        track->GetChannel(channelIndex).SetChannelPlacement(SF_CHANNEL_MAP_LFE);
        ++channelIndex;
      }
      if((channelMask & 16) != 0) {
        track->GetChannel(channelIndex).SetChannelPlacement(SF_CHANNEL_MAP_REAR_LEFT);
        ++channelIndex;
      }
      if((channelMask & 32) != 0) {
        track->GetChannel(channelIndex).SetChannelPlacement(SF_CHANNEL_MAP_REAR_RIGHT);
        ++channelIndex;
      }
      if((channelMask & 64) != 0) {
        track->GetChannel(channelIndex).SetChannelPlacement(SF_CHANNEL_MAP_FRONT_LEFT_OF_CENTER);
        ++channelIndex;
      }
      if((channelMask & 128) != 0) {
        track->GetChannel(channelIndex).SetChannelPlacement(SF_CHANNEL_MAP_FRONT_RIGHT_OF_CENTER);
        ++channelIndex;
      }
      if((channelMask & 256) != 0) {
        track->GetChannel(channelIndex).SetChannelPlacement(SF_CHANNEL_MAP_REAR_CENTER);
        ++channelIndex;
      }
      if((channelMask & 512) != 0) {
        track->GetChannel(channelIndex).SetChannelPlacement(SF_CHANNEL_MAP_SIDE_LEFT);
        ++channelIndex;
      }
      if((channelMask & 1024) != 0) {
        track->GetChannel(channelIndex).SetChannelPlacement(SF_CHANNEL_MAP_SIDE_RIGHT);
        ++channelIndex;
      }
    }

    int compressionMode = WavPackApi::GetMode(wavpackFile);
    int bitsPerSample = WavPackApi::GetBitsPerSample(wavpackFile);
    int bytesPerSample = WavPackApi::GetBytesPerSample(wavpackFile);

    const std::size_t ChunkSize = 16384;
    std::vector<std::int32_t> nativeBuffer;
    //std::vector<TSample> buffer;
    nativeBuffer.resize(ChunkSize * channelCount);
    //buffer.resize(ChunkSize * channelCount);

    for(;;) {
      int unpackedSampleCount = WavPackApi::UnpackSamples(
        wavpackFile, nativeBuffer.data(), ChunkSize
      );
      if(unpackedSampleCount == 0) {
        break;
      }

      if((compressionMode && MODE_FLOAT) && (bytesPerSample == 4)) {
        if constexpr(std::is_same<TSample, float>::value) {
          float *data = reinterpret_cast<float *>(nativeBuffer.data());
          for(std::size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
            track->GetChannel(channelIndex).AppendSamples(
              data + channelIndex, unpackedSampleCount, channelCount - 1
            );
          }
        } else if constexpr(std::is_same<TSample, std::uint32_t>::value) {
          throw std::runtime_error(u8"Sample conversion not implemented yet");
        } else if constexpr(std::is_same<TSample, std::uint16_t>::value) {
          throw std::runtime_error(u8"Sample conversion not implemented yet");
        }
      } else {
        if constexpr(std::is_same<TSample, float>::value) {
          throw std::runtime_error(u8"Sample conversion not implemented yet");
        } else if constexpr(std::is_same<TSample, std::uint32_t>::value) {
          throw std::runtime_error(u8"Sample conversion not implemented yet");
        } else if constexpr(std::is_same<TSample, std::uint16_t>::value) {
          throw std::runtime_error(u8"Sample conversion not implemented yet");
        }
      }
    }

    return track;
  }

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Reads a whole audio track using libsndfile</summary>
  /// <typeparam name="TSample">Data type as which the samples should be returned</typeparam>
  /// <param name="path">Path of the audio file that will be loaded</param>
  /// <param name="stopToken">Token by which the load can be canceled</param>
  /// <returns>An audio track containing the audio data from the specifid file</returns>
  template<typename TSample>
  std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track<TSample>> readAudioTrack(
    const std::string &path,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &stopToken = (
      std::shared_ptr<const Nuclex::Support::Threading::StopToken>()
    )
  ) {
    // There was been talk about integrated WavPack into libsndfile, but I can't find
    // any hint of WavPack in the sources, so we'll cheaply check if the file ending
    // is for WavPack (.wv) and load it directly via libwavpack then.
    if(pathEndsWith(path, u8".wv")) {
      return readAudioTrackViaWavPack<TSample>(path, stopToken);
    } else {
      return readAudioTrackViaLibSndFile<TSample>(path, stopToken);
    }
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
