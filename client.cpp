#include "client.h"

void client::run() {
    state rs = state::sleep;

    for ( ;; ) {
        if ( rs == state::sleep || rs == state::shutdown ) {
            // connection reset
            _link.disconnect();
        }

        if ( rs == state::shutdown ) {
            break;
        }

        rs = wait(rs);

        if ( rs == state::init ) {
            _link.connect(_peer);
            rs = state::init;
        }

        while ( rs == state::run ) {
            auto result = _link.read_from_port();
            if ( net_protocol::connection_status::connected != result ) {
                ::MessageBoxW(nullptr, L"Server Connection Problem", L"Connection to the server has been closed, see log for details", MB_OK | MB_ICONERROR);
                BOOST_LOG_TRIVIAL(error) << L"Connection to server has been closed, result code was " << int( result );
                if ( _cli ) {
                    // reset the ui
                    _cli->on_connection_error();
                }
                rs = state::sleep;
                break;
            }

            rs = wait_for(rs, std::chrono::milliseconds(25));
        }
    }
}

bool client::on_string_lookup(string_id ident_) {
    // look up string and send it back if we know it
    if ( _strings->is_set(ident_) ) {
        _link.send_string_info(ident_, _strings->get(ident_));
    }
    return true;
}

bool client::on_string_info( string_id ident_, const wchar_t* string_start_, const wchar_t* string_end_ ) {
    // insert it, if we know it, it will not change as for the hash map insert rules
    // TODO: may force update if string is empty?
    _strings->set( ident_, std::wstring { string_start_, string_end_ } );
    return true;
}

bool client::on_set_name( string_id ident_, const wchar_t* name_start_, const wchar_t* name_end_ ) {
    // server tells us to link a name to the selected ident_
    _chars->set_player_name( std::wstring { name_start_, name_end_ }, ident_ );
    return true;
}

bool client::on_remove_name(string_id ident_) {
    // server tells us to remove the name links to ident_
    _chars->remove_player_name( ident_ );
    return true;
}

bool client::on_combat_event(const combat_log_entry& e_) {
    // feed combat event to system
    if ( _cli ) {
        _cli->on_combat_log_event(e_);
    }
    return true;
}

client::client()
    : _strings(nullptr)
    , _chars(nullptr)
    , _cli(nullptr) {
    _link.set_client(this);
}

client::client(string_db& smap_
              ,player_db& clist_
              ,client_core_interface* cli_)
    : _strings(&smap_)
    , _chars(&clist_)
    , _cli(cli_) {
    _link.set_client(this);
}


client::client(client&& other_)
    : client() {
    *this = std::move(other_);
}

client& client::operator=(client && other_) {
    active<client>::operator=( std::move(other_) );
    _strings = std::move(other_._strings);
    _chars = std::move(other_._chars);
    _cli = std::move(other_._cli);
    _link = std::move(other_._link);
    _link.set_client(this);
    return *this;
}

void client::connect(const sockaddr& server_) {
    if ( !is_runging() ) {
        start();
    }
    _peer = server_;
    change_state_and_wait(state::init);
}

void client::disconnect() {
    change_state_and_wait(state::sleep);
}

// sends log entry to server
void client::combat_event(const combat_log_entry& e_) {
    if ( _link.is_open() ) {
        _link.send_combat_event(e_);
    }
}