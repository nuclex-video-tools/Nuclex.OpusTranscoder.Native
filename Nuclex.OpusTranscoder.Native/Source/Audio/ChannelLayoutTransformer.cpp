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

#include "./ChannelLayoutTransformer.h"

namespace {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Half the square root of 2 or the Sine of one fourth of PI</summary>
  const float Diagonal = 0.7071067811865475244008443621048490392848359376884740365883398689953662f;

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Linearly interpolates between two values</summary>
  /// <param name="from">Interpolation value at 0.0</param>
  /// <param name="to">Interpolation value at 1.0</param>
  /// <param name="t">Interval between 0.0 and 1.0 to interpolate the value for</param>
  inline float lerp(float from, float to, float t) {
    return from * (1.0f - t) + to * t;
    //return (to - from) * t + from;
  }

  // ------------------------------------------------------------------------------------------- //

} // anonymous namespace

namespace Nuclex::OpusTranscoder::Audio {

  // ------------------------------------------------------------------------------------------- //

  void ChannelLayoutTransformer::DownmixToStereo(
    const std::shared_ptr<Track> &track,
    float nightmodeLevel,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
    Nuclex::Support::Events::Delegate<void(float)> &progressEvent
  ) {
    if((track->Channels.size() != 6) && (track->Channels.size() != 8)) {
      throw std::runtime_error(u8"Only 5.1 and 7.1 surround can be downmixed to stereo");
    }

    // Note: this only checks the channel count, but not the actual channel mappings.
    // If those are unusual or quirky, then downmix will be too quiet, too loud or empty.

    struct ChannelContribution {
      public: ChannelContribution(std::size_t offset, float factor) :
        InterleaveOffset(offset),
        Factor(factor) {}
      public: std::size_t InterleaveOffset;
      public: float Factor;
    };

    // Create a list of channels that should contribute to each of the stereo channels.
    // For now, we'll only consider 5.1 and 7.1 layouts.
    std::vector<ChannelContribution> mapping[2];
    for(std::size_t index = 0; index < track->Channels.size(); ++index) {
      switch(track->Channels[index].Placement) {
        case Nuclex::Audio::ChannelPlacement::FrontCenter: {
          float contribution = lerp(Diagonal, 1.0f, nightmodeLevel);
          mapping[0].emplace_back(index, contribution);
          mapping[1].emplace_back(index, contribution);
          break;
        }
        case Nuclex::Audio::ChannelPlacement::FrontLeft: {
          float contribution = lerp(1.0f, 0.3f, nightmodeLevel);
          mapping[0].emplace_back(index, contribution);
          break;
        }
        case Nuclex::Audio::ChannelPlacement::FrontRight: {
          float contribution = lerp(1.0f, 0.3f, nightmodeLevel);
          mapping[1].emplace_back(index, contribution);
          break;
        }
        case Nuclex::Audio::ChannelPlacement::SideLeft:
        case Nuclex::Audio::ChannelPlacement::BackLeft: {
          float contribution = lerp(Diagonal, 0.3f, nightmodeLevel);
          if(6 < track->Channels.size()) {
            contribution /= 2.0f; // If side AND back channel present, each adds half
          }
          mapping[0].emplace_back(index, contribution);
          break;
        }
        case Nuclex::Audio::ChannelPlacement::SideRight:
        case Nuclex::Audio::ChannelPlacement::BackRight: {
          float contribution = lerp(Diagonal, 0.3f, nightmodeLevel);
          if(6 < track->Channels.size()) {
            contribution /= 2.0f; // If side AND back channel present, each adds half
          }
          mapping[1].emplace_back(index, contribution);
          break;
        }
      }
    }
    
    // If the expected channels were there, we should have 3 contributions for
    // each stereo channel now (or 4 with split back/side channels)
    if((mapping[0].size() != 3) && (mapping[0].size() != 4)) {
      throw std::runtime_error(
        u8"Channel layout is non-standard and can't be downmixed to stereo"
      );
    }
    if(mapping[1].size() != mapping[0].size()) {
      throw std::runtime_error(
        u8"Channel layout is non-standard and can't be downmixed to stereo"
      );
    }

    // Now use the list of channel contributions to downmix the channels in-place
    {
      std::size_t channelCount = track->Channels.size();
      std::uint64_t frameCount = track->Samples.size() / channelCount;

      float *write = track->Samples.data();
      const float *read = write;
      for(std::uint64_t index = 0; index < frameCount; ++index) {
        float left = 0.0f, right = 0.0f;

        for(const ChannelContribution &contribution : mapping[0]) {
          left += read[contribution.InterleaveOffset] * contribution.Factor;
        }
        for(const ChannelContribution &contribution : mapping[1]) {
          right += read[contribution.InterleaveOffset] * contribution.Factor;
        }

        write[0] = left;
        write[1] = right;

        if((index & 0x2fff) == 0) {
          canceler->ThrowIfCanceled();
          progressEvent(static_cast<float>(index) / static_cast<float>(frameCount));
        }

        read += channelCount;
        write += 2;
      }

      // Now we've got stereo, truncate the samples we no longer need
      track->Samples.resize(frameCount * 2);
      track->Samples.shrink_to_fit();
    }

    // Set the records straight, we've downmixed the input to stereo,
    // thus we only have two channels now and their ordering is clear.
    track->Channels.resize(2);
    track->Channels[0].InputOrder = 0;
    track->Channels[0].Placement = Nuclex::Audio::ChannelPlacement::FrontLeft;
    track->Channels[1].InputOrder = 1;
    track->Channels[1].Placement = Nuclex::Audio::ChannelPlacement::FrontRight;
  }

  // ------------------------------------------------------------------------------------------- //

  void ChannelLayoutTransformer::DownmixToFiveDotOne(
    const std::shared_ptr<Track> &track,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
    Nuclex::Support::Events::Delegate<void(float)> &progressEvent
  ) {
    if(track->Channels.size() != 8) {
      throw std::runtime_error(u8"Only 7.1 surround can be downmixed to 5.1 surround");
    }

    // We'll use two lists, one holds matching channels that can be copied and one
    // will hold the left and right side+back channel pairs that contribute by 0.5
    std::size_t invalid = std::size_t(-1);
    std::size_t fullMapping[4] = { invalid, invalid, invalid, invalid };
    std::size_t halfMapping[4] = { invalid, invalid, invalid, invalid };
    for(std::size_t index = 0; index < track->Channels.size(); ++index) {
      switch(track->Channels[index].Placement) {
        case Nuclex::Audio::ChannelPlacement::FrontLeft: {
          fullMapping[0] = index;
          break;
        }
        case Nuclex::Audio::ChannelPlacement::FrontCenter: {
          fullMapping[1] = index;
          break;
        }
        case Nuclex::Audio::ChannelPlacement::FrontRight: {
          fullMapping[2] = index;
          break;
        }
        case Nuclex::Audio::ChannelPlacement::LowFrequencyEffects: {
          fullMapping[3] = index; // actually has spot 6, handled  in the downmix loop
          break;
        }
        case Nuclex::Audio::ChannelPlacement::SideLeft: {
          halfMapping[0] = index;
          break;
        }
        case Nuclex::Audio::ChannelPlacement::BackLeft: {
          halfMapping[1] = index;
          break;
        }
        case Nuclex::Audio::ChannelPlacement::SideRight: {
          halfMapping[2] = index;
          break;
        }
        case Nuclex::Audio::ChannelPlacement::BackRight: {
          halfMapping[3] = index;
          break;
        }
      }
    }

    // All expected channels should be filled. If not, one of the encountered
    // channels was not in the standard surround layout or a duplicate
    for(std::size_t index = 0; index < 4; ++index) {
      if((fullMapping[index] == invalid) || (halfMapping[index] == invalid)) {
        throw std::runtime_error(
          u8"Non-standard 7.1 surround channel layout cannot be downmixed to 5.1"
        );
      }
    }

    // Now use the list of channel contributions to downmix the channels in-place
    {
      std::size_t channelCount = track->Channels.size();
      std::uint64_t frameCount = track->Samples.size() / channelCount;

      float *write = track->Samples.data();
      const float *read = write;
      for(std::uint64_t index = 0; index < frameCount; ++index) {
        write[0] = read[fullMapping[0]];
        write[1] = read[fullMapping[1]];
        write[2] = read[fullMapping[2]];
        write[3] = (read[halfMapping[0]] + read[halfMapping[1]]) / 2.0f;
        write[4] = (read[halfMapping[2]] + read[halfMapping[2]]) / 2.0f;
        write[5] = read[fullMapping[3]];

        if((index & 0x2fff) == 0) {
          canceler->ThrowIfCanceled();
          progressEvent(static_cast<float>(index) / static_cast<float>(frameCount));
        }

        read += 8;
        write += 6;
      }

      // Now we've got stereo, truncate the samples we no longer need
      track->Samples.resize(frameCount * 6);
      track->Samples.shrink_to_fit();
    }

    // Set the records straight, we've downmixed the input to 5.1 surround
    // and ordered the channels according to the Vorbis I specification.
    track->Channels.resize(6);
    track->Channels[0].InputOrder = 0;
    track->Channels[0].Placement = Nuclex::Audio::ChannelPlacement::FrontLeft;
    track->Channels[1].InputOrder = 1;
    track->Channels[1].Placement = Nuclex::Audio::ChannelPlacement::FrontCenter;
    track->Channels[2].InputOrder = 2;
    track->Channels[2].Placement = Nuclex::Audio::ChannelPlacement::FrontRight;
    track->Channels[3].InputOrder = 3;
    track->Channels[3].Placement = Nuclex::Audio::ChannelPlacement::BackLeft;
    track->Channels[4].InputOrder = 4;
    track->Channels[4].Placement = Nuclex::Audio::ChannelPlacement::BackRight;
    track->Channels[5].InputOrder = 5;
    track->Channels[5].Placement = Nuclex::Audio::ChannelPlacement::LowFrequencyEffects;
  }

