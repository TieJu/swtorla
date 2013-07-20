#pragma once

#include "ui_base.h"
#include "window.h"

#include <CommCtrl.h>
#include <Shlobj.h>


class main_ui : public ui_base {
    window_class            _wnd_class;
    std::unique_ptr<window> _wnd;
    std::unique_ptr<window> _progress_bar;
    std::unique_ptr<window> _status_text;

    void display_log_dir_select(display_log_dir_select_event) {
        display_dir_select();
    }

    template<typename EventType, typename HandlerType>
    bool handle_event(const any& v_, HandlerType handler_) {
        auto e = tj::any_cast<EventType>( &v_ );
        if ( e ) {
            handler_(*e);
        }
        return e != nullptr;
    }
    virtual void on_event(const any& v_) {
#define do_handle_event(type,handler) handle_event<type>(v_,[=](const type& e_) { handler(e_); })
#define do_handle_event_e(event_) do_handle_event(event_##_event,event_)
        if ( !do_handle_event_e(display_log_dir_select) ) {
            post_event(_wnd->native_window_handle(), v_);
        }
    }

    virtual void handle_os_events() {
        MSG msg
        {};
        if ( GetMessageW(&msg, _wnd->native_window_handle(), 0, 0) ) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            invoke_event_handlers(quit_event());
        }
    }

    void display_dir_select() {
#if 0
        /*
        OPENFILENAME open;

        if ( GetOpenFileName(&open) ) {
        }*/

        wchar_t dir[MAX_PATH + 1]{};
        BROWSEINFOW dir_info
        { 0//_wnd->native_window_handle()
        , nullptr/*todo: replace this with a link to last known folder*/
        , dir
        , L"Select SW:ToR log path"
        , BIF_EDITBOX | BIF_VALIDATE | BIF_NEWDIALOGSTYLE | BIF_USENEWUI | BIF_UAHINT
        , nullptr
        , 0
        , 0
        };

        auto path_res = SHBrowseForFolder(&dir_info);

        if ( !path_res ) {
            return;
        }

        invoke_event_handlers(set_log_dir_event{ dir });

        //SHGetPathFromIDList(path_res, dir);

        CoTaskMemFree(path_res);
#else
        std::wstring current_path;
        invoke_event_handlers(get_log_dir_event{ &current_path });

        BROWSEINFO bi{};
        // todo: this does not realy belongs here, do this if the config string was empty.
        if ( current_path.empty() ) {
            wchar_t base_loc[MAX_PATH * 2]{};
            if ( SHGetSpecialFolderPath(_wnd->native_window_handle(), base_loc, CSIDL_PERSONAL, FALSE) ) {
                wcscat_s(base_loc, L"\\Star Wars - The Old Republic\\CombatLogs");
                PIDLIST_ABSOLUTE id{};
                SHParseDisplayName(base_loc, nullptr, &id, SFGAO_FOLDER, nullptr);
                bi.pidlRoot = id;
            }
        }

        bi.hwndOwner = _wnd->native_window_handle();
        bi.ulFlags = BIF_RETURNONLYFSDIRS;	// do NOT use BIF_NEWDIALOGSTYLE, 
        // or BIF_STATUSTEXT        
        bi.ulFlags |= BIF_EDITBOX;
        bi.ulFlags |= BIF_NONEWFOLDERBUTTON;
        //bi.lpfn = BrowseCallbackProc;
        //bi.lParam = ( LPARAM ) &fp;
        //bi.pidlRoot
        bi.lpszTitle = L"Select SW:TOR Log Path";

        BOOL bRet = FALSE;

        auto pidlFolder = SHBrowseForFolder(&bi);

        if ( pidlFolder ) {
            wchar_t path_buffer[MAX_PATH * 2]{};

            if ( SHGetPathFromIDList(pidlFolder, path_buffer) ) {
                invoke_event_handlers(set_log_dir_event{ path_buffer });
            }
        }

        // free up pidls
        IMalloc *pMalloc = NULL;
        if ( SUCCEEDED(SHGetMalloc(&pMalloc)) && pMalloc ) {
            if ( pidlFolder )
                pMalloc->Free(pidlFolder);
            if ( bi.pidlRoot )
                pMalloc->Free((void*)bi.pidlRoot);
            pMalloc->Release();
        }
#endif
    }

public:
    main_ui()
        : _wnd_class(L"swtorla_main_window_class") {
            _wnd_class.style(_wnd_class.style() | CS_HREDRAW | CS_VREDRAW);
            _wnd_class.register_class();
            _wnd.reset(new window(0
                , _wnd_class
                , L"SW:ToR log analizer"
                , WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE
                , CW_USEDEFAULT
                , CW_USEDEFAULT
                , 350
                , 125
                , nullptr
                , nullptr
                , _wnd_class.source_instance()));

            _progress_bar.reset(new window(0, PROGRESS_CLASS, L"", WS_CHILD | WS_VISIBLE, 50, 15, 250, 25, _wnd->native_window_handle(), nullptr, _wnd_class.source_instance()));

            _status_text.reset(new window(0, L"static", L"static", SS_CENTER | WS_CHILD | WS_VISIBLE, 50, 50, 250, 25, _wnd->native_window_handle(), nullptr, _wnd_class.source_instance()));

            _wnd->callback([=](window* window_, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
                return os_callback_handler_default(window_, uMsg, wParam, lParam);
            });
            _wnd->update();

    }
    virtual ~main_ui() {
    }
};