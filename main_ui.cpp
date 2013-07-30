#include "main_ui.h"


#include <sstream>
#include <boost/scope_exit.hpp>

#include <CommCtrl.h>
#include <Shlobj.h>
#include <Shellapi.h>

void main_ui::update_stat_display() {
    if ( !_analizer || _analizer->count_encounters() < 1 ) {
        for ( size_t i = 0; i < _stat_display.size(); ++i ) {
            const auto& row = _stat_display[i];
            row.name->hide();
            row.value->hide();
            row.bar->hide();
            row.perc->hide();
        }
        return;
    }

    auto encounter_id = _analizer->count_encounters() - 1;
    auto& encounter = _analizer->from(encounter_id);

    if ( encounter.timestamp() <= _last_update ) {
        return;
    }

    _last_update = encounter.timestamp();

    auto player_records = encounter.select<combat_log_entry>( [=](const combat_log_entry& e_) {return e_; } )
        .where([=](const combat_log_entry& e_) {
            return e_.src == 0 && e_.src_minion == string_id(-1) && e_.ability != string_id(-1);
    }).commit < std::vector < combat_log_entry >> ( );

    if ( player_records.empty() ) {
        for ( size_t i = 0; i < _stat_display.size(); ++i ) {
            const auto& row = _stat_display[i];
            row.name->hide();
            row.value->hide();
            row.bar->hide();
            row.perc->hide();
        }
        return;
    }

    auto& first = player_records.front();
    auto& last = player_records.back();

    unsigned long long start_time = first.time_index.milseconds + first.time_index.seconds * 1000 + first.time_index.minutes * 1000 * 60 + first.time_index.hours * 1000 * 60 * 60;
    unsigned long long end_time = last.time_index.milseconds + last.time_index.seconds * 1000 + last.time_index.minutes * 1000 * 60 + last.time_index.hours * 1000 * 60 * 60;
    unsigned long long epleased = end_time - start_time;

    long long total_heal = 0;
    long long total_damage = 0;
    
    /*auto player_heal = select_from<combat_log_entry_ex>( [=, &total_heal](const combat_log_entry& e_) {
        combat_log_entry_ex ex
        { e_ };
        ex.hits = 1;
        ex.crits = ex.was_crit_effect;
        ex.misses = 0;
        total_heal += ex.effect_value;
        return ex;
    }, player_records )
        .where([=](const combat_log_entry& e_) {
            return e_.effect_action == ssc_ApplyEffect && e_.effect_type == ssc_Heal;
    }).group_by([=](const combat_log_entry_ex& lhs_, const combat_log_entry_ex& rhs_) {
        return lhs_.ability == rhs_.ability;
    }, [=](const combat_log_entry_ex& lhs_, const combat_log_entry_ex& rhs_) {
        combat_log_entry_ex res = lhs_;
        res.effect_value += rhs_.effect_value;
        res.effect_value2 += rhs_.effect_value2;
        res.hits += rhs_.hits;
        res.crits += rhs_.crits;
        res.misses += rhs_.misses;
        return res;
    }).order_by([=](const combat_log_entry_ex& lhs_, const combat_log_entry_ex& rhs_) {
        return lhs_.effect_value < rhs_.effect_value;
    }).commit < std::vector < combat_log_entry_ex >> ( );
*/
    auto player_damage = select_from<combat_log_entry_ex>( [=, &total_damage](const combat_log_entry& e_) {
        combat_log_entry_ex ex
        { e_ };
        ex.hits = 1;
        ex.crits = ex.was_crit_effect;
        ex.misses = ex.effect_value == 0;
        total_damage += ex.effect_value;
        return ex;
    }, player_records )
        .where([=](const combat_log_entry& e_) {
            return e_.effect_action == ssc_ApplyEffect && e_.effect_type == ssc_Damage;
    }).group_by([=](const combat_log_entry_ex& lhs_, const combat_log_entry_ex& rhs_) {
        return lhs_.ability == rhs_.ability;
    }, [=](const combat_log_entry_ex& lhs_, const combat_log_entry_ex& rhs_) {
        combat_log_entry_ex res = lhs_;
        res.effect_value += rhs_.effect_value;
        res.effect_value2 += rhs_.effect_value2;
        res.hits += rhs_.hits;
        res.crits += rhs_.crits;
        res.misses += rhs_.misses;
        return res;
    }).order_by([=](const combat_log_entry_ex& lhs_, const combat_log_entry_ex& rhs_) {
        return lhs_.effect_value > rhs_.effect_value;
    }).commit < std::vector < combat_log_entry_ex >> ( );

    auto rect = _wnd->client_area_rect();

    auto base_set = GetDialogBaseUnits();
    auto base_x = LOWORD(base_set);
    auto base_y = HIWORD(base_set);

    rect.top += MulDiv(34 + 39, base_y, 8);
    rect.left += 8;
    rect.right -= 8;
    rect.bottom -= 8;

    const auto name_space = MulDiv(60, base_x, 4);
    const auto value_space = MulDiv(30, base_x, 4);
    const auto perc_space = MulDiv(30, base_x, 4);
    const auto bar_space = rect.right - rect.left - name_space - value_space - perc_space;
    const auto line_with = MulDiv(14, base_y, 8);

    auto font = ( HFONT )::SendMessageW(_wnd->native_handle(), WM_GETFONT, 0, 0);

    while ( _stat_display.size() < player_damage.size() ) {
        auto y = rect.top + line_with * _stat_display.size();
        combat_stat_display disp;
        disp.name.reset(new window(0, L"static", L"", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, rect.left, y, name_space, line_with, _wnd->native_window_handle(), nullptr, _wnd->instance_handle()));
        disp.value.reset(new window(0, L"static", L"", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, rect.left + name_space, y, value_space, line_with, _wnd->native_window_handle(), nullptr, _wnd->instance_handle()));
        disp.bar.reset(new progress_bar(line_with + name_space + value_space + 16, y, bar_space - 64, line_with - 10, _wnd->native_window_handle(), _stat_display.size(), _wnd->instance_handle()));
        disp.perc.reset(new window(0, L"static", L"", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, rect.left + name_space + value_space + bar_space, y, perc_space, line_with, _wnd->native_window_handle(), nullptr, _wnd->instance_handle()));
        ::SendMessageW(disp.name->native_window_handle(), WM_SETFONT, (WPARAM)font, TRUE);
        ::SendMessageW(disp.value->native_window_handle(), WM_SETFONT, (WPARAM)font, TRUE);
        ::SendMessageW(disp.bar->native_window_handle(), WM_SETFONT, (WPARAM)font, TRUE);
        ::SendMessageW(disp.perc->native_window_handle(), WM_SETFONT, (WPARAM)font, TRUE);
        _stat_display.push_back(std::move(disp));
    }

    size_t i = 0;
    for (; i < player_damage.size(); ++i ) {
        const auto& row = player_damage[i];
        const auto& value = (*_string_map)[row.ability];

        auto& display = _stat_display[i];
        display.name->caption(value);
        display.name->normal();

        display.value->caption(std::to_wstring(row.effect_value));
        display.value->normal();

        display.bar->range(0, total_damage);
        display.bar->progress(row.effect_value);
        display.bar->normal();

        display.perc->caption(std::to_wstring(double( row.effect_value ) / total_damage * 100.0) + L"%");
        display.perc->normal();
    }

    for ( ; i < _stat_display.size(); ++i ) {
        const auto& row = _stat_display[i];
        row.name->hide();
        row.value->hide();
        row.bar->hide();
        row.perc->hide();
    }

    auto dps = ( double( total_damage ) / epleased ) * 1000.0;
    auto hps = ( double( total_heal ) / epleased ) * 1000.0;
    auto ep = epleased / 1000.0;
    auto dsp_diplay = GetDlgItem(_wnd->native_handle(), IDC_GLOBAL_STATS);

    auto dps_text = std::to_wstring(dps) + L" dps";
    auto hps_text = std::to_wstring(hps) + L" hps";
    auto damage_text = L"Damage: " + std::to_wstring(total_damage);
    auto heal_text = L"Healing: " + std::to_wstring(total_heal);
    auto dur_text = L"Duration: " + std::to_wstring(ep) + L" seconds";

    auto final_text = dps_text + L"\r\n"
                    + hps_text + L"\r\n"
                    + damage_text + L"\r\n"
                    + heal_text + L"\r\n"
                    + dur_text + L"\r\n";

    ::SetWindowTextW(dsp_diplay, final_text.c_str());
}

void main_ui::set_analizer(const set_analizer_event& e_) {
    _analizer = e_.anal;
    _string_map = e_.smap;
}

void main_ui::display_log_dir_select(display_log_dir_select_event) {
    display_dir_select();
}

void main_ui::on_event(const any& v_) {
#define do_handle_event(type,handler) handle_event<type>(v_,[=](const type& e_) { handler(e_); })
#define do_handle_event_e(event_) do_handle_event(event_##_event,event_)
    if ( !do_handle_event_e(display_log_dir_select)
        && !do_handle_event_e(set_analizer) ) {
        post_event(v_);
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
    auto edit = GetDlgItem(_wnd->native_handle(), IDC_URI_EDIT);
    std::wstring current_path;
    current_path.resize(::GetWindowTextLengthW(edit) + 1, L' ');
    ::GetWindowTextW(edit, const_cast<wchar_t*>( current_path.data() ), int( current_path.size() ));

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
            ::SetWindowTextW(edit, path_buffer);
        }
    }
}

LRESULT main_ui::os_callback_handler(dialog* window_, UINT uMsg, WPARAM wParam, LPARAM lParam) {
#if 0
    if ( uMsg == WM_COMMAND ) {
        auto id = LOWORD(wParam);
        auto code = HIWORD(wParam);
        switch ( id ) {
        case control_path_button:
            if ( code == BN_CLICKED ) {
                display_dir_select();
            }
            break;
        case control_start_solo_button:
            if ( code == BN_CLICKED ) {
                bool ok = true;
                invoke_event_handlers(start_tracking{&ok} );
                if ( ok ) {
                    _start_solo_button->disable();
                    _stop_button->enable();
                }
            }
            break;
        case control_stop_button:
            if ( code == BN_CLICKED ) {
                invoke_event_handlers(stop_tracking{});
                _stop_button->disable();
                _start_solo_button->enable();
            }
            break;
        case control_path_edit:
        default:
            break;
        }
    } else if ( uMsg == WM_SIZE ) {
        on_size(window_, wParam, lParam);
    } else if ( uMsg == WM_GETMINMAXINFO ) {
        /*auto info = reinterpret_cast<MINMAXINFO*>( lParam );
        info->ptMinTrackSize.x = min_width;
        info->ptMinTrackSize.y = min_height;
        // currently disable maxing
        info->ptMaxTrackSize.x = min_width;
        info->ptMaxTrackSize.y = min_height;*/
    } else if ( uMsg == WM_TIMER ) {
        //if ( wParam == _timer ) {
            update_stat_display();
        //}
    }
    return os_callback_handler_default(window_, uMsg, wParam, lParam);
#endif
    if ( WM_COMMAND == uMsg ) {
        auto id = LOWORD(wParam);
        auto code = HIWORD(wParam);
        switch ( id ) {
        case ID_HELP_ABOUT:
            {
                dialog about(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_ABOUT), _wnd->native_handle());
                about.callback([=](dialog* dlg_, UINT msg_, WPARAM w_param_, LPARAM l_param_) {
                    switch ( msg_ ) {
                    case WM_CLOSE:
                    case WM_DESTROY:
                        ::PostQuitMessage(0);
                        break;
                    case WM_NOTIFY:
                        auto from_info = reinterpret_cast<LPNMHDR>( l_param_ );
                        switch ( from_info->idFrom ) {
                        default:
                            break;
                        case IDC_ABOUT_HOME_LINK:
                            auto info = reinterpret_cast<PNMLINK>( l_param_ );
                            switch ( from_info->code ) {
                            case NM_CLICK:
                            case NM_RETURN:
                                ::ShellExecuteW(nullptr, L"open", info->item.szUrl, nullptr, nullptr, SW_SHOW);
                                break;
                            }
                            break;
                        }
                        break;
                    }
                    return FALSE;
                });
                auto version = GetDlgItem(about.native_handle(), IDC_ABOUT_VERSION);
                program_version version_num;
                invoke_event_handlers(get_program_version_event{ &version_num });
                std::wstringstream verstr;
                verstr << version_num.major << L"." << version_num.minor << L"." << version_num.patch << " Build " << version_num.build;
                ::SetWindowTextW(version, verstr.str().c_str());
                MSG msg{};
                while ( GetMessageW(&msg, about.native_handle(), 0, 0) ) {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
            }
            // display about dlg
            break;
        case ID_EDIT_OPTIONS:
            ::MessageBoxW(nullptr, L"No options yet, sorry!", L"Options", MB_OK | MB_ICONINFORMATION);
            // display options
            break;
        case ID_FILE_EXIT:
            ::PostQuitMessage(0);
            break;
        case IDC_BROWSE_BUTTON:
            if ( code == BN_CLICKED ) {
                display_dir_select();
            }
            break;
        case IDC_START_SOLO_BUTTON:
            if ( code == BN_CLICKED ) {
                bool ok = true;
                invoke_event_handlers(start_tracking{ &ok });
                if ( ok ) {
                    _timer = SetTimer(_wnd->native_window_handle(), _timer, 1000, nullptr);
                    ::EnableWindow(GetDlgItem(_wnd->native_handle(), IDC_BROWSE_BUTTON), FALSE);
                    ::EnableWindow(GetDlgItem(_wnd->native_handle(), IDC_START_SOLO_BUTTON), FALSE);
                    ::EnableWindow(GetDlgItem(_wnd->native_handle(), IDC_SYNC_TO_RAID_BUTTON), FALSE);
                    ::EnableWindow(GetDlgItem(_wnd->native_handle(), IDC_STOP_BUTTON), TRUE);
                }
            }
            break;
        case IDC_SYNC_TO_RAID_BUTTON:
            break;
        case IDC_STOP_BUTTON:
            if ( code == BN_CLICKED ) {
                invoke_event_handlers(stop_tracking{});
                KillTimer(_wnd->native_window_handle(), _timer);
                ::EnableWindow(GetDlgItem(_wnd->native_handle(), IDC_BROWSE_BUTTON), TRUE);
                ::EnableWindow(GetDlgItem(_wnd->native_handle(), IDC_START_SOLO_BUTTON), TRUE);
                ::EnableWindow(GetDlgItem(_wnd->native_handle(), IDC_SYNC_TO_RAID_BUTTON), TRUE);
                ::EnableWindow(GetDlgItem(_wnd->native_handle(), IDC_STOP_BUTTON), FALSE);
            }
            break;
        }
    } else if ( uMsg == WM_TIMER ) {
        //if ( wParam == _timer ) {
        update_stat_display();
        //}
        return TRUE;
    }
    return os_callback_handler_default(window_, uMsg, wParam, lParam);
}

main_ui::main_ui(const std::wstring& log_path_) {
    _wnd.reset(new dialog(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_MAIN_WINDOW), nullptr));

    _last_update = std::chrono::high_resolution_clock::now();

    _wnd->callback([=](dialog* window_, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        return os_callback_handler(window_, uMsg, wParam, lParam);
    });

    auto edit = GetDlgItem(_wnd->native_handle(), IDC_URI_EDIT);
    ::SetWindowTextW(edit, log_path_.c_str());
    ::EnableWindow(GetDlgItem(_wnd->native_handle(), IDC_STOP_BUTTON), FALSE);
    // if sync to raid is supported, we can enable this again
    ::EnableWindow(GetDlgItem(_wnd->native_handle(), IDC_SYNC_TO_RAID_BUTTON), FALSE);
}

main_ui::~main_ui() {
    KillTimer(_wnd->native_window_handle(), _timer);
}
