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

#ifndef NUCLEX_OPUSTRANSCODER_SERVICES_TRANSCODER_H
#define NUCLEX_OPUSTRANSCODER_SERVICES_TRANSCODER_H

#include "../Config.h"

#include <Nuclex/Support/Threading/ConcurrentJob.h> // for ConcurrentJob
#include <Nuclex/Support/Events/ConcurrentEvent.h> // for ConcurrentEvent
#include <Nuclex/Audio/TrackInfo.h> // for TrackInfo

#include <memory> // for std::shared_ptr
#include <mutex> // for std::mutex

namespace Nuclex::Audio::Storage {

  // ------------------------------------------------------------------------------------------- //

  class AudioLoader;
  class VirtualFile;

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
  class Transcoder : public Nuclex::Support::Threading::ConcurrentJob {

    /// <summary>Fired when the transcoder has started a new action</summary>
    public: Nuclex::Support::Events::ConcurrentEvent<void()> StepBegun;
    /// <summary>Fired when the transcoder's current action has made progress</summary>
    public: Nuclex::Support::Events::ConcurrentEvent<void()> Progressed;
    /// <summary>Fired when the transcoder has shut down for whatever reason</summary>
    public: Nuclex::Support::Events::ConcurrentEvent<void()> Ended;

    // ----------------------------------------------------------------------------------------- //

    /// <summary>Initializes a new audio metadata reader</summary>
    public: Transcoder(const std::shared_ptr<Nuclex::Audio::Storage::AudioLoader> &loader);
    /// <summary>Stops the checking thread and frees all resources</summary>
    public: ~Transcoder() override;

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

    /// <summary>Sets the target bitrate for the Opus file in kilobits per second</summary>
    /// <param name="birateInKilobits">Target bitrate of the Opus file</param>
    public: void SetTargetBitrate(float birateInKilobits);

    /// <summary>Enables signal level normalization for the encoded audio data</summary>
    /// <param name="enable">True to enable normalization, false to disable it</param>
    public: void EnableNormalization(bool enable = true);

    /// <summary>Chooses the amount of effort to invest into optimal compression</summary>
    /// <param name="effort">The amount of effort to invest on a scale from 0.0 to 1.0</param>
    public: void SetEffort(float effort);

    /// <summary>Transcodes the specified audio file to an Opus audio file</summary>
    /// <param name="inputPath">Path to the audio file that will be transcoded</param>
    /// <param name="outputPath">Path where the produced Opus file will be saved</param>
    public: void TranscodeAudioFile(
      const std::string &inputPath,
      const std::string &outputPath
    );

    /// <summary>Queries the step the transcoder is currently executing</summary>
    /// <returns>A human-readable description of the currently running step<?returns>
    public: std::string GetCurrentTranscodeStep() const;

    /// <summary>Queries the progress of the currently running step</summary>
    /// <returns>The progress of the currently running step</returns>
    public: float GetCurrentStepProgress() const;

    /// <summary>Returns whether the transcode was successful or failed</summary>
    /// <returns>
    ///   True if the transcoded succeeded, false if it failed, nothing if it is still ongoing
    /// </returns>
    public: std::optional<bool> GetOutcome() const;

    // ----------------------------------------------------------------------------------------- //

    /// <summary>Called in the background thread to perform the actual work</summary>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    protected: void DoWork(
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
    ) override;

    // ----------------------------------------------------------------------------------------- //

    /// <summary>Decodes all audio samples from the input file into memory</summary>
    /// <param name="file">File from which the Opus audio data will be decoded</param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    /// <returns>The decoded input file and all needed metadata</returns>
    private: std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> decodeInputFile(
      const std::shared_ptr<const Nuclex::Audio::Storage::VirtualFile> &file,
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
    );

    /// <summary>Transforms the input channels ot the selected output layout</summary>
    /// <param name="track">Track that will be transformed to the output channel layout</param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    /// <remarks>
    ///   This will upmix, downmix or reorder the audio samples to either of the two
    ///   support channel layouts - stereo or 5.1 surround.
    /// </remarks>
    private: void transformToOutputLayout(
      const std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> &track,
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
    );

