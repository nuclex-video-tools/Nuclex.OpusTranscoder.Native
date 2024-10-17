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
#include "../Audio/Normalizer.h"
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
    targetBitrate(192),
    normalize(false),
    effort(1.0f),
    inputPath(),
    inputChannelOrder(),
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

  void Transcoder::SetNightmodeLevel(float newNightmodeLevel /* = 0.5f */) {
    this->nightmodeLevel = newNightmodeLevel;
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::SetOutputChannels(Nuclex::Audio::ChannelPlacement channels) {
    this->outputChannels = channels;
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::SetTargetBitrate(float birateInKilobits) {
    this->targetBitrate = birateInKilobits;
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::EnableNormalization(bool enable /* = true */) {
    this->normalize = enable;
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::SetEffort(float newEffort) {
    this->effort = newEffort;
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::TranscodeAudioFile(
    const std::string &activeInputPath,
    const std::string &activeOutputPath
  ) {
    {
      std::lock_guard<std::mutex> trackAccessScope(this->trackAccessMutex);

      this->inputPath = activeInputPath;
      this->outputPath = activeOutputPath;

      this->outcome.reset();
      this->currentStepDescription.assign(u8"Starting...", 11);
      this->currentStepProgress = -1.0f;
    }

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
      std::shared_ptr<const Nuclex::Audio::Storage::VirtualFile> file;
      {
        std::string localInputPath;
        {
          std::lock_guard<std::mutex> metadataAccessScope(this->trackAccessMutex);
          localInputPath.swap(this->inputPath);
        }

        file = Nuclex::Audio::Storage::VirtualFile::OpenRealFileForReading(localInputPath);
      }

      // Read the entire input file with all audio samples into memory
      std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> track = (
        decodeInputFile(file, canceler)
      );

      // If normalization is enabled (to bring up the volume for too quiet tracks),
      // do it before downmixing. This way around there should be less precision loss.
      if(this->normalize) {
        normalizeTrack(track, canceler);
      }

      // Downmix and/or reorder the audio channels to the Vorbis channel order
      transformToOutputLayout(track, canceler);

      // DISABLED: There are lots of quirky surround mixes. Sometimes the combined
      // volume goes over 1.0 (a conforming surround mix should keep the overall volume
      // at the same level as stereo, not use the additional speaker for yield),
      // sometimes it is too quiet. Our 'Nightmode' option also makes this unpredictable.
      // 
      // If the layout transform is a downmix, normalize again because the channels
      // might not add up to the full stereo range.
      //bool isDownMix = (this->outputChannelOrder.size() < this->inputChannelOrder.size());
      //if(this->normalize && isDownMix) {
      //  normalizeTrack()
      //}

      // TODO: Check overall signal level and warn user if downmix is too loud.

      // If de-clipping is active, scan the original audio samples for clipping
      if(this->declip) {
        findClippingHalfwaves(track, canceler);

        // For single-pass declipping, all we do is de-clip the original audio track.
        // In case iterative declipping is chosen, we encode and verify first.
        if(!this->iterativeDeclip) {
          declipTrack(track, canceler);
        }
      }

      // Now encode the file. Unless iterative declipping is used, this will be
      // saved to disk right after. Otherwise, we begin the long-winded declipping loop
      std::shared_ptr<const Nuclex::Audio::Storage::VirtualFile> encodedOpusFile = (
        encodeTrack(track, track->Samples, canceler)
      );
      if(this->declip && this->iterativeDeclip) {
        for(;;) {

          // Decode the Opus file again to see where the codec introduced clipping.
          // This will now add a second, full and uncompressed copy of the raw audio
          // data into memory, possibly amounting to 10+ GiB of data overall.
          std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> decodedOpusFile = (
            decodeInputFile(encodedOpusFile, canceler)
          );

          findClippingHalfwaves(decodedOpusFile, canceler);
          Audio::ClippingDetector::IntegrateClippingInstances(track, decodedOpusFile);

          std::size_t remaining = updateClippingHalfwaves(
            track, decodedOpusFile->Samples, canceler
          );
          if(remaining == 0) {
            break;
          }

          // We'll sneakily reuse the decoded Opus file's sample array. This saves
          // us one full reallocation to hold the original samples with de-clipped
          // half-waves (we don't touch the original audio track because we don't
          // want to modify it multiple times in succession - generation loss and all).
          copyAndDeclipTrack(track, decodedOpusFile->Samples, canceler);
          encodedOpusFile = encodeTrack(track, decodedOpusFile->Samples, canceler);

          // Free the memory of the raw samples used for encoding
          std::vector<float>().swap(decodedOpusFile->Samples); // free all memory

        } // for each iteration attempting to de-clip the output
      } // if iterative clipping enabled

      // If this point is reached, either declipping was off, or only a single pass was
      // requested, or the iterative declipper has done its work.
      writeVirtualFileToDisk(encodedOpusFile, this->outputPath);
    }
    catch(const std::exception &error) {
      this->currentStepDescription.assign(
        std::string(u8"Transcoding failed: ", 20) + error.what()
      );
      this->currentStepProgress = -1.0f;
      this->outcome = false;

      this->Ended.Emit();

      throw; //std::rethrow_exception(std::current_exception());
    }

    this->currentStepDescription.assign(u8"Transcoding complete!", 21);
    this->currentStepProgress = -1.0f;
    this->outcome = true;

    this->Ended.Emit();
  }

  // ------------------------------------------------------------------------------------------- //

  std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> Transcoder::decodeInputFile(
    const std::shared_ptr<const Nuclex::Audio::Storage::VirtualFile> &file,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {
    onStepBegun(std::string(u8"Opening input audio file...", 27));
    onStepProgressed(0.0f);

    Nuclex::Audio::TrackInfo trackInfo;

    // Open a decoder for the input file
    std::shared_ptr<Nuclex::Audio::Storage::AudioTrackDecoder> decoder;
    {
      // TODO: This information should be available from the decoder, too
      std::optional<Nuclex::Audio::ContainerInfo> metadata = this->loader->TryReadInfo(file);
      if(!metadata.has_value()) {
        throw std::runtime_error(u8"Unsupported file type");
      }
      if(metadata.value().Tracks.size() == 0) {
        throw std::runtime_error(u8"File contains no audio streams");
      }
      trackInfo = metadata.value().Tracks[0];

      decoder = this->loader->OpenDecoder(file);
    }

    canceler->ThrowIfCanceled();
    onStepBegun(std::string(u8"Allocating memory...", 20));

    // Create a track with the appropriate number of channels
    std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> newTrack;
    {
      newTrack = std::make_shared<Nuclex::OpusTranscoder::Audio::Track>();

      newTrack->Channels.resize(decoder->CountChannels());
      newTrack->Samples.resize(decoder->CountFrames() * decoder->CountChannels());
      newTrack->SampleRate = trackInfo.SampleRate;
    }

    canceler->ThrowIfCanceled();
    onStepBegun(std::string(u8"Decoding input audio file..."));

    // Remember the channel order in the input audio file (that's the one we'll read)
    {
      this->inputChannelOrder = decoder->GetChannelOrder();
      for(std::size_t index = 0; index < decoder->CountChannels(); ++index) {
        newTrack->Channels[index].InputOrder = index;
        newTrack->Channels[index].Placement = this->inputChannelOrder[index];
      }
    }

    canceler->ThrowIfCanceled();

    // Figure out a chunk size that is not more than 1 second (for the sole reason
    // that the user should be able to cancel the transcode without much delay)
    std::size_t framesPerChunk;
    {
      std::uint64_t totalFrameCount = decoder->CountFrames();
      while(48000 < totalFrameCount) {
        totalFrameCount >>= 1;
      }

      framesPerChunk = static_cast<std::size_t>(totalFrameCount);
    }

    canceler->ThrowIfCanceled();

    // Decode all of the audio data (yes, potentially Gigabytes of it) into our
    // samples array. It will be interleaved using the source channel order.
    {
      std::uint64_t remainingFrameCount = decoder->CountFrames();

      float *writeOffset = newTrack->Samples.data();
      std::uint64_t writeFrameIndex = 0;

      while(0 < remainingFrameCount) {
        if(remainingFrameCount < framesPerChunk) {
          framesPerChunk = static_cast<std::size_t>(remainingFrameCount);
        }

        decoder->DecodeInterleaved<float>(writeOffset, writeFrameIndex, framesPerChunk);
        canceler->ThrowIfCanceled();

        writeOffset += framesPerChunk * decoder->CountChannels();
        writeFrameIndex += framesPerChunk;
        remainingFrameCount -= framesPerChunk;

        onStepProgressed(
          static_cast<float>(writeFrameIndex) / static_cast<float>(
            writeFrameIndex + remainingFrameCount
          )
        );
      }
    }

    // The track is all set up, hand it out
    return newTrack;
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::normalizeTrack(
    const std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> &track,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {
    using Nuclex::Support::Events::Delegate;

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<Transcoder, &Transcoder::onStepProgressed>(this)
    );

    // If true, the normalizer will also go to work if the overall volume is above 1.0.
    // This would sabotage the de-clipper, so we don't want it. Normalization here is
    // only to bring too audio tracks that are too quiet back in line.
    constexpr bool allowVolumeDecrease = false;

    onStepBegun(std::string(u8"Normalizing track volume...", 27));
    Audio::Normalizer::Normalize(
      track, allowVolumeDecrease, canceler, progressCallback
    );
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::transformToOutputLayout(
    const std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> &track,
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
    if(track->Channels.size() < outputChannelCount) {
      onStepBegun(std::string(u8"Upmixing to stereo...", 21));
      ChannelLayoutTransformer::UpmixToStereo(
        track, canceler, progressCallback
      );
    } else if(outputChannelCount < track->Channels.size()) {
      if(this->outputChannels == Stereo) {
        onStepBegun(std::string(u8"Downmixing to stereo...", 23));
        ChannelLayoutTransformer::DownmixToStereo(
          track, this->nightmodeLevel, canceler, progressCallback
        );
      } else if(this->outputChannels == FiveDotOne) {
        onStepBegun(std::string(u8"Upmixing 7.1 to 5.1...", 22));
        ChannelLayoutTransformer::DownmixToFiveDotOne(
          track, canceler, progressCallback
        );
      } else {
        throw std::runtime_error(u8"Non-standard output channel layouts are not supported");
      }
    } else if(this->inputChannelOrder != this->outputChannelOrder) {
      onStepBegun(std::string(u8"Reordering audio channels...", 28));
      ChannelLayoutTransformer::ReweaveToVorbisLayout(
        track, canceler, progressCallback
      );
    }
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::findClippingHalfwaves(
    const std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> &track,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {
    using Nuclex::Support::Events::Delegate;

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<Transcoder, &Transcoder::onStepProgressed>(this)
    );

    onStepBegun(std::string(u8"Checking audio track for clipping...", 36));
    Audio::ClippingDetector::FindClippingHalfwaves(
      track, canceler, progressCallback
    );
  }

  // ------------------------------------------------------------------------------------------- //

  std::size_t Transcoder::updateClippingHalfwaves(
    const std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> &track,
    const std::vector<float> &samples,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {
    using Nuclex::Support::Events::Delegate;

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<Transcoder, &Transcoder::onStepProgressed>(this)
    );

    onStepBegun(std::string(u8"Checking audio track for clipping...", 36));
    return Audio::ClippingDetector::UpdateClippingHalfwaves(
      track, samples, canceler, progressCallback
    );
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::declipTrack(
    const std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> &track,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {
    using Nuclex::Support::Events::Delegate;

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<Transcoder, &Transcoder::onStepProgressed>(this)
    );

    onStepBegun(std::string(u8"Tucking in clipping segments...", 31));
    Audio::HalfwaveTucker::TuckClippingHalfwaves(
      track, canceler, progressCallback
    );
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::copyAndDeclipTrack(
    const std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> &track,
    std::vector<float> &samples,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {
    using Nuclex::Support::Events::Delegate;

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<Transcoder, &Transcoder::onStepProgressed>(this)
    );

    onStepBegun(std::string(u8"Tucking in clipping segments...", 31));
    Audio::HalfwaveTucker::TuckClippingHalfwaves(
      track, samples, canceler, progressCallback
    );
  }

  // ------------------------------------------------------------------------------------------- //

  std::shared_ptr<const Nuclex::Audio::Storage::VirtualFile> Transcoder::encodeTrack(
    const std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> &track,
    const std::vector<float> &samples,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
  ) {
    using Nuclex::Support::Events::Delegate;

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<Transcoder, &Transcoder::onStepProgressed>(this)
    );

    onStepBegun(std::string(u8"Encoding Opus audio stream...", 29));

    return Audio::OpusEncoder::Encode(
      track,
      samples,
      this->targetBitrate,
      this->effort,
      canceler,
      progressCallback
    );
  }

  // ------------------------------------------------------------------------------------------- //

  void Transcoder::writeVirtualFileToDisk(
    const std::shared_ptr<const Nuclex::Audio::Storage::VirtualFile> &file,
    const std::string &fileOutputPath
  ) {
    std::vector<std::byte> buffer(65536);

    std::shared_ptr<Nuclex::Audio::Storage::VirtualFile> outFile = (
      Nuclex::Audio::Storage::VirtualFile::OpenRealFileForWriting(fileOutputPath)
    );

    std::uint64_t length = file->GetSize();
    std::uint64_t offset = 0;
    while(0 < length) {
      std::size_t chunkSize;
      if(length < 65536) {
        chunkSize = static_cast<std::size_t>(length);
      } else {
        chunkSize = 65536;
      }

      file->ReadAt(offset, chunkSize, buffer.data());
      outFile->WriteAt(offset, chunkSize, buffer.data());

      offset += chunkSize;
      length -= chunkSize;
    }
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
