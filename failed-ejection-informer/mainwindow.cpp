#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSystemTrayIcon>
#include <QStandardPaths>
#include <QDir>

#include "customtoasthandler.h"

#include "wintoastlib/wintoastlib.h"

#pragma comment(lib, "wevtapi.lib")

using namespace WinToastLib;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , hSubscription(nullptr)
{
    ui->setupUi(this);

//    QApplication::setQuitOnLastWindowClosed(false);

    QSystemTrayIcon* icon = new QSystemTrayIcon(this);
    icon->setIcon(QIcon(":/sivvus_pendrive_icon_4.png"));
    QMenu* menu = new QMenu(this);
    QAction* quitAction = new QAction("&Quit", this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    QAction* startupAction = new QAction("&Run on startup", this);
    startupAction->setCheckable(true);
    connect(startupAction, &QAction::toggled, this, [this](bool checked) {
        QDir dir(QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation));
        dir.cd("Startup");
        QFile exeFile(QCoreApplication::applicationFilePath(), this);
        QString linkPath = dir.filePath(QFileInfo(exeFile).completeBaseName() + ".lnk");
        if (checked)
        {
            exeFile.link(linkPath);
        }
        else
        {
            QFile::moveToTrash(linkPath);
        }
    });
    startupAction->setChecked(true);
    menu->addAction(startupAction);
    menu->addSeparator();
    menu->addAction(quitAction);
    icon->setToolTip("Failed Ejection Informer");
    icon->setContextMenu(menu);
    icon->show();


    // Toast
    if (!WinToast::isCompatible())
    {
        wprintf(L"Error, your system in not supported!\n\n");
        fflush(stdout);
        return;
    }

    WinToast::instance()->setAppName(L"Ejection Failed Information");
    const auto aumi = WinToast::configureAUMI(L"Radiance Technologies", L"Ejection Failed Information", L"App", L"20210819");
    WinToast::instance()->setAppUserModelId(aumi);

    if (!WinToast::instance()->initialize())
    {
      std::wcout << L"Error, could not initialize the lib!" << std::endl;
      return;
    }

//    CustomToastHandler* handler = new CustomToastHandler;
//    WinToastTemplate templ = WinToastTemplate(WinToastTemplate::Text01);
//    templ.setDuration(WinToastTemplate::Duration::Short);
//    QString line = QString("\"%1\" (PID %2) is preventing ejection").arg("Example").arg(1234);
//    templ.setTextField(line.toStdWString(), WinToastTemplate::FirstLine);

//    long toastResult = WinToast::instance()->showToast(templ, handler);
//    if (toastResult == -1)
//    {
//        std::cout << "Toast Error" << std::endl;
//        std::wcout << L"Error: Could not launch your toast notification!" << std::endl;
//    }

    // Event watcher
    DWORD status = ERROR_SUCCESS;
    LPCWSTR pwsPath = L"System";
    LPCWSTR pwsQuery = L"*[System[(EventID=225)]]";

    // The subscription will return any future events that are raised while the application is active.
    hSubscription = EvtSubscribe(NULL, NULL, pwsPath, pwsQuery, NULL, NULL,
                                 (EVT_SUBSCRIBE_CALLBACK)SubscriptionCallback, EvtSubscribeToFutureEvents); // EvtSubscribeStartAtOldestRecord
    if (NULL == hSubscription)
    {
        status = GetLastError();

        if (ERROR_EVT_CHANNEL_NOT_FOUND == status)
        {
            wprintf(L"Channel %s was not found.\n", pwsPath);
            fflush(stdout);
        }
        else if (ERROR_EVT_INVALID_QUERY == status)
        {
            // You can call EvtGetExtendedStatus to get information as to why the query is not valid.
            wprintf(L"The query \"%s\" is not valid.\n", pwsQuery);
            fflush(stdout);
        }
        else
        {
            wprintf(L"EvtSubscribe failed with %lu.\n", status);
            fflush(stdout);
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;

    if (hSubscription)
        EvtClose(hSubscription);
}

DWORD WINAPI MainWindow::SubscriptionCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent)
{
    UNREFERENCED_PARAMETER(pContext);

    DWORD status = ERROR_SUCCESS;

    switch(action)
    {
        // You should only get the EvtSubscribeActionError action if your subscription flags
        // includes EvtSubscribeStrict and the channel contains missing event records.
        case EvtSubscribeActionError:
            if (ERROR_EVT_QUERY_RESULT_STALE == (DWORD)hEvent)
            {
                wprintf(L"The subscription callback was notified that event records are missing.\n");
                fflush(stdout);
                // Handle if this is an issue for your application.
            }
            else
            {
                wprintf(L"The subscription callback received the following Win32 error: %lu\n", (DWORD)hEvent);
                fflush(stdout);
            }
            break;

        case EvtSubscribeActionDeliver:
            if (ERROR_SUCCESS != (status = PrintEventValues(hEvent)))
            {
                goto cleanup;
            }
            break;

        default:
            wprintf(L"SubscriptionCallback: Unknown action.\n");
            fflush(stdout);
    }

cleanup:

    if (ERROR_SUCCESS != status)
    {
        // End subscription - Use some kind of IPC mechanism to signal
        // your application to close the subscription handle.
    }

    return status; // The service ignores the returned status.
}

DWORD MainWindow::PrintEventValues(EVT_HANDLE hEvent)
{
    DWORD status = ERROR_SUCCESS;
    EVT_HANDLE hContext = NULL;
    DWORD dwBufferSize = 0;
    DWORD dwBufferUsed = 0;
    DWORD dwPropertyCount = 0;
    PEVT_VARIANT pRenderedValues = NULL;
//    LPCWSTR ppValues[] = {L"Event/System/Provider/@Name", L"Event/System/Channel"};
    LPCWSTR ppValues[] = {L"Event/EventData/Data[@Name=\"ProcessId\"]", L"Event/EventData/Data[@Name=\"ProcessName\"]"};
    DWORD count = sizeof(ppValues)/sizeof(LPCWSTR);
    WinToastTemplate templ;
    QString line1;
    QString line2;
    std::wstring procName;
    std::wstring devPrefix(L"\\Device\\");

    // Identify the components of the event that you want to render. In this case,
    // render the provider's name and channel from the system section of the event.
    // To get user data from the event, you can specify an expression such as
    // L"Event/EventData/Data[@Name=\"<data name goes here>\"]".
    hContext = EvtCreateRenderContext(count, (LPCWSTR*)ppValues, EvtRenderContextValues);
    if (NULL == hContext)
    {
        wprintf(L"EvtCreateRenderContext failed with %lu\n", status = GetLastError());
        fflush(stdout);
        goto cleanup;
    }

    // The function returns an array of variant values for each element or attribute that
    // you want to retrieve from the event. The values are returned in the same order as
    // you requested them.
    if (!EvtRender(hContext, hEvent, EvtRenderEventValues, dwBufferSize, pRenderedValues, &dwBufferUsed, &dwPropertyCount))
    {
        if (ERROR_INSUFFICIENT_BUFFER == (status = GetLastError()))
        {
            dwBufferSize = dwBufferUsed;
            pRenderedValues = (PEVT_VARIANT)malloc(dwBufferSize);
            if (pRenderedValues)
            {
                EvtRender(hContext, hEvent, EvtRenderEventValues, dwBufferSize, pRenderedValues, &dwBufferUsed, &dwPropertyCount);
            }
            else
            {
                wprintf(L"malloc failed\n");
                fflush(stdout);
                status = ERROR_OUTOFMEMORY;
                goto cleanup;
            }
        }

        if (ERROR_SUCCESS != (status = GetLastError()))
        {
            wprintf(L"EvtRender failed with %d\n", GetLastError());
            fflush(stdout);
            goto cleanup;
        }
    }

    // Print the selected values.
//    wprintf(L"\nProvider Name: %s\n", pRenderedValues[0].StringVal);
//    wprintf(L"\nProcess Name: %s\n", pRenderedValues[1].StringVal);
//    fflush(stdout);
//    wprintf(L"Channel: %s\n", (EvtVarTypeNull == pRenderedValues[1].Type) ? L"" : pRenderedValues[1].StringVal);
//    wprintf(L"PID: %d\n", (EvtVarTypeNull == pRenderedValues[0].Type) ? 0 : pRenderedValues[0].Int32Val);
//    fflush(stdout);

    procName = std::wstring(pRenderedValues[1].StringVal);

    if (procName.find(devPrefix) == 0)
    {
        std::wstring devName = procName.substr(0, procName.find(L"\\", devPrefix.length()));;
        std::wstring volName = VolumeNameFromDeviceName(devName);
        procName = procName.replace(procName.find(devName), devName.length() + 1, volName);

    }

    CustomToastHandler* handler = new CustomToastHandler;
//    templ = WinToastTemplate(WinToastTemplate::ImageAndText01);
    templ = WinToastTemplate(WinToastTemplate::Text01);
//    templ.setImagePath(L":/sivvus_pendrive_icon_4.png");
//    templ.setImagePath(L"C:/projects/misc/failed-ejection-informer/failed-ejection-informer/sivvus_pendrive_icon_4.png");
    templ.setAudioOption(WinToastTemplate::AudioOption::Silent);
    templ.setDuration(WinToastTemplate::Duration::Long);
    line1 = QString("\"%1\" (PID %2) is preventing ejection").arg(procName).arg(pRenderedValues[0].Int32Val);
    std::wcout << line1.toStdWString() << std::endl;
//    line2 = QString("Click to view the event logs");


//    line = QString("\"%1\" (PID %2) is preventing ejection").arg("test").arg(1234);
    templ.setTextField(line1.toStdWString(), WinToastTemplate::FirstLine);
//    templ.setTextField(line2.toStdWString(), WinToastTemplate::SecondLine);
    templ.addAction(L"View event logs");

    WinToast::WinToastError err;
    long toastResult = WinToast::instance()->showToast(templ, handler, &err);
    if (toastResult == -1)
    {
        std::cout << "Toast Error" << std::endl;
        std::wcout << L"Error: Could not launch your toast notification! (" << err << ")" << std::endl;
    }

//    templ.setExpiration(1000);

cleanup:

    if (hContext)
        EvtClose(hContext);

    if (pRenderedValues)
        free(pRenderedValues);

    return status;
}

std::wstring MainWindow::VolumeNameFromDeviceName(std::wstring devName)
{
    std::wstring procName;
    DWORD CharCount = MAX_PATH;
    WCHAR Names[MAX_PATH];
    std::wstring volName;

    WCHAR volumeName[MAX_PATH] = L"";
    HANDLE findHandle = FindFirstVolumeW(volumeName, ARRAYSIZE(volumeName));

    if (findHandle == INVALID_HANDLE_VALUE)
    {
        DWORD Error = GetLastError();
        wprintf(L"FindFirstVolumeW failed with error code %d\n", Error);
        return 0;
    }

    BOOL success = true;
    while (success)
    {
        size_t lastIdx = wcslen(volumeName) - 1;

        if (volumeName[0]     != L'\\' ||
                    volumeName[1]     != L'\\' ||
                    volumeName[2]     != L'?'  ||
                    volumeName[3]     != L'\\' ||
                    volumeName[lastIdx] != L'\\')
        {
//                DWORD Error = ERROR_BAD_PATHNAME;
            wprintf(L"FindFirstVolumeW/FindNextVolumeW returned a bad path: %s\n", volumeName);
            break;
        }

        WCHAR DeviceName[MAX_PATH] = L"";
        volumeName[lastIdx] = L'\0';
        DWORD CharCount = QueryDosDeviceW(&volumeName[4], DeviceName, ARRAYSIZE(DeviceName));
        volumeName[lastIdx] = L'\\';

        if ( CharCount == 0 )
        {
            DWORD Error = GetLastError();
            wprintf(L"QueryDosDeviceW failed with error code %d\n", Error);
            break;
        }

        if (std::wstring(DeviceName) == devName)
        {
            break;
        }

        success = FindNextVolumeW(findHandle, volumeName, ARRAYSIZE(volumeName));
    }

    if (success)
    {
        GetVolumePathNamesForVolumeNameW(volumeName, Names, CharCount, &CharCount);
    }
    volName = std::wstring(Names);

    return volName;
}

