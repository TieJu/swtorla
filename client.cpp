#include "client.h"

void client::run() {
    state rs = state::sleep;

    for ( ;; ) {
        if ( rs == state::sleep || rs == state::shutdown ) {
            // connection reset
            _link.reset();
        }

        if ( rs == state::shutdown ) {
            break;
        }

        rs = wait(rs);

        if ( rs == state::init ) {
            _link.reset(new net_protocol(*_service));
            _link->set_client(this);
            _link->connect(_peer);
        }

        while ( rs == state::run ) {
            auto result = _link->read_from_port();
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
    auto at = _strings->find(ident_);
    if ( at != end(*_strings) ) {
        _link->send_string_info(at->first, at->second);
    }
    return true;
}

bool client::on_string_info(string_id ident_, const wchar_t* string_start_, const wchar_t* string_end_) {
    // insert it, if we know it, it will not change as for the hash map insert rules
    // TODO: may force update if string is empty?
    _strings->insert(std::make_pair(ident_, std::wstring(string_start_, string_end_)));
    return true;
}

bool client::on_set_name(string_id ident_, const wchar_t* name_start_, const wchar_t* name_end_) {
    // server tells us to link a name to the selected ident_
    if ( _chars->size() < ident_ + 1 ) {
        _chars->resize(ident_ + 1);
    }
    ( *_chars )[ident_].assign(name_start_, name_end_);
    return true;
}

bool client::on_remove_name(string_id ident_) {
    // server tells us to remove the name links to ident_
    if ( ident_ < _chars->size() ) {
        ( *_chars )[ident_].clear();
    }
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
    : _service(nullptr)
    , _strings(nullptr)
    , _chars(nullptr)
    , _cli(nullptr) {

}

client::client(boost::asio::io_service& service_
              ,string_to_id_string_map& smap_
              ,character_list& clist_
              ,client_core_interface* cli_)
    : _service(&service_)
    , _strings(&smap_)
    , _chars(&clist_)
    , _cli(cli_) {
}


client::client(client&& other_)
    : client() {
    *this = std::move(other_);
}

client& client::operator=(client && other_) {
    active<client>::operator=( std::move(other_) );
    _service = other_._service;
    _strings = other_._strings;
    _chars = other_._chars;
    _cli = other_._cli;
    return *this;
}

void client::connect(boost::asio::ip::tcp::endpoint server_) {
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
    if ( _link ) {
        _link->send_combat_event(e_);
    }
}