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
#include <Nuclex/Audio/Storage/AudioSaver.h>
#include <Nuclex/Audio/Storage/AudioTrackEncoderBuilder.h>
#include <Nuclex/Audio/Storage/AudioTrackEncoder.h>

#include <cassert> // for assert()
#include <algorithm> // for std::copy_n()

namespace {

  // ------------------------------------------------------------------------------------------- //
  // ------------------------------------------------------------------------------------------- //

  /// <summary>Serves data contained in a byte buffer as a virtual file</summary>
  class MemoryFile : public Nuclex::Audio::Storage::VirtualFile {

    /// <summary>Initializes a new memory buffer based file</summary>
    /// <param name="contents">Memory buffer holding the data of the virtual file</param>
    public: MemoryFile(std::vector<std::byte> &&contents) :
      contents(std::move(contents)) {}

    /// <summary>Initializes a new memory buffer based file</summary>
    public: MemoryFile() :
      contents() {}

    /// <summary>Frees all memory used by the instance</summary>
    public: ~MemoryFile() override = default;

    /// <summary>Determines the current size of the file in bytes</summary>
    /// <returns>The size of the file in bytes</returns>
    public: std::uint64_t GetSize() const override { return this->contents.size(); }

    /// <summary>Reads data from the file</summary>
    /// <param name="start">Offset in the file at which to begin reading</param>
    /// <param name="byteCount">Number of bytes that will be read</param>
    /// <parma name="buffer">Buffer into which the data will be read</param>
    public: void ReadAt(
      std::uint64_t start, std::size_t byteCount, std::byte *buffer
    ) const override;

    /// <summary>Writes data into the file</summary>
    /// <param name="start">Offset at which writing will begin in the file</param>
    /// <param name="byteCount">Number of bytes that should be written</param>
    /// <param name="buffer">Buffer holding the data that should be written</param>
    public: void WriteAt(
      std::uint64_t start, std::size_t byteCount, const std::byte *buffer
    ) override;

    /// <summary>Memory buffer the virtual file implementation is serving data from</summary>
    private: std::vector<std::byte> contents;

  };

  // ------------------------------------------------------------------------------------------- //

  void MemoryFile::ReadAt(
    std::uint64_t start, std::size_t byteCount, std::byte *buffer
  ) const {
    assert((start < this->contents.size()) && u8"Read starts within file boundaries");
    assert((this->contents.size() >= start + byteCount) && u8"Read ends within file boundaries");
    std::copy_n(this->contents.data() + start, byteCount, buffer);
  }

  // ------------------------------------------------------------------------------------------- //

  void MemoryFile::WriteAt(
    std::uint64_t start, std::size_t byteCount, const std::byte *buffer
  ) {
    if(start < this->contents.size()) {
      std::size_t byteCountToCopy = std::min(this->contents.size() - start, byteCount);
      std::copy_n(buffer, byteCountToCopy, this->contents.data() + start);

      buffer += byteCountToCopy;
      byteCount -= byteCountToCopy;
    }

    this->contents.insert(this->contents.end(), buffer, buffer + byteCount);
  }

  // ------------------------------------------------------------------------------------------- //

} // anonymous namespace

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  std::shared_ptr<const Nuclex::Audio::Storage::VirtualFile> OpusEncoder::Encode(
    const std::shared_ptr<Track> &track,
    float bitRateInKilobits,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
    Nuclex::Support::Events::Delegate<void(float)> &progressCallback
  ) {
    Nuclex::Audio::Storage::AudioSaver saver;

    std::shared_ptr<MemoryFile> encodedFile = std::make_shared<MemoryFile>();

    std::shared_ptr<Nuclex::Audio::Storage::AudioTrackEncoder> encoder;
    if(track->Channels.size() == 2) {
      encoder = saver.ProvideBuilder(u8"Opus")->
        SetStereoChannels().
        SetCompressionEffort(1.0f).
        SetSampleRate(48000).
        SetTargetBitrate(bitRateInKilobits).
        Build(encodedFile);
    } else {
      encoder = saver.ProvideBuilder(u8"Opus")->
        SetFiveDotOneChannelsInVorbisOrder().
        SetCompressionEffort(1.0f).
        SetSampleRate(48000).
        SetTargetBitrate(bitRateInKilobits).
        Build(encodedFile);
    }

    // The samples are already interleaved in Vorbis channel order, so we can
    // use them as-is and simply divide by the channel count to obtain the frame count.
    std::uint64_t totalFrameCount = track->Samples.size() / track->Channels.size();
    std::uint64_t remainingFrameCount = totalFrameCount;
    const float *samples = track->Samples.data();

    //FILE *x = fopen(u8"/srv/video/test.dat", "wb");
    //fwrite(samples, sizeof(float), track->Samples.size(), x);
    //fflush(x);
    //fclose(x);

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
      encoder->EncodeInterleaved(samples, frameCountInChunk);

      // The Opus encoder always processes all samples it is fed (probably keeping
      // additional samples in an internal buffer, thus the need for ope_encoder_drain()).
      samples += frameCountInChunk * track->Channels.size();
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

    // Finalize the stream. This probably processes any input samples the opus encoder
    // had buffered, waiting for enough data to output a full block / packet.
    encoder->Flush();

    return encodedFile;
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio
