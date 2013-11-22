#include "upnp.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

struct clb_wrap
    : public INATExternalIPAddressCallback {
    upnp_callback_interface*    _cbi;
    size_t                      _ref;
    clb_wrap(upnp_callback_interface* cbi_)
        : _cbi(cbi_)
        , _ref(1) {

    }

    virtual HRESULT STDMETHODCALLTYPE NewExternalIPAddress(BSTR bstrNewExternalIPAddress) override {
        if ( _cbi ) {
            _cbi->on_ip_change(std::wstring(bstrNewExternalIPAddress, SysStringLen(bstrNewExternalIPAddress)));
        }
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override {
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef(void) {
        return ++_ref;
    }

    virtual ULONG STDMETHODCALLTYPE Release(void) {
        if ( _ref-- == 1 ) {
            delete this;
            return 0;
        }
        return _ref;
    }
};

void upnp::init_interfaces() {
    if ( _upnp ) {
        return;
    }

    CoInitialize(nullptr);

    auto error = _upnp.CreateInstance(CLSID_UPnPNAT, nullptr, CLSCTX_INPROC_SERVER);

    if ( !_upnp ) {
        BOOST_LOG_TRIVIAL(error) << L"CreateInstance(CLSID_UPnPNAT,...) failed with: " << error;
        return;
    }

    IStaticPortMappingCollection* spm;
    error = _upnp->get_StaticPortMappingCollection(&spm);
    _ports.Attach(spm, false);

    if ( !_ports ) {
        BOOST_LOG_TRIVIAL(error) << L"Unable to retrive mort mapping list, upnp meight be blocked by the firewall and/or the router is not upnp compatible";
    }

    INATEventManager* em;
    error = _upnp->get_NATEventManager(&em);
    _events.Attach(em, false);
    
    if ( _clb && _events ) {
        _clb_wrap.Attach(new clb_wrap(_clb), false);
        _events->put_ExternalIPAddressCallback(_clb_wrap);
    }
}

upnp::upnp()
: upnp(nullptr) {

}

upnp::upnp(upnp_callback_interface* clb_)
: _clb(clb_) {
}

upnp::~upnp() {
    if ( _ports ) {
        for ( auto& map : _map ) {
            long port;
            map->get_ExternalPort(&port);
            BSTR proto = nullptr;
            map->get_Protocol(&proto);
            auto result = _ports->Remove(port, proto);
            SysFreeString(proto);
        }
    }
}

upnp::upnp(upnp && other_)
    : upnp() {
    *this = std::move(other_);
}

upnp& upnp::operator=(upnp&& other_) {
    _upnp = std::move(other_._upnp);
    _ports = std::move(other_._ports);
    _events = std::move(other_._events);
    _clb = std::move(other_._clb);
    _map = std::move(other_._map);
    return *this;
}

bool upnp::map_tcp(unsigned short public_, unsigned short local_, const std::wstring& local_ip_) {
    init_interfaces();
    if ( !_ports ) {
        BOOST_LOG_TRIVIAL(error) << L"Can not map port from " << public_ << L" to " << local_ip_ << L":" << local_ << L" because upnp init has failed";
        return false;
    }

    auto local = SysAllocString(local_ip_.c_str());
    auto proto = SysAllocString(L"TCP");
    auto name = SysAllocString(L"TCP port for swtorla");

    IStaticPortMapping* map = nullptr;
    auto result = _ports->Add(public_, proto, local_, local, VARIANT_TRUE, name, &map);
    if ( map ) {
        _map.emplace_back(map, false);
        BSTR ext;
        map->get_ExternalIPAddress( &ext );
        BOOST_LOG_TRIVIAL(info) << L"New TCP port mapping from " << std::wstring(ext, SysStringLen(ext)) << L":" << public_ << L" to " << local_ip_ << L":" << local_;
    } else {
        BOOST_LOG_TRIVIAL(error) << L"Port mapping from " << public_ << L" to " << local_ip_ << L":" << local_ << L" has been failed, error code was " << result;
    }

    SysFreeString(local);
    SysFreeString(proto);
    SysFreeString(name);
    return map != nullptr;
}

bool upnp::map_udp(unsigned short public_, unsigned short local_, const std::wstring& local_ip_) {
    init_interfaces();
    if ( !_ports ) {
        BOOST_LOG_TRIVIAL(error) << L"Can not map port " << public_ << L" to " << local_ip_ << L":" << local_ << L" because upnp init has failed";
        return false;
    }

    auto local = SysAllocString(local_ip_.c_str());
    auto proto = SysAllocString(L"UDP");
    auto name = SysAllocString(L"UDP port for swtorla");

    IStaticPortMapping* map = nullptr;
    auto result = _ports->Add(public_, proto, local_, local, VARIANT_TRUE, name, &map);
    if ( map ) {
        _map.emplace_back( map, false );
        BSTR ext;
        map->get_ExternalIPAddress( &ext );
        BOOST_LOG_TRIVIAL( info ) << L"New UDP port mapping from " << std::wstring( ext, SysStringLen( ext ) ) << L":" << public_ << L" to " << local_ip_ << L":" << local_;
    } else {
        BOOST_LOG_TRIVIAL(error) << L"Port mapping from " << public_ << L" to " << local_ip_ << L":" << local_ << L" has been failed, error code was " << result;
    }

    SysFreeString(local);
    SysFreeString(proto);
    SysFreeString(name);
    return map != nullptr;
}