#pragma once

#include <string>
#include <http_client.h>

class update_dialog;

class updater {
    std::wstring    _server;

public:
    updater( const std::wstring& server_ );

    pplx::task<size_t> query_build( update_dialog& ui_ );
    pplx::task<void> download_update( update_dialog& ui_, size_t from_, size_t to_, const std::wstring& target_ );
    pplx::task<std::string> get_patchnotes( size_t ver_ );
};