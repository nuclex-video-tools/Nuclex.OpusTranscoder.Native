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
#include "../../Source/Audio/ChannelLayoutTransformer.h"

#include <Nuclex/Support/Threading/StopSource.h>

#include <gtest/gtest.h>

namespace {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Does absolutely nothing</summary>
  void doNothing(float) {}

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Creates a new 5.1 surround track without audio samples</summary>
  /// <returns>The new 5.1 surround track</returns>
  std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> makeFiveDotOneTrack() {
    using Nuclex::OpusTranscoder::Audio::Track;
    std::shared_ptr<Track> track = std::make_shared<Track>();

    track->Channels.resize(6);
    track->Channels[0].InputOrder = 0;
    track->Channels[0].Placement = Nuclex::Audio::ChannelPlacement::FrontLeft;
    track->Channels[1].InputOrder = 1;
    track->Channels[1].Placement = Nuclex::Audio::ChannelPlacement::FrontRight;
    track->Channels[2].InputOrder = 2;
    track->Channels[2].Placement = Nuclex::Audio::ChannelPlacement::FrontCenter;
    track->Channels[3].InputOrder = 3;
    track->Channels[3].Placement = Nuclex::Audio::ChannelPlacement::LowFrequencyEffects;
    track->Channels[4].InputOrder = 4;
    track->Channels[4].Placement = Nuclex::Audio::ChannelPlacement::BackLeft;
    track->Channels[5].InputOrder = 5;
    track->Channels[5].Placement = Nuclex::Audio::ChannelPlacement::BackRight;

    return track;
  }

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Creates a new 7.1 surround track without audio samples</summary>
  /// <returns>The new 7.1 surround track</returns>
  std::shared_ptr<Nuclex::OpusTranscoder::Audio::Track> makeSevenDotOneTrack() {
    using Nuclex::OpusTranscoder::Audio::Track;
    std::shared_ptr<Track> track = std::make_shared<Track>();

    track->Channels.resize(8);
    track->Channels[0].InputOrder = 0;
    track->Channels[0].Placement = Nuclex::Audio::ChannelPlacement::FrontLeft;
    track->Channels[1].InputOrder = 1;
    track->Channels[1].Placement = Nuclex::Audio::ChannelPlacement::FrontRight;
    track->Channels[2].InputOrder = 2;
    track->Channels[2].Placement = Nuclex::Audio::ChannelPlacement::FrontCenter;
    track->Channels[3].InputOrder = 3;
    track->Channels[3].Placement = Nuclex::Audio::ChannelPlacement::LowFrequencyEffects;
    track->Channels[4].InputOrder = 4;
    track->Channels[4].Placement = Nuclex::Audio::ChannelPlacement::BackLeft;
    track->Channels[5].InputOrder = 5;
    track->Channels[5].Placement = Nuclex::Audio::ChannelPlacement::BackRight;
    track->Channels[6].InputOrder = 6;
    track->Channels[6].Placement = Nuclex::Audio::ChannelPlacement::SideLeft;
    track->Channels[7].InputOrder = 7;
    track->Channels[7].Placement = Nuclex::Audio::ChannelPlacement::SideRight;

    return track;
  }

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Half the square root of 2 or the Sine of one fourth of PI</summary>
  const float Diagonal = 0.7071067811865475244008443621048490392848359376884740365883398689953662f;

