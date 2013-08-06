#include "data_display_mode.h"

void data_display_entity_dmg_done::update_display(combat_analizer& analizer_, ui_element_manager<main_ui>& ui_element_manager_, change_display_mode_callback clb) {
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
            return e_.src == *_entity_name && e_.src_minion == _minion_name;
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

        display_row.callback([=](string_id skill_name_) {
            auto display = new data_display_entity_skill_dmg_done;
            display->_ability_name = skill_name_;
            display->_encounter = _encounter;
            display->_entity_name = _entity_name;
            display->_minion_name = _minion_name;
            clb(display);
        });

        auto& view = ui_element_manager_.list_view_element(dmg_row.ability);
        view.value(lvc_value, std::to_wstring(dmg_row.effect_value));
        view.value(lvc_perc, std::to_wstring(double( dmg_row.effect_value ) / total_damage * 100.0));
        view.value(lvc_applys, std::to_wstring(dmg_row.hits));
        view.value(lvc_misses, std::to_wstring(dmg_row.misses));
        view.value(lvc_crits, std::to_wstring(dmg_row.crits));
        view.value(lvc_misses_perc, std::to_wstring(double(dmg_row.misses) / dmg_row.hits * 100.0));
        view.value(lvc_crit_perc, std::to_wstring(double(dmg_row.crits) / dmg_row.hits * 100.0));
        /*
        lvc_absorbs,
        lvc_absorbs_perc,
        lvc_absorbs_value,
        lvc_absorbs_value_perc,*/
    }

    ui_element_manager_.show_only_num_rows(player_damage.size());
    ui_element_manager_.update_list_view();


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

    ui_element_manager_.enable_stop(true);
    ui_element_manager_.info_text(final_text);
}

void data_display_entity_skill_dmg_done::update_display(combat_analizer& analizer_, ui_element_manager<main_ui>& ui_element_manager_, change_display_mode_callback clb) {
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
            return e_.src == *_entity_name && e_.src_minion == _minion_name && e_.ability == _ability_name;
    }).commit < std::vector < combat_log_entry >> ( );

    if ( player_records.empty() ) {
        ui_element_manager_.show_only_num_rows(0);
        return;
    }

    auto& first = player_records.front();
    auto& last = player_records.back();

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
        .group_by([=](const combat_log_entry_ex& lhs_, const combat_log_entry_ex& rhs_) {
            return lhs_.dst == rhs_.dst && lhs_.dst_minion == rhs_.dst_minion && lhs_.dst_id == rhs_.dst_id;
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

        display_row.name(dmg_row.dst);
        display_row.value_max(total_damage);
        display_row.value(dmg_row.effect_value);
        display_row.callback([](string_id) {});
    }

    ui_element_manager_.show_only_num_rows(player_damage.size());

    ui_element_manager_.enable_stop(true);
    ui_element_manager_.info_text(L"<a id=\"back_to_entity_view\">Back</a>");
    ui_element_manager_.info_callback(L"back_to_entity_view", [=]() {
        auto display = new data_display_entity_dmg_done;
        display->_encounter = _encounter;
        display->_entity_name = _entity_name;
        display->_minion_name = _minion_name;
        clb(display);
    });
}