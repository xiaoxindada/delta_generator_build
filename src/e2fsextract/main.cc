#include <filesystem>
#include <fmt/format.h>
#include <cstring>
#include "extract.h"
#include "version.h"

using namespace std;
namespace fs = std::filesystem;

extract_config_t extract_config;

void print_version(const char* p)
{
    auto current_time = std::chrono::system_clock::now();
    auto now = std::chrono::system_clock::to_time_t(current_time);
    auto local_time = std::localtime(&now);
    int32_t year = local_time->tm_year + 1900;
    int32_t month = local_time->tm_mon + 1;
    int32_t day = local_time->tm_mday;

    const char *e2fs_ver;
    const char *e2fs_date;
    ext2fs_get_library_version(&e2fs_ver, &e2fs_date);

     string date = fmt::format("{}-{}-{}", year, month, day);
     string version = fmt::format("{} version: {}.{} ({})", fs::path(p).filename().string(), VERSION, PATCHLEVEL, date);
     cout << version << endl;
     cout << "\te2fsprogs version: " << e2fs_ver << endl
          << "\te2fsprogs date: " << e2fs_date << endl;
     cout << "\tUsing " << error_message(EXT2_ET_BASE) << endl;
     exit(0);
}

void print_usage(const char* p)
{
    string s = "";
    s += fmt::format("{} <image> [<outdir>]", fs::path(p).filename().string());
    cout << s << endl;
    exit(0);
}

int main(int argc, char **argv)
{
    string file_path;
    string extract_dir;
    errcode_t ret = 0;
    errcode_t retval_csum = 0;
    extract_ctx ctx;

    initialize_ext2_error_table();

    if (argc < 2 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
    {
        print_usage(argv[0]);
    }
    if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))
    {
        print_version(argv[0]);
    }
    if (argc == 2)
    {
        file_path = fs::absolute(argv[1]).string();
        extract_dir = fs::absolute("out").string();
    }
    if (argc == 3)
    {
        file_path = fs::absolute(argv[1]).string();
        extract_dir = fs::absolute(argv[2]).string();
    }

    auto start_time = std::chrono::system_clock::now();
    init_extract_ctx(&ctx);
    int flags = EXT2_FLAG_SOFTSUPP_FEATURES | EXT2_FLAG_SKIP_MMP |
                EXT2_FLAG_64BITS | EXT2_FLAG_THREADS |
                EXT2_FLAG_EXCLUSIVE | EXT2_FLAG_THREADS |
                EXT2_FLAG_PRINT_PROGRESS | EXT2_FLAG_FORCE;

try_open_again:
    ret = ext2fs_open(
        file_path.c_str(),
        flags,
        0,
        0,
        default_io_manager,
        &ctx.fs);

    flags |= EXT2_FLAG_IGNORE_CSUM_ERRORS;
    if (ret && !retval_csum)
    {
        retval_csum = ret;
        goto try_open_again;
    }
    if (ret)
    {
        com_err(argv[0], ret, "while opening filesystem");
        return 1;
    }

#ifdef _WIN32
#ifdef WINDOWS_IO_MANAGER_USE_MMAP_READ
    if (ctx.fs && ctx.fs->io) {
        ret = ctx.fs->io->manager->set_option(ctx.fs->io, "mmap", "1");
        if (ret) {
            cout << "Warning: Failed to enable mmap: " << error_message(ret) << endl;
        } else {
            cout << "mmap acceleration enable" << endl;
        }
    }
#else
    cout << "mmap acceleration disable" << endl;
#endif
#endif

    auto xfile_name = fs::path(file_path).filename().string();
    if (xfile_name[xfile_name.length() - 4] == '.')
        xfile_name = xfile_name.substr(0, xfile_name.length() - 4);
    extract_config.volume_name = xfile_name;
    extract_config.extract_dir = extract_dir;
    extract_config.outdir = (fs::path(extract_config.extract_dir) / fs::path(extract_config.volume_name)).string();
    extract_config.config_dir = (fs::path(extract_config.extract_dir) / "config").string();
    auto volume_name = reinterpret_cast<char *>(ctx.fs->super->s_volume_name);
    
    cout << "Image volume name: " << volume_name << endl;
    cout << "Setup extract dir: " << extract_config.extract_dir << endl;
    cout << "Setup image dir: " << extract_config.outdir << endl;
    cout << "Setup config dir: " << extract_config.config_dir << endl;
    cout << "Record inode num: " << ctx.fs->super->s_inodes_count << endl;
    cout << "Record free inode num: " << ctx.fs->super->s_free_inodes_count << endl;
    cout << "Record used inode num: " << ctx.fs->super->s_inodes_count - ctx.fs->super->s_free_inodes_count << endl;

    // count how many file need to be extract
    extract_config.total_files = count_files_recursive(ctx.fs, EXT2_ROOT_INO);
    cout << "Extracting " << extract_config.total_files << " files begin ..." << endl;

    // Extract
    process_directory(ctx.parent_ino, &ctx);

    // configs
    // for (const auto& element : extract_config.configs) {
    //    cout << fmt::format("path: {}, uid: {}, gid: {}, mode: {:#4o}, is_symlink: {}, context: {}, capabilities: 0x{:x}",
    //        element.path, element.uid, element.gid, element.mode & 0777, element.is_symlink, element.context, element.capabilities
    //    ) << endl;
    //}
    extract_fs_config();
    extract_file_contexts();

    ext2fs_close(ctx.fs);

    auto end_time = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    cout << fmt::format("Tooks: {:.2f} seconds", duration.count() / 1000.0) << endl;

    return 0;
}
