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

#ifndef NUCLEX_OPUSTRANSCODER_MAINWINDOW_H
#define NUCLEX_OPUSTRANSCODER_MAINWINDOW_H

#include "./Config.h"

#include <Nuclex/Audio/TrackInfo.h>

#include <QMainWindow> // for QMainWindow
#include <QItemSelection> // for QItemSelection
#include <QMutex> // for QMutex
#include <QGraphicsScene> // for QGraphicsScene

#include <memory> // for std::unique_ptr
#include <optional> // for std::optional
#include <map> // for std::pair

namespace Ui {

  // ------------------------------------------------------------------------------------------- //

  class MainWindow;

  // ------------------------------------------------------------------------------------------- //

}

namespace Nuclex::OpusTranscoder::Services {

  // ------------------------------------------------------------------------------------------- //

  class ServicesRoot;
  class MetadataReader;

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder::Services

namespace Nuclex::OpusTranscoder {

  // ------------------------------------------------------------------------------------------- //

  /// <summary>Manages the main window of the application</summary>
  class MainWindow : public QMainWindow {
    Q_OBJECT

    /// <summary>Initializes a new main window for the frame fixer application</summary>
    /// <param name="parent">Parent widget the main window will be placed in. Null.</param>
    public: explicit MainWindow(QWidget *parent = nullptr);
    /// <summary>Frees all resources owned by the main window</summary>
    public: ~MainWindow();

    /// <summary>Binds the main window to the specified service container</summary>
    /// <param name="servicesRoot">Service container the main window will use</param>
    /// <remarks>
    ///   The root service container contains all the services that perform the actual work
    ///   of the application (while this dialog just displays the current state reported by
    ///   the services or calls into the relevant services when the user clicks a button to
    ///   enact a change).
    /// </remarks>
    public: void BindToServicesRoot(
      const std::shared_ptr<Services::ServicesRoot> &servicesRoot
    );

    /// <summary>Suggests an output name if one hasn't already been chosen</summary>
    private: void suggestOutputFilename();

    /// <summary>Reads the properties of the currently selected input file</summary>
    private: void readInputFileProperties();

    /// <summary>Receives notifications from the metadata updater</summary>
    private: void metadataUpdatedInBackgroundThread();

    /// <summary>Called in the UI thread when the input file was analyzed</summary>
    private: void updateMetadata();

    /// <summary>
    ///   Enables or disables the controls that should only appear when a valid input file
    ///   has been selected
    /// </summary>
    /// <param name="enable">True to enable the controls, false to disable</param>
    private: void enableControlsDependingOnValidInputFile(bool enable = true);

    /// <summary>Shows or hides the 'nightmode' slider</summary>
    /// <param name="show">True to show the slider, false to hide it</param>
    private: void showNightmodeSlider(bool show = true);

    /// <summary>Hides the warning frame at the bottom</summary>
    private: void hideWarningFrame();

    /// <summary>Shows a message in the warning frame at the bottom</summary>
    /// <param name="message">Message that will be shown in the frame</param>
    private: void showWarningFrame(const std::string &message);

    /// <summary>Responds to the user entering the path to the input file</param>
    private: void inputFileEntered();
    /// <summary>Shows the file selector when the user clicks the browse button</param>
    private: void browseInputFileClicked();
    /// <summary>Shows the file selector when the user clicks the browse button</param>
    private: void browseOutputFileClicked();
    /// <summary>Updates the UI whe nthe user picks another output channel layout</summary>
    /// <param name="layoutIndex">Index of the layout the user has selected</param>
    private: void selectedOutputChannelLayoutChanged(int layoutIndex);
    /// <summary>Updates the displayed bit rate when the bitrate slider is moved</summary>
    /// <param name="bitrate">Bitrate the user has chosen in the slider</param>
    private: void bitrateSliderMoved(int bitrate);
    /// <summary>Updates the bitrate slider when the number input box changes</summary>
    /// <param name="bitrate">Bitrate the user has entered in the input box</param>
    private: void bitrateNumberChanged(int bitrate);
    /// <summary>Aborts the encode or quits the application depending in its state</param>
    private: void quitClicked();

    /// <summary>The user interface arrangement generated from the .ui file</summary>
    private: std::unique_ptr<Ui::MainWindow> ui;
    /// <summary>Visualization of the audio channel mappings</summary>
    private: std::unique_ptr<QGraphicsScene> visualizationScene;
    /// <summary>Reader that is used to obtain metadata on the input file</summary>
    private: std::shared_ptr<Services::MetadataReader> metadataReader;
    /// <summary>Metadata for the currently selected input audio file</summary>
    private: std::optional<Audio::TrackInfo> metadata;

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder

#endif // NUCLEX_OPUSTRANSCODER_MAINWINDOW_H
