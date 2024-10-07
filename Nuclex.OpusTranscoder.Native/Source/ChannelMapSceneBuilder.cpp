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

#include "./ChannelMapSceneBuilder.h"

#include <Nuclex/Support/BitTricks.h>

#include <QPixmap>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QApplication>
#include <QPalette>

namespace {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Looks up the image for the specified channel placement</summary>
  /// <param name="channel">Channel placement for which to get the image path</param>
  /// <returns>The name of the image representing the channel placement</retuns>
  std::string imageNameFromChannel(Nuclex::Audio::ChannelPlacement channel) {
    switch(channel) {
      case Nuclex::Audio::ChannelPlacement::TopFrontLeft:
      case Nuclex::Audio::ChannelPlacement::FrontLeft: {
        return u8":/svgrepo-speaker-front-left.png";
      }

      case Nuclex::Audio::ChannelPlacement::TopFrontRight:
      case Nuclex::Audio::ChannelPlacement::FrontRight: {
        return u8":/svgrepo-speaker-front-right.png";
      }

      case Nuclex::Audio::ChannelPlacement::TopFrontCenter:
      case Nuclex::Audio::ChannelPlacement::FrontCenter: {
        return u8":/svgrepo-speaker-front-center.png";
      }

      case Nuclex::Audio::ChannelPlacement::SideLeft: {
        return u8":/svgrepo-speaker-side-left.png";
      }

      case Nuclex::Audio::ChannelPlacement::SideRight: {
        return u8":/svgrepo-speaker-side-right.png";
      }

      case Nuclex::Audio::ChannelPlacement::TopBackLeft:
      case Nuclex::Audio::ChannelPlacement::BackLeft: {
        return u8":/svgrepo-speaker-back-left.png";
      }

      case Nuclex::Audio::ChannelPlacement::TopBackRight:
      case Nuclex::Audio::ChannelPlacement::BackRight: {
        return u8":/svgrepo-speaker-back-right.png";
      }

      case Nuclex::Audio::ChannelPlacement::TopBackCenter:
      case Nuclex::Audio::ChannelPlacement::BackCenter: {
        return u8":/svgrepo-speaker-back-center.png";
      }

      case Nuclex::Audio::ChannelPlacement::LowFrequencyEffects: {
        return u8":/svgrepo-speaker-bass.png";
      }

      default: {
        return u8":/svgrepo-speaker-unknown.png";
      }

    } // switch
  }

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Looks up the color that should be used to display a channel</summary>
  /// <param name="channel">Channel placement for which to obtain the color</param>
  /// <returns>The color in which the specified channel should be displayed</returns>
  QColor getChannelColor(Nuclex::Audio::ChannelPlacement channel) {
    switch(channel) {
      case Nuclex::Audio::ChannelPlacement::FrontLeft:
      case Nuclex::Audio::ChannelPlacement::BackLeft:
      case Nuclex::Audio::ChannelPlacement::SideLeft: {
        return QColor(0, 121, 171); // A blue that fits dark and light theme
      }
      case Nuclex::Audio::ChannelPlacement::FrontRight:
      case Nuclex::Audio::ChannelPlacement::BackRight:
      case Nuclex::Audio::ChannelPlacement::SideRight: {
        return QColor(183, 60, 62); // A red that fits dark and light theme
      }
      case Nuclex::Audio::ChannelPlacement::FrontCenter:
      case Nuclex::Audio::ChannelPlacement::BackCenter: {
        return QColor(23, 156, 66); // A green that fits dark and light theme
      }
      case Nuclex::Audio::ChannelPlacement::TopFrontLeft:
      case Nuclex::Audio::ChannelPlacement::TopBackLeft: {
        return QColor(59, 106, 133);
      }
      case Nuclex::Audio::ChannelPlacement::TopFrontRight:
      case Nuclex::Audio::ChannelPlacement::TopBackRight: {
        return QColor(163, 76, 67);
      }
      case Nuclex::Audio::ChannelPlacement::TopFrontCenter:
      case Nuclex::Audio::ChannelPlacement::TopBackCenter: {
        return QColor(72, 139, 84);
      }
      case Nuclex::Audio::ChannelPlacement::FrontCenterLeft: {
        return QColor(60, 166, 149);
      }
      case Nuclex::Audio::ChannelPlacement::FrontCenterRight: {
        return QColor(191, 141, 55);
      }
      case Nuclex::Audio::ChannelPlacement::LowFrequencyEffects: {
        return QColor(150, 23, 156); // A violet that fits dark and light theme
      }
      default: {
        return QColor(127, 127, 127);
      }
    }
  }

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Checks whether the specified channel is on the left side</summary>
  /// <param name="channel">Channel that will be checked</param>
  /// <returns>True if the channel is on the left side</returns>
  bool isLeftChannel(Nuclex::Audio::ChannelPlacement channel) {
    return (
      (channel == Nuclex::Audio::ChannelPlacement::BackLeft) ||
      (channel == Nuclex::Audio::ChannelPlacement::SideLeft) ||
      (channel == Nuclex::Audio::ChannelPlacement::FrontLeft)/* ||
      (channel == Nuclex::Audio::ChannelPlacement::TopBackLeft) ||
      (channel == Nuclex::Audio::ChannelPlacement::TopFrontLeft) ||
      (channel == Nuclex::Audio::ChannelPlacement::FrontCenterLeft) */
    );
  }

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Checks whether the specified channel is on the right side</summary>
  /// <param name="channel">Channel that will be checked</param>
  /// <returns>True if the channel is on the right side</returns>
  bool isRightChannel(Nuclex::Audio::ChannelPlacement channel) {
    return (
      (channel == Nuclex::Audio::ChannelPlacement::BackRight) ||
      (channel == Nuclex::Audio::ChannelPlacement::SideRight) ||
      (channel == Nuclex::Audio::ChannelPlacement::FrontRight)/* ||
      (channel == Nuclex::Audio::ChannelPlacement::TopBackRight) ||
      (channel == Nuclex::Audio::ChannelPlacement::TopFrontRight) ||
      (channel == Nuclex::Audio::ChannelPlacement::FrontCenterRight)*/
    );
  }

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Checks whether the specified channel is a center channel</summary>
  /// <param name="channel">Channel that will be checked</param>
  /// <returns>True if the channel is a center channel</returns>
  bool isCenterChannel(Nuclex::Audio::ChannelPlacement channel) {
    return (
      (channel == Nuclex::Audio::ChannelPlacement::FrontCenter)
    );
  }

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Checks whether the specified channels should be connected</summary>
  /// <param name="from">Source channel to check for the target channel</param>
  /// <param name="to">Target channel for which the connection is checked</param>
  /// <returns>True if the source channel contributes to the target channel</returns>
  bool shouldConnectForMono(
    Nuclex::Audio::ChannelPlacement from, Nuclex::Audio::ChannelPlacement to
  ) {
    return isCenterChannel(from);
  }

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Checks whether the specified channels should be connected</summary>
  /// <param name="from">Source channel to check for the target channel</param>
  /// <param name="to">Target channel for which the connection is checked</param>
  /// <returns>True if the source channel contributes to the target channel</returns>
  bool shouldConnectForStereo(
    Nuclex::Audio::ChannelPlacement from, Nuclex::Audio::ChannelPlacement to
  ) {
    if(to == Nuclex::Audio::ChannelPlacement::FrontLeft) {
      return isLeftChannel(from) || isCenterChannel(from);
    }
    if(to == Nuclex::Audio::ChannelPlacement::FrontRight) {
      return isRightChannel(from) || isCenterChannel(from);
    }

    return false;
  }

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Checks whether the specified channels should be connected</summary>
  /// <param name="from">Source channel to check for the target channel</param>
  /// <param name="to">Target channel for which the connection is checked</param>
  /// <returns>True if the source channel contributes to the target channel</returns>
  bool shouldConnectForSurround(
    Nuclex::Audio::ChannelPlacement from, Nuclex::Audio::ChannelPlacement to
  ) {
    switch(to) {
      case Nuclex::Audio::ChannelPlacement::SideLeft:
      case Nuclex::Audio::ChannelPlacement::BackLeft: {
        return (
          (from == Nuclex::Audio::ChannelPlacement::SideLeft) ||
          (from == Nuclex::Audio::ChannelPlacement::BackLeft)
        );
      }
      case Nuclex::Audio::ChannelPlacement::SideRight:
      case Nuclex::Audio::ChannelPlacement::BackRight: {
        return (
          (from == Nuclex::Audio::ChannelPlacement::SideRight) ||
          (from == Nuclex::Audio::ChannelPlacement::BackRight)
        );
      }
      default: {
        return (from == to);
      }
    }
  }

