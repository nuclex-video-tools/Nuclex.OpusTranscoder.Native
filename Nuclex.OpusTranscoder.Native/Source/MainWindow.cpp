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

#include "./MainWindow.h"
#include "ui_MainWindow.h"
#include "./Services/ServicesRoot.h"
#include "./Services/MetadataReader.h"
#include "./ChannelMapSceneBuilder.h"

#include <QFileDialog>
#include <QComboBox>
#include <QVariant>

namespace {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Builds a list of compatible input formats</summary>
  /// <param name="metadata">Metadata of the input file</param>
  /// <returns>A list of compatible input formats in a human-readable form</returns>
  /// <remarks>
  ///   Usually, there is just one compatible input format. But for 5.1 surround audio,
  ///   there are two standards: 5.1 and 5.1(side). The difference is the valid angles
  ///   in which the two extra channels are assumed to be placed. Practically, this is
  ///   placebo stuff for now as Opus uses the Vorbis layouts which do not differentiate
  ///   between 5.1 and 5.1(side).
  /// </remarks>
  QStringList getCompatibleInputFormatNames(const Nuclex::Audio::TrackInfo &metadata) {
    using Nuclex::Audio::ChannelPlacement;

    QStringList compatibleFormats;

    std::size_t channelCount = metadata.ChannelCount;
    ChannelPlacement channelPlacements = metadata.ChannelPlacements;

    // For input files with no bass channel
    ChannelPlacement none = ChannelPlacement::Unknown;
    if((channelPlacements & ChannelPlacement::LowFrequencyEffects) == none) {
      if(channelCount == 1) {
        compatibleFormats.append(u8"Mono");
      } else if(channelCount == 2) {
        compatibleFormats.append(u8"Stereo");
      } else {
        compatibleFormats.append(
          QVariant(static_cast<int>(channelCount)).toString() + u8".0 Surround"
        );
      }
    } else { // input file has ^^ no bass channel ^^ / vv bass channel present vv
      if(channelCount == 1) {
        compatibleFormats.append(u8"Bass");
      } else if(channelCount == 2) {
        compatibleFormats.append(u8"1.1 Mono");
      } else if(channelCount == 3) {
        compatibleFormats.append(u8"2.1 Stereo");
      } else {
        bool isSideSurround = (
          ((metadata.ChannelPlacements & ChannelPlacement::SideLeft) != none) ||
          ((metadata.ChannelPlacements & ChannelPlacement::SideRight) != none)
        );

        // If this appears to be surround with side channels, put side surround first
        if((channelCount == 6) && isSideSurround) {    
          compatibleFormats.append(u8"5.1 (Side) Surround");
        }
        compatibleFormats.append(
          QVariant(static_cast<int>(channelCount - 1)).toString() + u8".1 Surround"
        );
        // If this appears to be surround with back channels, put side surround last
        if((channelCount == 6) && !isSideSurround) {    
          compatibleFormats.append(u8"5.1 (Side) Surround");
        }
      }
    } // if bass channel missing / present

    return compatibleFormats;
  }

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Returns a human-readable list of possible output formats</summary>
  /// <param name="metadata">Input file metadata to determine the possible outputs from</param>
  /// <returns>The possible output formats for the input file</returns>
  QStringList getPossibleOutputLayoutNames(const Nuclex::Audio::TrackInfo &metadata) {
    QStringList outputLayouts;

    // If the input file has 4 or more audio channels, it's a candidate for surround
    // audio, so we'll offer 5.1 as the first option (7.1 will be transcoded to 5.1).
    // Sorry advanced home threather owners, if requests for 7.1 come piling in,
    // I might add this as another option.
    std::size_t channelCount = metadata.ChannelCount;
    if(channelCount >= 4) {
      outputLayouts.append(u8"5.1 Surround");
    }

    // Stereo is always an option. We'll even transcode mono to stereo as it won't
    // increase size in Opus and we want to output only two standards for now.
    outputLayouts.append(u8"Stereo");

    return outputLayouts;
  }

  // ------------------------------------------------------------------------------------------- //

} // anonymous namespace

namespace Nuclex::OpusTranscoder {

  // ------------------------------------------------------------------------------------------- //

  MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(std::make_unique<Ui::MainWindow>()) {

    this->ui->setupUi(this);

    // Connect to various events from the UI widgets so we know when the user
    // wants to pick a file, changes settings or wants to begin the transcode.
    connect(
      this->ui->inputPathLine, &QLineEdit::returnPressed,
      this, &MainWindow::inputFileEntered
    );
    connect(
      this->ui->browseInputFileButton, &QPushButton::clicked,
      this, &MainWindow::browseInputFileClicked
    );
    connect(
      this->ui->browseOutputFileButton, &QPushButton::clicked,
      this, &MainWindow::browseOutputFileClicked
    );
    connect(
      this->ui->outputChannelsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
      this, &MainWindow::selectedOutputChannelLayoutChanged
    );
    connect(
      this->ui->bitrateSlider, &QSlider::valueChanged,
      this, &MainWindow::bitrateSliderMoved
    );
    connect(
      this->ui->bitrateNumber, QOverload<int>::of(&QSpinBox::valueChanged),
      this, &MainWindow::bitrateNumberChanged
    );
    connect(
      this->ui->quitButton, &QPushButton::clicked,
      this, &MainWindow::quitClicked
    );

    // When the application is launched, no input file is selected yet,
    // so put the widgets in the appropriate state
    enableControlsDependingOnValidInputFile(false);
    showNightmodeSlider(false);
    hideWarningFrame();
  }

  // ------------------------------------------------------------------------------------------- //

