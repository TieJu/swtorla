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
/*
template<typename SrcRange,typename Predicate>
struct select_range {
    SrcRange    _src;
    Predicate   _pred;

    typedef typename std::decay<decltype( Predicate()( SrcRange().front() ) )>::type value_type;

    value_type front() {
        return _pred(_src.front());
    }

    bool next() {
        return _src.next();
    }
};
*/

#include <boost/range.hpp>
#include <boost/iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/range/iterator_range.hpp>

template<typename Selector, typename RangeType>
inline auto select_from(Selector select_, const RangeType& source_)->decltype(boost::make_iterator_range(boost::transform_iterator<Selector, decltype(source_.begin())>(source_.begin(), select_), boost::transform_iterator<Selector, decltype(source_.begin())>(source_.end(), select_))) {
    typedef boost::transform_iterator<Selector, decltype(source_.begin())> iter;
    return boost::make_iterator_range(iter(source_.begin(), select_), iter(source_.end(), select_));
}

template<typename Filter, typename RangeType>
inline auto where(Filter filter_, const RangeType& source_)->decltype(boost::make_iterator_range(boost::filter_iterator<Filter, decltype(source_.begin())>(filter_, source_.begin(), source_.end()), boost::filter_iterator<Filter, decltype(source_.begin())>(filter_, source_.end(), source_.end()))) {
    typedef boost::filter_iterator<Filter, decltype(source_.begin())> iter;
    return boost::make_iterator_range(iter(filter_, source_.begin(), source_.end()), iter(filter_, source_.end(), source_.end()));
}

template<typename Predicate, typename Iterator>
class order_iterator {
    Predicate   _cmp;
    Iterator    _from;
    Iterator    _to;
    Iterator    _pos;

    void find_first() {
        _pos = _from;
        for (auto at = _from; at != _to; ++at) {
            if ( _cmp(*at, *_pos) ) {
                _pos = at;
            }
        }
    }
    void find_next() {
        auto marker = _pos;
        for ( auto at = _from; at != _to; ++at ) {
            if ( at == marker ) {
                // skip the marker
                continue;
            }
            if ( _cmp(*at, *marker) ) {
                // skip any entry that was allready returned
                continue;
            }
            if ( marker == _pos ) {
                // first that did not match the criteria will used as base
                _pos = at;
                continue;
            }
            if ( _cmp(*at, *_pos) ) {
                _pos = at;
            }
        }

        if ( marker == _pos ) {
            // make sure we reach end if no new entrys are found
            _pos = _to;
        }
    }

public:
    typedef std::forward_iterator_tag                           iterator_category;
    typedef typename boost::iterator_value<Iterator>::type      value_type;
    typedef typename boost::iterator_reference<Iterator>::type  reference; 
    typedef typename boost::iterator_difference<Iterator>::type difference_type;
    typedef typename boost::iterator_pointer<Iterator>::type    pointer;
    order_iterator() = default;
    order_iterator(const order_iterator&) = default;
    order_iterator& operator=(const order_iterator&) = default;
    ~order_iterator() = default;

    order_iterator(Predicate cmp_, Iterator from_, Iterator to_)
        : _cmp(cmp_)
        , _from(from_)
        , _to(to_) {
        find_first();
    }
    
    Predicate predicate() const { return _cmp; }
    Iterator end() const { return _to; }
    Iterator const& base() const { return _from; }
    reference operator*() const { return *_pos; }
    order_iterator& operator++() { find_next(); return *this; }
    bool operator==(const order_iterator& other_) const {
        return other_._pos == _pos;
    }
    bool operator!=(const order_iterator& other_) const {
        return other_._pos != _pos;
    }
};

template<typename Compare, typename RangeType>
inline auto order_by(Compare compare_, const RangeType& source_)->decltype(boost::make_iterator_range(order_iterator<Compare, decltype(source_.begin())>(compare_, source_.begin(), source_.end()), order_iterator<Compare, decltype(source_.begin())>(compare_, source_.end(), source_.end()))) {
    typedef order_iterator<Compare, decltype(source_.begin())> iter;
    return boost::make_iterator_range(iter(compare_, source_.begin(), source_.end()), iter(compare_, source_.end(), source_.end()));
}

template<typename Compare, typename Merge, typename RangeType>
inline auto group_by(Compare compare_, Merge merge_, const RangeType& source_) -> std::vector<decltype(merge_(*source_.begin(), *source_.begin()))> {
    typedef decltype(merge_(*source_.begin(), *source_.begin()))  result_type;
    typedef decltype(source_.begin()) iterator_type;
    std::vector<result_type> groups;
    std::vector<iterator_type> group_markers;
    for ( auto at = source_.begin(); at != source_.end(); ++at ) {
        const auto& value = *at;
        auto group = std::find_if(begin(group_markers), end(group_markers), [&](const iterator_type& value_) {
            return compare_(*value_, *at);
        });

        if ( group == end(group_markers) ) {
            group_markers.push_back(at);
            groups.push_back(result_type{});
            group = group_markers.end() - 1;
        }

        groups[std::distance(begin(group_markers), group)] = merge_(*at, groups[std::distance(begin(group_markers), group)]);
    }
    return groups;
}
/*
template<typename Compare, typename Merge, typename Iterator>
class group_by_iterator {
    Compare     _cmp;
    Merge       _merge;
    Iterator    _from;
    Iterator    _to;
    Iterator    _pos;

public:
    order_iterator() = default;
    order_iterator(const order_iterator&) = default;
    order_iterator& operator(const order_iterator&) = default;
    ~order_iterator() = default;

    order_iterator(Predicate cmp_, Iterator from_, Iterator to_)
        : _cmp(cmp_)
        , _from(from_)
        , _to(to_) {
        ++(*this);
    }

    Predicate predicate() const { return _cmp; }
    Iterator end() const { return _to; }
    Iterator const& base() const { return _from; }
    reference operator*() const { return *_pos; }
    filter_iterator& operator++() { _pos = cmp_(_from, _to, _pos); }
};

template<typename Compare, typename Merge, typename RangeType>
inline auto group_by(Compare compare_, Merge merge_, const RangeType& source_) {

}*/