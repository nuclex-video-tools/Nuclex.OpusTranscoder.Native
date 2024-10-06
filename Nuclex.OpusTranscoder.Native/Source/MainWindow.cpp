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

#include <QFileDialog>
#include <QComboBox>
#include <QVariant>

namespace {

  // ------------------------------------------------------------------------------------------- //

  QStringList GetCompatibleInputFormatNames(const Nuclex::Audio::TrackInfo &metadata) {
    using Nuclex::Audio::ChannelPlacement;

    QStringList compatibleFormats;

    std::size_t channelCount = metadata.ChannelCount;
    ChannelPlacement channelPlacements = metadata.ChannelPlacements;

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
    } else {
      if(channelCount == 1) {
        compatibleFormats.append(u8"Bass");
      } else if(channelCount == 2) {
        compatibleFormats.append(u8"1.1 Mono");
      } else if(channelCount == 3) {
        compatibleFormats.append(u8"2.1 Stereo");
      } else {
        compatibleFormats.append(
          QVariant(static_cast<int>(channelCount - 1)).toString() + u8".1 Surround"
        );
      }
    }

    return compatibleFormats;
  }

  // ------------------------------------------------------------------------------------------- //

  QStringList GetSupportedOutputLayoutNames(const Nuclex::Audio::TrackInfo &metadata) {
    QStringList outputLayouts;

    outputLayouts.append(u8"Stereo");

    std::size_t channelCount = metadata.ChannelCount;
    if(channelCount >= 5) {
      outputLayouts.append(u8"5.1 Surround");
    }

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

    this->ui->nightModeLabel->hide();
    this->ui->nightModeSlider->hide();
    this->ui->warningFrame->hide();

    connect(
      this->ui->browseInputFileButton, &QPushButton::clicked,
      this, &MainWindow::browseInputFileClicked
    );
    connect(
      this->ui->browseOutputFileButton, &QPushButton::clicked,
      this, &MainWindow::browseOutputFileClicked
    );
    connect(
      this->ui->quitButton, &QPushButton::clicked,
      this, &MainWindow::quitClicked
    );
  }

  // ------------------------------------------------------------------------------------------- //

  MainWindow::~MainWindow() {
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
    
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::readInputFileProperties() {
    this->metadataReader->AnalyzeAudioFile(this->ui->inputPathLine->text().toStdString());
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::metadataUpdatedInBackgroundThread() {
    QMetaObject::invokeMethod(
      this,
      &MainWindow::updateMetadata,
      Qt::ConnectionType::QueuedConnection
    );
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::updateMetadata() {
    this->metadata = this->metadataReader->GetMetadata();
    
    if(this->metadata.has_value()) {
      this->ui->inputChannelsCombo->clear();
      this->ui->inputChannelsCombo->addItems(
        GetCompatibleInputFormatNames(this->metadata.value())
      );
      this->ui->inputChannelsCombo->setCurrentIndex(0);

      this->ui->outputChannelsCombo->clear();
      this->ui->outputChannelsCombo->addItems(
        GetSupportedOutputLayoutNames(this->metadata.value())
      );
      this->ui->outputChannelsCombo->setCurrentIndex(0);
    }

  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::browseInputFileClicked() {
    std::unique_ptr<QFileDialog> selectDirectoryDialog = (
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
    selectDirectoryDialog->setFileMode(QFileDialog::FileMode::ExistingFile);
    selectDirectoryDialog->setOption(QFileDialog::Option::ReadOnly);
    selectDirectoryDialog->setNameFilters(filters);
    selectDirectoryDialog->setWindowTitle(
      QString(u8"Select audio file to transcode to OPUS")
    );

    // Display the dialog, the user can select a single file or hit cancel
    int result = selectDirectoryDialog->exec();

    // If the user selected a file and did not cancel,
    // store its full path in the input file path text box.
    if(result == QDialog::Accepted) {
      QStringList selectedFiles = selectDirectoryDialog->selectedFiles();
      if(!selectedFiles.empty()) {
        this->ui->inputPathLine->setText(selectedFiles[0]);
        suggestOutputFilename();
        readInputFileProperties();
      }
    }
  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::browseOutputFileClicked() {

  }

  // ------------------------------------------------------------------------------------------- //

  void MainWindow::quitClicked() {
    close();
  }

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder
