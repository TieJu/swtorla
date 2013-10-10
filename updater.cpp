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


updater::updater( boost::property_tree::wptree& config_ )
 : _config( &config_ ) {
}

void updater::config( boost::property_tree::wptree& config_ ) {
    _config = &config_;
}

pplx::task<size_t> updater::query_build( update_dialog& ui_ ) {
    using namespace web::http;

    ui_.progress( 0 );
    ui_.info_msg( L"...connecting to patch list server..." );
    BOOST_LOG_TRIVIAL( debug ) << L"...connecting to patch list server...";

    client::http_client client( _config->get<std::wstring>( L"update.server", L"http://homepages.thm.de" ) );

    auto path = uri_builder( _config->get<std::wstring>( L"update.list", L"/~hg14866/swtorla/update.php" ) )
              . append_query( L"arch", _config->get<std::wstring>( L"app.arch",
#ifdef _M_X64
        L"x64"
#else
        L"x86"
#endif
        ) );

    ui_.unknown_progress( true );
    ui_.info_msg( L"...sending request to server..." );
    BOOST_LOG_TRIVIAL( debug ) << L"...sending request to server...";

    return client.request(methods::GET, path.to_string())
        .then( [=, &ui_]( http_response result_ ) {
            ui_.info_msg( L"...response recived..." );
            BOOST_LOG_TRIVIAL( debug ) << L"...response recived, result code " << result_.status_code() << L"...";
            if ( result_.status_code() == status_codes::OK ) {
                return result_.extract_json();
            } else {
                return pplx::task_from_result( web::json::value() );
            }
    } ).then( [=, &ui_]( pplx::task<web::json::value> json_ ) {
        ui_.info_msg( L"...analizing results..." );
        BOOST_LOG_TRIVIAL( debug ) << L"...analizing results, listing updates on reported by the server...";
        size_t v = 0;

        const auto& set = json_.get();

        for ( auto at = set.cbegin(), ed = set.cend(); at != ed; ++at ) {
            auto str = at->second.to_string();
            BOOST_LOG_TRIVIAL( debug ) << L"..." << str.c_str() << L"...";
            size_t cv = 0;
            swscanf_s( str.c_str(), L"\"updates/%I64u.update\"", &cv );
            v = cv > v ? cv : v;
        }

        return v;
    });
}

pplx::task<void> updater::download_update( update_dialog& ui_, size_t from_, size_t to_, const std::wstring& target_ ) {
}

pplx::task<std::wstring> updater::get_patchnotes( size_t from_, size_t to_ ) {
    using namespace web::http;

    BOOST_LOG_TRIVIAL( debug ) << L"...connecting to patch info server...";

    client::http_client client( _config->get<std::wstring>( L"update.server", L"http://homepages.thm.de" ) );

    auto path = uri_builder( _config->get<std::wstring>( L"update.info", L"/~hg14866/swtorla/update.php" ) )
              . append_query( L"from", std::to_wstring( from_ ) )
              . append_query( L"to", std::to_wstring( to_ ) );

    return client.request(methods::GET, path.to_string())
                 .then( [=]( http_response result_ ) {
            BOOST_LOG_TRIVIAL( debug ) << L"...response recived, result code " << result_.status_code() << L"...";
            if ( result_.status_code() == status_codes::OK ) {
                return result_.extract_string();
            } else {
                return pplx::task_from_result( std::wstring() );
            }
    } );
}