#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <windows.h>
#include <winevt.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static DWORD WINAPI SubscriptionCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent);

private:
    static DWORD PrintEventValues(EVT_HANDLE hEvent);
    static std::wstring VolumeNameFromDeviceName(std::wstring devName);

    Ui::MainWindow *ui;

    EVT_HANDLE hSubscription;
};
#endif // MAINWINDOW_H
