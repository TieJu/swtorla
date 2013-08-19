#pragma once

#include <string>
#include <chrono>

extern "C" {
#include <unzip.h>
#include <zip.h>
#include <iowin32.h>
}

class archive_zip {
public:
    enum open_mode {
        read,
        write
    };

private:
    void*       _handle;
    open_mode   _mode;

public:
    archive_zip() : _handle(nullptr), _mode(open_mode::read) {}
    archive_zip(const std::wstring& path_, open_mode mode_, bool append_)
        : archive_zip() {
        open(path_, mode_, append_);
    }

    ~archive_zip() {
        close();
    }

    bool open(const std::wstring& path_, open_mode mode_, bool append_) {
        close();

        _mode = mode_;
        zlib_filefunc64_def clbs{};
        fill_win32_filefunc64W(&clbs);
        if ( mode_ == open_mode::read ) {
            _handle = unzOpen2_64(path_.c_str(), &clbs);
        } else {
            _handle = zipOpen2_64(path_.c_str(), append_ ? APPEND_STATUS_ADDINZIP : APPEND_STATUS_CREATE, nullptr, &clbs);
        }

        return is_open();
    }

    void close() {
        if ( is_open() ) {
            if ( _mode == open_mode::read ) {
                unzClose(_handle);
            } else {
                zipClose(_handle, nullptr);
            }
            _handle = nullptr;
        }
    }

    bool is_open() const {
        return _handle != nullptr;
    }

    bool select_file(const std::string& name_) {
        if ( _mode == open_mode::write ) {
            return false;
        }

        return unzLocateFile(_handle, name_.c_str(), 0) == UNZ_OK;
    }

    size_t read_file(void* buffer_, size_t max_bytes_) {
        auto result = unzReadCurrentFile(_handle, buffer_, max_bytes_);

        if ( result < 0 ) {
            return 0;
        }
        return result;
    }

    bool create_file(const std::string& name_,const std::chrono::system_clock::time_point& file_date_) {
        if ( _mode == open_mode::read ) {
            return false;
        }

        zip_fileinfo info{};
        
        auto tt = std::chrono::system_clock::to_time_t(file_date_);
        struct tm ti;
        localtime_s(&ti,&tt);

        info.tmz_date.tm_hour = ti.tm_hour;
        info.tmz_date.tm_min = ti.tm_min;
        info.tmz_date.tm_sec = ti.tm_sec;
        info.tmz_date.tm_mday = ti.tm_mday;
        info.tmz_date.tm_mon = ti.tm_mon;
        info.tmz_date.tm_year = ti.tm_year;

        return zipOpenNewFileInZip64(_handle, name_.c_str(), &info, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_BEST_COMPRESSION, 1) == ZIP_OK;
    }

    size_t write_file(const void* data_, size_t bytes_) {
        auto result = zipWriteInFileInZip(_handle, data_, bytes_);

        if ( result < 0 ) {
            return 0;
        }
        return result;
    }

    void close_file() {
        if ( _mode == open_mode::read ) {
            unzCloseCurrentFile(_handle);
        } else {
            zipCloseFileInZip(_handle);
        }
    }
};