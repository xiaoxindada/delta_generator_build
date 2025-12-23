#ifndef __EXT2FS_EXTRACT_H
#define __EXT2FS_EXTRACT_H

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <cerrno>
#include <config.h>
#include <ext2fs/ext2fs.h>
#include <ext2fs/ext2_err.h>
#include <ext2fs/ext2_io.h>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef __MINGW32__
#define __USE_MINGW_ANSI_STDIO 0
#endif

#ifdef _WIN32
#include <windows.h>
#include <mman.h>
#include <io.h>
#else
#include <sys/mman.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#else
#include <sys/io.h>
#endif
#endif

typedef struct
{
    ext2_filsys fs;
    ext2_ino_t parent_ino;

} extract_ctx;

struct CountContext
{
    uint32_t count = 0;
    ext2_filsys fs;
};

struct config_ctx
{
    std::string path;

    uid_t uid;
    gid_t gid;
    mode_t mode;

    bool is_symlink = false;
    std::string symlink = "";

    bool have_capabilities = false;
    uint64_t capabilities = 0;

    bool have_context = false;
    std::string context = "";
};

struct extract_config_t
{
    std::string outdir;
    std::string extract_dir;
    std::string config_dir;
    std::string volume_name;
    int32_t total_files = 0;
    int32_t processed_files = 0;
    int32_t last_percent = -1;
    std::vector<config_ctx> configs;
};
extern extract_config_t extract_config;

// capabilities
struct vfs_cap_data
{
    uint32_t magic_etc;
    struct
    {
        uint32_t permitted;
        uint32_t inheritable;
    } data[2];
};

void init_extract_ctx(extract_ctx *ctx);
int process_directory(ext2_ino_t dir_ino, void *ctx);
uint32_t count_files_recursive(ext2_filsys fs, ext2_ino_t dir_ino, CountContext *ctx = nullptr);
void progress_plus();

int extract_fs_config();
int extract_file_contexts();

#define EXTRACT_FUNC(name)                         \
    int extract_##name(                            \
        [[maybe_unused]] ext2_ino_t ino,           \
        [[maybe_unused]] struct ext2_inode *inode, \
        const std::string path,                    \
        [[maybe_unused]] extract_ctx *ctx)

EXTRACT_FUNC(regular_file);
EXTRACT_FUNC(directory);
int extract_link(std::string path, std::string link_target);

#define IS_DOT(name, len) (len == 1 && (name)[0] == '.')
#define IS_DOTDOT(name, len) (len == 2 && (name)[0] == '.' && (name)[1] == '.')
#define IS_DOT_DOTDOT(name, len) (IS_DOT(name, len) || IS_DOTDOT(name, len))

#ifndef PATH_MAX
#ifndef _WIN32
#define PATH_MAX 4095
#else
#define PATH_MAX MAX_PATH
#endif
#endif

#ifdef _WIN32
#define _mkdir(x, y) mkdir(x)
#else
#define _mkdir(x, y) mkdir(x, y)
#endif

#define _(x) (x)

#define EXTRACT_XATTR_SELINUX_KEY "security.selinux"
#define EXTRACT_XATTR_CAPABILITIES_KEY "security.capability"
#define EXTRACT_DIRECT_FILE_LIMIT (1u << 20) // 1MB

#endif //__EXT2FS_EXTRACT_H