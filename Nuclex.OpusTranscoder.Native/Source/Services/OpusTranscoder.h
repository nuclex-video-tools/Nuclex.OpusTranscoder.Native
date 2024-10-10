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

#ifndef NUCLEX_OPUSTRANSCODER_SERVICES_OPUSTRANSCODER_H
#define NUCLEX_OPUSTRANSCODER_SERVICES_OPUSTRANSCODER_H

#include "../Config.h"

#include <Nuclex/Support/Threading/ConcurrentJob.h> // for ConcurrentJob
#include <Nuclex/Support/Events/ConcurrentEvent.h> // for ConcurrentEvent
#include <Nuclex/Audio/TrackInfo.h> // for TrackInfo

#include <memory> // for std::shared_ptr
#include <mutex> // for std::mutex

namespace Nuclex::Audio::Storage {

  // ------------------------------------------------------------------------------------------- //

  class AudioLoader;

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::Audio::Storage

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  class Track;

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio

namespace Nuclex::OpusTranscoder::Services {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Transcodes an input audio file into the Opus format</summary>
  class OpusTranscoder : public Nuclex::Support::Threading::ConcurrentJob {

    /// <summary>Fired when the transcoder has started a new action</summary>
    public: Nuclex::Support::Events::ConcurrentEvent<void(const std::string &)> StepBegun;
    /// <summary>Fired when the transcoder's current action has made progress</summary>
    public: Nuclex::Support::Events::ConcurrentEvent<void(float)> Progressed;

    // ----------------------------------------------------------------------------------------- //

    /// <summary>Initializes a new audio metadata reader</summary>
    public: OpusTranscoder(const std::shared_ptr<Nuclex::Audio::Storage::AudioLoader> &loader);
    /// <summary>Stops the checking thread and frees all resources</summary>
    public: ~OpusTranscoder() override;

    // ----------------------------------------------------------------------------------------- //

    /// <summary>Enables normal clipping prevention, tucking in half-waves</summary>
    /// <param name="enable">True to enable clipping prevention, false to disable</param>
    /// <remarks>
    ///   If this option is on, the input audio signal will be modified by tucking in
    ///   any half-wave whose peak exceeds the maximum signal level (this is possible
    ///   for float audio input files).
    /// </remarks>
    public: void EnableClippingPrevention(bool enable = true);

    /// <summary>Enables iterative decoding and adjustment of the input signal</summary>
    /// <param name="enable">True to enable iterative declipping, false to disable</param>
    /// <remarks>
    ///   With this option, the encoded Opus audio stream will be decoded right after
    ///   completing it and checked for clipping. The Opus codec often introduces small
    ///   clipping waves even in places where the signal was fine before. Such clipping
    ///   half-waves will then be tucked in further in the original audio data but based
    ///   on the clipping observed in the decoded Opus stream. This cycle is repeated
    ///   until the decoded Opus audio stream contains no more clipping or adjustment stops
    ///   producing any improvement.
    /// </remarks>
    public: void EnableIterativeDeclipping(bool enable = true);

    /// <summary>
    ///   Sets the level to which the alternative nightmode downmix formula will be applied
    /// </summary>
    /// <param name="nightmodeLevel">Level to which the nightmode formula will be applied</param>
    public: void SetNightmodeLevel(float nightmodeLevel = 0.5f);

    /// <summary>Selects the channels that will be present in the output file</summary>
    /// <param name="channels">Channels that should be present in the output file</param>
    public: void SetOutputChannels(Nuclex::Audio::ChannelPlacement channels);

    /// <summary>Analyzes the specified audio file</summary>
    /// <param name="path">Path to the audio file that will be analyzed</param>
    /// <remarks>
    ///   Immediately clears the current analyzed file and begins analyzing
    ///   the specified file, aborting any running analysis.
    /// </remarks>
    public: void TranscodeAudioFile(
      const std::string &inputPath,
      const std::string &outputPath,
      Nuclex::Audio::ChannelPlacement &outputChannels
    );

    // ----------------------------------------------------------------------------------------- //

    /// <summary>Called in the background thread to perform the actual work</summary>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    protected: void DoWork(
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
    ) override;

    // ----------------------------------------------------------------------------------------- //

    /// <summary>Decodes all audio samples from the input file into memory</summary>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    private: void decodeInputFile(
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
    );

    /// <summary>Performs a downmix of the input channels to stereo</summary>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    private: void downmixInputFile(
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
    );

    /// <summary>Performs an upmix of the input channel to stereo</summary>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    private: void upmixInputFile(
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
    );

    /// <summary>Reorders the channels of the input file to the Vorbis order</summary>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    private: void reweaveInputFile(
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
    );

    // ----------------------------------------------------------------------------------------- //

    /// <summary>Handles loading and decoding of audio files</summary>
    private: std::shared_ptr<Nuclex::Audio::Storage::AudioLoader> loader;
    /// <summary>Must be held when accessing the paths or audio data</summary>
    private: mutable std::mutex trackAccessMutex;
    /// <summary>Path of the file being transcoded</summary>
    private: std::string inputPath;
    /// <summary>Path under which the encoded Opus file will be saved</summary>
    private: std::string outputPath;
    /// <summary>Metadata for the current audio file, if any</summary>
    private: std::optional<Nuclex::Audio::TrackInfo> metadata;
    /// <summary>Stores the full decoded audio track and clipping information</summary>
    private: std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> track;
    /// <summary>Whether to de-clip the input file before encoding</summary>
    private: bool declip;
    /// <summary>Whether to check the encoded Opus file for clipping, too</summary>
    private: bool iterativeDeclip;
    /// <summary>Level to which to apply the alternative Nightmode downmix formula</summary>
    private: float nightmodeLevel;
    /// <summary>Channels that should be present in the output file</summary>
    private: Nuclex::Audio::ChannelPlacement outputChannels;
  
  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Services

#endif // NUCLEX_OPUSTRANSCODER_SERVICES_OPUSTRANSCODER_H
