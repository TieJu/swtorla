#include "app.h"

#include <stdexcept>
#include <memory>

#include <Windows.h>
#include <shellapi.h>

#define APP_CATION "SW:ToR Log Analizer"
#define CONFIG_PATH "./config.json"

#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif


#include "swtor_log_parser.h"

#include "resource.h"

// google breakpad
#include <client/windows/handler/exception_handler.h>

INT_PTR CALLBACK DialogProc(
    _In_  HWND hwndDlg,
    _In_  UINT uMsg,
    _In_  WPARAM wParam,
    _In_  LPARAM lParam
    ) {
        
        if ( WM_INITDIALOG == uMsg ) {
            auto value = GetWindowLongPtrW(hwndDlg, DWLP_USER);
            return true;
        } else if ( WM_CLOSE == uMsg || WM_DESTROY == uMsg ) {
            PostQuitMessage(0);
            return true;
        }
        return false;
}

int APIENTRY WinMain(HINSTANCE hInstance
                    ,HINSTANCE /*hPrevInstance*/
                    ,LPSTR /*lpCmdLine*/
                    ,int /*nCmdShow*/) {

    auto exh = std::make_unique<google_breakpad::ExceptionHandler>( L".", nullptr, nullptr, nullptr, google_breakpad::ExceptionHandler::HANDLER_ALL, MiniDumpWithFullMemory, (const wchar_t*)nullptr, nullptr );
    //int arg_c;
    //auto arg_v = CommandLineToArgvW(GetCommandLine(), &arg_c);
    auto hMutex = CreateMutexW(nullptr, TRUE, L"swtor_log_analizer_unique");
    WaitForSingleObject(hMutex, INFINITE);
    /*
    auto wnd = CreateDialogParamW(hInstance, MAKEINTRESOURCEW(IDD_MAIN_WINDOW), nullptr, DialogProc, 5);
    ShowWindow(wnd,SW_NORMAL);
    MSG msg
    {};
    while ( GetMessageW(&msg, wnd, 0, 0) ) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyWindow(wnd);*/

    try {
        app core(APP_CATION, CONFIG_PATH);

        core();
    } catch ( const std::exception& e ) {
        MessageBoxA(nullptr, e.what(), "Crash!", 0);
    } catch ( ... ) {
        MessageBoxA(nullptr, "Very bad crash", "Crash!", 0);
    }
    CloseHandle(hMutex);
}