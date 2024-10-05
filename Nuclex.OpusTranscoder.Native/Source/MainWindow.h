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

#include <QMainWindow> // for QMainWindow
#include <QItemSelection> // for QItemSelection
#include <QMutex> // for QMutex

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

    private: void metadataUpdated();

    /// <summary>Shows the file selector when the user clicks the browse button</param>
    private: void browseInputFileClicked();
    /// <summary>Shows the file selector when the user clicks the browse button</param>
    private: void browseOutputFileClicked();
    /// <summary>Aborts the encode or quits the application depending in its state</param>
    private: void quitClicked();

    /// <summary>The user interface arrangement generated from the .ui file</summary>
    private: std::unique_ptr<Ui::MainWindow> ui;
    /// <summary>Reader that is used to obtain metadata on the input file</summary>
    private: std::shared_ptr<Services::MetadataReader> metadataReader;

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder

#endif // NUCLEX_OPUSTRANSCODER_MAINWINDOW_H
