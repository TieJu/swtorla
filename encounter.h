#pragma once

#include "swtor_log_parser.h"

#include "query_set.h"

#include <utility>
#include <vector>
#include <concurrent_vector.h>
#include <chrono>

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
    concurrency::concurrent_vector<combat_log_entry>    _table;
    std::chrono::high_resolution_clock::time_point      _last_update;

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
    void insert(const combat_log_entry& row_) {
        _table.push_back(row_);
        _last_update = std::chrono::high_resolution_clock::now();
    }

    template<typename DstType,typename U>
    query_set<concurrency::concurrent_vector<combat_log_entry>, DstType> select(U v_) {
        return select_from<DstType>( std::forward<U>( v_ ), _table );
    }

    std::chrono::high_resolution_clock::time_point timestamp() {
        return _last_update;
    }
};