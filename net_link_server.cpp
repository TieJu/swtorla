#include "app.h"

void net_link_server::run() {
    boost::system::error_code error;
    state r_state = state::sleep;
    auto socket = std::make_unique<boost::asio::ip::tcp::socket>( _ci->get_io_service() );

    for ( ;; ) {
        r_state = wait(r_state);

        if ( state::sleep == r_state || state::shutdown == r_state ) {
            _link->close();
        }

        if ( state::shutdown == r_state ) {
            break;
        }

        if ( state::init == r_state ) {
            _link->open(boost::asio::ip::tcp::v4());
            _link->non_blocking(true);
            _link->listen();
        }

        while ( state::run == r_state ) {
            _link->accept(*socket, error);
            if ( error ) {
                if ( error != boost::asio::error::would_block ) {
                    // error...
                    break;
                }
            } else {
                _ci->new_client(socket.release());
                socket.reset(new boost::asio::ip::tcp::socket(_ci->get_io_service()));
            }
            r_state = wait_for(r_state, std::chrono::milliseconds(25));
        }
    }
}

net_link_server::net_link_server()
    : _ci(nullptr) {
}

net_link_server::net_link_server(app& ci_)
    : _ci(&ci_)
    , _link(new boost::asio::ip::tcp::acceptor(ci_.get_io_service())) {
}
net_link_server::net_link_server(net_link_server&& other_)
    : net_link_server() {
    *this = std::move(other_);
}

net_link_server::~net_link_server() {
}

net_link_server& net_link_server::operator=( net_link_server && other_ ) {
    active<net_link_server>::operator=( std::move(other_) );
    _link = std::move(other_._link);
    _port = std::move(other_._port);
    return *this;
}

void net_link_server::start(const std::string& port_) {
    if ( !is_runging() ) {
        active<net_link_server>::start();
    }
    _port = port_;
    change_state(state::init);
}

void net_link_server::stop() {
    change_state(state::sleep);
}