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

    /// <summary>The user interface arrangement generated from the .ui file</summary>
    private: std::unique_ptr<Ui::MainWindow> ui;

  };

  // ------------------------------------------------------------------------------------------- //

} // namespace Nuclex::OpusTranscoder

#endif // NUCLEX_OPUSTRANSCODER_MAINWINDOW_H
