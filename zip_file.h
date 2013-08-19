#pragma once

#include "handle_wrap.h"

#include <string>
#include <unordered_map>

#include <boost/optional.hpp>

#include <Windows.h>

class read_memory_range {
    char*   _at;
    size_t  _size;
    bool    _owned;

    read_memory_range& operator=(const read_memory_range&);
    read_memory_range(const read_memory_range&);

public:
    read_memory_range(read_memory_range&& other_)
        : read_memory_range() {
        *this = std::move(other_);
    }

    read_memory_range()
        : read_memory_range(nullptr, 0, false) {
    }

    read_memory_range(void* ptr_, size_t size_, bool owned_)
        : _at(reinterpret_cast<char*>( ptr_ ))
        , _size(size_)
        , _owned(owned_) {
    }

    ~read_memory_range() {
        if ( _at && _owned ) {
            delete[] _at;
        }
        _at = nullptr;
        _owned = 0;
        _owned = false;
    }

    read_memory_range& operator=(read_memory_range&& other_) {
        std::swap(other_._at, _at);
        std::swap(other_._size, _size);
        std::swap(other_._owned, _owned);
        return *this;
    }

    bool empty() const { return _at == nullptr; }
    size_t size() const { return _size; }
    char operator[](size_t index_) const { return _at[index_]; }
    char& operator[](size_t index_) { return _at[index_]; }
    void* data() { return _at; }
    const void* data() const { return _at; }
};

enum class zip_compression_methods {
    store,
    shrunk,
    compress_1,
    compress_2,
    compress_3,
    compress_4,
    implode,
    tokenized_reserved,
    deflate,
    deflate_64,
    old_ibm_terse,
    bzip2 = 12,
    lzma_efsm = 14,
    ibm_terse = 18,
    ibm_lz77_z_pfs,
    wav_pack = 97,
    ppmd_v_1_r_1 = 98,
};

class zip_file;
class ziped_file {
protected:
    friend class zip_file;
    size_t                  _header_offset;
    size_t                  _directory_offset;
    zip_file*               _archive;
    zip_compression_methods _method;
    std::vector<char>       _write_buffer;

    void init_from_zip() {}

    ziped_file(zip_file* archive_, size_t header_offset_, size_t directory_offset_)
        : _archive(archive_)
        , _header_offset(header_offset_)
        , _directory_offset(directory_offset_) {
        init_from_zip();
    }

    ziped_file()
        : ziped_file(nullptr, size_t(-1), size_t(-1)) {
    }

public:
    bool is_open() const { return _header_offset != size_t(-1); }
    zip_compression_methods compression() const { return _method; }
    void compression(zip_compression_methods method_) { _method = method_; }
    read_memory_range read(size_t offset_, size_t size_);
    std::tuple<void *,size_t> read(size_t offset_, void* buffer_, size_t size_);
    void* read_or_throw(size_t offset_, void* buffer_, size_t size_);
    void write(size_t offset_, const void* buffer_, size_t size_);
};

enum class zip_file_open_mode {
    undefined = 0,
    read = 1,
    write = 2,
    truncate = 4,
    allow_maping = 8,
};

inline zip_file_open_mode operator |( zip_file_open_mode lhs_, zip_file_open_mode rhs_ ) {
    return zip_file_open_mode(unsigned( lhs_ ) | unsigned( rhs_ ));
}

inline zip_file_open_mode operator &( zip_file_open_mode lhs_, zip_file_open_mode rhs_ ) {
    return zip_file_open_mode(unsigned( lhs_ ) & unsigned( rhs_ ));
}

inline zip_file_open_mode operator ^( zip_file_open_mode lhs_, zip_file_open_mode rhs_ ) {
    return zip_file_open_mode(unsigned( lhs_ ) ^ unsigned( rhs_ ));
}

