#pragma once

#include <natupnp.h>
#include <string>
#include <vector>
#include <comutil.h>
#include <comip.h>
#include <comdef.h>

class upnp_callback_interface {
public:
    virtual void on_ip_change(const std::wstring& new_ip_);
};

class upnp {
    _COM_SMARTPTR_TYPEDEF(IUPnPNAT, __uuidof( IUPnPNAT ));
    _COM_SMARTPTR_TYPEDEF(IStaticPortMappingCollection, __uuidof( IStaticPortMappingCollection ));
    _COM_SMARTPTR_TYPEDEF(INATEventManager, __uuidof( INATEventManager ));
    _COM_SMARTPTR_TYPEDEF(IStaticPortMapping, __uuidof( IStaticPortMapping ));
    _COM_SMARTPTR_TYPEDEF(INATExternalIPAddressCallback, __uuidof( INATExternalIPAddressCallback ));
    IUPnPNATPtr                         _upnp;
    IStaticPortMappingCollectionPtr     _ports;
    INATEventManagerPtr                 _events;
    upnp_callback_interface*            _clb;
    INATExternalIPAddressCallbackPtr    _clb_wrap;
    std::vector<IStaticPortMappingPtr>  _map;

    void init_interfaces();

public:
    upnp();
    upnp(upnp_callback_interface* clb_);
    ~upnp();
    upnp(upnp&& other_);
    upnp& operator=(upnp&& other_);

    bool map_tcp(unsigned short public_, unsigned short local_, const std::wstring& local_ip_);
    bool map_udp(unsigned short public_, unsigned short local_, const std::wstring& local_ip_);
};