  // ------------------------------------------------------------------------------------------- //

  void ChannelLayoutTransformer::UpmixToStereo(
    const std::shared_ptr<Track> &track,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
    Nuclex::Support::Events::Delegate<void(float)> &progressEvent
  ) {
    if(track->Channels.size() != 1) {
      throw std::runtime_error(u8"Only mono can be upmixed to stereo");
    }
    if(track->Channels[0].Placement != Nuclex::Audio::ChannelPlacement::FrontCenter) {
      throw std::runtime_error(u8"Non-standad mono channel can not be upmixed to stereo");
    }

    // Because the data doubles in size, we have to do the in-place conversion
    // in reverse, otherwise we'd overwrite samples. Goodbye cache prefetcher.
    {
      std::uint64_t frameCount = track->Samples.size();
      track->Samples.resize(frameCount * 2);

      float *write = track->Samples.data() + (frameCount * 2) - 2;
      const float *read = track->Samples.data() - 1;

      for(std::uint64_t index = 0; index < frameCount; ++index) {
        float sample = read[0];
        write[0] = sample; // * Diagonal
        write[1] = sample; // * Diagonal

        if((index & 0x2fff) == 0) {
          canceler->ThrowIfCanceled();
          progressEvent(static_cast<float>(index) / static_cast<float>(frameCount));
        }

        read -= 1;
        write -= 2;
      }
    }

    // Update the channel descriptions to indicate two channels, stereo.
    track->Channels.resize(2);
    track->Channels[0].InputOrder = 0;
    track->Channels[0].Placement = Nuclex::Audio::ChannelPlacement::FrontLeft;
    track->Channels[1].InputOrder = 1;
    track->Channels[1].Placement = Nuclex::Audio::ChannelPlacement::FrontRight;
  }

  // ------------------------------------------------------------------------------------------- //

