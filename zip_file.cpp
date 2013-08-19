#include "zip_file.h"

#include <memory>
#include <boost/crc.hpp>

bool zip_file::inspect_file() {
    try {
        auto offset = find_central_directory_end(size_t(-1));

        if ( !load_central_directory(offset) ) {
            return false;
        }

    } catch ( const std::runtime_error& ) {
        return false;
    }
    return true;
}

size_t zip_file::find_central_directory_end(size_t offset_) {
    size_t size;
    if ( FALSE == ::GetFileSizeEx(_file.get(), reinterpret_cast<LARGE_INTEGER*>( &size )) ) {
        throw std::runtime_error("GetFileSizeEx failed");
    }

    size_t start = size - sizeof( end_central_direcorty );
    if ( offset_ < start ) {
        start = offset_;
    }

    decltype(read_4bytes(0)) magic;
    for ( ; start != 0; --start ) {
        magic = read_4bytes(start);
        if ( !magic ) {
            throw std::runtime_error("Unable to read next 4 bytes");
        }
        auto m = *magic;

        if ( *magic == end_central_directory_magic ) {
            if ( verify_central_directory_end_at(start, size) ) {
                break;
            }
            magic.reset();
        }/* else if ( *magic == end_central_directory_64_magic ) {
            if ( verify_central_directory_end_64_at(start, size) ) {
                break;
            }
            magic.reset();
        }*/
    }

    if ( !magic ) {
        throw std::runtime_error("Unable to locate canetral direcotry header");
    }

    return start;
}

bool zip_file::verify_central_directory_end_at(size_t offset_, size_t file_size_) {
    end_central_direcorty buffer;
    // note using the return value of read instead of buffer, because if the file
    // is memory mapped, read returns the pointer to the memory in the file at
    // offset_ and does not copy the contents into buffer!
    auto cdir = reinterpret_cast<const end_central_direcorty*>( read_or_throw(offset_, &buffer, sizeof( buffer )) );

    if ( cdir->magic != end_central_directory_magic ) {
        // sanity check
        return false;
    }

    auto comment_room = file_size_ - offset_ - sizeof( buffer );
    return comment_room == cdir->comment_length;
}

bool zip_file::verify_central_directory_end_64_at(size_t offset_, size_t file_size_) {
    // zip64 not supported yet
    return false;
}

bool zip_file::load_central_directory(size_t offset_of_end_) {
    auto magic = read_4bytes(offset_of_end_);
    if ( *magic == end_central_directory_magic ) {
        end_central_direcorty buffer;
        // note using the return value of read instead of buffer, because if the file
        // is memory mapped, read returns the pointer to the memory in the file at
        // offset_ and does not copy the contents into buffer!
        auto cdir = reinterpret_cast<const end_central_direcorty*>( read_or_throw(offset_of_end_, &buffer, sizeof( buffer )) );

        if ( cdir->disc_dir_records != cdir->total_dir_records ) {
            throw std::runtime_error("Multidisc mode not supported yet");
        }

        return load_central_directory_at(cdir->dir_offset, cdir->dir_size);
    } else if ( *magic == end_central_directory_64_magic ) {
        return false;
    }
}

bool zip_file::load_central_directory_at(size_t offset_to_directory_, size_t size_of_directory_) {
    size_t pos = 0;
    central_dircory_file_header buffer;

    std::vector<char> name_buffer;
    while ( pos < size_of_directory_ ) {
        auto header = reinterpret_cast<const central_dircory_file_header*>( read_or_throw(pos + offset_to_directory_, &buffer, sizeof( buffer )) );

        if ( header->magic != central_directory_magic ) {
            throw std::runtime_error("Invalid centrla directory entry found");
        }

        pos += sizeof( central_dircory_file_header );

        name_buffer.resize(header->name_length);
        auto name_start = reinterpret_cast<const char*>( read_or_throw(pos + offset_to_directory_, name_buffer.data(), header->name_length) );
        auto name_end = name_start + header->name_length;
        offset_info oi;
        oi.directory_header_offset = offset_to_directory_ + pos - sizeof( central_dircory_file_header );
        oi.file_header_offset = header->relative_offset;
        _name_to_offset.push_back(std::make_pair(std::string(begin(name_buffer), end(name_buffer)), oi));

        pos += header->name_length;
        pos += header->extra_length;
        pos += header->comment_length;
    }
}

boost::optional<unsigned char> zip_file::read_byte(size_t offset_) {
    try {
        unsigned char buf;
        return *reinterpret_cast<unsigned char*>( read_or_throw(offset_, &buf, 1) );
    } catch ( const std::exception& ) {
    }
    return boost::optional<unsigned char>{};
}

boost::optional<unsigned short> zip_file::read_2bytes(size_t offset_) {
    try {
        unsigned short buf;
        return *reinterpret_cast<unsigned short*>( read_or_throw(offset_, &buf, 2) );
    } catch ( const std::exception& ) {
    }
    return boost::optional<unsigned short>{};
}

boost::optional<unsigned long> zip_file::read_4bytes(size_t offset_) {
    try {
        unsigned long buf;
        return *reinterpret_cast<unsigned long*>( read_or_throw(offset_, &buf, 4) );
    } catch ( const std::exception& ) {
    }
    return boost::optional<unsigned long>{};
}

read_memory_range zip_file::read(size_t offset_, size_t bytes_) {
    std::unique_ptr<char[]> memory(new char[bytes_]);
    DWORD to_read = bytes_;
    DWORD do_read;
    ::SetFilePointerEx(_file.get(), *reinterpret_cast<const LARGE_INTEGER*>( &offset_ ), nullptr, FILE_BEGIN);
    ::ReadFile(_file.get(), memory.get(), to_read, &do_read, nullptr);
    return read_memory_range(memory.release(), do_read, true);
}

