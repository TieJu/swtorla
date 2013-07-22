#include "main_ui.h"

#include <boost/scope_exit.hpp>

#include <CommCtrl.h>
#include <Shlobj.h>
#include <Shellapi.h>

void main_ui::display_log_dir_select(display_log_dir_select_event) {
    display_dir_select();
}

void main_ui::on_event(const any& v_) {
#define do_handle_event(type,handler) handle_event<type>(v_,[=](const type& e_) { handler(e_); })
#define do_handle_event_e(event_) do_handle_event(event_##_event,event_)
    if ( !do_handle_event_e(display_log_dir_select) ) {
        post_event(_wnd->native_window_handle(), v_);
    }
}

void main_ui::handle_os_events() {
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
void main_ui::ScreenToClientX(HWND hWnd, LPRECT lpRect) {
    ::ScreenToClient(hWnd, (LPPOINT)lpRect);
    ::ScreenToClient(hWnd, ( (LPPOINT)lpRect ) + 1);
}

void main_ui::MoveWindowX(HWND hWnd, RECT& rect, BOOL bRepaint) {
    ::MoveWindow(hWnd, rect.left, rect.top,
                    rect.right - rect.left, rect.bottom - rect.top, bRepaint);
}

void main_ui::SizeBrowseDialog(HWND hWnd) {
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
        RECT rectOK
        {};
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
int CALLBACK main_ui::BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
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

            auto pre_select = reinterpret_cast<std::wstring*>( lpData );
            if ( pre_select ) {
                ::SendMessageW(hWnd, BFFM_SETSELECTION, TRUE, (LPARAM)pre_select->c_str());
            }

            ::SetWindowTextW(hWnd, L"Select Path to SW:TOR Combat Logs");
#if defined(USE_CUSTOME_SELECTOR)
            SizeBrowseDialog(hWnd);
#endif
        }
        break;

    case BFFM_SELCHANGED:
        {
            wchar_t dir[MAX_PATH * 2]{};

            // fail if non-filesystem
            BOOL bRet = SHGetPathFromIDListW((LPITEMIDLIST)lParam, dir);
            if ( bRet ) {
                // fail if folder not accessible
                if ( _waccess_s(dir, 00) != 0 ) {
                    bRet = FALSE;
                } else {
                    SHFILEINFOW sfi;
                    ::SHGetFileInfoW((LPCWSTR)lParam
                                     , 0
                                     , &sfi
                                     , sizeof( sfi )
                                     , SHGFI_PIDL | SHGFI_ATTRIBUTES);

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

void main_ui::display_dir_select() {
    std::wstring current_path = _path_edit->caption();

    BROWSEINFO bi{};
    bi.hwndOwner    = _wnd->native_window_handle();
    bi.ulFlags      = BIF_RETURNONLYFSDIRS;
#if !defined(USE_CUSTOME_SELECTOR)
    bi.ulFlags      |= BIF_NEWDIALOGSTYLE | BIF_STATUSTEXT;
#endif
    bi.ulFlags      |= BIF_EDITBOX;
    bi.ulFlags      |= BIF_NONEWFOLDERBUTTON;
    bi.lpfn         = BrowseCallbackProc;
    bi.lParam       = ( LPARAM ) &current_path;

    auto pidlFolder = SHBrowseForFolderW(&bi);
    BOOST_SCOPE_EXIT_ALL(= ) {
        CoTaskMemFree(pidlFolder);
    };

    if ( pidlFolder ) {
        wchar_t path_buffer[MAX_PATH * 2]{};

        if ( SHGetPathFromIDListW(pidlFolder, path_buffer) ) {
            invoke_event_handlers(set_log_dir_event{ path_buffer });
            _path_edit->caption(path_buffer);
        }
    }
}

void main_ui::on_size(window* window_, WPARAM wParam, LPARAM lParam) {

}

LRESULT main_ui::os_callback_handler(window* window_, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if ( uMsg == WM_COMMAND ) {
        auto id = LOWORD(wParam);
        auto code = HIWORD(wParam);
        switch ( id ) {
        case control_path_button:
            if ( code == BN_CLICKED ) {
                display_dir_select();
            }
            break;
        case control_path_edit:
        default:
            break;
        }
    } else if ( uMsg == WM_SIZE ) {
        on_size(window_, wParam, lParam);
    } else if ( uMsg == WM_GETMINMAXINFO ) {
        auto info = reinterpret_cast<MINMAXINFO*>( lParam );
        info->ptMinTrackSize.x = min_width;
        info->ptMinTrackSize.y = min_height;
        // currently disable maxing
        info->ptMaxTrackSize.x = min_width;
        info->ptMaxTrackSize.y = min_height;
    } else if ( uMsg == WM_NOTIFY ) {
        _tab->handle_event(*reinterpret_cast<LPNMHDR>(lParam));
    }
    return os_callback_handler_default(window_, uMsg, wParam, lParam);
}

main_ui::main_ui(const std::wstring& log_path_)
: _wnd_class(L"swtorla_main_window_class") {
    setup_default_window_class(_wnd_class);
    _wnd_class.register_class();
    _wnd.reset(new window(0
        , _wnd_class
        , L"SW:ToR log analizer"
        , WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE
        , CW_USEDEFAULT
        , CW_USEDEFAULT
        , 600
        , 600
        , nullptr
        , nullptr
        , _wnd_class.source_instance()));

    RECT wr{};
    GetClientRect(_wnd->native_window_handle(), &wr);

    auto char_size_x = LOWORD(GetDialogBaseUnits());
    auto char_size_y = HIWORD(GetDialogBaseUnits());

    _path_button.reset(new window(0, L"button", L"Browse", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, char_size_x, 8, 8 * char_size_x, 7 * char_size_y / 6, _wnd->native_window_handle(), (HMENU)control_path_button, _wnd_class.source_instance()));
    _tab.reset(new tab_set(2, char_size_y * 2 + 2, wr.right - 2, wr.bottom - 2, _wnd->native_window_handle(), control_tab, _wnd_class.source_instance()));

    auto pos_left = 600 - char_size_x;;
    pos_left -= 15 * char_size_x - char_size_x - char_size_x;
    _sync_raid_button.reset(new window(0, L"button", L"Sync to Raid", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, pos_left, 8, 12 * char_size_x, 7 * char_size_y / 6, _wnd->native_window_handle(), (HMENU)control_sync_to_raid_button, _wnd_class.source_instance()));
    // for now you cant sync to raids
    _sync_raid_button->disable();

    pos_left -= 6 * char_size_x - char_size_x - char_size_x;
    _start_solo_button.reset(new window(0, L"button", L"Solo", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, pos_left, 8, 4 * char_size_x, 7 * char_size_y / 6, _wnd->native_window_handle(), (HMENU)control_start_solo_button, _wnd_class.source_instance()));

    pos_left -= 11 * char_size_x - char_size_x - char_size_x;
    _path_edit.reset(new window(0, L"edit", log_path_.c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL, 9 * char_size_x, 8, pos_left, 7 * char_size_y / 6, _wnd->native_window_handle(), (HMENU)control_path_edit, _wnd_class.source_instance()));


    std::vector<std::unique_ptr<window>> tab_elements;
    tab_elements.emplace_back(new window(0, L"static", L"you!", WS_CHILD, 0, 0, 0, 0, _tab->native_window_handle(), nullptr, _wnd_class.source_instance()));
    auto you_tab = _tab->add_tab(L"You", std::move(tab_elements));
    tab_elements.emplace_back(new window(0, L"static", L"raid!", WS_CHILD, 0, 0, 0, 0, _tab->native_window_handle(), nullptr, _wnd_class.source_instance()));
    _tab->add_tab(L"Raid", std::move(tab_elements));

    wr = _tab->get_client_area_rect();
    _tab->get_client_rect_for_window_rect(wr);

    for ( size_t t = 0; t < _tab->get_tab_element_count(); ++t ) {
        auto& tab = _tab->get_tab_elements(t);

        for ( auto& element : tab ) {
            element->move(wr.left, wr.top, wr.right - wr.left, 7 * char_size_y / 6);
        }
    }

    _tab->switch_tab(you_tab);


    _wnd->callback([=](window* window_, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        return os_callback_handler(window_, uMsg, wParam, lParam);
    });
    _wnd->update();
    _wnd->size(600, 600);

}

main_ui::~main_ui() {
}
