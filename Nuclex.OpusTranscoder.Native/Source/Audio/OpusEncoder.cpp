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

#include "./OpusEncoder.h"

#include <opusenc.h>

// NOTE:
//
// This is just a stand-in until I have completed encoding support in Nuclex;Audio
//

namespace {

  // ------------------------------------------------------------------------------------------- //

  int opusWrite(void *userData, const std::uint8_t *data, ::opus_int32 length) {
    std::vector<std::byte> &fileContents = *(
      reinterpret_cast<std::vector<std::byte> *>(userData)
    );
    fileContents.insert(
      fileContents.end(),
      reinterpret_cast<const std::byte *>(data),
      reinterpret_cast<const std::byte *>(data + length)
    );
    return 0;
  }

  // ------------------------------------------------------------------------------------------- //

  int opusClose(void *userData) {
    (void)userData;
    return 0;
  }

  // ------------------------------------------------------------------------------------------- //

} // anonymous namespace

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  void OpusEncoder::Encode(
    const std::shared_ptr<Track> &track,
    int sampleRate, float bitRateInKilobits,
    const std::shared_ptr<Nuclex::Audio::Storage::VirtualFile> &target,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
    Nuclex::Support::Events::Delegate<void(float)> &progressCallback
  ) {
    ::OpusEncCallbacks callbacks;
    callbacks.write = &opusWrite;
    callbacks.close = &opusClose;

    std::vector<std::byte> fileContents;

    std::shared_ptr<::OggOpusComments> comments;
    {
      ::OggOpusComments *rawComments = ::ope_comments_create();
      if(rawComments == nullptr) {
        throw std::runtime_error(u8"Unable to create Opus comment container");
      }

      comments.reset(rawComments, &ope_comments_destroy);
    }

    int family = (track->Channels.size() == 2) ? 0 : 1;
    int error = 0;

    std::shared_ptr<::OggOpusEnc> encoder;
    {
      OggOpusEnc *rawEncoder = ::ope_encoder_create_callbacks(
        &callbacks, &fileContents, comments.get(),
        sampleRate, track->Channels.size(), family, &error
      );
      if(rawEncoder == nullptr) {
        throw std::runtime_error(u8"Unable to create Opus encoder");
      }

      encoder.reset(rawEncoder, &::ope_encoder_destroy);
    }

    // Set some encoder options
    //
    int result = ::ope_encoder_ctl(
      encoder.get(), OPUS_SET_BITRATE_REQUEST, static_cast<int>(bitRateInKilobits * 1000.0f)
    );
    if(result != OPE_OK) {
      throw std::runtime_error(u8"Unable to set target bitrate of Opus encoder");
    }
    result = ::ope_encoder_ctl(
      encoder.get(), OPUS_SET_APPLICATION_REQUEST, OPUS_APPLICATION_AUDIO
    );
    if(result != OPE_OK) {
      throw std::runtime_error(u8"Unable to set application in Opus encoder");
    }
    result = ::ope_encoder_ctl(
      encoder.get(), OPUS_SET_SIGNAL_REQUEST, OPUS_SIGNAL_MUSIC
    );
    if(result != OPE_OK) {
      throw std::runtime_error(u8"Unable to set signal type to music in Opus encoder");
    }
    result = ::ope_encoder_ctl(
      encoder.get(), OPUS_SET_COMPLEXITY_REQUEST, 10
    );
    if(result != OPE_OK) {
      throw std::runtime_error(u8"Unable to set quality level in Opus encoder");
    }

    // The samples are already interleaved in Vorbis channel order, so we can
    // use them as-is and simply divide by the channel count to obtain the frame count.
    std::uint64_t totalFrameCount = track->Samples.size() / track->Channels.size();
    std::uint64_t remainingFrameCount = totalFrameCount;
    const float *samples = track->Samples.data();

    while(0 < remainingFrameCount) {
      
      // Encode up to 12000 samples per call (arbitrarily chosen but regular enough
      // that cancellation from the user will yield a quick reaction)
      std::size_t frameCountInChunk;
      if(12000 < remainingFrameCount) {
        frameCountInChunk = 12000;
      } else {
        frameCountInChunk = remainingFrameCount;
      }

      // Feed the samples to the Opus encoder.
      result = ::ope_encoder_write_float(encoder.get(), samples, frameCountInChunk);
      if(result != OPE_OK) {
        throw std::runtime_error("Opus encoder failed to process samples");
      }

      // The Opus encoder always processes all samples it is fed (probably keeping
      // additional samples in an internal buffer, thus the need for ope_encoder_drain()).
      samples += frameCountInChunk;
      remainingFrameCount -= frameCountInChunk;

      // Check if the user wants to cancel and send out a progress report
      canceler->ThrowIfCanceled();
      progressCallback(
        (
          static_cast<float>(totalFrameCount) -
          static_cast<float>(remainingFrameCount)
        ) / (
          static_cast<float>(totalFrameCount)
        )
      );

    } // while samples to encode remain

    // Finalize the stream
    result = ::ope_encoder_drain(encoder.get());
    if(result != OPE_OK) {
      throw std::runtime_error("Opus encoder could not finalize output stream");
    }
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio
