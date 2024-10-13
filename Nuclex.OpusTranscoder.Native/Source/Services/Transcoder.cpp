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

#include "./Transcoder.h"

#include <Nuclex/Support/Threading/StopToken.h>
#include <Nuclex/Support/Threading/Thread.h>
#include <Nuclex/Support/BitTricks.h>

#include <Nuclex/Audio/Storage/AudioLoader.h>
#include <Nuclex/Audio/Storage/AudioTrackDecoder.h>

#include "../Audio/Track.h"
#include "../Audio/ChannelLayoutTransformer.h"
#include "../Audio/ClippingDetector.h"
#include "../Audio/HalfwaveTucker.h"
#include "../Audio/OpusEncoder.h"

#include <QDir>

namespace {

  // ------------------------------------------------------------------------------------------- //

  const Nuclex::Audio::ChannelPlacement Stereo = (
    Nuclex::Audio::ChannelPlacement::FrontLeft |
    Nuclex::Audio::ChannelPlacement::FrontRight
  );

  // ------------------------------------------------------------------------------------------- //

  const Nuclex::Audio::ChannelPlacement FiveDotOne = (
    Nuclex::Audio::ChannelPlacement::FrontLeft |
    Nuclex::Audio::ChannelPlacement::FrontRight |
    Nuclex::Audio::ChannelPlacement::FrontCenter |
    Nuclex::Audio::ChannelPlacement::LowFrequencyEffects |
    Nuclex::Audio::ChannelPlacement::BackLeft |
    Nuclex::Audio::ChannelPlacement::BackRight
  );

  // ------------------------------------------------------------------------------------------- //

  const Nuclex::Audio::ChannelPlacement SevenDotOne = (
    Nuclex::Audio::ChannelPlacement::FrontLeft |
    Nuclex::Audio::ChannelPlacement::FrontRight |
    Nuclex::Audio::ChannelPlacement::FrontCenter |
    Nuclex::Audio::ChannelPlacement::LowFrequencyEffects |
    Nuclex::Audio::ChannelPlacement::BackLeft |
    Nuclex::Audio::ChannelPlacement::BackRight |
    Nuclex::Audio::ChannelPlacement::SideLeft |
    Nuclex::Audio::ChannelPlacement::SideRight
  );

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

