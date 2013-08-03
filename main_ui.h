#pragma once

#include <future>
#include <sstream>

#include "ui_base.h"
#include "window.h"
#include "dialog.h"
#include "progress_bar.h"
#include "tab_set.h"
#include "combat_analizer.h"

struct set_analizer_event {
    combat_analizer*            anal;
    string_to_id_string_map*    smap;
    character_list*             names;
};

struct string_id_lookup {
    string_to_id_string_map*    smap;
    character_list*             names;

    bool is_name(string_id id_) const {
        return id_ < names->size();
    }

    const std::wstring& operator()(string_id id_) const {
        if ( is_name(id_) ) {
            return ( *names )[id_];
        } else {
            return ( *smap )[id_];
        }
    }
};

#define SKILL_BASE_ID 2000

class ui_data_row {
    string_id_lookup                    _slookup;
    std::unique_ptr<window>             _name;
    std::unique_ptr<window>             _value;
    std::unique_ptr<window>             _perc;
    std::unique_ptr<progress_bar>       _bar;
    string_id                           _skill_name;
    unsigned long long                  _skill_value;
    unsigned long long                  _max_value;
    std::function<void ( string_id )>   _callback;

    void setup(size_t ui_id_, HWND parent_, HINSTANCE instance_) {
        auto font = ( HFONT )::SendMessageW(parent_, WM_GETFONT, 0, 0);

        _name.reset(new window(0, WC_LINK, L"", WS_CHILD, 0, 0, 0, 0, parent_, (HMENU)ui_id_, instance_));
        _value.reset(new window(0, WC_LINK, L"", WS_CHILD, 0, 0, 0, 0, parent_, (HMENU)ui_id_, instance_));
        _bar.reset(new progress_bar(0, 0, 0, 0, parent_, ui_id_, instance_));
        _perc.reset(new window(0, WC_LINK, L"", WS_CHILD, 0, 0, 0, 0, parent_, (HMENU)ui_id_, instance_));
        _bar->hide();
        ::SendMessageW(_name->native_window_handle(), WM_SETFONT, (WPARAM)font, TRUE);
        ::SendMessageW(_value->native_window_handle(), WM_SETFONT, (WPARAM)font, TRUE);
        ::SendMessageW(_bar->native_window_handle(), WM_SETFONT, (WPARAM)font, TRUE);
        ::SendMessageW(_perc->native_window_handle(), WM_SETFONT, (WPARAM)font, TRUE);
    }

public:
    ui_data_row(string_id_lookup slooup_, size_t ui_id_, HWND parent_, HINSTANCE instance_) : _slookup(slooup_) {
        setup(ui_id_, parent_, instance_);
    }

    ui_data_row(ui_data_row && o_)
        : _slookup(std::move(o_._slookup))
        , _name(std::move(o_._name))
        , _value(std::move(o_._value))
        , _perc(std::move(o_._perc))
        , _bar(std::move(o_._bar))
        , _skill_name(std::move(o_._skill_name))
        , _skill_value(std::move(o_._skill_value))
        , _max_value(std::move(o_._max_value))
        , _callback(std::move(o_._callback)) {
    }

    ui_data_row& operator=( ui_data_row && o_ ) {
        _slookup = std::move(o_._slookup);
        _name = std::move(o_._name);
        _value = std::move(o_._value);
        _perc = std::move(o_._perc);
        _bar = std::move(o_._bar);
        _skill_name = std::move(o_._skill_name);
        _skill_value = std::move(o_._skill_value);
        _max_value = std::move(o_._max_value);
        _callback = std::move(o_._callback);
        return *this;
    }

    void name(string_id name_) {
        _name->caption(L"<a>" + _slookup(name_) + L"</a>");
        _skill_name = name_;
    }

    void value(unsigned long long value_) {
        _skill_value = value_;
        _value->caption(std::to_wstring(_skill_value));
        _bar->progress(_skill_value);
        _perc->caption(std::to_wstring(double( _skill_value ) / _max_value * 100.0) + L"%");
    }

    void value_max(unsigned long long max_) {
        _max_value = max_;
        _bar->range(0, _max_value);
        _perc->caption(std::to_wstring(double( _skill_value ) / _max_value * 100.0) + L"%");
    }

    template<typename T>
    void callback(T t_) {
        _callback = t_;
    }

    void disable() {
        _name->enable(false);
        _name->hide();

        _value->enable(false);
        _value->hide();

        _perc->enable(false);
        _perc->hide();

        _bar->enable(false);
        _bar->hide();
    }