  // ------------------------------------------------------------------------------------------- //

} // anonymous namespace

namespace Nuclex::OpusTranscoder {

  // ------------------------------------------------------------------------------------------- //

  void ChannelMapSceneBuilder::BuildScene(
    QGraphicsScene &scene,
    Audio::ChannelPlacement inputChannels,
    Audio::ChannelPlacement outputChannels
  ) {

    // Draw boxes for the input channels. The "pins" are the Y coordinates
    // where the connecting lines will come out.
    std::vector<double> inputPins = addInputChannels(scene, inputChannels);

    std::vector<double> outputPins;
    {
      double startY;
      {
        std::size_t outputChannelCount = Nuclex::Support::BitTricks::CountBits(
          static_cast<std::uint32_t>(outputChannels) & 0x1ffff
        );

        startY = inputPins.size() * 48.0;
        startY -= 48.0 * outputChannelCount;
        startY /= 2.0;
      }

      outputPins = addOutputChannels(scene, outputChannels, startY);
    }

    // Here we'll go for a cheap shot. Currently this application has no real channel
    // mapping, it just outputs either stereo or 5.1 surround.
    {
      Audio::ChannelPlacement none = Audio::ChannelPlacement::Unknown;

      // Select the function to use for checking whether the input channel
      // contributes to the output channel.
      bool (*checkFunction)(Audio::ChannelPlacement, Audio::ChannelPlacement);
      if(inputPins.size() == 1) {
        checkFunction = &shouldConnectForMono;
      } else if(outputPins.size() == 2) {
        checkFunction = &shouldConnectForStereo;
      } else {
        checkFunction = &shouldConnectForSurround;
      }

      // Build a list of output channels. This will make it easier to check for
      // connections between inputs and outputs below.
      std::vector<Audio::ChannelPlacement> outputChannelList;
      for(std::size_t bitIndex = 0; bitIndex < 17; ++bitIndex) {
        Audio::ChannelPlacement channel = static_cast<Audio::ChannelPlacement>(1 << bitIndex);
        if((outputChannels & channel) != none) {
          outputChannelList.push_back(channel);
        }
      }

      QColor foregroundColor = QApplication::palette().color(QPalette::WindowText);
      QPen linePen(foregroundColor);

      std::size_t inputIndex = 0;
      for(std::size_t bitIndex = 0; bitIndex < 17; ++bitIndex) {
        Audio::ChannelPlacement from = static_cast<Audio::ChannelPlacement>(1 << bitIndex);
        if((inputChannels & from) != none) {
          for(std::size_t outputIndex = 0; outputIndex < outputChannelList.size(); ++outputIndex) {
            if(checkFunction(from, outputChannelList[outputIndex])) {
              QGraphicsLineItem *line = scene.addLine(
                -64.0, inputPins.at(inputIndex),
                64.0, outputPins.at(outputIndex)
              );
              line->setPen(linePen);
            } // if connection needed
          } // for
          ++inputIndex;
        }
      }
    
    }
  }

