#include "main_ui.h"

void data_display_entity_dmg_done::update_display(combat_analizer& analizer_, ui_element_manager<main_ui>& ui_element_manager_, main_ui& ui_) {
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
            return e_.effect_action == ssc_ApplyEffect && e_.effect_type == ssc_Damage && e_.ability != string_id(0);
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

        display_row.callback([=, &ui_](string_id skill_name_) {
            auto display = new data_display_entity_skill_dmg_done;
            display->_ability_name = skill_name_;
            display->_encounter = _encounter;
            display->_entity_name = _entity_name;
            display->_minion_name = _minion_name;
            ui_.change_display_mode_with_history(display);
        });
    }

    ui_element_manager_.show_only_num_rows(player_damage.size());

    auto epleased = std::chrono::duration_cast<std::chrono::milliseconds>(encounter.get_combat_length());
    auto dps = ( double( total_damage ) / epleased.count() ) * 1000.0;
    auto hps = ( double( total_heal ) / epleased.count() ) * 1000.0;
    auto ep = epleased.count() / 1000.0;

    auto dps_text = std::to_wstring(dps) + L" dps";
    auto hps_text = std::to_wstring(hps) + L" hps";
    auto damage_text = L"Damage: " + std::to_wstring(total_damage);
    auto heal_text = L"Healing: " + std::to_wstring(total_heal);
    auto dur_text = L"Duration: " + std::to_wstring(ep) + L" seconds";

    auto final_text = L"<a id=\"back_to_entity_view\">Back</a>"
        L"\r\nDamage done by " + ui_element_manager_.lookup_info()( _entity_name ) + L"\r\n"
        + dps_text + L"\r\n"
        + hps_text + L"\r\n"
        + damage_text + L"\r\n"
        + heal_text + L"\r\n"
        + dur_text + L"\r\n"
        ;

    ui_element_manager_.enable_stop(true);
    ui_element_manager_.info_text(final_text);
    ui_element_manager_.info_callback(L"back_to_entity_view", [=, &ui_]() {
        ui_.data_display_mode_go_history_back();
    });
}

void data_display_entity_healing_done::update_display(combat_analizer& analizer_, ui_element_manager<main_ui>& ui_element_manager_, main_ui& ui_) {
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
            return e_.effect_action == ssc_ApplyEffect && e_.effect_type == ssc_Heal && e_.ability != string_id(0);
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
        const auto& hps_row = player_damage[i];

        display_row.name(hps_row.ability);
        display_row.value_max(total_damage);
        display_row.value(hps_row.effect_value);

        display_row.callback([=, &ui_](string_id skill_name_) {
            auto display = new data_display_entity_skill_healing_done;
            display->_ability_name = skill_name_;
            display->_encounter = _encounter;
            display->_entity_name = _entity_name;
            display->_minion_name = _minion_name;
            ui_.change_display_mode_with_history(display);
        });
    }

    ui_element_manager_.show_only_num_rows(player_damage.size());

    auto epleased = std::chrono::duration_cast<std::chrono::milliseconds>( encounter.get_combat_length() );
    auto dps = ( double( total_damage ) / epleased.count() ) * 1000.0;
    auto hps = ( double( total_heal ) / epleased.count() ) * 1000.0;
    auto ep = epleased.count() / 1000.0;

    auto dps_text = std::to_wstring(dps) + L" dps";
    auto hps_text = std::to_wstring(hps) + L" hps";
    auto damage_text = L"Damage: " + std::to_wstring(total_damage);
    auto heal_text = L"Healing: " + std::to_wstring(total_heal);
    auto dur_text = L"Duration: " + std::to_wstring(ep) + L" seconds";

    auto final_text = L"<a id=\"back_to_entity_view\">Back</a>"
        L"\r\nHealing done by " + ui_element_manager_.lookup_info()( _entity_name ) + L"\r\n"
        + dps_text + L"\r\n"
        + hps_text + L"\r\n"
        + damage_text + L"\r\n"
        + heal_text + L"\r\n"
        + dur_text + L"\r\n";

    ui_element_manager_.enable_stop(true);
    ui_element_manager_.info_text(final_text);
    ui_element_manager_.info_callback(L"back_to_entity_view", [=, &ui_]() {
        ui_.data_display_mode_go_history_back();
    });
}

void data_display_entity_dmg_recived::update_display(combat_analizer& analizer_, ui_element_manager<main_ui>& ui_element_manager_, main_ui& ui_) {
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
            return e_.dst == _entity_name && e_.dst_minion == _minion_name;
    }).commit < std::vector < combat_log_entry >> ( );

    if ( player_records.empty() ) {
        ui_element_manager_.show_only_num_rows(0);
        return;
    }

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
            return e_.effect_action == ssc_ApplyEffect && e_.effect_type == ssc_Damage && e_.ability != string_id(0);
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

        display_row.callback([=, &ui_](string_id skill_name_) {
            auto display = new data_display_entity_skill_dmg_recived;
            display->_ability_name = skill_name_;
            display->_encounter = _encounter;
            display->_entity_name = _entity_name;
            display->_minion_name = _minion_name;
            ui_.change_display_mode_with_history(display);
        });
    }

    ui_element_manager_.show_only_num_rows(player_damage.size());

    auto epleased = std::chrono::duration_cast<std::chrono::milliseconds>( encounter.get_combat_length() );
    auto dps = ( double( total_damage ) / epleased.count() ) * 1000.0;
    auto hps = ( double( total_heal ) / epleased.count() ) * 1000.0;
    auto ep = epleased.count() / 1000.0;

    auto dps_text = std::to_wstring(dps) + L" dps";
    auto hps_text = std::to_wstring(hps) + L" hps";
    auto damage_text = L"Damage: " + std::to_wstring(total_damage);
    auto heal_text = L"Healing: " + std::to_wstring(total_heal);
    auto dur_text = L"Duration: " + std::to_wstring(ep) + L" seconds";

    auto final_text = L"<a id=\"back_to_entity_view\">Back</a>"
        L"\r\nDamage recived by " + ui_element_manager_.lookup_info()( _entity_name ) + L"\r\n"
        + dps_text + L"\r\n"
        + hps_text + L"\r\n"
        + damage_text + L"\r\n"
        + heal_text + L"\r\n"
        + dur_text + L"\r\n";

    ui_element_manager_.enable_stop(true);
    ui_element_manager_.info_text(final_text);
    ui_element_manager_.info_callback(L"back_to_entity_view", [=, &ui_]() {
        ui_.data_display_mode_go_history_back();
    });
}

void data_display_entity_healing_recived::update_display(combat_analizer& analizer_, ui_element_manager<main_ui>& ui_element_manager_, main_ui& ui_) {
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
            return e_.dst == _entity_name && e_.dst_minion == _minion_name;
    }).commit < std::vector < combat_log_entry >> ( );

    if ( player_records.empty() ) {
        ui_element_manager_.show_only_num_rows(0);
        return;
    }

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
            return e_.effect_action == ssc_ApplyEffect && e_.effect_type == ssc_Heal && e_.ability != string_id(0);
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
        const auto& hps_row = player_damage[i];

        display_row.name(hps_row.ability);
        display_row.value_max(total_damage);
        display_row.value(hps_row.effect_value);

        display_row.callback([=, &ui_](string_id skill_name_) {
            auto display = new data_display_entity_skill_healing_recived;
            display->_ability_name = skill_name_;
            display->_encounter = _encounter;
            display->_entity_name = _entity_name;
            display->_minion_name = _minion_name;
            ui_.change_display_mode_with_history(display);
        });
    }

    ui_element_manager_.show_only_num_rows(player_damage.size());

    auto epleased = std::chrono::duration_cast<std::chrono::milliseconds>( encounter.get_combat_length() );
    auto dps = ( double( total_damage ) / epleased.count() ) * 1000.0;
    auto hps = ( double( total_heal ) / epleased.count() ) * 1000.0;
    auto ep = epleased.count() / 1000.0;

    auto dps_text = std::to_wstring(dps) + L" dps";
    auto hps_text = std::to_wstring(hps) + L" hps";
    auto damage_text = L"Damage: " + std::to_wstring(total_damage);
    auto heal_text = L"Healing: " + std::to_wstring(total_heal);
    auto dur_text = L"Duration: " + std::to_wstring(ep) + L" seconds";

    auto final_text = L"<a id=\"back_to_entity_view\">Back</a>"
        L"\r\nHealing recived by " + ui_element_manager_.lookup_info()( _entity_name ) + L"\r\n"
        + dps_text + L"\r\n"
        + hps_text + L"\r\n"
        + damage_text + L"\r\n"
        + heal_text + L"\r\n"
        + dur_text + L"\r\n";

    ui_element_manager_.enable_stop(true);
    ui_element_manager_.info_text(final_text);
    ui_element_manager_.info_callback(L"back_to_entity_view", [=, &ui_]() {
        ui_.data_display_mode_go_history_back();
    });
}

void data_display_entity_skill_dmg_done::update_display(combat_analizer& analizer_, ui_element_manager<main_ui>& ui_element_manager_, main_ui& ui_) {
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
            return e_.src == _entity_name && e_.src_minion == _minion_name && e_.ability == _ability_name;
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
        display_row.callback([=, &ui_](string_id) {
            auto display = new data_display_entity_dmg_recived;
            display->_encounter = _encounter;
            display->_entity_name = dmg_row.dst;
            display->_minion_name = dmg_row.dst_minion;
            ui_.change_display_mode_with_history(display);
        });
    }

    ui_element_manager_.show_only_num_rows(player_damage.size());

    ui_element_manager_.enable_stop(true);
    ui_element_manager_.info_text(L"<a id=\"back_to_entity_view\">Back</a>"
                                  L"\r\nDamage done with " + ui_element_manager_.lookup_info()( _ability_name ) + L" by " + ui_element_manager_.lookup_info()( _entity_name ) + L"\r\n");
    ui_element_manager_.info_callback(L"back_to_entity_view", [=, &ui_]() {
        ui_.data_display_mode_go_history_back();
    });
}

void data_display_entity_skill_healing_done::update_display(combat_analizer& analizer_, ui_element_manager<main_ui>& ui_element_manager_, main_ui& ui_) {
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
            return e_.src == _entity_name && e_.src_minion == _minion_name && e_.ability == _ability_name;
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
        const auto& heal_row = player_damage[i];

        display_row.name(heal_row.dst);
        display_row.value_max(total_damage);
        display_row.value(heal_row.effect_value);
        display_row.callback([=, &ui_](string_id) {
            auto display = new data_display_entity_healing_recived;
            display->_encounter = _encounter;
            display->_entity_name = heal_row.dst;
            display->_minion_name = heal_row.dst_minion;
            ui_.change_display_mode_with_history(display);
        });
    }

    ui_element_manager_.show_only_num_rows(player_damage.size());

    ui_element_manager_.enable_stop(true);
    ui_element_manager_.info_text(L"<a id=\"back_to_entity_view\">Back</a>"
                                  L"\r\nHealing done with " + ui_element_manager_.lookup_info()( _ability_name ) + L" by " + ui_element_manager_.lookup_info()( _entity_name ) + L"\r\n");
    ui_element_manager_.info_callback(L"back_to_entity_view", [=, &ui_]() {
        ui_.data_display_mode_go_history_back();
    });
}

void data_display_entity_skill_dmg_recived::update_display(combat_analizer& analizer_, ui_element_manager<main_ui>& ui_element_manager_, main_ui& ui_) {
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
            return e_.dst == _entity_name && e_.dst_minion == _minion_name && e_.ability == _ability_name;
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
            return lhs_.src == rhs_.src && lhs_.src_minion == rhs_.src_minion && lhs_.src_id == rhs_.src_id;
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

        display_row.name(dmg_row.src);
        display_row.value_max(total_damage);
        display_row.value(dmg_row.effect_value);
        display_row.callback([=, &ui_](string_id) {
            auto display = new data_display_entity_dmg_done;
            display->_encounter = _encounter;
            display->_entity_name = dmg_row.src;
            display->_minion_name = dmg_row.src_minion;
            ui_.change_display_mode_with_history(display);
        });
    }

    ui_element_manager_.show_only_num_rows(player_damage.size());

    ui_element_manager_.enable_stop(true);
    ui_element_manager_.info_text(L"<a id=\"back_to_entity_view\">Back</a>"
                                  L"\r\nDamage recived through " + ui_element_manager_.lookup_info()( _ability_name ) + L" by " + ui_element_manager_.lookup_info()( _entity_name ) + L"\r\n");
    ui_element_manager_.info_callback(L"back_to_entity_view", [=, &ui_]() {
        ui_.data_display_mode_go_history_back();
    });
}

void data_display_entity_skill_healing_recived::update_display(combat_analizer& analizer_, ui_element_manager<main_ui>& ui_element_manager_, main_ui& ui_) {
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
            return e_.dst == _entity_name && e_.dst_minion == _minion_name && e_.ability == _ability_name;
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
            return lhs_.src == rhs_.src && lhs_.src_minion == rhs_.src_minion && lhs_.src_id == rhs_.src_id;
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
        const auto& heal_row = player_damage[i];

        display_row.name(heal_row.src);
        display_row.value_max(total_damage);
        display_row.value(heal_row.effect_value);
        display_row.callback([=, &ui_](string_id) {
            auto display = new data_display_entity_healing_done;
            display->_encounter = _encounter;
            display->_entity_name = heal_row.src;
            display->_minion_name = heal_row.src_minion;
            ui_.change_display_mode_with_history(display);
        });
    }

    ui_element_manager_.show_only_num_rows(player_damage.size());

    ui_element_manager_.enable_stop(true);
    ui_element_manager_.info_text(L"<a id=\"back_to_entity_view\">Back</a>"
                                  L"\r\nHealing recived through " + ui_element_manager_.lookup_info()( _ability_name ) + L" by " + ui_element_manager_.lookup_info()( _entity_name ) + L"\r\n");
    ui_element_manager_.info_callback(L"back_to_entity_view", [=, &ui_]() {
        ui_.data_display_mode_go_history_back();
    });
}