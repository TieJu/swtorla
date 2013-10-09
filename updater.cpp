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


updater::updater( const std::wstring& server_ )
    : _server( server_ ) {
}

pplx::task<size_t> updater::query_build( update_dialog& ui_ ) {
    using namespace web::http;

    ui_.progress( 0 );
    ui_.info_msg( L"...connecting to patch list server..." );
    BOOST_LOG_TRIVIAL( debug ) << L"...connecting to patch list server...";

    client::http_client client( _server );

    auto q_string =
#ifdef _M_X64
        L"/~hg14866/swtorla/update.php?a=x64";
#else
        L"/~hg14866/swtorla/update.php?a=x86";
#endif

    ui_.unknown_progress( true );
    ui_.info_msg( L"...sending request to server..." );
    BOOST_LOG_TRIVIAL( debug ) << L"...sending request to server...";

    return client.request(methods::GET, q_string)
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
        BOOST_LOG_TRIVIAL( debug ) << L"...analizing results...";
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

pplx::task<std::string> updater::get_patchnotes( size_t ver_ ) {
}