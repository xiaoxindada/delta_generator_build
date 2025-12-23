#include <fstream>
#include <filesystem>
#include <fmt/format.h>
#include <sys/fcntl.h>
#include <vector>
#include "extract.h"
#include "symlink.h"

#ifndef _WIN32
#define _open open
#define _close close
#define _write write
#define _O_BINARY 0x800
#define _O_CREAT O_CREAT
#define _O_TRUNC O_TRUNC
#define _O_WRONLY O_WRONLY
#define _O_RDWR O_RDWR
#endif

namespace fs = std::filesystem;

static inline fs::path ensure_parent_dir(const std::string &file_path)
{
    fs::path full_path = fs::absolute(extract_config.outdir);
    std::string file = file_path;
    if (file.at(0) == '/')
    { // start with abs path
        file = file_path.substr(1, file_path.length());
    }
    full_path = full_path / file;

    // std::cerr << "Ensure parent dir: " << full_path << std::endl;

    if (!fs::exists(full_path.parent_path()))
    {
        if (!fs::create_directories(full_path.parent_path()))
        {
            std::cerr << "Error: Could not create directory: " << full_path.parent_path().string() << std::endl;
        }
    }

    return full_path.make_preferred();
}

#ifdef _WIN32
static bool setSystemAttribute(const std::wstring &filePath)
{
    DWORD attrs = GetFileAttributesW(filePath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES)
    {
        std::cerr << "Error: Could not get file attribute" << std::endl;
        return false;
    }

    if (!SetFileAttributesW(filePath.c_str(), attrs | FILE_ATTRIBUTE_SYSTEM))
    {
        std::cerr << "Error: Set system attrib to symlink failed" << std::endl;
        return false;
    }

    return true;
}

std::vector<uint8_t> stringToUtf16Le(const std::string &utf8_str, bool include_bom = true)
{
    std::vector<uint8_t> result;

    if (include_bom)
    {
        result.push_back(0xFF);
        result.push_back(0xFE);
    }

    if (utf8_str.empty())
    {
        return result;
    }

    int wide_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if (wide_len == 0)
    {
        return result;
    }

    std::vector<wchar_t> wide_buffer(wide_len);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, wide_buffer.data(), wide_len);

    for (size_t i = 0; i < (size_t)wide_len - 1; ++i)
    {
        wchar_t c = wide_buffer[i];
        result.push_back(static_cast<uint8_t>(c & 0xFF));
        result.push_back(static_cast<uint8_t>((c >> 8) & 0xFF));
    }

    result.push_back(0x00);
    result.push_back(0x00);

    return result;
}
#endif

int xsymlink(const std::string link_target, const std::string path)
{
    auto full_path = ensure_parent_dir(path);

#ifdef _WIN32
    std::ofstream file(full_path, std::ios::out | std::ios::binary);

    auto fd = _open(full_path.string().c_str(), _O_BINARY | _O_TRUNC | _O_WRONLY | _O_CREAT, 0644);
    if (fd < 0)
        return -1;

    auto utf16le_buf = stringToUtf16Le(link_target);

    _write(fd, cygwin_symlink_magic, sizeof(cygwin_symlink_magic));
    _write(fd, utf16le_buf.data(), utf16le_buf.size());

    if (fd > 0)
        _close(fd);

    if (!setSystemAttribute(full_path.wstring()))
        std::cerr << "Error: Could not set system attrib on file:" << full_path << std::endl;
#else
    if (fs::is_symlink(full_path) || fs::exists(full_path))
    {
        if (!fs::remove(full_path))
        {
            std::cerr << "Could not remove exist symlink path: " << full_path << std::endl;
            return 1;
        }
    }
    fs::create_symlink(link_target, full_path);
#endif

    return 0; // always
}

void init_extract_ctx(extract_ctx *ctx)
{
    ctx->parent_ino = EXT2_ROOT_INO;
}

