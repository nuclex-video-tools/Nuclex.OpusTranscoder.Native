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

#include "WavPackApi.h"

#include <Nuclex/Support/Text/StringConverter.h>

#include <stdexcept>

namespace {

  // ------------------------------------------------------------------------------------------- //
  // ------------------------------------------------------------------------------------------- //

} // anonymous namespace

namespace Nuclex { namespace OpusTranscoder { namespace Platform {

  // ------------------------------------------------------------------------------------------- //

  std::shared_ptr<::WavpackContext> WavPackApi::OpenInputFile(
    const std::string &path, int flags /* = OPEN_FILE_UTF8 */, int normOffset /* = 0 */
  ) {
    std::string errorMessage;
    errorMessage.resize(81);

    WavpackContext *context = ::WavpackOpenFileInput(
      path.c_str(), errorMessage.data(), flags, normOffset
    );
    if(context == nullptr) {
      std::string message(u8"Error opening '", 15);
      message.append(path);
      message.append(u8"': ", 3);
      message.append(errorMessage.c_str()); // because it's only part filled and zero t'ed.
      throw std::runtime_error(message);
    }

    return std::shared_ptr<WavpackContext>(context, &::WavpackCloseFile);
  }

  // ------------------------------------------------------------------------------------------- //

  int WavPackApi::GetMode(const std::shared_ptr<::WavpackContext> &context) {
    // No error return (confirmed via common_utils.c in 2024)
    return ::WavpackGetMode(context.get());
  }

  // ------------------------------------------------------------------------------------------- //

  int WavPackApi::GetNumChannels(const std::shared_ptr<::WavpackContext> &context) {
    // No error return (confirmed via common_utils.c in 2024)
    return ::WavpackGetNumChannels(context.get());
  }

  // ------------------------------------------------------------------------------------------- //

  int WavPackApi::GetChannelMask(const std::shared_ptr<::WavpackContext> &context) {
    // No error return (confirmed via common_utils.c in 2024)
    return ::WavpackGetChannelMask(context.get());
  }

  // ------------------------------------------------------------------------------------------- //

  std::uint32_t WavPackApi::GetSampleRate(const std::shared_ptr<::WavpackContext> &context) {
    // No error return (confirmed via common_utils.c in 2024)
    return ::WavpackGetSampleRate(context.get());
  }

  // ------------------------------------------------------------------------------------------- //

  int WavPackApi::GetBitsPerSample(const std::shared_ptr<::WavpackContext> &context) {
    // No error return (confirmed via common_utils.c in 2024)
    return ::WavpackGetBitsPerSample(context.get());
  }

  // ------------------------------------------------------------------------------------------- //

  int WavPackApi::GetBytesPerSample(const std::shared_ptr<::WavpackContext> &context) {
    // No error return (confirmed via common_utils.c in 2024)
    return ::WavpackGetBytesPerSample(context.get());
  }

  // ------------------------------------------------------------------------------------------- //

  std::int64_t WavPackApi::GetNumSamples64(
    const std::shared_ptr<::WavpackContext> &context
  ) {
    std::int64_t sampleCount = ::WavpackGetNumSamples(context.get());
    if(sampleCount == -1) {
      throw std::runtime_error(u8"Unable to determine sample rate of WavPack audio file");
    }

    return sampleCount;
  }

  // ------------------------------------------------------------------------------------------- //

  std::uint32_t WavPackApi::UnpackSamples(
    const std::shared_ptr<::WavpackContext> &context,
    std::int32_t *buffer,
    std::uint32_t sampleCount
  ) {
    std::uint32_t unpackedSampleCount = ::WavpackUnpackSamples(
      context.get(), buffer, sampleCount
    );

    // I can't really find any solid documentation on how errors during unpacking are
    // supposed to surface, but there are several places inside libwavpack where it
    // checks the first character of the context's error_message, so we'll do the same.
    // This might mean that after one failure, the context will appear to be in
    // a permanent error state, unless the context's error_message is cleared somewhere.
    const char *errorMessage = ::WavpackGetErrorMessage(context.get());
    if(errorMessage != nullptr) {
      if(errorMessage[0] != 0) {
        std::string message(u8"Error unpacking samples from WavPack audio file: ", 49);
        message.append(errorMessage);
        throw std::runtime_error(message);
      }
    }

    return unpackedSampleCount;
  }

  // ------------------------------------------------------------------------------------------- //

}}} // namespace Nuclex::OpusTranscoder::Platform
