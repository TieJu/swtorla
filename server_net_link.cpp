#include "app.h"
#include "server_net_link.h"

void server_net_link::on_net_link_command(command command_, const char* data_begin_, const char* data_end_) {
    switch ( command_ ) {
    case command::client_register:
        _ci->on_client_register(this, std::wstring(reinterpret_cast<const wchar_t*>( data_begin_ ), reinterpret_cast<const wchar_t*>( data_end_ )));
        break;
    case command::string_lookup:
        _ci->on_string_lookup(this, *reinterpret_cast<const string_id*>( data_begin_ ));
        break;
    case command::string_info:
        _ci->on_string_info(this
                            , *reinterpret_cast<const string_id*>( data_begin_ )
                            , std::wstring(reinterpret_cast<const wchar_t*>( data_begin_ + sizeof( string_id ) ), reinterpret_cast<const wchar_t*>( data_end_ )));
        break;
    case command::combat_event:
        _ci->on_combat_event(this, std::get<0>( uncompress(data_begin_, 0) ));
        break;
    default:
        throw std::runtime_error("unknown net command recived");
    }
}

boost::asio::ip::tcp::socket& server_net_link::get_link() {
    return *_link;
}

bool server_net_link::is_link_active() {
    return check_state(state::run);
}

void server_net_link::run() {
    boost::system::error_code error;
    boost::array<char, 1024> read_buffer;
    state r_state = state::sleep;

    for ( ;; ) {
        r_state = wait(r_state);

        if ( state::sleep == r_state || state::shutdown == r_state ) {
            _link->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            _link->close();
            _ci->on_client_disconnect(this);
        }

        if ( state::shutdown == r_state ) {
            break;
        }

        if ( state::init == r_state ) {
            r_state = state::run;
            change_state(state::run);
        }

        while ( state::run == r_state ) {
            auto result = _link->read_some(boost::asio::buffer(read_buffer), error);
            if ( result > 0 ) {
                on_net_packet(read_buffer.data(), result);
                continue; // dont wait, just try to get next data set
            }

            if ( error ) {
                if ( error != boost::asio::error::would_block ) {
                    // error...
                    break;
                }
            }
            // only block if no data is available
            r_state = wait_for(r_state, std::chrono::milliseconds(25));
        }
    }
}


server_net_link::server_net_link(app& app_, boost::asio::ip::tcp::socket* socket_)
    : _ci(&app_)
    , _link(socket_) {
    start(state::run);
}

server_net_link::~server_net_link() {
}

void server_net_link::send_set_name(string_id id_, const std::wstring& name_) {
    if ( is_link_active() ) {
        auto header = gen_packet_header(command::server_set_name, sizeof( id_ ) + name_.length() * sizeof(wchar_t));
        _link->write_some(boost::asio::buffer(&header, sizeof( header )));
        _link->write_some(boost::asio::buffer(&id_, sizeof( id_ )));
        _link->write_some(boost::asio::buffer(name_.data(), name_.length() * sizeof(wchar_t)));
    }
}