  void ChannelLayoutTransformer::ReweaveToVorbisLayout(
    const std::shared_ptr<Track> &track,
    const std::shared_ptr<const Nuclex::Support::Threading::StopToken> &canceler,
    Nuclex::Support::Events::Delegate<void(float)> &progressEvent
  ) {
    if(track->Channels.size() != 6) {
      throw std::runtime_error(u8"Only 5.1 surround can be re-weaved to the Vorbis layout");
    }

    // Construct a mapping table that tells which source channels have to go in
    // which place in the target layout
    std::size_t invalid = std::size_t(-1);
    std::size_t mapping[6] = { invalid, invalid, invalid, invalid, invalid, invalid };
    for(std::size_t index = 0; index < track->Channels.size(); ++index) {
      switch(track->Channels[index].Placement) {
        case Nuclex::Audio::ChannelPlacement::FrontLeft: {
          mapping[0] = index;
          break;
        }
        case Nuclex::Audio::ChannelPlacement::FrontCenter: {
          mapping[1] = index;
          break;
        }
        case Nuclex::Audio::ChannelPlacement::FrontRight: {
          mapping[2] = index;
          break;
        }
        case Nuclex::Audio::ChannelPlacement::SideLeft:
        case Nuclex::Audio::ChannelPlacement::BackLeft: {
          mapping[3] = index;
          break;
        }
        case Nuclex::Audio::ChannelPlacement::SideRight:
        case Nuclex::Audio::ChannelPlacement::BackRight: {
          mapping[4] = index;
          break;
        }
        case Nuclex::Audio::ChannelPlacement::LowFrequencyEffects: {
          mapping[5] = index;
          break;
        }
        default: { break; }
      }
    }

    // All expected channels should be filled. If not, one of the encountered
    // channels was not in the standard surround layout or a duplicate
    for(std::size_t index = 0; index < 6; ++index) {
      if(mapping[index] == invalid) {
        throw std::runtime_error(
          u8"Non-standard 5.1 surround channel layout cannot be re-weaved to 5.1"
        );
      }
    }

    // Now use the list of channel contributions to downmix the channels in-place
    {
      std::size_t channelCount = track->Channels.size();
      std::uint64_t frameCount = track->Samples.size() / channelCount;

      float *write = track->Samples.data();
      const float *read = write;
      for(std::uint64_t index = 0; index < frameCount; ++index) {
        float temp[6] = { read[0], read[1], read[2], read[3], read[4], read[5] };
        write[0] = temp[mapping[0]];
        write[1] = temp[mapping[1]];
        write[2] = temp[mapping[2]];
        write[3] = temp[mapping[3]];
        write[4] = temp[mapping[4]];
        write[5] = temp[mapping[5]];

        if((index & 0x2fff) == 0) {
          canceler->ThrowIfCanceled();
          progressEvent(static_cast<float>(index) / static_cast<float>(frameCount));
        }

        read += 6;
        write += 6;
      }
    }

    // We've re-ordered the input channels to 5.1 surround in
    // the order specified by the Vorbis I specification.
    track->Channels.resize(6);
    track->Channels[0].InputOrder = 0;
    track->Channels[0].Placement = Nuclex::Audio::ChannelPlacement::FrontLeft;
    track->Channels[1].InputOrder = 1;
    track->Channels[1].Placement = Nuclex::Audio::ChannelPlacement::FrontCenter;
    track->Channels[2].InputOrder = 2;
    track->Channels[2].Placement = Nuclex::Audio::ChannelPlacement::FrontRight;
    track->Channels[3].InputOrder = 3;
    track->Channels[3].Placement = Nuclex::Audio::ChannelPlacement::BackLeft;
    track->Channels[4].InputOrder = 4;
    track->Channels[4].Placement = Nuclex::Audio::ChannelPlacement::BackRight;
    track->Channels[5].InputOrder = 5;
    track->Channels[5].Placement = Nuclex::Audio::ChannelPlacement::LowFrequencyEffects;
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Audio
