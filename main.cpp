#include "app.h"

#include <stdexcept>
#include <memory>

#include <Windows.h>
#include <shellapi.h>

#define APP_CATION "SW:ToR Log Analizer"
#define CONFIG_PATH "./config.json"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "swtor_log_parser.h"

// google breakpad
#include <client/windows/handler/exception_handler.h>

int APIENTRY WinMain(HINSTANCE /*hInstance*/
                    ,HINSTANCE /*hPrevInstance*/
                    ,LPSTR /*lpCmdLine*/
                    ,int /*nCmdShow*/) {

    auto exh = std::make_unique<google_breakpad::ExceptionHandler>( L".", nullptr, nullptr, nullptr, google_breakpad::ExceptionHandler::HANDLER_ALL );
    //int arg_c;
    //auto arg_v = CommandLineToArgvW(GetCommandLine(), &arg_c);
    auto hMutex = CreateMutexW(nullptr, TRUE, L"swtor_log_analizer_unique");
    WaitForSingleObject(hMutex, INFINITE);
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