  // ------------------------------------------------------------------------------------------- //

} // anonymous namespace

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  TEST(ChannelLayoutTransformerTests, CanDownmixFiveDotOneToStereo) {
    using Nuclex::Support::Events::Delegate;
    using Nuclex::Support::Threading::StopSource;
    using Nuclex::Support::Threading::StopToken;

    std::shared_ptr<Track> track = makeFiveDotOneTrack();
    track->Samples.resize(18);
    track->Samples[0] = 1.0f; // left
    track->Samples[1] = 1.0f; // right
    track->Samples[2] = 1.0f; // center
    track->Samples[3] = 1.0f; // lfe
    track->Samples[4] = 1.0f; // back left
    track->Samples[5] = 1.0f; // back right
    track->Samples[6] = 0.5f; // left
    track->Samples[7] = 0.25f; // right
    track->Samples[8] = 0.0f; // center
    track->Samples[9] = 2.0f; // lfe
    track->Samples[10] = 0.0f; // back left
    track->Samples[11] = 0.0f; // back right
    track->Samples[12] = 0.0f; // left
    track->Samples[13] = 0.0f; // right
    track->Samples[14] = 1.5f; // center
    track->Samples[15] = 0.0f; // lfe
    track->Samples[16] = 0.25f; // back left
    track->Samples[17] = 0.5f; // back right

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<&doNothing>()
    );
    ChannelLayoutTransformer::DownmixToStereo(
      track, 0.0f, StopSource::Create()->GetToken(), progressCallback
    );

    ASSERT_EQ(track->Channels.size(), 2U);
    EXPECT_EQ(track->Channels[0].Placement, Nuclex::Audio::ChannelPlacement::FrontLeft);
    EXPECT_EQ(track->Channels[1].Placement, Nuclex::Audio::ChannelPlacement::FrontRight);

    ASSERT_EQ(track->Samples.size(), 6U);
    EXPECT_EQ(track->Samples[0], Diagonal + 1.0f + Diagonal);
    EXPECT_EQ(track->Samples[1], Diagonal + 1.0f + Diagonal);
    EXPECT_EQ(track->Samples[2], 0.5f);
    EXPECT_EQ(track->Samples[3], 0.25f);
    EXPECT_EQ(track->Samples[4], Diagonal * 1.5f + Diagonal * 0.25f);
    EXPECT_EQ(track->Samples[5], Diagonal * 1.5f + Diagonal * 0.5f);
  }

  // ------------------------------------------------------------------------------------------- //

  TEST(ChannelLayoutTransformerTests, CanDownmixSevenDotOneToStereo) {
    using Nuclex::Support::Events::Delegate;
    using Nuclex::Support::Threading::StopSource;
    using Nuclex::Support::Threading::StopToken;

    std::shared_ptr<Track> track = makeSevenDotOneTrack();
    track->Samples.resize(24);
    track->Samples[0] = 1.0f; // left
    track->Samples[1] = 1.0f; // right
    track->Samples[2] = 1.0f; // center
    track->Samples[3] = 1.0f; // lfe
    track->Samples[4] = 1.0f; // back left
    track->Samples[5] = 1.0f; // back right
    track->Samples[6] = 1.0f; // side left
    track->Samples[7] = 1.0f; // side right
    track->Samples[8] = 0.5f; // left
    track->Samples[9] = 0.25f; // right
    track->Samples[10] = 0.0f; // center
    track->Samples[11] = 2.0f; // lfe
    track->Samples[12] = 0.0f; // back left
    track->Samples[13] = 0.0f; // back right
    track->Samples[14] = 0.0f; // side left
    track->Samples[15] = 0.0f; // side right
    track->Samples[16] = 0.0f; // left
    track->Samples[17] = 0.0f; // right
    track->Samples[18] = 1.5f; // center
    track->Samples[19] = 0.0f; // lfe
    track->Samples[20] = 0.125f; // back left
    track->Samples[21] = 0.25f; // back right
    track->Samples[22] = 0.375f; // side left
    track->Samples[23] = 0.5f; // side right

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<&doNothing>()
    );
    ChannelLayoutTransformer::DownmixToStereo(
      track, 0.0f, StopSource::Create()->GetToken(), progressCallback
    );

    ASSERT_EQ(track->Channels.size(), 2U);
    EXPECT_EQ(track->Channels[0].Placement, Nuclex::Audio::ChannelPlacement::FrontLeft);
    EXPECT_EQ(track->Channels[1].Placement, Nuclex::Audio::ChannelPlacement::FrontRight);

    ASSERT_EQ(track->Samples.size(), 6U);
    EXPECT_EQ(track->Samples[0], Diagonal + 1.0f + Diagonal + Diagonal);
    EXPECT_EQ(track->Samples[1], Diagonal + 1.0f + Diagonal + Diagonal);
    EXPECT_EQ(track->Samples[2], 0.5f);
    EXPECT_EQ(track->Samples[3], 0.25f);
    EXPECT_EQ(track->Samples[4], Diagonal * 1.5f + Diagonal * 0.125f + Diagonal * 0.375f);
    EXPECT_EQ(track->Samples[5], Diagonal * 1.5f + Diagonal * 0.25f + Diagonal * 0.5f);
  }

  // ------------------------------------------------------------------------------------------- //

  TEST(ChannelLayoutTransformerTests, CanDownmixSevenDotOneToFiveDotOne) {
    using Nuclex::Support::Events::Delegate;
    using Nuclex::Support::Threading::StopSource;
    using Nuclex::Support::Threading::StopToken;

    std::shared_ptr<Track> track = makeSevenDotOneTrack();
    track->Samples.resize(24);
    track->Samples[0] = 1.0f; // left
    track->Samples[1] = 1.0f; // right
    track->Samples[2] = 1.0f; // center
    track->Samples[3] = 1.0f; // lfe
    track->Samples[4] = 1.0f; // back left
    track->Samples[5] = 1.0f; // back right
    track->Samples[6] = 1.0f; // side left
    track->Samples[7] = 1.0f; // side right
    track->Samples[8] = 0.5f; // left
    track->Samples[9] = 0.25f; // right
    track->Samples[10] = 0.0f; // center
    track->Samples[11] = 2.0f; // lfe
    track->Samples[12] = 0.0f; // back left
    track->Samples[13] = 0.0f; // back right
    track->Samples[14] = 0.0f; // side left
    track->Samples[15] = 0.0f; // side right
    track->Samples[16] = 0.0f; // left
    track->Samples[17] = 0.0f; // right
    track->Samples[18] = 1.5f; // center
    track->Samples[19] = 0.0f; // lfe
    track->Samples[20] = 0.125f; // back left
    track->Samples[21] = 0.25f; // back right
    track->Samples[22] = 0.375f; // side left
    track->Samples[23] = 0.5f; // side right

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<&doNothing>()
    );
    ChannelLayoutTransformer::DownmixToFiveDotOne(
      track, StopSource::Create()->GetToken(), progressCallback
    );

    ASSERT_EQ(track->Channels.size(), 6U);
    EXPECT_EQ(track->Channels[0].Placement, Nuclex::Audio::ChannelPlacement::FrontLeft);
    EXPECT_EQ(track->Channels[1].Placement, Nuclex::Audio::ChannelPlacement::FrontCenter);
    EXPECT_EQ(track->Channels[2].Placement, Nuclex::Audio::ChannelPlacement::FrontRight);
    EXPECT_EQ(track->Channels[3].Placement, Nuclex::Audio::ChannelPlacement::BackLeft);
    EXPECT_EQ(track->Channels[4].Placement, Nuclex::Audio::ChannelPlacement::BackRight);
    EXPECT_EQ(track->Channels[5].Placement, Nuclex::Audio::ChannelPlacement::LowFrequencyEffects);

    ASSERT_EQ(track->Samples.size(), 18U);
    EXPECT_EQ(track->Samples[0], 1.0f); // front left
    EXPECT_EQ(track->Samples[1], 1.0f); // front center
    EXPECT_EQ(track->Samples[2], 1.0f); // front right
    EXPECT_EQ(track->Samples[3], 2.0f); // back left
    EXPECT_EQ(track->Samples[4], 2.0f); // back right
    EXPECT_EQ(track->Samples[5], 1.0f); // lfe
    EXPECT_EQ(track->Samples[6], 0.5f); // front left
    EXPECT_EQ(track->Samples[7], 0.0f); // front center
    EXPECT_EQ(track->Samples[8], 0.25f); // front right
    EXPECT_EQ(track->Samples[9], 0.0f); // back left
    EXPECT_EQ(track->Samples[10], 0.0f); // back right
    EXPECT_EQ(track->Samples[11], 2.0f); // lfe
    EXPECT_EQ(track->Samples[12], 0.0f); // front left
    EXPECT_EQ(track->Samples[13], 1.5f); // front center
    EXPECT_EQ(track->Samples[14], 0.0f); // front right
    EXPECT_EQ(track->Samples[15], 0.125f + 0.375f); // back left
    EXPECT_EQ(track->Samples[16], 0.25f + 0.5f); // back right
    EXPECT_EQ(track->Samples[17], 0.0f); // lfe
  }

  // ------------------------------------------------------------------------------------------- //

  TEST(ChannelLayoutTransformerTests, CanUpmixMonoToStereo) {
    using Nuclex::Support::Events::Delegate;
    using Nuclex::Support::Threading::StopSource;
    using Nuclex::Support::Threading::StopToken;

    std::shared_ptr<Track> track = std::make_shared<Track>();
    track->Channels.resize(1);
    track->Channels[0].Placement = Nuclex::Audio::ChannelPlacement::FrontCenter;
    
    track->Samples.resize(3);
    track->Samples[0] = 0.25f;
    track->Samples[1] = 0.5f;
    track->Samples[2] = 2.0f;

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<&doNothing>()
    );
    ChannelLayoutTransformer::UpmixToStereo(
      track, StopSource::Create()->GetToken(), progressCallback
    );

    ASSERT_EQ(track->Channels.size(), 2U);
    EXPECT_EQ(track->Channels[0].Placement, Nuclex::Audio::ChannelPlacement::FrontLeft);
    EXPECT_EQ(track->Channels[1].Placement, Nuclex::Audio::ChannelPlacement::FrontRight);

    ASSERT_EQ(track->Samples.size(), 6U);
    EXPECT_EQ(track->Samples[0], 0.25);
    EXPECT_EQ(track->Samples[1], 0.25);
    EXPECT_EQ(track->Samples[2], 0.5f);
    EXPECT_EQ(track->Samples[3], 0.5f);
    EXPECT_EQ(track->Samples[4], 2.0f);
    EXPECT_EQ(track->Samples[5], 2.0f);
  }

  // ------------------------------------------------------------------------------------------- //

  TEST(ChannelLayoutTransformerTests, CanReweaveFiveDotOne) {
    using Nuclex::Support::Events::Delegate;
    using Nuclex::Support::Threading::StopSource;
    using Nuclex::Support::Threading::StopToken;

    std::shared_ptr<Track> track = makeFiveDotOneTrack();
    track->Samples.resize(18);
    track->Samples[0] = 1.0f; // left
    track->Samples[1] = 1.0f; // right
    track->Samples[2] = 1.0f; // center
    track->Samples[3] = 1.0f; // lfe
    track->Samples[4] = 1.0f; // back left
    track->Samples[5] = 1.0f; // back right
    track->Samples[6] = 0.1f; // left
    track->Samples[7] = 0.2f; // right
    track->Samples[8] = 0.3f; // center
    track->Samples[9] = 0.4f; // lfe
    track->Samples[10] = 0.5f; // back left
    track->Samples[11] = 0.6f; // back right
    track->Samples[12] = 1.0f; // left
    track->Samples[13] = 2.0f; // right
    track->Samples[14] = 3.0f; // center
    track->Samples[15] = 4.0f; // lfe
    track->Samples[16] = 5.0f; // back left
    track->Samples[17] = 6.0f; // back right

    Delegate<void(float)> progressCallback = (
      Delegate<void(float)>::Create<&doNothing>()
    );
    ChannelLayoutTransformer::ReweaveToVorbisLayout(
      track, StopSource::Create()->GetToken(), progressCallback
    );

    ASSERT_EQ(track->Channels.size(), 6U);
    EXPECT_EQ(track->Channels[0].Placement, Nuclex::Audio::ChannelPlacement::FrontLeft);
    EXPECT_EQ(track->Channels[1].Placement, Nuclex::Audio::ChannelPlacement::FrontCenter);
    EXPECT_EQ(track->Channels[2].Placement, Nuclex::Audio::ChannelPlacement::FrontRight);
    EXPECT_EQ(track->Channels[3].Placement, Nuclex::Audio::ChannelPlacement::BackLeft);
    EXPECT_EQ(track->Channels[4].Placement, Nuclex::Audio::ChannelPlacement::BackRight);
    EXPECT_EQ(track->Channels[5].Placement, Nuclex::Audio::ChannelPlacement::LowFrequencyEffects);

    ASSERT_EQ(track->Samples.size(), 18U);
    EXPECT_EQ(track->Samples[0], 1.0f); // front left
    EXPECT_EQ(track->Samples[1], 1.0f); // front center
    EXPECT_EQ(track->Samples[2], 1.0f); // front right
    EXPECT_EQ(track->Samples[3], 1.0f); // back left
    EXPECT_EQ(track->Samples[4], 1.0f); // back right
    EXPECT_EQ(track->Samples[5], 1.0f); // lfe
    EXPECT_EQ(track->Samples[6], 0.1f); // front left
    EXPECT_EQ(track->Samples[7], 0.3f); // front center
    EXPECT_EQ(track->Samples[8], 0.2f); // front right
    EXPECT_EQ(track->Samples[9], 0.5f); // back left
    EXPECT_EQ(track->Samples[10], 0.6f); // back right
    EXPECT_EQ(track->Samples[11], 0.4f); // lfe
    EXPECT_EQ(track->Samples[12], 1.0f); // front left
    EXPECT_EQ(track->Samples[13], 3.0f); // front center
    EXPECT_EQ(track->Samples[14], 2.0f); // front right
    EXPECT_EQ(track->Samples[15], 5.0f); // back left
    EXPECT_EQ(track->Samples[16], 6.0f); // back right
    EXPECT_EQ(track->Samples[17], 4.0f); // lfe
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio
