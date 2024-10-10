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

#include "./OpusTranscoder.h"

#include <Nuclex/Support/Threading/StopToken.h>
#include <Nuclex/Support/Threading/Thread.h>
#include <Nuclex/Support/BitTricks.h>

#include <Nuclex/Audio/Storage/AudioLoader.h>
#include <Nuclex/Audio/Storage/AudioTrackDecoder.h>

#include "../Audio/Track.h"

#include <QDir>

namespace {

  // ------------------------------------------------------------------------------------------- //

  // TODO: Lifted from the ChannelOrderFactory in Nuclex.Audio.Native
  //   Once I have the Encoder there implemented, it will allow me to query
  //   the interleaved channel order for encoding from there and this can go away.

  /// <summary>
  ///   Generates an ordered channel list according to the conventions used by
  ///   the Vorbis specification (which also applies to Opus)
  /// </summary>
  /// <param name="mappingFamily">Vorbis mapping family the channels conform to</param>
  /// <param name="channelCount">Number of audio channels in the audiop file</param>
  /// <returns>A vector containing the audio channels in the interleaved order</returns>
  std::vector<Nuclex::Audio::ChannelPlacement> ChannelOrderFromVorbisFamilyAndCount(
    int mappingFamily, std::size_t channelCount
  ) {
    std::vector<Nuclex::Audio::ChannelPlacement> channelOrder;
    channelOrder.reserve(channelCount);

    if((mappingFamily == 0) || (mappingFamily == 1)) {
      std::size_t originalChannelCount = channelCount;
      if(originalChannelCount == 1) {
        channelOrder.push_back(Nuclex::Audio::ChannelPlacement::FrontCenter);
        --channelCount;
      } else if(originalChannelCount < 9) {
        channelOrder.push_back(Nuclex::Audio::ChannelPlacement::FrontLeft);
        channelCount -= 2;

        if((originalChannelCount == 3) || (originalChannelCount >= 5)) {
          channelOrder.push_back(Nuclex::Audio::ChannelPlacement::FrontCenter);
          --channelCount;
        }

        channelOrder.push_back(Nuclex::Audio::ChannelPlacement::FrontRight);

        if(originalChannelCount >= 7) {
          channelOrder.push_back(Nuclex::Audio::ChannelPlacement::SideLeft);
          channelCount -= 2;
          channelOrder.push_back(Nuclex::Audio::ChannelPlacement::SideRight);
        }

        if(originalChannelCount == 7) {
          channelOrder.push_back(Nuclex::Audio::ChannelPlacement::BackCenter);
          --channelCount;
        }

        if((originalChannelCount >= 4) && (originalChannelCount != 7)) {
          channelOrder.push_back(Nuclex::Audio::ChannelPlacement::BackLeft);
          channelCount -= 2;
          channelOrder.push_back(Nuclex::Audio::ChannelPlacement::BackRight);
        }

        if(originalChannelCount >= 6) {
          channelOrder.push_back(Nuclex::Audio::ChannelPlacement::LowFrequencyEffects);
          --channelCount;
        }
      }
    }

    while(channelCount >= 1) {
      channelOrder.push_back(Nuclex::Audio::ChannelPlacement::Unknown);
      --channelCount;
    }

    return channelOrder;
  }

  // ------------------------------------------------------------------------------------------- //

} // anonymous namespace

namespace Nuclex::OpusTranscoder::Services {

  // ------------------------------------------------------------------------------------------- //

  OpusTranscoder::OpusTranscoder(
    const std::shared_ptr<Nuclex::Audio::Storage::AudioLoader> &loader
  ) :
    loader(loader),
    trackAccessMutex(),
    declip(false),
    iterativeDeclip(false),
    nightmodeLevel(0.5f),
    outputChannels(Nuclex::Audio::ChannelPlacement::Unknown),
    inputPath(),
    inputChannelOrder(),
    track(),
    outputPath(),
    outputChannelOrder() {}

  // ------------------------------------------------------------------------------------------- //

  OpusTranscoder::~OpusTranscoder() {
    Cancel();
    Join();
  }

  // ------------------------------------------------------------------------------------------- //

  void OpusTranscoder::EnableClippingPrevention(bool enable /* = true */) {
    this->declip = enable;
  }

  // ------------------------------------------------------------------------------------------- //

  void OpusTranscoder::EnableIterativeDeclipping(bool enable /* = true */) {
    this->iterativeDeclip = enable;
  }

  // ------------------------------------------------------------------------------------------- //

  void OpusTranscoder::SetNightmodeLevel(float nightmodeLevel /* = 0.5f */) {
    this->nightmodeLevel = nightmodeLevel;
  }

