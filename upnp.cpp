#include "upnp.h"



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
        throw std::runtime_error("Unable to create IUPnPNAT instance");
    }

    IStaticPortMappingCollection* spm;
    error = _upnp->get_StaticPortMappingCollection(&spm);

    _ports.Attach(spm, false);

    INATEventManager* em;
    error = _upnp->get_NATEventManager(&em);
    _events.Attach(em, false);
    
    _clb_wrap.Attach(new clb_wrap(_clb), false);
    _events->put_ExternalIPAddressCallback(_clb_wrap);
}

upnp::upnp()
: _clb(nullptr) {

}

upnp::upnp(upnp_callback_interface* clb_)
: _clb(clb_) {
    init_interfaces();
}

upnp::~upnp() {
    if ( _ports ) {
        for ( auto& map : _map ) {
            long port;
            map->get_ExternalPort(&port);
            // TODO: this meight be wrong
            BSTR str = nullptr;
            map->get_InternalClient(&str);
            _ports->Remove(port, str);
            SysFreeString(str);
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

void upnp::map_tcp(unsigned short public_, unsigned short local_, const std::wstring& local_ip_) {
    init_interfaces();

    auto local = SysAllocString(local_ip_.c_str());
    auto proto = SysAllocString(L"TCP");
    auto name = SysAllocString(L"TCP port for swtorla");

    IStaticPortMapping* map = nullptr;
    auto result = _ports->Add(public_, proto, local_, local, VARIANT_TRUE, name, &map);
    if ( map ) {
        _map.emplace_back(map, false);
    }

    SysFreeString(local);
    SysFreeString(proto);
    SysFreeString(name);
}

void upnp::map_udp(unsigned short public_, unsigned short local_, const std::wstring& local_ip_) {
    init_interfaces();

    auto local = SysAllocString(local_ip_.c_str());
    auto proto = SysAllocString(L"TCP");
    auto name = SysAllocString(L"TCP port for swtorla");

    IStaticPortMapping* map = nullptr;
    auto result = _ports->Add(public_, proto, local_, local, VARIANT_TRUE, name, &map);
    if ( map ) {
        _map.emplace_back(map, false);
    }

    SysFreeString(local);
    SysFreeString(proto);
    SysFreeString(name);
}