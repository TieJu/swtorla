#pragma once

#include "ui_element_manager.h"
#include "combat_db.h"

class main_ui;

struct data_display_mode {
    typedef std::function<void (data_display_mode*)> change_display_mode_callback;
    size_t                                          _encounter;

    data_display_mode() : _encounter(size_t(-1)) {}
    virtual void update_display(combat_db& combat_db_, ui_element_manager<main_ui>& ui_element_manager_, main_ui& ui_) = 0;
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
    void update_display(combat_db& combat_db_, ui_element_manager<main_ui>& ui_element_manager_, main_ui& ui_) override;
};
struct data_display_entity_healing_done : public data_display_entity_base {
    void update_display(combat_db& combat_db_, ui_element_manager<main_ui>& ui_element_manager_, main_ui& ui_) override;
};
struct data_display_entity_dmg_recived : public data_display_entity_base {
    void update_display(combat_db& combat_db_, ui_element_manager<main_ui>& ui_element_manager_, main_ui& ui_) override;
};
struct data_display_entity_healing_recived : public data_display_entity_base {
    void update_display(combat_db& combat_db_, ui_element_manager<main_ui>& ui_element_manager_, main_ui& ui_) override;
};

struct data_display_entity_skill_base : public data_display_entity_base {
    string_id       _ability_name;
}; 

struct data_display_entity_skill_dmg_done : public data_display_entity_skill_base {
    void update_display(combat_db& combat_db_, ui_element_manager<main_ui>& ui_element_manager_, main_ui& ui_) override;
};

struct data_display_entity_skill_healing_done : public data_display_entity_skill_base {
    void update_display(combat_db& combat_db_, ui_element_manager<main_ui>& ui_element_manager_, main_ui& ui_) override;
};

struct data_display_entity_skill_dmg_recived : public data_display_entity_skill_base {
    void update_display(combat_db& combat_db_, ui_element_manager<main_ui>& ui_element_manager_, main_ui& ui_) override;
};

struct data_display_entity_skill_healing_recived : public data_display_entity_skill_base {
    void update_display(combat_db& combat_db_, ui_element_manager<main_ui>& ui_element_manager_, main_ui& ui_) override;
};