    void enable() {
        _name->enable(true);
        _name->normal();

        _value->enable(true);
        _value->normal();

        _perc->enable(true);
        _perc->normal();

        _bar->enable(true);
        _bar->normal();
    }

    void move(const RECT& rect_) {
        auto base_set = GetDialogBaseUnits();
        auto base_x = LOWORD(base_set);
        auto base_y = HIWORD(base_set);

        const auto name_space = MulDiv(60, base_x, 4);
        const auto value_space = MulDiv(30, base_x, 4);
        const auto perc_space = MulDiv(30, base_x, 4);
        const auto bar_space = rect_.right - rect_.left - name_space - value_space - perc_space;
        const auto line_h = MulDiv(14, base_y, 8);

        _name->move(rect_.left, rect_.top + 4, name_space, line_h);
        _value->move(rect_.left + name_space, rect_.top + 4, value_space, line_h);
        _bar->move(rect_.left + name_space + value_space, rect_.top, bar_space - 16, line_h - 10);
        _perc->move(rect_.left + name_space + value_space + bar_space, rect_.top + 4, perc_space, line_h);
    }

    void on_click() {
        if ( _callback ) {
            _callback(_skill_name);
        }
    }
};

inline std::wstring get_info_text_default_links(bool enable_solo_, bool enable_raid_, bool enable_stop_) {
    const wchar_t* solo_disabled = L"Start Solo";
    const wchar_t* solo_enabled = L"<a id=\"parse_solo\">Start Solo</a>";
    const wchar_t* raid_disabled = L"Sync to Raid";
    const wchar_t* raid_enabled = L"<a id=\"parse_raid\">Sync to Raid</a>";
    const wchar_t* stop_disabled = L"Stop";
    const wchar_t* stop_enabled = L"<a id=\"parse_stop\">Stop</a>";

    std::wstringstream buf;

    buf << ( enable_solo_ ? solo_enabled : solo_disabled );
    buf << L" ";
    buf << ( enable_raid_ ? raid_enabled : raid_disabled );
    buf << L" ";
    buf << ( enable_stop_ ? stop_enabled : stop_disabled );
    return buf.str();
}

class ui_element_manager {
public:
    typedef std::function<void ()> info_link_callback;

private:
    string_id_lookup                                    _slookup;
    std::vector<ui_data_row>                            _rows;
    std::unordered_map<std::wstring, info_link_callback>_info_callbacks;
    HWND                                                _parent;
    HINSTANCE                                           _instance;
    bool                                                _enable_solo;
    bool                                                _enable_raid;

public:
    size_t new_data_row() {
        RECT r;
        ::GetClientRect(_parent, &r);

        auto base_set = GetDialogBaseUnits();
        auto base_x = LOWORD(base_set);
        auto base_y = HIWORD(base_set);

        const auto line_h = MulDiv(14, base_y, 8);
        const auto id = _rows.size();

        r.top += MulDiv(34 + 39, base_y, 8) + ( id * line_h );
        r.left += 8;
        r.right -= 8;
        r.bottom -= 8;

        _rows.emplace_back(_slookup, SKILL_BASE_ID + id, _parent, _instance);
        _rows[id].move(r);
        return id;
    }

    ui_data_row& row(size_t index_) {
        return _rows[index_];
    }

    size_t rows() const {
        return _rows.size();
    }

    void show_only_num_rows(size_t num_rows_) {
        for ( size_t i = 0; i < num_rows_; ++i ) {
            _rows[i].enable();
        }
        for ( size_t i = num_rows_; i < _rows.size(); ++i ) {
            _rows[i].disable();
        }
    }

    string_id_lookup& lookup_info() {
        return _slookup;
    }

    void app_instance(HINSTANCE instance_) {
        _instance = instance_;
    }

    void parent(HWND parent_) {
        _parent = parent_;

        on_resize();
    }

    void on_resize() {
    }

