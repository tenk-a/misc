/**
 *  @file   redate_dir.cpp
 *  @brief  Update all directory timestamps to the latest file/directory time below them.
 *  @author Masashi Kitamura (tenka@6809.net)
 *  @license Boost software license Version 1.0
 */
#include <filesystem>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <clocale>

#if defined(_WIN32)
#include <windows.h>
#undef min
#endif

using namespace std;
namespace fs   = std::filesystem;
using FileTime = fs::file_time_type;

#if !defined(_WIN32)
#define PATH_TO_STRZ(p)     ((p).c_str())
#define STRZ_TO_PATH(s)     std::path(s)
#else   // for windows
#define PATH_TO_STRZ(p)     (reinterpret_cast<char const*>((p).u8string().c_str()))
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
#define STRZ_TO_PATH(s)     fs::path(reinterpret_cast<char8_t const*>(s))
#define U8_STRING           std::u8string
#else
#define STRZ_TO_PATH(s)     fs::path(reinterpret_cast<char const*>(s))
#define U8_STRING           std::string
#endif
#endif


#if defined(_WIN32)
struct ConsoleOutputCP {
    ConsoleOutputCP(int cp = 65001) {
        save_cp_ = GetConsoleOutputCP();
        SetConsoleOutputCP(cp);
    }
    ~ConsoleOutputCP() {
        SetConsoleOutputCP(save_cp_);
    }
private:
    int     save_cp_;
};
#endif


struct App {
    enum { Ok = 0, Er = 1 };

    static int usage() {
        printf(
            "usage>redate_dir [options] directory_path\n"
            "  Update all directory timestamps to the latest file/directory time below them.\n"
            "  -t[hh:mm:ss]   Ignore files newer than (now + hh:mm:ss).\n"
            "                 You may specify as -t[mm:ss] or -t[ss] as well.\n"
            "  -v             verbose.\n"
        );
        return Er;
    }

    int main(int argc, char* argv[]) {
        fs::path    target_dir;

        setlocale(LC_ALL, "");

        for (int i = 1; i < argc; ++i) {
            char* arg = argv[i];
            if (*arg == '-') {
                if (checkOpt(arg, "-t")) {
                    if (!parseTimeOut(arg, offset_sec_)) {
                        fprintf(stderr, "Invalid -t time: %s\n", arg);
                        return Er;
                    }
                } else if (checkOpt(arg, "-v")) {
                    verbose_ = *arg != '-';
                } else {
                    return usage();
                }
            } else {
                target_dir = STRZ_TO_PATH(arg);
            }
        }

        if (target_dir.empty()) {
            return usage();
        }

        if (!fs::exists(target_dir) || !fs::is_directory(target_dir)) {
            fprintf(stderr, "Directory not found: %s\n", PATH_TO_STRZ(target_dir));
            return Er;
        }

        max_time_ = FileTime::clock::now() + std::chrono::seconds(offset_sec_);
        updateLatestTime(target_dir);

        printf("Done. Processed %zu files, %zu directories.\n", file_count_, dir_count_);
        return Ok;
    }

private:
    static bool checkOpt(char* &arg, char const* opt) noexcept {
        size_t opt_len = char_traits<char>::length(arg);
        if (memcmp(arg, opt, opt_len) == 0) {
            arg += opt_len;
            if (*(char const*)arg == '=')
                ++arg;
            return true;
        }
        return false;
    }

    FileTime updateLatestTime(fs::path const& dir) {
        FileTime latest_time = FileTime::min();

        for (auto& entry : fs::directory_iterator(dir)) {
            auto& fpath = entry.path();
            if (fpath == "." || fpath == "..") {
                continue;
            }
            try {
                if (entry.is_directory()) {
                    FileTime sub_latest = updateLatestTime(fpath);
                    if (latest_time < sub_latest) {
                        latest_time = sub_latest;
                    }
                    ++dir_count_;
                } else if (entry.is_regular_file()) {
                    FileTime t = entry.last_write_time();
                    if (t >= max_time_) {
                        max_time_   = FileTime::clock::now() + std::chrono::seconds(offset_sec_);
                    }
                    if (t <= max_time_) {
                        if (latest_time < t)
                            latest_time = t;
                    }
                    ++file_count_;
                }
            } catch (std::exception& e) {
                fprintf(stderr, "Warning: %s: %s\n", PATH_TO_STRZ(fpath), e.what());
            }
        }

        if (latest_time > FileTime::min()) {
            FileTime old_time = fs::last_write_time(dir);
            if (old_time != latest_time && latest_time <= max_time_) {
                try {
                    fs::last_write_time(dir, latest_time);
                    if (verbose_) {
                     #if 1
                        printf("Updated: %s\n", PATH_TO_STRZ(dir));
                     #else
                        printf("Updated: %s (%llu -> %llu)\n",
                            PATH_TO_STRZ(dir),
                            static_cast<long long>(old_time.time_since_epoch().count()),
                            static_cast<long long>(latest_time.time_since_epoch().count())
                        );
                     #endif
                    }
                } catch (std::exception& e) {
                    fprintf(stderr, "Failed to update %s: %s\n", PATH_TO_STRZ(dir), e.what());
                }
            }
        }

        return latest_time;
    }

    static bool parseTimeOut(const char* arg, ptrdiff_t& out_seconds) noexcept {
        int         parts[3] = {0};
        int         h = 0, m = 0, s = 0;
        int         n = 0;
        char const* p = arg;
        while (*p && n < 3) {
            parts[n++] = std::strtol(p, const_cast<char**>(&p), 10);
            if (*p == ':')
                ++p;
            else
                break;
        }
        if (n == 3) {
            h = parts[0];
            m = parts[1];
            s = parts[2];
        } else if (n == 2) {
            m = parts[0];
            s = parts[1];
        } else if (n == 1) {
            s = parts[0];
        } else {
            return false;
        }
        out_seconds = h*3600 + m*60 + s;
        return true;
    }

private:
    FileTime    max_time_;
    ptrdiff_t   offset_sec_ = 60;
    size_t      file_count_ = 0;
    size_t      dir_count_  = 0;

    bool        verbose_    = false;
};


#if !defined(_WIN32)
int main(int argc, char* argv[]) {
    return App().main(argc, argv);
}
#elif defined(FOR_WIN10_1903_OR_LATER)
int main(int argc, char* argv[]) {
    ConsoleOutputCP utf8console(65001);
    return App().main(argc, argv);
}
#else
int wmain(int argc, wchar_t* argv[]) {
    ConsoleOutputCP         utf8console(65001);
    std::vector<char*>      cargv(argc+1);
    std::vector<U8_STRING>  sargv;
    sargv.reserve(argc+1);
    for (size_t i = 0; i < argc; ++i) {
        sargv.emplace_back(fs::path(argv[i]).u8string());
        cargv[i] = reinterpret_cast<char*>(&sargv.back()[0]);
    }
    return App().main(argc, &cargv[0]);
}
#endif