  MainWindow::~MainWindow() {
    this->ui->channelGraphics->setScene(nullptr);

    if(static_cast<bool>(this->metadataReader)) {
      this->metadataReader->Updated.Unsubscribe<
        MainWindow, &MainWindow::metadataUpdatedInBackgroundThread
      >(this);
    }
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::BindToServicesRoot(
    const std::shared_ptr<Services::ServicesRoot> &servicesRoot
  ) {
    this->metadataReader = servicesRoot->GetMetadataReader();
    this->metadataReader->Updated.Subscribe<
      MainWindow, &MainWindow::metadataUpdatedInBackgroundThread
    >(this);
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::suggestOutputFilename() {
    QFileInfo inputFileInfo(this->ui->inputPathLine->text());
    QFileInfo outputFileInfo(this->ui->outputPathLine->text());

    // Pick the target directory. If the output path is empty, use the input path.
    // Otherwise, reuse the output path and just update the file name. The jury is
    // out wether that will sabotage the user or aid them in transcoding projects,
    // for now it suits my workflow, so it stays in :)
    QString outputDirectory = outputFileInfo.path();
    if(outputDirectory.isEmpty()) {
      outputDirectory = inputFileInfo.path();
    }

    // Now pick the output file name. If a file with the same name already exists,
    // we just keep appending '_2' with increasing numbers until we find a free name,
    // but the user can edit it or use the browse button to overwrite a file, too.
    QFileInfo newOutputFileInfo(
      QDir(outputDirectory).filePath(inputFileInfo.baseName() + u8".opus")
    );
    {
      std::size_t index = 2;
      while(newOutputFileInfo.exists()) {
        newOutputFileInfo.setFile(
          QDir(outputDirectory).filePath(
            inputFileInfo.baseName() +
            u8"_" +
            QVariant(static_cast<int>(index)).toString() +
            u8".opus"
          )
        );
        ++index;
      }
    }

    this->ui->outputPathLine->setText(newOutputFileInfo.filePath());
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::readInputFileProperties() {

    // Fire off the request. The metadata reader will check the audio file in another
    // thread and let us know via a call to metadataUpdatedInBackgroundThread()
    this->metadataReader->AnalyzeAudioFile(this->ui->inputPathLine->text().toStdString());

  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::metadataUpdatedInBackgroundThread() {

    // The event will have arrived from another thread. We neither want to hold up that
    // thread, nor can we touch our Qt UI widgets from this other thread, so we'll simply
    // schedule a call on the UI thread.
    QMetaObject::invokeMethod(
      this,
      &MainWindow::updateMetadata,
      Qt::ConnectionType::QueuedConnection
    );

  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::updateMetadata() {
    this->metadata = this->metadataReader->GetMetadata();

    // Clear the current contents of the input and output channel layouts.
    // If the input file could not be analyzed, they'll stay that way.
    this->ui->inputChannelsCombo->clear();
    this->ui->outputChannelsCombo->clear();

    // If the file was successfully analyzed, show its channel layout and
    // give the user the appropriate options for the output channel layout.
    if(this->metadata.has_value()) {
      this->ui->inputChannelsCombo->addItems(
        getCompatibleInputFormatNames(this->metadata.value())
      );
      this->ui->inputChannelsCombo->setCurrentIndex(0);

      this->ui->outputChannelsCombo->addItems(
        getPossibleOutputLayoutNames(this->metadata.value())
      );
      this->ui->outputChannelsCombo->setCurrentIndex(0);
    }

    // Enable or disable various widgets that the user should only be able
    // to access if a valid input file is selected.
    enableControlsDependingOnValidInputFile(this->metadata.has_value());
    showNightmodeSlider(false); // only for 5.1 to stereo, which is unselected now

    if(!this->metadata.has_value()) {
      if(!this->ui->inputPathLine->text().isEmpty()) {
        showWarningFrame(u8"Selected input file is not valid for transcoding");
      }
    }
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::enableControlsDependingOnValidInputFile(bool enable /* = true */) {
    this->ui->channelsLabel->setEnabled(enable);
    this->ui->inputChannelsCombo->setEnabled(enable);
    this->ui->outputChannelsCombo->setEnabled(enable);
    this->ui->channelsLabel->setEnabled(enable);
    this->ui->channelGraphics->setEnabled(enable);
    this->ui->bitrateLabel->setEnabled(enable);
    this->ui->bitrateSlider->setEnabled(enable);
    this->ui->bitrateNumber->setEnabled(enable);
    this->ui->bitrateKilobitsLabel->setEnabled(enable);
    this->ui->antiClipLabel->setEnabled(enable);
    this->ui->ignoreClippingOption->setEnabled(enable);
    this->ui->tuckHalfWavesOption->setEnabled(enable);
    this->ui->iterativeAntiClipOption->setEnabled(enable);
    this->ui->encodeOrCancelButton->setEnabled(enable);
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::showNightmodeSlider(bool show /* = true */) {
    this->ui->nightModeLabel->setVisible(show);
    this->ui->nightModeSlider->setVisible(show);
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::hideWarningFrame() {
    this->ui->warningFrame->hide();
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::showWarningFrame(const std::string &message) {
    this->ui->messageLabel->setText(QString::fromStdString(message));
    // TODO: Apply style sheet
    this->ui->warningFrame->show();
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::browseInputFileClicked() {
    std::unique_ptr<QFileDialog> selectInputFileDialog = (
      std::make_unique<QFileDialog>(this)
    );

    QStringList filters(
      {
        u8"Supported audio files (*.wv *.wav *.flac)",
        u8"WavPack audio files (*.wv)",
        u8"Waveform audio files (*.wav)",
        u8"FLAC audio files (*.flac)",
        u8"All files (*)"
      }
    );

    // Configure the dialog to let the user browse for an audio file
    selectInputFileDialog->setFileMode(QFileDialog::FileMode::ExistingFile);
    selectInputFileDialog->setOption(QFileDialog::Option::ReadOnly);
    selectInputFileDialog->setNameFilters(filters);
    selectInputFileDialog->setWindowTitle(
      QString(u8"Select audio file to transcode to Opus")
    );

    // Display the dialog, the user can select a single file or hit cancel
    int result = selectInputFileDialog->exec();

    // If the user selected a file and did not cancel,
    // store its full path in the input file path text box.
    if(result == QDialog::Accepted) {
      QStringList selectedFiles = selectInputFileDialog->selectedFiles();
      if(!selectedFiles.empty()) {
        this->ui->inputPathLine->setText(selectedFiles[0]);
        inputFileEntered();
      }
    }
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::inputFileEntered() {
    suggestOutputFilename();
    readInputFileProperties();
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::browseOutputFileClicked() {
    std::unique_ptr<QFileDialog> selectOutputFileDialog = (
      std::make_unique<QFileDialog>(this)
    );

    QStringList filters(
      {
        u8"Opus audio files (*.opus)",
        u8"All files (*)"
      }
    );

    // Configure the dialog to let the user enter the audio file to create
    selectOutputFileDialog->setAcceptMode(QFileDialog::AcceptMode::AcceptSave);
    selectOutputFileDialog->setFileMode(QFileDialog::FileMode::AnyFile);
    selectOutputFileDialog->setDefaultSuffix(u8"opus");
    selectOutputFileDialog->setNameFilters(filters);
    selectOutputFileDialog->setWindowTitle(
      QString(u8"Select output file to save transcoded Opus audio to")
    );

    // If there's an existing path in the output line, use it as the initial path
    if(!this->ui->outputPathLine->text().isEmpty()) {
      selectOutputFileDialog->selectFile(this->ui->outputPathLine->text());
    }

    // Display the dialog, the user can enter a desired path or hit cancel
    int result = selectOutputFileDialog->exec();

    // If the user entered or selected a file's path and did not cancel,
    // store its full path in the output file path text box.
    if(result == QDialog::Accepted) {
      QStringList selectedFiles = selectOutputFileDialog->selectedFiles();
      if(!selectedFiles.empty()) {
        this->ui->outputPathLine->setText(selectedFiles[0]);
      }
    }
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::selectedOutputChannelLayoutChanged(int layoutIndex) {

    // The layout index can only be one if a downmix from surround to stereo
    // is chosen. Stereo inputs only 1 output option.
    showNightmodeSlider(layoutIndex == 1);

    bool isStereo = true;
    if(this->metadata.has_value()) {
      if(this->metadata.value().ChannelCount >= 4) {
        isStereo = (layoutIndex == 1);
      }
    }

    // Obtain the bitrate as an interval between minimum and maximum
    double relativeBitrate;
    {
      double minimumBitrate = static_cast<double>(this->ui->bitrateNumber->minimum());
      double maximumBitrate = static_cast<double>(this->ui->bitrateNumber->maximum());

      relativeBitrate = static_cast<double>(this->ui->bitrateNumber->value());
      relativeBitrate -= minimumBitrate;
      relativeBitrate /= (maximumBitrate - minimumBitrate);
    }

    // Update the bitrate to limits appropriate to the channel layout
    if(isStereo) {
      this->ui->bitrateNumber->setMinimum(64);
      this->ui->bitrateNumber->setMaximum(352);
      this->ui->bitrateSlider->setMinimum(64);
      this->ui->bitrateSlider->setMaximum(352);
    } else {
      this->ui->bitrateNumber->setMinimum(256);
      this->ui->bitrateNumber->setMaximum(992);
      this->ui->bitrateSlider->setMinimum(256);
      this->ui->bitrateSlider->setMaximum(992);
    }

    // Calculate the bitrate in kbit/s that is on the same interval.
    // Assuming the minimum and maximum we picked amount to the same quality,
    // this will preserves the quality chosen by the user when switching from
    // stereo to surround and back.
    int newBitrate;
    {
      double minimumBitrate = static_cast<double>(this->ui->bitrateNumber->minimum());
      double maximumBitrate = static_cast<double>(this->ui->bitrateNumber->maximum());

      newBitrate = static_cast<int>(
        relativeBitrate * (maximumBitrate - minimumBitrate) + minimumBitrate + 0.5
      );
    }

    this->ui->bitrateNumber->setValue(newBitrate);
    this->ui->bitrateSlider->setValue(newBitrate);

    std::unique_ptr<QGraphicsScene> scene = std::make_unique<QGraphicsScene>();
    ChannelMapSceneBuilder::BuildScene(
      *scene,
      this->metadata.value().ChannelPlacements, // input channels
      isStereo ? (
        Audio::ChannelPlacement::FrontLeft |
        Audio::ChannelPlacement::FrontRight
      ) : (
        Audio::ChannelPlacement::FrontLeft |
        Audio::ChannelPlacement::FrontRight |
        Audio::ChannelPlacement::FrontCenter |
        Audio::ChannelPlacement::LowFrequencyEffects |
        Audio::ChannelPlacement::BackLeft |
        Audio::ChannelPlacement::BackRight
      )
    );

    this->ui->channelGraphics->setScene(scene.get());
    this->visualizationScene.swap(scene);
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::bitrateSliderMoved(int bitrate) {
    this->ui->bitrateNumber->setValue(bitrate);
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::bitrateNumberChanged(int bitrate) {
    this->ui->bitrateSlider->setValue(bitrate);
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::quitClicked() {
    close();
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder
