#pragma once
#include <boost/asio.hpp>

#include "active.h"
#include "net_protocol.h"
#include "swtor_log_parser.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

class client_core_interface {
protected:
    client_core_interface() {}
    virtual ~client_core_interface() {}

public:
    virtual void on_combat_log_event(const combat_log_entry& e_) = 0;
    virtual void on_connection_error() = 0;
};

class client
: public active<client> {
    client(const client&);
    client& operator=(const client&);

protected:
    friend class active<client>;
    void run();

private:
    net_protocol                _link;
    sockaddr                    _peer;
    string_to_id_string_map*    _strings;
    character_list*             _chars;
    client_core_interface*      _cli;

protected:
    friend class net_protocol;
    bool on_string_lookup(string_id ident_);
    bool on_string_info(string_id ident_, const wchar_t* string_start_, const wchar_t* string_end_);
    bool on_set_name(string_id ident_, const wchar_t* name_start_, const wchar_t* name_end_);
    bool on_remove_name(string_id ident_);
    bool on_combat_event(const combat_log_entry& e_);

public:
    client();
    client(string_to_id_string_map& smap_, character_list& clist_, client_core_interface* cli_);
    client(client&& other_);
    client& operator=(client&& other_);
    void connect(const sockaddr& server_);
    void disconnect();
    void combat_event(const combat_log_entry& e_);
};