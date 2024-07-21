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

#include "./Config.h"

#include "./MainWindow.h"

#include <QApplication>
#include <QMessageBox>

// --------------------------------------------------------------------------------------------- //

#if !defined(NUCLEX_OPUSTRANSCODER_UNIT_TEST_EXECUTABLE)

/// <summary>Entry point for the application</summary>
/// <param name="argc">The number of command line arguments provided</param>
/// <param name="argv">The values of all command line arguments</param>
/// <returns>The exit code the application has terminated with</returns>
int main(int argc, char *argv[]) {
  int exitCode;
  {
    QApplication application(argc, argv);

    //application.setWindowIcon(QIcon(u8":/freepik-film-512x512.png"));

    std::shared_ptr<Nuclex::OpusTranscoder::MainWindow> mainWindow = (
      std::make_shared<Nuclex::OpusTranscoder::MainWindow>()
    );
    mainWindow->show();

    exitCode = application.exec();
  }

  return exitCode;
}

#endif // !defined(NUCLEX_FRAMEFIXER_UNIT_TEST_EXECUTABLE)

// --------------------------------------------------------------------------------------------- //