class zip_file {
protected:
    friend class ziped_file;
#pragma pack(push)
#pragma pack(1)
    struct file_hader {
        unsigned long   magic;
        unsigned short  version;
        unsigned short  flags;
        unsigned short  method;
        unsigned short  mod_time;
        unsigned short  mod_date;
        unsigned long   crc32;
        unsigned long   compressed_size;
        unsigned long   uncompressed_size;
        unsigned short  name_length;
        unsigned short  extra_length;
    };

    struct data_descriptor {
        unsigned long   crc32;
        unsigned long   compressed_size;
        unsigned long   uncompressed_size;
    };

    struct data_descriptor_64 {
        unsigned long       crc32;
        unsigned long long  compressed_size;
        unsigned long long  uncompressed_size;
    };

    struct data_descriptor_magic {
        unsigned long   magic;
        data_descriptor desc;
    };

    struct data_descriptor_64_magic {
        unsigned long       magic;
        data_descriptor_64  desc;
    };

    struct central_dircory_file_header {
        unsigned long   magic;
        unsigned short  by;
        unsigned short  version;
        unsigned short  flags;
        unsigned short  method;
        unsigned short  mod_time;
        unsigned short  mod_date;
        unsigned long   crc32;
        unsigned long   compressed_size;
        unsigned long   uncompressed_size;
        unsigned short  name_length;
        unsigned short  extra_length;
        unsigned short  comment_length;
        unsigned short  disc_index;
        unsigned short  internal_attributes;
        unsigned long   external_attributes;
        unsigned long   relative_offset;
    };

    struct end_central_direcorty {
        unsigned long   magic;
        unsigned short  disc_id;
        unsigned short  dir_disc_id;
        unsigned short  disc_dir_records;
        unsigned short  total_dir_records;
        unsigned long   dir_size;
        unsigned long   dir_offset;
        unsigned short  comment_length;
    };
#pragma pack(pop)
    enum file_header_flags : short {
        data_discriptor_present = 0x0080
    };

    enum zip_magics {
        file_header_magic = 0x04034b50,

        central_directory_magic = 0x02014b50,

        end_central_directory_magic = 0x06054b50,
        end_central_directory_64_magic = 0x06064b50,
    };

    struct offset_info {
        size_t  file_header_offset;
        size_t  directory_header_offset;
    };

    std::vector<std::pair<std::string, offset_info>>_name_to_offset;
    handle_wrap<HANDLE, INVALID_HANDLE_VALUE>       _file;
    zip_file_open_mode                              _mode;

    bool inspect_file();
    size_t find_central_directory_end(size_t offset_);
    bool verify_central_directory_end_at(size_t offset_, size_t file_size_);
    bool verify_central_directory_end_64_at(size_t offset_, size_t file_size_);
    bool load_central_directory(size_t offset_of_end_);
    bool load_central_directory_at(size_t offset_to_directory_,size_t size_of_directory_);
    boost::optional<unsigned char> read_byte(size_t offset_);
    boost::optional<unsigned short> read_2bytes(size_t offset_);
    boost::optional<unsigned long> read_4bytes(size_t offset_);
    // returned pointer may be a pointer to a memory region 
    read_memory_range read(size_t offset_, size_t bytes_);
    read_memory_range read_or_throw(size_t offset_, size_t bytes_);
    std::tuple<void*, size_t> read(size_t offset_, void* buffer_, size_t bytes_);
    void* read_or_throw(size_t offset_, void* buffer_, size_t bytes_);
    bool write_byte(size_t offset_, unsigned char data_);
    bool write_2bytes(size_t offset_, unsigned short data_);
    bool write_4bytes(size_t offset_, unsigned long data_);
    size_t write(size_t offset_, const void* buffer_, size_t bytes_);


public:
    bool is_open() const;
    bool open(const std::string& name_, zip_file_open_mode mode_);
    zip_file_open_mode mode() const;
    void close();

    ziped_file open_ziped(const std::string& name_);
    void delete_ziped(const std::string& name_);
    bool check_ziped_exist(const std::string& name_) const;
    ziped_file open_ziped_by_index(size_t index_);
    const std::string& ziped_name_by_index(size_t index_) const;
    size_t count_ziped() const;
};