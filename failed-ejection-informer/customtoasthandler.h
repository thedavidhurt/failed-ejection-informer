#ifndef CUSTOMTOASTHANDLER_H
#define CUSTOMTOASTHANDLER_H

#include "wintoastlib/wintoastlib.h"

using namespace WinToastLib;

class CustomToastHandler : public IWinToastHandler
{
public:
    void toastActivated() const {
        std::wcout << L"The user clicked in this toast" << std::endl;
    }

    void toastActivated(int actionIndex) const {
        std::wcout << L"The user clicked on button #" << actionIndex << L" in this toast" << std::endl;

        if (actionIndex == 0)
        {
            system("eventvwr /c:System /f:\"*[System[(EventID=225)]]\"");
        }
    }

    void toastFailed() const {
        std::wcout << L"Error showing current toast" << std::endl;
    }
    void toastDismissed(WinToastDismissalReason state) const {
        switch (state) {
        case UserCanceled:
            std::wcout << L"The user dismissed this toast" << std::endl;
            break;
        case ApplicationHidden:
            std::wcout <<  L"The application hid the toast using ToastNotifier.hide()" << std::endl;
            break;
        case TimedOut:
            std::wcout << L"The toast has timed out" << std::endl;
            break;
        default:
            std::wcout << L"Toast not activated" << std::endl;
            break;
        }
    }
};

#endif // CUSTOMTOASTHANDLER_H