    /// <summary>Normalizes the volume of the track</summary>
    /// <param name="track">Track that will be normalized</param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    private: void normalizeTrack(
      const std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> &track,
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
    );

    /// <summary>Looks for instances of clipping in the input file</summary>
    /// <param name="track">Track in which clipping half-waves will be looked for</param>
    /// <param name="samples">Samples in which the clipping half-waves will be checked</param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    private: void findClippingHalfwaves(
      const std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> &track,
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
    );

    /// <summary>Re-examines the clipping hakf-waves in the new audio sample array</summary>
    /// <param name="track">Track in which clipping half-waves will be looked for</param>
    /// <param name="samples">Samples in which the clipping half-waves will be checked</param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    /// <returns>The number of clipping half-waves still present</returns>
    private: std::size_t updateClippingHalfwaves(
      const std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> &track,
      const std::vector<float> &samples,
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
    );

    /// <summary>Removes clipping from the original audio track</summary>
    /// <param name="track">Track that will be de-clipped</param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    private: void declipTrack(
      const std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> &track,
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
    );

    /// <summary>Copies an audio track while removing clipping from it</summary>
    /// <param name="track">Track that will be de-clipped</param>
    /// <param name="samples">Sample array the copy will be written into</param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    private: void copyAndDeclipTrack(
      const std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> &track,
      std::vector<float> &samples,
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
    );

    /// <summary>Remvoes clipping from the original audio track</summary>
    /// <param name="track">Track that will be encoded to Op[us</param>]
    /// <param name="sample">Array containing the interleaved samples to encode</param>
    /// <param name="canceler">Token by which the operation can be signalled to cancel</param>
    private: std::shared_ptr<const Nuclex::Audio::Storage::VirtualFile> encodeTrack(
      const std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> &track,
      const std::vector<float> &samples,
      const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler
    );

    /// <summary>Writes the contents of the specified virtual file to real file</summary>
    /// <param name="file">File that will be written to disk</param>
    /// <param name="outputPath">Path in which the real file will be stored</param>
    private: void writeVirtualFileToDisk(
      const std::shared_ptr<const Nuclex::Audio::Storage::VirtualFile> &file,
      const std::string &outputPath
    );

    /// <summary>Reports when the transcoding step has started</summary>
    /// <param name="stepDescription">Description of the currently running step</param>
    private: void onStepBegun(const std::string &stepDescription);

    /// <summary>Reports the progress of the currently running step</summary>
    /// <param name="progress">Progress of the currently running step</param>
    private: void onStepProgressed(float progress);

    // ----------------------------------------------------------------------------------------- //

    /// <summary>Handles loading and decoding of audio files</summary>
    private: std::shared_ptr<Nuclex::Audio::Storage::AudioLoader> loader;
    /// <summary>Must be held when accessing the paths or audio data</summary>
    private: mutable std::mutex trackAccessMutex;
    /// <summary>Whether to de-clip the input file before encoding</summary>
    private: bool declip;
    /// <summary>Whether to check the encoded Opus file for clipping, too</summary>
    private: bool iterativeDeclip;
    /// <summary>Level to which to apply the alternative Nightmode downmix formula</summary>
    private: float nightmodeLevel;
    /// <summary>Channels that should be present in the encoded output file</summary>
    private: Nuclex::Audio::ChannelPlacement outputChannels;
    /// <summary>Target bitrate to encode the Opus output file to</summary>
    private: float targetBitrate;
    /// <summary>Whether the signal should be normalized before encoding</summary>
    private: bool normalize;
    /// <summary>Amount of compression effort on a scale from 0.0 to 1.0</summary>
    private: float effort;

    /// <summary>Path of the file being transcoded</summary>
    private: std::string inputPath;
    /// <summary>Order in which the input channels appear</summary>
    private: std::vector<Nuclex::Audio::ChannelPlacement> inputChannelOrder;

    /// <summary>Path under which the encoded Opus file will be saved</summary>
    private: std::string outputPath;
    /// <summary>Order in which the output channels appear</summary>
    private: std::vector<Nuclex::Audio::ChannelPlacement> outputChannelOrder;

    /// <summary>Description of the currently running transcode step</summary>
    private: std::string currentStepDescription;
    /// <summary>Progress of the currently running transcode step</summary>
    private: float currentStepProgress;
    /// <summary>Success/failure state of the transcode after it finished</summary>
    private: std::optional<bool> outcome;

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Services

#endif // NUCLEX_OPUSTRANSCODER_SERVICES_TRANSCODER_H