  // ------------------------------------------------------------------------------------------- //

  void OpusTranscoder::SetOutputChannels(Nuclex::Audio::ChannelPlacement channels) {
    this->outputChannels = channels;
  }

  // ------------------------------------------------------------------------------------------- //

  void OpusTranscoder::TranscodeAudioFile(
    const std::string &inputPath,
    const std::string &outputPath,
    Nuclex::Audio::ChannelPlacement &outputChannels
  ) {
    {
      std::unique_lock<std::mutex> trackAccessScope(this->trackAccessMutex);
      this->inputPath = inputPath;
      this->outputPath = outputPath;

      StartOrRestart();
    }

    //this->Updated.Emit();
  }

  // ------------------------------------------------------------------------------------------- //

  void OpusTranscoder::DoWork(
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {

    // Read the entire input file with all audio samples into memory
    decodeInputFile(canceler);

    // Figure out the correct output channel order for the Opus file
    std::size_t outputChannelCount = Nuclex::Support::BitTricks::CountBits(
      static_cast<std::size_t>(this->outputChannels)
    );
    this->outputChannelOrder = ChannelOrderFromVorbisFamilyAndCount(1, outputChannelCount);

    // Now transform the input audio samples, downmixing, upmixing or re-weaving
    // the interleaved channels into the correct order.
    if(this->track->Channels.size() < outputChannelCount) {
      upmixInputFile(canceler);
    } else if(outputChannelCount < this->track->Channels.size()) {
      downmixInputFile(canceler);
    } else if(this->inputChannelOrder != this->outputChannelOrder) {
      reweaveInputFile(canceler);
    }

    // TODO: Encode
  }

  // ------------------------------------------------------------------------------------------- //

  void OpusTranscoder::decodeInputFile(
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {

    // Open a decoder for the input file
    std::shared_ptr<Nuclex::Audio::Storage::AudioTrackDecoder> decoder;
    {
      std::string inputPath;
      {
        std::lock_guard<std::mutex> metadataAccessScope(this->trackAccessMutex);
        inputPath.swap(this->inputPath);
      }

      decoder = this->loader->OpenDecoder(inputPath);
    }

    canceler->ThrowIfCanceled();

    // Create a track with the appropriate number of channels
    std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> track;
    {
      track = std::make_shared<Nuclex::OpusTranscoder::Audio::Track>();

      track->Channels.resize(decoder->CountChannels());
      track->Samples.resize(decoder->CountFrames() * decoder->CountChannels());
    }

    canceler->ThrowIfCanceled();

    // Remember the channel order in the input audio file (that's the one we'll read)
    {
      this->inputChannelOrder = decoder->GetChannelOrder();
      for(std::size_t index = 0; index < decoder->CountChannels(); ++index) {
        track->Channels[index].InputOrder = index;
        track->Channels[index].Placement = this->inputChannelOrder[index];
      }
    }

    canceler->ThrowIfCanceled();

    // Figure out a chunk size that is not more than 1 second (for the sole reason
    // that the user should be able to cancel the transcode without much delay)
    std::size_t chunkSize;
    {
      std::uint64_t totalFrameCount = decoder->CountFrames();
      while(48000 < totalFrameCount) {
        totalFrameCount >>= 1;
      }

      chunkSize = static_cast<std::size_t>(totalFrameCount);
    }

    canceler->ThrowIfCanceled();

    // Decode all of the audio data (yes, potentially Gigabytes of it) into our
    // samples array. It will be interleaved using the source channel order.
    {
      std::uint64_t remainingFrameCount = decoder->CountFrames();

      float *writeOffset = track->Samples.data();
      std::uint64_t writeFrameIndex = 0;

      while(0 < remainingFrameCount) {
        if(remainingFrameCount < chunkSize) {
          chunkSize = static_cast<std::size_t>(remainingFrameCount);
        }

        decoder->DecodeInterleaved<float>(writeOffset, writeFrameIndex, chunkSize);
        canceler->ThrowIfCanceled();

        writeOffset += chunkSize;
        writeFrameIndex += chunkSize;
        remainingFrameCount -= chunkSize;
      }
    }

    // The track is all set up, hand it out
    this->track.swap(track);

  }

  // ------------------------------------------------------------------------------------------- //

  void OpusTranscoder::downmixInputFile(
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {

  }

  // ------------------------------------------------------------------------------------------- //

  void OpusTranscoder::upmixInputFile(
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {

  }

  // ------------------------------------------------------------------------------------------- //

  void OpusTranscoder::reweaveInputFile(
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {
    
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::CriuGui::Services
