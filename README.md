# failed-ejection-informer

Tells you which process is holding onto your USB drive

Latest executable is failed-ejection-informer.zip at https://github.com/thedavidhurt/failed-ejection-informer/releases/latest/download/failed-ejection-informer.zip

## Building
*  Use Qt Creator to open and build the project: https://www.qt.io/download-qt-installer
*  When opening the project in Qt for the first time, select a MSVC kit
*  Use windeployqt to make the build folder redistributable:
    *  Qt > Projects > Build > Build Steps > Add Build Step > Custom Process Step
        *  Command: %{Qt:QT_INSTALL_BINS}\windeployqt.exe
        *  Arguments: %{CurrentRun:Executable:FileName}
        *  Working Directory: %{CurrentRun:Executable:Path}
