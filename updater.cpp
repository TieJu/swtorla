#include "updater.h"
#include "update_dialog.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include <filestream.h>

pplx::task<update_server_info> updater::get_update_server_info( const std::wstring& name_, const std::wstring& path_, size_t api_ ) {
    using namespace web::http;
    uri_builder path { path_ };

    switch ( api_ ) {
    case 1:
    default:
        path.append_path( L"update.php" );
        path.append_query( L"arch",
#ifdef _M_X64
                           L"x64"
#else
                           L"x86"
#endif
                           );
        break;
    case 2:
        path.append_path( L"patchlist.php" );
        path.append_query( L"a",
#ifdef _M_X64
                           L"x64"
#else
                           L"x86"
#endif
                           );
        break;
    }

    auto qs = path.to_string();

    client::http_client client { name_ };
    BOOST_LOG_TRIVIAL( debug ) << L"...sending request " << qs << L" to server " << name_ << L"...";

    update_server_info info { name_, path_, api_, 0 };
    return client.request( methods::GET, qs )
        .then( [=]( http_response response_ ) {
            BOOST_LOG_TRIVIAL( debug ) << L"...server " << name_ << L" returned " << response_.status_code() << L"...";
            if ( response_.status_code() != status_codes::OK ) {
                return pplx::task_from_result( web::json::value() );
            } else {
                return response_.extract_json();
            }
    } ) . then( [=]( pplx::task<web::json::value> json_ ) mutable {
        const auto& set = json_.get();

        switch ( api_) {
        case 1:
        default:
            for ( auto at = set.cbegin(), ed = set.cend(); at != ed; ++at ) {
                auto str = at->second.to_string();
                size_t cv = 0;
                swscanf_s( str.c_str(), L"\"updates/%I64u.update\"", &cv );
                info.latest_version = cv > info.latest_version ? cv : info.latest_version;
                BOOST_LOG_TRIVIAL( debug ) << L"...version " << cv << L" is at server " << name_ << L"...";
            }
            break;
        case 2:
            for ( auto at = set.cbegin(), ed = set.cend(); at != ed; ++at ) {
                auto cv = size_t(at->second.as_integer());
                info.latest_version = cv > info.latest_version ? cv : info.latest_version;
                BOOST_LOG_TRIVIAL( debug ) << L"...version " << cv << L" is at server " << name_ << L"...";
            }
            break;
        }

        BOOST_LOG_TRIVIAL( debug ) << L"...latest version on server " << name_ << L" is " << info.latest_version << L"...";

        return info;
    });
}


updater::updater( boost::property_tree::wptree& config_ )
 : _config( &config_ ) {
}

void updater::config( boost::property_tree::wptree& config_ ) {
    _config = &config_;
}

pplx::task<update_server_info> updater::get_best_update_server() {
    using boost::property_tree::wptree;
    wptree temp;
    auto list = _config->get_child_optional( L"update_servers" );
    if ( !list ) {
        list = temp;
        wptree thm {};
        thm.put( L"name", L"http://homepages.thm.de" );
        thm.put( L"path", L"/~hg14866/swtorla" );
        thm.put( L"api", 1 );

        wptree clouded {};
        clouded.put( L"name", L"http://swtorlapatch.cloudcontrolled.com" );
        clouded.put( L"path", L"/" );
        clouded.put( L"api", 2 );

        wptree cloudedapp {};
        cloudedapp.put( L"name", L"http://swtorlapatch.cloudcontrolapp.com" );
        cloudedapp.put( L"path", L"/" );
        cloudedapp.put( L"api", 2 );

        list->push_back( std::make_pair( L"", thm ) );
        list->push_back( std::make_pair( L"", clouded ) );
        list->push_back( std::make_pair( L"", cloudedapp ) );
    }

    std::vector<pplx::task<update_server_info>> info_list;
    for ( const auto& server : *list ) {
        const auto& info = server.second;
        info_list.push_back( get_update_server_info( info.get<std::wstring>( L"name", L"??" ), info.get<std::wstring>( L"path", L"" ), info.get<size_t>( L"api", 1 ) ) );
    }

    return pplx::create_task( [=]() {
        update_server_info info { L"", L"", 0, 0 };
        for ( const auto& task : info_list ) {
            try {
                auto result = task.get();
                if ( result.latest_version > info.latest_version ) {
                    info = result;
                }
            } catch ( const std::exception& e_ ) {
                BOOST_LOG_TRIVIAL( error ) << L"...error while downloading patch info, " << e_.what() << L"...";
            }
        }
        return info;
    } );
}

pplx::task<size_t> updater::download_update( const update_server_info& info_, size_t from_, size_t to_, const std::wstring& target_ ) {
    using namespace web::http;
    client::http_client client { info_.name };

    uri_builder path { info_.path };
    switch ( info_.api ) {
    case 1:
    default:
        path.append_path( L"updates" )
            .append_path( std::to_wstring( to_ ) + L".update" );
        break;
    case 2:
        path.append_path( L"patch" )
            .append_path( std::to_wstring( to_ ) + L".patch" );
        break;
    }

    return client.request( methods::GET, path.to_string() )
        .then( [=]( http_response result_ ) {
            BOOST_LOG_TRIVIAL( debug ) << L"...response recived, result code " << result_.status_code() << L"...";
            if ( result_.status_code() == status_codes::OK ) {
                return result_.body();
            } else {
                pplx::cancel_current_task();
            }
    } ).then( [=]( concurrency::streams::istream stream_ ) {
        auto file = concurrency::streams::fstream::open_ostream( target_, std::ios_base::out | std::ios_base::binary );
        return file.then( [=, &stream_]( concurrency::streams::ostream file_ ) {
            return stream_.read_to_end( file_.streambuf() );
        } );
    } );
}

pplx::task<std::wstring> updater::download_patchnotes( const update_server_info& info_, size_t from_, size_t to_ ) {
    using namespace web::http;
    client::http_client client { info_.name };

    uri_builder path { info_.path };
    switch ( info_.api ) {
    case 1:
    default:
        path.append_path( L"update" )
            .append_query( L"from", std::to_wstring( from_ ) )
            .append_query( L"to", std::to_wstring( to_ ) );
        break;
    case 2:
        path.append_path( L"info" )
            .append_query( L"from", std::to_wstring( from_ ) )
            .append_query( L"to", std::to_wstring( to_ ) );
        break;
    }

    return client.request( methods::GET, path.to_string() )
        .then( [=]( http_response result_ ) {
            if ( result_.status_code() == status_codes::OK ) {
                return result_.extract_string();
            } else {
                return pplx::task_from_result( std::wstring() );
            }
    } );
}