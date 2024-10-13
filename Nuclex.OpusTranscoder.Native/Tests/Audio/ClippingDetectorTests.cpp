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

#include "../../Source/Config.h"
#include "../../Source/Audio/Track.h"
#include "../../Source/Audio/ClippingDetector.h"

#include <Nuclex/Support/Threading/StopSource.h>

#include <gtest/gtest.h>

namespace {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Does absolutely nothing</summary>
  void doNothing(float) {}

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Creates a new stereo track without audio samples</summary>
  /// <returns>The new stereo track</returns>
  std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> makeStereoTrack() {
    using Nuclex::OpusTranscoder::Audio::Track;
    std::shared_ptr<Track> track = std::make_shared<Track>();

    track->Channels.resize(2);
    track->Channels[0].InputOrder = 0;
    track->Channels[0].Placement = Nuclex::Audio::ChannelPlacement::FrontLeft;
    track->Channels[1].InputOrder = 1;
    track->Channels[1].Placement = Nuclex::Audio::ChannelPlacement::FrontRight;

    return track;
  }

  // ------------------------------------------------------------------------------------------- //

} // anonymous namespace

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  TEST(ClippingDetectorTests, HandlesTracksWithoutClipping) {
    using Nuclex::Support::Events::Delegate;
    using Nuclex::Support::Threading::StopSource;
    using Nuclex::Support::Threading::StopToken;

    std::shared_ptr<Track> track = makeStereoTrack();
    track->Samples.resize(18);

    track->Samples[0] = -0.1f;  // 0
    track->Samples[2] = 0.1f;   // 1
    track->Samples[4] = 0.3f;   // 2
    track->Samples[6] = 0.5f;   // 3
    track->Samples[8] = 0.3f;   // 4
    track->Samples[10] = 0.1f;  // 5
    track->Samples[12] = -0.1f; // 6
    track->Samples[14] = -0.3f; // 7
    track->Samples[16] = -0.5f; // 8

    track->Samples[1] = 0.1f;   // 0
    track->Samples[3] = -0.1f;  // 1
    track->Samples[5] = -0.3f;  // 2
    track->Samples[7] = -0.1f;  // 3
    track->Samples[9] = 0.1f;   // 4
    track->Samples[11] = 0.3f;  // 5
    track->Samples[13] = 0.1f;  // 6
    track->Samples[15] = -0.1f; // 7
    track->Samples[17] = -0.3f; // 8

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<&doNothing>()
    );
    ClippingDetector::FindClippingHalfwaves(
      track, StopSource::Create()->GetToken(), progressCallback
    );

    EXPECT_EQ(track->Channels[0].ClippingHalfwaves.size(), 0U);
    EXPECT_EQ(track->Channels[1].ClippingHalfwaves.size(), 0U);
  }

  // ------------------------------------------------------------------------------------------- //

  TEST(ClippingDetectorTests, ReportsClippingFromBeginning) {
    using Nuclex::Support::Events::Delegate;
    using Nuclex::Support::Threading::StopSource;
    using Nuclex::Support::Threading::StopToken;

    std::shared_ptr<Track> track = makeStereoTrack();
    track->Samples.resize(18);

    track->Samples[0] = 1.1f;   // 0
    track->Samples[2] = 0.9f;   // 1
    track->Samples[4] = 0.5f;   // 2
    track->Samples[6] = 0.3f;   // 3
    track->Samples[8] = 0.1f;   // 4
    track->Samples[10] = -0.1f; // 5
    track->Samples[12] = -0.3f; // 6
    track->Samples[14] = -0.5f; // 7
    track->Samples[16] = -0.3f; // 8

    track->Samples[1] = 0.1f;   // 0
    track->Samples[3] = -0.1f;  // 1
    track->Samples[5] = -0.3f;  // 2
    track->Samples[7] = -0.5f;  // 3
    track->Samples[9] = -0.7f;  // 4
    track->Samples[11] = -0.9f; // 5
    track->Samples[13] = -1.1f; // 6
    track->Samples[15] = -0.5f; // 7
    track->Samples[17] = 0.1f;  // 8

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<&doNothing>()
    );
    ClippingDetector::FindClippingHalfwaves(
      track, StopSource::Create()->GetToken(), progressCallback
    );

    ASSERT_EQ(track->Channels[0].ClippingHalfwaves.size(), 1U);
    EXPECT_EQ(track->Channels[0].ClippingHalfwaves[0].PriorZeroCrossingIndex, 0);
    EXPECT_EQ(track->Channels[0].ClippingHalfwaves[0].NextZeroCrossingIndex, 5);
    ASSERT_EQ(track->Channels[1].ClippingHalfwaves.size(), 1U);
    EXPECT_EQ(track->Channels[1].ClippingHalfwaves[0].PriorZeroCrossingIndex, 1);
    EXPECT_EQ(track->Channels[1].ClippingHalfwaves[0].NextZeroCrossingIndex, 8);
  }

  // ------------------------------------------------------------------------------------------- //

  TEST(ClippingDetectorTests, ReportsClippingIntoEnding) {
    using Nuclex::Support::Events::Delegate;
    using Nuclex::Support::Threading::StopSource;
    using Nuclex::Support::Threading::StopToken;

    std::shared_ptr<Track> track = makeStereoTrack();
    track->Samples.resize(18);

    track->Samples[0] = 0.1f;  // 0
    track->Samples[2] = 0.3f;  // 1
    track->Samples[4] = 0.5f;  // 2
    track->Samples[6] = 0.9f;  // 3
    track->Samples[8] = 1.1f;  // 4
    track->Samples[10] = 0.9f; // 5
    track->Samples[12] = 0.7f; // 6
    track->Samples[14] = 0.3f; // 7
    track->Samples[16] = 0.1f; // 8

    track->Samples[1] = 0.3f;  // 0
    track->Samples[3] = 0.1f;  // 1
    track->Samples[5] = -0.1f; // 2
    track->Samples[7] = -0.3f; // 3
    track->Samples[9] = -0.1f; // 4
    track->Samples[11] = 0.3f; // 5
    track->Samples[13] = 0.9f; // 6
    track->Samples[15] = 1.3f; // 7
    track->Samples[17] = 0.9f; // 8

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<&doNothing>()
    );
    ClippingDetector::FindClippingHalfwaves(
      track, StopSource::Create()->GetToken(), progressCallback
    );

    ASSERT_EQ(track->Channels[0].ClippingHalfwaves.size(), 1U);
    EXPECT_EQ(track->Channels[0].ClippingHalfwaves[0].PriorZeroCrossingIndex, 0);
    EXPECT_EQ(track->Channels[0].ClippingHalfwaves[0].NextZeroCrossingIndex, 9);
    ASSERT_EQ(track->Channels[1].ClippingHalfwaves.size(), 1U);
    EXPECT_EQ(track->Channels[1].ClippingHalfwaves[0].PriorZeroCrossingIndex, 5);
    EXPECT_EQ(track->Channels[1].ClippingHalfwaves[0].NextZeroCrossingIndex, 9);
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio
