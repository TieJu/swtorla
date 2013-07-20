#pragma once

#include "ui_base.h"
#include "window.h"

#include <CommCtrl.h>
#include <Shlobj.h>
#include <Shellapi.h>

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

#if defined(USE_CUSTOME_SELECTOR)
    static void ScreenToClientX(HWND hWnd, LPRECT lpRect) {
        ::ScreenToClient(hWnd, (LPPOINT)lpRect);
        ::ScreenToClient(hWnd, ( (LPPOINT)lpRect ) + 1);
    }

    static void MoveWindowX(HWND hWnd, RECT& rect, BOOL bRepaint) {
        ::MoveWindow(hWnd, rect.left, rect.top,
                     rect.right - rect.left, rect.bottom - rect.top, bRepaint);
    }
    
    static void SizeBrowseDialog(HWND hWnd) {
        // find the folder tree and make dialog larger
        HWND hwndTree = FindWindowEx(hWnd, NULL, L"SysTreeView32", NULL);

        if ( !hwndTree ) {
            // ... this usually means that BIF_NEWDIALOGSTYLE is enabled.
            // Then the class name is as used in the code below.
            hwndTree = FindWindowEx(hWnd, NULL, L"SHBrowseForFolder ShellNameSpace Control", NULL);
        }

        RECT rectDlg;

        if ( hwndTree ) {
            // check if edit box
            int nEditHeight = 0;
            HWND hwndEdit = FindWindowEx(hWnd, NULL, L"Edit", NULL);
            RECT rectEdit;
            if ( hwndEdit ) {
                ::GetWindowRect(hwndEdit, &rectEdit);
                ScreenToClientX(hWnd, &rectEdit);
                nEditHeight = rectEdit.bottom - rectEdit.top;
            } else if ( hwndEdit ) {
                ::MoveWindow(hwndEdit, 20000, 20000, 10, 10, FALSE);
                ::ShowWindow(hwndEdit, SW_HIDE);
                hwndEdit = 0;
            }

            // make the dialog larger
            ::GetWindowRect(hWnd, &rectDlg);
            rectDlg.right += 40;
            rectDlg.bottom += 30;
            if ( hwndEdit )
                rectDlg.bottom += nEditHeight + 5;
            MoveWindowX(hWnd, rectDlg, TRUE);
            ::GetClientRect(hWnd, &rectDlg);

            int hMargin = 10;
            int vMargin = 10;

            // check if new dialog style - this means that there will be a resizing
            // grabber in lower right corner
            //if ( fp->ulFlags & BIF_NEWDIALOGSTYLE )
                hMargin = ::GetSystemMetrics(SM_CXVSCROLL);

            // move the Cancel button
            RECT rectCancel;
            HWND hwndCancel = ::GetDlgItem(hWnd, IDCANCEL);
            if ( hwndCancel )
                ::GetWindowRect(hwndCancel, &rectCancel);
            ScreenToClientX(hWnd, &rectCancel);
            int h = rectCancel.bottom - rectCancel.top;
            int w = rectCancel.right - rectCancel.left;
            rectCancel.bottom = rectDlg.bottom - vMargin;//nMargin;
            rectCancel.top = rectCancel.bottom - h;
            rectCancel.right = rectDlg.right - hMargin; //(scrollWidth + 2*borderWidth);
            rectCancel.left = rectCancel.right - w;
            if ( hwndCancel ) {
                //TRACERECT(rectCancel);
                MoveWindowX(hwndCancel, rectCancel, FALSE);
            }

            // move the OK button
            RECT rectOK{};
            HWND hwndOK = ::GetDlgItem(hWnd, IDOK);
            if ( hwndOK ) {
                ::GetWindowRect(hwndOK, &rectOK);
            }
            ScreenToClientX(hWnd, &rectOK);
            rectOK.bottom = rectCancel.bottom;
            rectOK.top = rectCancel.top;
            rectOK.right = rectCancel.left - 10;
            rectOK.left = rectOK.right - w;
            if ( hwndOK ) {
                MoveWindowX(hwndOK, rectOK, FALSE);
            }

            // expand the folder tree to fill the dialog
            RECT rectTree;
            ::GetWindowRect(hwndTree, &rectTree);
            ScreenToClientX(hWnd, &rectTree);
            if ( hwndEdit ) {
                rectEdit.left = hMargin;
                rectEdit.right = rectDlg.right - hMargin;
                rectEdit.top = vMargin;
                rectEdit.bottom = rectEdit.top + nEditHeight;
                MoveWindowX(hwndEdit, rectEdit, FALSE);
                rectTree.top = rectEdit.bottom + 5;
            } else {
                rectTree.top = vMargin;
            }
            rectTree.left = hMargin;
            rectTree.bottom = rectOK.top - 10;//nMargin;
            rectTree.right = rectDlg.right - hMargin;
            //TRACERECT(rectTree);
            MoveWindowX(hwndTree, rectTree, FALSE);
        }
    }
#endif
    static int CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
        switch ( uMsg ) {
        case BFFM_INITIALIZED:
            {
#if defined(USE_CUSTOME_SELECTOR)
                // remove context help button from dialog caption
                LONG lStyle = ::GetWindowLong(hWnd, GWL_STYLE);
                lStyle &= ~DS_CONTEXTHELP;
                ::SetWindowLong(hWnd, GWL_STYLE, lStyle);
                lStyle = ::GetWindowLong(hWnd, GWL_EXSTYLE);
                lStyle &= ~WS_EX_CONTEXTHELP;
                ::SetWindowLong(hWnd, GWL_EXSTYLE, lStyle);
#endif

                auto pre_select = reinterpret_cast<std::wstring*>(lpData);
                if ( pre_select ) {
                    ::SendMessage(hWnd, BFFM_SETSELECTION, TRUE, (LPARAM)pre_select->c_str());
                } 
                
                ::SetWindowText(hWnd, L"Select Path to SW:TOR Combat Logs");
#if defined(USE_CUSTOME_SELECTOR)
                SizeBrowseDialog(hWnd);
#endif
            }
            break;

        case BFFM_SELCHANGED:
            {
                wchar_t dir[MAX_PATH * 2]{};

                // fail if non-filesystem
                BOOL bRet = SHGetPathFromIDList((LPITEMIDLIST)lParam, dir);
                if ( bRet ) {
                    // fail if folder not accessible
                    if ( _waccess_s(dir, 00) != 0 ) {
                        bRet = FALSE;
                    } else {
                        SHFILEINFO sfi;
                        ::SHGetFileInfo((LPCTSTR)lParam, 0, &sfi, sizeof( sfi ),
                                        SHGFI_PIDL | SHGFI_ATTRIBUTES);

                        // fail if pidl is a link
                        if ( sfi.dwAttributes & SFGAO_LINK ) {
                            bRet = FALSE;
                        }
                    }
                }

                // if invalid selection, disable the OK button
                if ( !bRet ) {
                    ::EnableWindow(GetDlgItem(hWnd, IDOK), FALSE);
                }
            }
            break;
        }

        return 0;
    }

    void display_dir_select() {
        std::wstring current_path;
        invoke_event_handlers(get_log_dir_event{ &current_path });

        BROWSEINFO bi{};
        // todo: this does not realy belongs here, do this if the config string was empty.
        if ( current_path.empty() ) {
            wchar_t base_loc[MAX_PATH * 2]{};
            if ( SHGetSpecialFolderPath(_wnd->native_window_handle(), base_loc, CSIDL_PERSONAL, FALSE) ) {
                current_path = base_loc;
                current_path += L"\\Star Wars - The Old Republic\\CombatLogs";
            }
        }

        //SHParseDisplayName(current_path.c_str(), nullptr, const_cast<PIDLIST_ABSOLUTE*>(&bi.pidlRoot), SFGAO_FOLDER, nullptr);

        bi.hwndOwner = _wnd->native_window_handle();
        bi.ulFlags = BIF_RETURNONLYFSDIRS;
#if !defined(USE_CUSTOME_SELECTOR)
        bi.ulFlags |= BIF_NEWDIALOGSTYLE | BIF_STATUSTEXT;	// do NOT use BIF_NEWDIALOGSTYLE, 
#endif
        // or BIF_STATUSTEXT        
        bi.ulFlags |= BIF_EDITBOX;
        bi.ulFlags |= BIF_NONEWFOLDERBUTTON;
        bi.lpfn = BrowseCallbackProc;
        bi.lParam = ( LPARAM ) &current_path;
        //bi.pidlRoot
        //bi.lpszTitle = L"Select SW:TOR Log Path";

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
                pMalloc->Free(const_cast<PIDLIST_ABSOLUTE>(bi.pidlRoot));
            pMalloc->Release();
        }
    }

public:
    main_ui()
        : _wnd_class(L"swtorla_main_window_class") {
            setup_default_window_class(_wnd_class);
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