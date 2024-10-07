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

#ifndef NUCLEX_OPUSTRANSCODER_CHANNELMAPSCENEBUILDER_H
#define NUCLEX_OPUSTRANSCODER_CHANNELMAPSCENEBUILDER_H

#include "./Config.h"

#include <QGraphicsScene>
#include <Nuclex/Audio/ChannelPlacement.h>

namespace Nuclex::OpusTranscoder {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Builds Qt graphics scenes to visualize audio channel downmixes</summary>
  class ChannelMapSceneBuilder {

    /// <summary>Builds a scene visualizing the input and output channel layouts</summary>
    /// <param name="scene">Scene in which the visualization will be stored</param>
    /// <param name="inputChannels">Channels present in the input audio file</param>
    /// <param name="outputChannels">Channels that will be encoded in the output file</param>
    /// <remarks>
    ///   For now, this application is a one-track transcoder. The mapping from any input
    ///   layout to any output layout is determined by the application itself. Custom
    ///   mappings, like Audacity lets you do them, would require such mappings to be
    ///   editable and stored somehow. If I ever feel that is needed, this method will
    ///   need to be updated accordingly.
    /// </remarks>
    public: static void BuildScene(
      QGraphicsScene &scene,
      Nuclex::Audio::ChannelPlacement inputChannels,
      Nuclex::Audio::ChannelPlacement outputChannels
    );

    /// <summary>Adds boxes for the input channels to the graphics scene</summary>
    /// <param name="scene">Scene to which the input channels will be added</param>
    /// <param name="channels">Flags for the input channels that are present</param>
    /// <returns>A list of the Y coordinates for the input connection pins</returns>
    private: static std::vector<double> addInputChannels(
      QGraphicsScene &scene,
      Nuclex::Audio::ChannelPlacement channels
    );

    /// <summary>Adds boxes for the output channels to the graphics scene</summary>
    /// <param name="scene">Scene to which the output channels will be added</param>
    /// <param name="channels">Flags for the output channels that are present</param>
    /// <returns>A list of the Y coordinates for the output connection pins</returns>
    private: static std::vector<double> addOutputChannels(
      QGraphicsScene &scene,
      Nuclex::Audio::ChannelPlacement channels, double startY
    );

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder

#endif // NUCLEX_OPUSTRANSCODER_CHANNELMAPSCENEBUILDER_H