  // ------------------------------------------------------------------------------------------- //

  std::vector<double> ChannelMapSceneBuilder::addInputChannels(
    QGraphicsScene &scene,
    Nuclex::Audio::ChannelPlacement inputChannels
  ) {
    Audio::ChannelPlacement none = Audio::ChannelPlacement::Unknown;
    if(inputChannels == none) {
      return std::vector<double>();
    }

    std::vector<double> inputPins;
    inputPins.reserve(
      Nuclex::Support::BitTricks::CountBits(
        static_cast<std::uint32_t>(inputChannels) & 0x1ffff
      )
    );

    double y = 0.0;
    for(std::size_t bitIndex = 0; bitIndex < 17; ++bitIndex) {
      Audio::ChannelPlacement channel = static_cast<Audio::ChannelPlacement>(1 << bitIndex);
      if((inputChannels & channel) != none) {

        QGraphicsRectItem *inputBox = scene.addRect(-256, y, 192, 32);
        QBrush brush(getChannelColor(channel));
        inputBox->setBrush(brush);

        QPixmap image(QString::fromStdString(imageNameFromChannel(channel)));
        QGraphicsPixmapItem *speakerImage = scene.addPixmap(image);
        speakerImage->setPos(-252, y + 4);
        speakerImage->setScale(0.05);

        std::string channelName = Audio::StringFromChannelPlacement(channel);
        QGraphicsTextItem *text = scene.addText(QString::fromStdString(channelName));
        text->setPos(-224, y + 4);

        inputPins.push_back(y + 16);

        y += 48.0;
      }
    } // for each possible channel bit

    return inputPins;
  }

  // ------------------------------------------------------------------------------------------- //

  std::vector<double> ChannelMapSceneBuilder::addOutputChannels(
    QGraphicsScene &scene,
    Nuclex::Audio::ChannelPlacement outputChannels, double startY
  ) {
    Audio::ChannelPlacement none = Audio::ChannelPlacement::Unknown;
    if(outputChannels == none) {
      return std::vector<double>();
    }

    std::vector<double> outputPins;
    outputPins.reserve(
      Nuclex::Support::BitTricks::CountBits(
        static_cast<std::uint32_t>(outputChannels) & 0x1ffff
      )
    );

    for(std::size_t bitIndex = 0; bitIndex < 17; ++bitIndex) {
      Audio::ChannelPlacement channel = static_cast<Audio::ChannelPlacement>(1 << bitIndex);
      if((outputChannels & channel) != none) {

        QGraphicsRectItem *inputBox = scene.addRect(64, startY, 192, 32);
        QBrush brush(getChannelColor(channel));
        inputBox->setBrush(brush);

        QPixmap image(QString::fromStdString(imageNameFromChannel(channel)));
        QGraphicsPixmapItem *speakerImage = scene.addPixmap(image);
        speakerImage->setPos(68, startY + 4);
        speakerImage->setScale(0.05);

        std::string channelName = Audio::StringFromChannelPlacement(channel);
        QGraphicsTextItem *text = scene.addText(QString::fromStdString(channelName));
        text->setPos(100, startY + 4);

        outputPins.push_back(startY + 16);

        startY += 48.0;
      }
    } // for each possible channel bit

    return outputPins;
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder