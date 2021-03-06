#pragma once

#include "window.h"

#include <vector>
#include <memory>

#include <Windows.h>
#include <CommCtrl.h>

class tab_set
    : public window {
    typedef std::vector<std::unique_ptr<window>>    tab_elements;
    typedef std::vector<tab_elements>               tab_sets;

    window_class                _draw_class;
    std::unique_ptr<window>     _draw_wnd;
    std::vector<std::wstring>   _tab_names;
    tab_sets                    _tabs;
    size_t                      _active_tab;

public:
    tab_set(int x_,int y_,int width_,int height_,HWND parent_,unsigned id_,HINSTANCE instance_)
        : window(0
                , WC_TABCONTROLW
                , L""
                , WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE
                , x_
                , y_
                , width_
                , height_
                , parent_
                , (HMENU)id_
                , instance_)
        , _active_tab(size_t(-1)) {
        _draw_class.name(L"tab_wnd_base");
        _draw_class.register_class();
        _draw_wnd.reset(new window(WS_EX_CONTROLPARENT, _draw_class, L"", WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, width_, height_ / 2, native_window_handle(), nullptr, _draw_class.source_instance()));
    }
    //~tab_set() {}

    template<typename U,typename V>
    size_t add_tab(U name_,V elements_) {
        _tab_names.push_back(std::forward<U>( name_ ));
        _tabs.push_back(std::forward<V>( elements_ ));
        auto index = _tab_names.size() - 1;

        TCITEM tie{};
        tie.mask = TCIF_TEXT;
        tie.pszText = const_cast<wchar_t*>(_tab_names[index].c_str());
        TabCtrl_InsertItem(native_window_handle(), index, &tie);

        if ( index == 0 ) {
            auto wr = get_client_area_rect();
            get_client_rect_for_window_rect(wr);
            _draw_wnd->move(wr.left, wr.top,wr.right - wr.left, wr.bottom - wr.top);
        }

        return index;
    }

    void switch_tab(size_t tab_id_, bool force_ = false) {
        if ( _active_tab == tab_id_ && !force_ ) {
            return;
        }

        TabCtrl_SetCurSel(native_window_handle(), tab_id_);

        if ( _active_tab < _tabs.size() ) {
            auto& tabs = _tabs[_active_tab];

            for ( auto& element : tabs ) {
                element->hide();
            }
        }

        _active_tab = tab_id_;

        if ( _active_tab < _tabs.size() ) {
            auto& tabs = _tabs[_active_tab];

            for ( auto& element : tabs ) {
                element->normal();
            }
        }
    }

    size_t active_tab() const {
        return _active_tab;
    }

    const std::vector<std::unique_ptr<window>>& get_tab_elements(size_t tab_id_) const {
        return _tabs[tab_id_];
    }

    // if you add elements, you need to call switch_tab(tab_id_,true);
    std::vector<std::unique_ptr<window>>& get_tab_elements(size_t tab_id_) {
        return _tabs[tab_id_];
    }

    size_t get_tab_element_count() const {
        return _tabs.size();
    }

    void handle_event(const NMHDR& event_) {
        if ( event_.hwndFrom == native_window_handle() ) {
            switch ( event_.code ) {
            case TCN_SELCHANGE:
                switch_tab(TabCtrl_GetCurSel(event_.hwndFrom));
                break;
            }
        }
    }

    void resize(RECT rect_) {
        move(rect_.left, rect_.top,rect_.right - rect_.left, rect_.bottom - rect_.top);
    }

    void get_window_rect_for_client_rect(RECT& rect_) {
        TabCtrl_AdjustRect(native_window_handle(), TRUE, &rect_);
    }

    void get_client_rect_for_window_rect(RECT& rect_) {
        TabCtrl_AdjustRect(native_window_handle(), FALSE, &rect_);
    }

    HWND get_draw_area_window_handle() {
        return _draw_wnd->native_window_handle();
    }
};