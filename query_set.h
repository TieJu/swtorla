#pragma once

#include <functional>
#include <algorithm>

template<typename SrcCont,typename DstType>
class query_set {
    typedef typename SrcCont::value_type SrcType;
    SrcCont&                                                _src;
    std::function<DstType(const SrcType&)>                  _transform;
    std::function<bool(const SrcType&)>                     _filter;
    std::function<bool( const DstType&, const DstType& )>   _is_same_group;
    std::function<DstType(const DstType&, const DstType&)>  _combiner;
    std::function<bool( const DstType&, const DstType& )>   _order;


public:
    template<typename W>
    query_set(SrcCont& v_, W x_)
        : _src(v_)
        , _transform(std::forward<W>( x_ )) {}

    template<typename U>
    query_set& where(U v_) {
        _filter = std::forward<U>( v_ );
        return *this;
    }

    template<typename U, typename W>
    query_set& group_by(U v_, W x_) {
        _is_same_group = std::forward<U>( v_ );
        _combiner = std::forward<W>( x_ );
        return *this;
    }

    template<typename U>
    query_set& order_by(U v_) {
        _order = std::forward<U>( v_ );
        return *this;
    }

    template<typename U>
    query_set& into(U& v_) {
        for ( const auto& row : _src ) {
            if ( _filter ) {
                if ( !_filter(row) ) {
                    continue;
                }
            }

            auto t = _transform(row);

            if ( _is_same_group && _combiner ) {
                auto at = std::find_if(begin(v_), end(v_), [=, &t](const DstType& e_) {
                    return _is_same_group(t, e_);
                });

                if ( at == end(v_) ) {
                    v_.push_back(t);
                } else {
                    *at = _combiner(*at, t);
                }
            } else {
                v_.push_back(t);
            }
        }

        if ( _order ) {
            std::sort(begin(v_), end(v_), _order);
        }
        return *this;
    }

    template<typename Cont>
    Cont commit() {
        Cont c;
        into(c);
        return c;
    }
};

template<typename DstType, typename SrcCont,typename U>
inline query_set<SrcCont, DstType> select_from(U v_, SrcCont& s_) {
    return query_set<SrcCont, DstType>( s_, v_ );
}