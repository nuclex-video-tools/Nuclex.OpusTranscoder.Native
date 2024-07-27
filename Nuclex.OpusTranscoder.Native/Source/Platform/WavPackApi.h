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

#ifndef NUCLEX_OPUSTRANSCODER_PLATFORM_WAVPACKAPI_H
#define NUCLEX_OPUSTRANSCODER_PLATFORM_WAVPACKAPI_H

#include "../Config.h"

#include <string> // for std::string
#include <memory> // for std::shared_ptr
#include <vector> // for std::vector
#include <cstdint>

#include <wavpack.h> // for WavPack

namespace Nuclex { namespace OpusTranscoder { namespace Platform {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Wraps the WavPack API with error checking</summary>
  class WavPackApi {

    /// <summary>Opens a WavPack audio file for reading</summary>
    /// <param name="path">Path of the audio file in the file system</param>
    /// <param name="flags">
    ///   Options that control several aspects of how the file is opened (see docs)
    /// </param>
    /// <param name="normOffset">
    ///   Normalization exponent, if enabled. For floating point audio, each step doubles
    //    the maximum amplitude to which audio samples will be normalized.
    /// </param>
    /// <returns>
    ///   A shared pointer to a WavPack context that can be used with other functions
    ///   in the WavPack API
    /// </returns>
    /// <remarks>
    ///   The returned shared pointer has a custom deleter set up, so this is fully RAII
    ///   compatible and once the pointer goes out of scope, the WavPack context is
    ///   closed again.
    /// </remarks>
    public: static std::shared_ptr<::WavpackContext> OpenInputFile(
      const std::string &path, int flags = OPEN_FILE_UTF8 | OPEN_WVC | OPEN_TAGS, int normOffset = 0
    );

    /// <summary>Retrieves some flags describing the data and compression method</summary>
    /// <param name="context">Context of the opened WavPack file</param>
    /// <returns>The mode flags of the WavPack file</returns>
    public: static int GetMode(const std::shared_ptr<::WavpackContext> &context);

    /// <summary>Retrieves the number of audio channels in the WavPack file</summary>
    /// <param name="context">Context of the opened WavPack file</param>
    /// <returns>The total number of audio channels in the file</returns>
    public: static int GetNumChannels(const std::shared_ptr<::WavpackContext> &context);

    /// <summary>Retrieves the audio channels present in the WavPack file</summary>
    /// <param name="context">Context of the opened WavPack file</param>
    /// <returns>A bit mask indicating which channels the WavPack file contains</returns>
    public: static int GetChannelMask(const std::shared_ptr<::WavpackContext> &context);

    /// <summary>Retrieves the sample rate of the WavPack file</summary>
    /// <param name="context">Context of the opened WavPack file</param>
    /// <returns>The sample rate of the WavPack file</returns>
    public: static std::uint32_t GetSampleRate(const std::shared_ptr<::WavpackContext> &context);

    /// <summary>Retrieves the number of bits per sample the WavPack file stores</summary>
    /// <param name="context">Context of the opened WavPack file</param>
    /// <returns>The number of bits per sample in the WavPack file</returns>
    /// <remarks>
    ///   According to the documentation, if the number of bits is not divisible by 8,
    ///   data is shifted to the next highest byte boundary, leaving the least significant
    ///   bits zeroed (so this value must absolutely be taken into considering when doing
    ///   conversion to float and other formats). It can be ignored for floating point
    ///   audio since that is always 32 bits.
    /// </remarks>
    public: static int GetBitsPerSample(const std::shared_ptr<::WavpackContext> &context);

    /// <summary>Retrieves the number of bytes per sample the WavPack file stores</summary>
    /// <param name="context">Context of the opened WavPack file</param>
    /// <returns>The number of bytes per sample</returns>
    /// <remarks>
    ///   This is the same bit depth there's currently so much fuzz about with HD FLAC audio
    ///   releases and whatnot. When working with WavPack, it is relevant because WavPack
    ///   will always return samples as <code>uint32_t</code> values, but the range of
    ///   the values found inside that type will depend on the bytes per sample
    ///   (and to a lesser degree, bits per sample, see the relevant query method).
    /// </remarks>
    public: static int GetBytesPerSample(const std::shared_ptr<::WavpackContext> &context);

    /// <summary>Retrieves the number of samples in the WavPack file</summary>
    /// <param name="context">Context of the opened WavPack file</param>
    /// <returns>The total number of samples stored in the WavPack file</returns>
    /// <remarks>
    ///   This is the number of samples in each channel. Audio libraries often call this
    ///   frames, such that (frame = sample times num_channels). WavPack uses samples to
    ///   refer to both (), so if you unpack samples, you get them interleaved for all
    ///   channels, but they're still called samples.
    /// </remarks>
    public: static std::int64_t GetNumSamples64(
      const std::shared_ptr<::WavpackContext> &context
    );

    /// <summary>Unpacks samples from the WavPack file</summary>
    /// <param name="context">Context of the opened WavPack file</param>
    /// <param name="buffer">Buffer in which the unpacked samples will be placed</param>
    /// <param name="sampleCount">
    ///   Number of &quot;complete&quot; samples to unpack. The total number samples
    ///   written into the buffer is this value multiplied by the number of channels.
    /// </param>
    /// <returns>The total number of &quot;complete&quot; samples unpacked</returns>
    /// <remarks>
    ///   <para>
    ///     Audio data is interleaved. So if you request two samples here for a 5.1 audio
    ///     file, you'd get 12 values returned, starting with the first sample of each
    ///     channel in order, then followed by the second sample of each channel in order.
    ///   </para>
    ///   <para>
    ///     The ordering of the samples follows the order Microsoft established for
    ///     WAVE files through their audio channel mask:
    ///   </para>
    ///   <list type="table">
    ///     <listheader>
    ///       <term>Bit Number</term><description>Placement</description>
    ///     </listheader>
    ///     <item> <term>0 (0x1)</term>   <description>Front Left</description>         </item>
    ///     <item> <term>1 (0x2)</term>   <description>Front Right</description>        </item>
    ///     <item> <term>2 (0x4)</term>   <description>Front Center</description>       </item>
    ///     <item> <term>3 (0x8)</term>   <description>LFE (bass)</description>         </item>
    ///     <item> <term>4 (0x10)</term>  <description>Back Left</description>          </item>
    ///     <item> <term>5 (0x20)</term>  <description>Back Right</description>         </item>
    ///     <item> <term>6 (0x40)</term>  <description>Front Left Center</description>  </item>
    ///     <item> <term>7 (0x80)</term>  <description>Front Right Center</description> </item>
    ///     <item> <term>8 (0x100)</term> <description>Back Center</description>        </item>
    ///     <item> <term>9 (0x200)</term> <description>Side Left</description>          </item>
    ///     <item> <term>10 (0x400)</term><description>Side Right</description>         </item>
    ///   </list>
    /// </remarks>
    public: static std::uint32_t UnpackSamples(
      const std::shared_ptr<::WavpackContext> &context,
      std::int32_t *buffer,
      std::uint32_t sampleCount
    );

  };

  // ------------------------------------------------------------------------------------------- //

}}} // namespace Nuclex::OpusTranscoder::Platform

#endif // NUCLEX_OPUSTRANSCODER_PLATFORM_WAVPACKAPI_H