  Transcoder::Transcoder(
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
    outputChannelOrder(),
    currentStepDescription(u8"Idle"),
    currentStepProgress(0.0f),
    outcome(true) {} // for consistency

  // ------------------------------------------------------------------------------------------- //

  Transcoder::~Transcoder() {
    Cancel();
    Join();
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::EnableClippingPrevention(bool enable /* = true */) {
    this->declip = enable;
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::EnableIterativeDeclipping(bool enable /* = true */) {
    this->iterativeDeclip = enable;
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::SetNightmodeLevel(float nightmodeLevel /* = 0.5f */) {
    this->nightmodeLevel = nightmodeLevel;
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::SetOutputChannels(Nuclex::Audio::ChannelPlacement channels) {
    this->outputChannels = channels;
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::TranscodeAudioFile(
    const std::string &inputPath,
    const std::string &outputPath
  ) {
    std::lock_guard<std::mutex> trackAccessScope(this->trackAccessMutex);
    this->inputPath = inputPath;
    this->outputPath = outputPath;

    this->outcome.reset();
    this->currentStepDescription.assign(u8"Starting...", 11);
    this->currentStepProgress = -1.0f;

    StartOrRestart();
  }

  // ------------------------------------------------------------------------------------------- //

  std::string Transcoder::GetCurrentTranscodeStep() const {
    std::lock_guard<std::mutex> trackAccessScope(this->trackAccessMutex);
    return this->currentStepDescription;
  }

  // ------------------------------------------------------------------------------------------- //
  
  float Transcoder::GetCurrentStepProgress() const {
    std::lock_guard<std::mutex> trackAccessScope(this->trackAccessMutex);
    return this->currentStepProgress;
  }

  // ------------------------------------------------------------------------------------------- //
  
  std::optional<bool> Transcoder::GetOutcome() const {
    std::lock_guard<std::mutex> trackAccessScope(this->trackAccessMutex);
    return this->outcome;
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::DoWork(
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {
    try {

      // Read the entire input file with all audio samples into memory
      decodeInputFile(canceler);

      // Downmix and/or reorder the audio channels to the Vorbis channel order
      transformToOutputLayout(canceler);

      if(this->declip && !this->iterativeDeclip) {
        findClippingHalfwaves(canceler);
        declipOriginalTrack(canceler);
      }

      encodeOriginalTrack(canceler);

      // TODO: Implement rest of transcode

    }
    catch(const std::exception &error) {
      this->currentStepDescription.assign(
        std::string(u8"Transcoding failed: ", 20) + error.what()
      );
      this->currentStepProgress = -1.0f;
      this->outcome = false;

      this->Ended.Emit();
      this->track.reset(); // to free the memory again (potentially Gigabytes)

      throw; //std::rethrow_exception(std::current_exception());
    }

    this->currentStepDescription.assign(u8"Transcoding complete!", 21);
    this->currentStepProgress = -1.0f;
    this->outcome = true;

    this->Ended.Emit();
    this->track.reset(); // to free the memory again (potentially Gigabytes)
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::decodeInputFile(
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {
    onStepBegun(std::string(u8"Opening input audio file...", 27));

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
    onStepBegun(std::string(u8"Allocating memory...", 20));

    // Create a track with the appropriate number of channels
    std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> track;
    {
      track = std::make_shared<Nuclex::OpusTranscoder::Audio::Track>();

      track->Channels.resize(decoder->CountChannels());
      track->Samples.resize(decoder->CountFrames() * decoder->CountChannels());
    }

    canceler->ThrowIfCanceled();
    onStepBegun(std::string(u8"Decoding input audio file..."));

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

        onStepProgressed(
          static_cast<float>(writeFrameIndex) / static_cast<float>(
            writeFrameIndex + remainingFrameCount
          )
        );
      }
    }

    // The track is all set up, hand it out
    this->track.swap(track);

  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::transformToOutputLayout(
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {
    using Nuclex::OpusTranscoder::Audio::ChannelLayoutTransformer;
    using Nuclex::Support::Events::Delegate;

    // Figure out the correct output channel order for the Opus file
    std::size_t outputChannelCount = Nuclex::Support::BitTricks::CountBits(
      static_cast<std::size_t>(this->outputChannels)
    );
    this->outputChannelOrder = ChannelOrderFromVorbisFamilyAndCount(1, outputChannelCount);

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<Transcoder, &Transcoder::onStepProgressed>(this)
    );

    // Now transform the input audio samples, downmixing, upmixing or re-weaving
    // the interleaved channels into the correct order.
    if(this->track->Channels.size() < outputChannelCount) {
      onStepBegun(std::string(u8"Upmixing to stereo...", 21));
      ChannelLayoutTransformer::UpmixToStereo(
        this->track, canceler, progressCallback
      );
    } else if(outputChannelCount < this->track->Channels.size()) {
      if(this->outputChannels == Stereo) {
        onStepBegun(std::string(u8"Downmixing to stereo...", 23));
        ChannelLayoutTransformer::DownmixToStereo(
          this->track, this->nightmodeLevel, canceler, progressCallback
        );
      } else if(this->outputChannels == FiveDotOne) {
        onStepBegun(std::string(u8"Upmixing 7.1 to 5.1...", 22));
        ChannelLayoutTransformer::DownmixToFiveDotOne(
          this->track, canceler, progressCallback
        );
      } else {
        throw std::runtime_error(u8"Non-standard output channel layouts are not supported");
      }
    } else if(this->inputChannelOrder != this->outputChannelOrder) {
      onStepBegun(std::string(u8"Reordering audio channels...", 28));
      ChannelLayoutTransformer::ReweaveToVorbisLayout(
        this->track, canceler, progressCallback
      );
    }
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::findClippingHalfwaves(
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {
    using Nuclex::Support::Events::Delegate;

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<Transcoder, &Transcoder::onStepProgressed>(this)
    );

    onStepBegun(std::string(u8"Checking audio track for clipping...", 36));
    Audio::ClippingDetector::FindClippingHalfwaves(
      this->track, canceler, progressCallback
    );
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::declipOriginalTrack(
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {
    using Nuclex::Support::Events::Delegate;

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<Transcoder, &Transcoder::onStepProgressed>(this)
    );

    onStepBegun(std::string(u8"Tucking in clipping segments...", 31));
    Audio::HalfwaveTucker::TuckClippingHalfwaves(
      this->track, canceler, progressCallback
    );
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::encodeOriginalTrack(
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {
    using Nuclex::Support::Events::Delegate;

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<Transcoder, &Transcoder::onStepProgressed>(this)
    );

    onStepBegun(std::string(u8"Encoding Opus audio stream...", 29));

    Audio::OpusEncoder::Encode(
      this->track,
      48000, // TODO: Obtain from settings in application
      192.0f, // TODO: Obtain from settings in application
      std::shared_ptr<Nuclex::Audio::Storage::VirtualFile>(),
      canceler, progressCallback
    );
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::onStepBegun(const std::string &stepDescription) {
    {
      std::lock_guard<std::mutex> trackAccessScope(this->trackAccessMutex);
      this->currentStepDescription = stepDescription;
    }

    this->StepBegun.Emit();
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::onStepProgressed(float progress) {
    {
      std::lock_guard<std::mutex> trackAccessScope(this->trackAccessMutex);
      this->currentStepProgress = progress;
    }

    this->Progressed.Emit();
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::CriuGui::Services