read_memory_range zip_file::read_or_throw(size_t offset_, size_t bytes_) {
    std::unique_ptr<char[]> memory(new char [bytes_]);
    DWORD to_read = bytes_;
    DWORD do_read;
    ::SetFilePointerEx(_file.get(), *reinterpret_cast<const LARGE_INTEGER*>( &offset_ ), nullptr, FILE_BEGIN);
    ::ReadFile(_file.get(), memory.get(), to_read, &do_read, nullptr);
    if ( do_read != to_read ) {
        throw std::runtime_error("unable to read requested bytes");
    }
    return read_memory_range(memory.release(),to_read,true);
}

std::tuple<void*, size_t> zip_file::read(size_t offset_, void* buffer_, size_t bytes_) {
    DWORD to_read = bytes_;
    DWORD do_read;
    ::SetFilePointerEx(_file.get(), *reinterpret_cast<const LARGE_INTEGER*>( &offset_ ), nullptr, FILE_BEGIN);
    ::ReadFile(_file.get(), buffer_, to_read, &do_read, nullptr);
    return std::make_tuple(buffer_, do_read);
}

void* zip_file::read_or_throw(size_t offset_, void* buffer_, size_t bytes_) {
    DWORD to_read = bytes_;
    DWORD do_read;
    ::SetFilePointerEx(_file.get(), *reinterpret_cast<const LARGE_INTEGER*>( &offset_ ), nullptr, FILE_BEGIN);
    ::ReadFile(_file.get(), buffer_, to_read, &do_read, nullptr);
    if ( do_read != to_read ) {
        throw std::runtime_error("unable to read requested bytes");
    }
    return buffer_;
}

bool zip_file::write_byte(size_t offset_, unsigned char data_) {
    return sizeof( data_ ) == write(offset_, &data_, sizeof( data_ ));
}

bool zip_file::write_2bytes(size_t offset_, unsigned short data_) {
    return sizeof( data_ ) == write(offset_, &data_, sizeof( data_ ));
}

bool zip_file::write_4bytes(size_t offset_, unsigned long data_) {
    return sizeof( data_ ) == write(offset_, &data_, sizeof( data_ ));
}

size_t zip_file::write(size_t offset_, const void* buffer_, size_t bytes_) {
    DWORD to_write = bytes_;
    DWORD do_write;
    ::SetFilePointerEx(_file.get(), *reinterpret_cast<const LARGE_INTEGER*>( &offset_ ), nullptr, FILE_BEGIN);
    ::WriteFile(_file.get(), buffer_, to_write, &do_write, nullptr);
    return do_write;
}

bool zip_file::is_open() const {
    return !_file.empty();
}

bool zip_file::open(const std::string& name_, zip_file_open_mode mode_) {
    close();

    _mode = mode_;

    DWORD access = 0;
    DWORD share = 0;
    DWORD create = OPEN_ALWAYS;
    if ( ( mode_ & zip_file_open_mode::read ) == zip_file_open_mode::read ) {
        access |= GENERIC_READ;
    }
    if ( ( mode_ & zip_file_open_mode::write ) == zip_file_open_mode::write ) {
        access |= GENERIC_WRITE;
    }
    if ( ( mode_ & zip_file_open_mode::truncate ) == zip_file_open_mode::truncate ) {
        create = CREATE_ALWAYS;
    }
    _file.reset(::CreateFileA(name_.c_str(), access, share, nullptr, create, FILE_ATTRIBUTE_NORMAL, nullptr)
               ,[=](HANDLE handle_) { ::CloseHandle(handle_); });

    return is_open()
        && inspect_file();
}

zip_file_open_mode zip_file::mode() const {
    return _mode;
}

void zip_file::close() {
    _name_to_offset.clear();
    _file.reset();
}

ziped_file zip_file::open_ziped(const std::string& name_) {
    if ( ( _mode & zip_file_open_mode::read ) == zip_file_open_mode::read ) {
        auto at = std::find_if(begin(_name_to_offset), end(_name_to_offset), [=](const std::pair<std::string, offset_info>& info_) {
            return info_.first == name_;
        });
        if ( at != end(_name_to_offset) ) {
            return ziped_file{ this, at->second.file_header_offset, at->second.directory_header_offset };
        }
    }
    return ziped_file{};
}

void zip_file::delete_ziped(const std::string& name_) {
    if ( ( _mode & zip_file_open_mode::write ) != zip_file_open_mode::write ) {
        return;
    }

    auto at = std::find_if(begin(_name_to_offset), end(_name_to_offset), [=](const std::pair<std::string, offset_info>& info_) {
        return info_.first == name_;
    });
    if ( at != end(_name_to_offset) ) {
        _name_to_offset.erase(at);
    }
}

bool zip_file::check_ziped_exist(const std::string& name_) const {
    return end(_name_to_offset) != std::find_if(begin(_name_to_offset), end(_name_to_offset), [=](const std::pair<std::string, offset_info>& info_) {
        return info_.first == name_;
    });
}

ziped_file zip_file::open_ziped_by_index(size_t index_) {
    if ( index_ >= _name_to_offset.size() ) {
        return ziped_file{};
    }
    return ziped_file{ this, _name_to_offset[index_].second.file_header_offset, _name_to_offset[index_].second.directory_header_offset };
}

const std::string& zip_file::ziped_name_by_index(size_t index_) const {
    if ( index_ >= _name_to_offset.size() ) {
        throw std::runtime_error("index out of range");
    }
    return _name_to_offset[index_].first;
}

size_t zip_file::count_ziped() const {
    return _name_to_offset.size();
}