    void on_event(UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if ( WM_NOTIFY == uMsg ) {
            auto notify_info = reinterpret_cast<const NMHDR*>( lParam );
            if ( notify_info->idFrom >= SKILL_BASE_ID ) {
                auto index = ( notify_info->idFrom - SKILL_BASE_ID );
                if ( index < _rows.size() ) {
                    auto& ref = _rows[index];
                    switch ( notify_info->code ) {
                    case NM_CLICK:
                    case NM_RETURN:
                        ref.on_click();
                        break;
                    }
                }
            } else if ( notify_info->idFrom == IDC_GLOBAL_STATS ) {
                auto link_info = reinterpret_cast<const NMLINK*>( lParam );
                auto at = _info_callbacks.find(link_info->item.szID);
                if ( at == end(_info_callbacks) ) {
                    at = _info_callbacks.find(link_info->item.szUrl);
                    if ( at == end(_info_callbacks) ) {
                        return;
                    }
                }

                at->second();
            }
        }
    }

    void info_text(const std::wstring& str_) {
        auto dsp_diplay = ::GetDlgItem(_parent, IDC_GLOBAL_STATS);
        ::SetWindowTextW(dsp_diplay, ( get_info_text_default_links(_enable_solo, _enable_raid, !_enable_solo && !_enable_raid) + L"\r\n\r\n" + str_ ).c_str());
    }

    void clear() {
        show_only_num_rows(0);
        info_text(L"");
    }

    void enable_solo(bool enable_ = true) {
        _enable_solo = enable_;
    }

    void enable_raid(bool enable_ = true) {
        _enable_raid = enable_;
    }

    void enable_stop(bool enable_ = true) {
        enable_solo(!enable_);
        enable_raid(!enable_);
    }

    void info_callback(const std::wstring& name_, info_link_callback clb) {
        _info_callbacks[name_] = clb;
    }
};
struct data_display_mode {
    typedef std::function<void (data_display_mode*)> change_display_mode_callback;
    size_t                                          _encounter;
    std::chrono::high_resolution_clock::time_point  _last_update;

    data_display_mode() : _encounter(size_t(-1)) {}
    virtual void update_display(combat_analizer& analizer_, ui_element_manager& ui_element_manager_, change_display_mode_callback clb) = 0;
};

struct data_display_raid_base : public data_display_mode {};
struct data_display_raid_dmg_done : public data_display_raid_base {};
struct data_display_raid_healing_done : public data_display_raid_base {};
struct data_display_raid_dmg_recived : public data_display_raid_base {};
struct data_display_raid_healing_recived : public data_display_raid_base {};

struct data_display_entity_base : public data_display_mode {
    string_id       _entity_name;
    string_id       _minion_name;
};

struct data_display_entity_dmg_done : public data_display_entity_base {
    virtual void update_display(combat_analizer& analizer_, ui_element_manager& ui_element_manager_, change_display_mode_callback clb) override;
};
struct data_display_entity_healing_done : public data_display_entity_base {};
struct data_display_entity_dmg_recived : public data_display_entity_base {};
struct data_display_entity_healing_recived : public data_display_entity_base {};

struct data_display_entity_skill_base : public data_display_entity_base {
    string_id       _ability_name;
    virtual void update_display(combat_analizer& analizer_, ui_element_manager& ui_element_manager_, change_display_mode_callback clb) override;
};

class main_ui
: public ui_base {
    ui_element_manager                              _ui_elements;
    std::unique_ptr<data_display_mode>              _data_display;
    std::unique_ptr<dialog>                         _wnd;
    std::future<bool>                               _update_state;
    UINT_PTR                                        _timer;
    combat_analizer*                                _analizer;

    HWND post_param() {
        return _wnd->native_window_handle();
    }


    bool show_options_dlg();
    void gather_options_state(dialog* dlg_, program_config& cfg_);
    INT_PTR options_dlg_handler(dialog* dlg_, UINT msg_, WPARAM w_param_, LPARAM l_param_);
    void show_about_dlg();
    INT_PTR about_dlg_handler(dialog* dlg_, UINT msg_, WPARAM w_param_, LPARAM l_param_);

    void update_stat_display();
    void set_analizer(const set_analizer_event& e_);
    void display_log_dir_select(display_log_dir_select_event);
    virtual void on_event(const any& v_);
public:
    virtual bool handle_os_events();

private:
#if defined(USE_CUSTOME_SELECTOR)
    static void ScreenToClientX(HWND hWnd, LPRECT lpRect);
    static void MoveWindowX(HWND hWnd, RECT& rect, BOOL bRepaint);
    static void SizeBrowseDialog(HWND hWnd);
#endif
    static int CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
    void display_dir_select(HWND edit_);

    LRESULT os_callback_handler(dialog* window_, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    main_ui(const std::wstring& log_path_);
    virtual ~main_ui();
};