/*
    EXTRACT FUNCTIONS
*/
EXTRACT_FUNC(regular_file)
{
    auto full_path = ensure_parent_dir(path);
    errcode_t err = 0;
    ext2_file_t file;
    char *buf = nullptr;
    ext2_off_t pos = 0;
    uint32_t read_size;
    int fd = -1;
    char *mp = static_cast<char *>(MAP_FAILED);

    err = ext2fs_file_open(ctx->fs, ino, 0, &file);
    if (err)
    {
        com_err("ext2fs_file_open", err, nullptr);
        return 1;
    }

    const auto fsize = ext2fs_file_get_size(file);

    if (fsize == 0)
    {
        auto fd = _open(full_path.string().c_str(), _O_CREAT | _O_BINARY | _O_TRUNC | _O_WRONLY, 0644);
        if (fd > 0)
            close(fd);
        else
            std::cerr << "Error: Could not create empty file" << std::endl;
        return 0;
    }

    fd = _open(full_path.string().c_str(),
               _O_BINARY | _O_CREAT | _O_RDWR | _O_TRUNC,
               0644);
    if (fd < 0)
    {
        std::cerr << "Error: Could not open file:" << full_path.string() << std::endl;
        goto extract_file_out;
    }
    #ifdef _WIN32
    _chsize(fd, fsize);
    #else
    ftruncate(fd, fsize);
    #endif

    if (fsize < EXTRACT_DIRECT_FILE_LIMIT)
    { // direct extract small file
extract_file_direct:
        buf = (char *)malloc(fsize);
        if (!buf)
        {
            std::cerr << "Error: Could not alloc memory" << std::endl;
            goto extract_file_out;
        }
        err = ext2fs_file_read(file, buf, fsize, &read_size);
        if (err && err != EXT2_ET_SHORT_READ)
        {
            com_err("ext2fs_file_read", err, NULL);
            goto extract_file_out;
        }
        _write(fd, buf, fsize);
    }
    else
    {
        mp = (char *)mmap(nullptr, fsize, PROT_WRITE, MAP_SHARED, fd, 0);
        if (mp == MAP_FAILED)
        {
            std::cerr << "Error: Map file failed." << std::endl;
            goto extract_file_direct; // if failed mmap file use regular way to extract
        }

        do
        {
            err = ext2fs_file_read(file, mp + pos, ctx->fs->blocksize << 2, &read_size);
            if (err && err != EXT2_ET_SHORT_READ)
            {
                com_err("ext2fs_file_read", err, NULL);
                break;
            }

            if (read_size > 0)
            {
                // memcpy(mp, buf, read_size);
                pos += read_size;
            }
            else
            {
                break;
            }
        } while (pos < fsize);
    }

extract_file_out:
    if (buf)
        free(buf);
    if (mp != MAP_FAILED)
        munmap(mp, fsize);
    if (fd > 0)
        _close(fd);
    err = ext2fs_file_close(file);
    if (err)
    {
        com_err("ext2fs_file_close", err, nullptr);
        return 1;
    }

    return 0;
}

EXTRACT_FUNC(directory)
{
    auto full_path = ensure_parent_dir(path);

    if (!(access(full_path.string().c_str(), F_OK) == 0) && !(_mkdir(full_path.string().c_str(), 0777) == 0))
    {
        std::cerr << "Error: could not create directory: " << full_path << std::endl;
        return 1;
    }
#if 0
    if (!(chmod(full_path.c_str(), inode->i_mode & 0777))) {
        std::cerr << "Error: could not set mode at path: " << full_path << std::endl;
    }
    // chown
#endif
    return 0;
}

// TODO: Add hardlink extract
int extract_link(const std::string path, const std::string link_target)
{
    auto full_path = ensure_parent_dir(path);

    return xsymlink(link_target, path);
}

std::string escape_replace(const std::string &input)
{
    static const std::vector<char> specialChars = {
        '.', '^', '$', '*', '+', '?', '(', ')',
        '[', ']', '{', '}', '|', '\\', '<', '>'};

    std::string result;
    for (char c : input)
    {
        if (std::find(specialChars.begin(), specialChars.end(), c) != specialChars.end())
        {
            result += "\\";
        }
        result += c;
    }

    return result;
}

int extract_fs_config()
{
    auto full_path = fs::absolute(
        fs::path(extract_config.config_dir) /
        fmt::format("{}_fs_config",
                    extract_config.volume_name));

    if (!fs::exists(full_path.parent_path()))
    {
        fs::create_directories(full_path.parent_path());
    }

    std::ofstream file(full_path, std::ios::binary);

    for (const auto &element : extract_config.configs)
    {
        if (element.path == "/")
        {
            auto root = fmt::format("/ {:d} {:d} {:#4o}\n"
                                    "{} {:d} {:d} {:#4o}\n",
                                    element.uid, element.gid, element.mode & 0777,
                                    extract_config.volume_name, element.uid, extract_config.volume_name == "vendor" ? 2000 : element.gid, element.mode & 0777);
            file.write(root.c_str(), root.size());
            continue;
        }
        auto common = fmt::format("{}{} {:d} {:d} {:#4o}", extract_config.volume_name, element.path, element.uid, element.gid, element.mode & 0777);

        if (element.is_symlink)
            common += fmt::format(" {}", element.symlink);

        if (element.have_capabilities)
            common += fmt::format(" capabilities=0x{:x}", element.capabilities);

        common += "\n";
        file.write(common.c_str(), common.size());
    }

    std::cout << "Extract fs_config at: " << full_path.string() << std::endl;

    return 0;
}

int extract_file_contexts()
{
    auto full_path = fs::absolute(
        fs::path(extract_config.config_dir) /
        fmt::format("{}_file_contexts",
                    extract_config.volume_name));

    if (!fs::exists(full_path.parent_path()))
    {
        fs::create_directories(full_path.parent_path());
    }

    std::ofstream file(full_path, std::ios::binary);

    for (const auto &element : extract_config.configs)
    {
        if (element.have_context)
        {
            if (element.path == "/")
            {
                auto root = fmt::format("/ {}\n"
                                        "/{} {}\n"
                                        "/{}(/.*)? {}\n",
                                        element.context,
                                        extract_config.volume_name, element.context,
                                        extract_config.volume_name, element.context);
                file.write(root.c_str(), root.size());
                continue;
            }
            auto common = fmt::format("/{}{} {}\n", extract_config.volume_name, escape_replace(element.path), element.context);
            file.write(common.c_str(), common.size());
        }
    }

    std::cout << "Extract file_contexts at: " << full_path.string() << std::endl;

    return 0;
}