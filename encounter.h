#pragma once

#include "swtor_log_parser.h"

#include "query_set.h"

#include <utility>
#include <vector>
#include <chrono>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

template<typename T>
class query_result {
    std::vector<T>  _rows;

    friend class query_result;
    friend class encounter;

protected:
    template<typename U>
    std::pair<typename std::vector<T>::iterator, bool> find_or_insert(const T& row_, U compare_) {
        auto at = std::find_if(begin(_rows), end(_rows), [=](const T& e_) {
            return compare_(e_, row_);
        });

        if ( at != end(_rows) ) {
            return std::make_pair(at, false);
        }

        return std::make_pair(_rows.insert(end(_rows), row_), true);
    }

public:
    template<typename R, typename U, typename V, typename W, typename X>
    query_result<R> select(U filter_, V transfrom_, W same_group_, X group_combine_) {
        query_result<R> r;

        for ( const auto& row : _rows ) {
            if ( !filter_(row) ) {
                continue;
            }

            auto tr = transfrom_(row);

            auto i = r.find_or_insert(tr, std::forward<W>( same_group_ ));

            if ( !i.second ) {
                group_combine_(*i.first, tr);
            }
        }

        return r;
    }

    template<typename U>
    void for_each(U v_) {
        for ( const auto& row : _rows ) {
            v_(row);
        }
    }
};

class encounter {
    std::vector<combat_log_entry>                   _table;
    size_t                                          _begins { 0 };
    size_t                                          _ends { 0 };

    static bool is_combat_begin( const combat_log_entry& cle_ ) {
        return cle_.effect_action == ssc_Event
            && cle_.effect_type == ssc_EnterCombat;
    }
    static bool is_combat_end( const combat_log_entry& cle_ ) {
        return cle_.effect_action == ssc_Event
            && cle_.effect_type == ssc_ExitCombat;
    }

public:/*
    template<typename R,typename U,typename V,typename W,typename X>
    query_result<R> select(U filter_, V transfrom_, W same_group_,X group_combine_) {
        query_result<R> r;

        for ( const auto& row : _table ) {
            if ( !filter_(row) ) {
                continue;
            }

            auto tr = transfrom_(row);

            auto i = r.find_or_insert(tr, std::forward<W>( same_group_ ));

            if ( !i.second ) {
                group_combine_(*i.first, tr);
            }
        }

        return r;
    }

    template<typename U>
    void for_each(U v_) {
        for ( const auto& row : _table ) {
            v_(row);
        }
    }
    */
    bool insert( const combat_log_entry& row_ ) {
        _begins += is_combat_begin( row_ ) ? 1 : 0;
        _ends += is_combat_end( row_ ) ? 1 : 0;

        if ( _begins > 0 ) {
            //BOOST_LOG_TRIVIAL( debug ) << to_wstring( row_ );
            _table.push_back( row_ );
            return true;
        }
        return false;
    }

    template<typename DstType,typename U>
    query_set<std::vector<combat_log_entry>, DstType> select( U v_ ) {/*
        BOOST_LOG_TRIVIAL( debug ) << "select from { ";
        for ( const auto& e : _table ) {
            BOOST_LOG_TRIVIAL( debug ) << to_wstring( e );
        }
        BOOST_LOG_TRIVIAL( debug ) << "} ";
        BOOST_LOG_TRIVIAL( debug ) << _table.size() << " records listed";*/
        return select_from<DstType>( std::forward<U>( v_ ), _table );
    }

    std::vector<combat_log_entry>& get_data() { return _table; }

    std::chrono::milliseconds get_combat_length() {
        if ( _table.size() < 2 ) {
            return std::chrono::milliseconds(1);
        }

        auto& first = _table.front();
        auto& last = _table.back(); 

        return std::chrono::duration_cast<std::chrono::milliseconds>( last.time_index - first.time_index );
    }

    void compatct() {
        _table.shrink_to_fit();
    }

    void remap_player( string_id old_, string_id new_ ) {
        for ( auto& e : _table ) {
            if ( e.src == old_ ) {
                e.src = new_;
            }
            if ( e.dst == old_ ) {
                e.dst = new_;
            }
        }
    }

    bool combat_has_finished( ) {
        //BOOST_LOG_TRIVIAL( debug ) << L"combat_has_finished() = " << ( ( _begins > 0 ) ? ( _begins == _ends ) : false );
        if ( _begins > 0 ) {
            return _begins == _ends;
        }
        return false;
    }
};