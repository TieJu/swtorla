#pragma once

#include <future>

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

class ui_element_manager {
    string_id_lookup            _slookup;
    std::vector<ui_data_row>    _rows;
    HWND                        _parent;
    HINSTANCE                   _instance;

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
            }
        }
    }

    void info_text(const std::wstring& str_) {
        auto dsp_diplay = ::GetDlgItem(_parent, IDC_GLOBAL_STATS);
        ::SetWindowTextW(dsp_diplay, str_.c_str());
    }
};
struct data_display_mode {
    size_t                                          _encounter;
    std::chrono::high_resolution_clock::time_point  _last_update;

    data_display_mode() : _encounter(size_t(-1)) {}
    virtual void update_display(combat_analizer& analizer_, ui_element_manager& ui_element_manager_) = 0;
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
    virtual void update_display(combat_analizer& analizer_, ui_element_manager& ui_element_manager_) {
        if ( analizer_.count_encounters() < _encounter ) {
            ui_element_manager_.show_only_num_rows(0);
            return;
        }

        auto& encounter = analizer_.from(_encounter);

        if ( encounter.timestamp() <= _last_update ) {
            return;
        }

        _last_update = encounter.timestamp();

        auto player_records = encounter.select<combat_log_entry>( [=](const combat_log_entry& e_) {return e_; } )
            .where([=](const combat_log_entry& e_) {
                return e_.src == _entity_name && e_.src_minion == _minion_name;
        }).commit < std::vector < combat_log_entry >> ( );

        if ( player_records.empty() ) {
            ui_element_manager_.show_only_num_rows(0);
            return;
        }

        auto& first = player_records.front();
        auto& last = player_records.back();

        unsigned long long start_time = first.time_index.milseconds + first.time_index.seconds * 1000 + first.time_index.minutes * 1000 * 60 + first.time_index.hours * 1000 * 60 * 60;
        unsigned long long end_time = last.time_index.milseconds + last.time_index.seconds * 1000 + last.time_index.minutes * 1000 * 60 + last.time_index.hours * 1000 * 60 * 60;
        unsigned long long epleased = end_time - start_time;

        long long total_heal = 0;
        long long total_damage = 0;

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
                return e_.effect_action == ssc_ApplyEffect && e_.effect_type == ssc_Damage && e_.ability != string_id(-1);
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

        while ( ui_element_manager_.rows() < player_damage.size() ) {
            ui_element_manager_.new_data_row();
        }

        for ( size_t i = 0; i < player_damage.size(); ++i ) {
            auto& display_row = ui_element_manager_.row(i);
            const auto& dmg_row = player_damage[i];

            display_row.name(dmg_row.ability);
            display_row.value_max(total_damage);
            display_row.value(dmg_row.effect_value);
        }

        ui_element_manager_.show_only_num_rows(player_damage.size());

        auto dps = ( double( total_damage ) / epleased ) * 1000.0;
        auto hps = ( double( total_heal ) / epleased ) * 1000.0;
        auto ep = epleased / 1000.0;

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

        ui_element_manager_.info_text(final_text);
    }
};
struct data_display_entity_healing_done : public data_display_entity_base {};
struct data_display_entity_dmg_recived : public data_display_entity_base {};
struct data_display_entity_healing_recived : public data_display_entity_base {};

struct data_display_entity_skill_base : public data_display_mode {};

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