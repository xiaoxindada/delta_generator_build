#include "extract.h"
#include <fmt/format.h>
#include <filesystem>

static int process_file_ex(
    ext2_ino_t ino,
    extract_ctx *ctx,
    config_ctx *config)
{
    errcode_t err = 0;
    ext2_xattr_handle *handle = nullptr;
    char *value = nullptr;
    size_t value_len = 0;
    vfs_cap_data *cap;

    err = ext2fs_xattrs_open(ctx->fs, ino, &handle);
    if (err)
    {
        com_err("ext2fs_xattrs_open", err, NULL);
        return 1;
    }

    err = ext2fs_xattrs_read(handle);
    if (err)
    {
        com_err("ext2fs_xattrs_read", err, NULL);
        ext2fs_xattrs_close(&handle);
        return 1;
    }

    err = ext2fs_xattr_get(handle, EXTRACT_XATTR_SELINUX_KEY, reinterpret_cast<void **>(&value), &value_len);
    if (err)
    {
        if (err && err != EXT2_ET_EA_KEY_NOT_FOUND)
            com_err("ext2fs_xattr_get", err, NULL);
        ext2fs_xattrs_close(&handle);
        return 1;
    }

    // setup selinux context
    if (value)
    {
        config->have_context = true;
        config->context.assign(value, value_len - 1);

        free(value);
        value_len = 0;
    }

    err = ext2fs_xattr_get(handle, EXTRACT_XATTR_CAPABILITIES_KEY, reinterpret_cast<void **>(&value), &value_len);
    if (err)
    {
        if (err && err != EXT2_ET_EA_KEY_NOT_FOUND)
            com_err("ext2fs_xattr_get", err, NULL);
        ext2fs_xattrs_close(&handle);
        return 1;
    }

    // setup capabilities
    if (value)
    {
        cap = (vfs_cap_data *)value;
        config->have_capabilities = true;
        config->capabilities = (uint64_t)cap->data[0].permitted |
                               ((uint64_t)cap->data[1].permitted << 32);

        free(value);
        value_len = 0;
    }

    err = ext2fs_xattrs_close(&handle);
    if (err)
    {
        com_err("ext2fs_xattr_close", err, NULL);
        return 1;
    }

    return 0;
}

static std::string process_readlink(ext2_inode *inode, extract_ctx *ctx)
{
    std::string link_target;
    errcode_t err = 0;

    if (inode->i_blocks == 0) // fast link store at i_block
    {
        unsigned long len = inode->i_size;
        if (len > sizeof(inode->i_block))
        {
            len = sizeof(inode->i_block);
        }

        link_target.assign(reinterpret_cast<const char *>(inode->i_block), len);
    }
    else
    {
        char *buf = (char *)malloc(ctx->fs->blocksize);
        if (!buf)
        {
            std::cerr << "Error: " << "Could malloc memories" << std::endl;
            return link_target;
        }

        err = ext2fs_read_dir_block(ctx->fs, inode->i_block[0], buf);
        if (err)
        {
            com_err("ext2fs_read_dir_block", err, NULL);
            free(buf);
            return link_target;
        }

        link_target.assign(buf, inode->i_size);
        free(buf);
    }

    return link_target;
}

static int process_file(
    ext2_dir_entry *dirent,
    [[maybe_unused]] int offset,
    [[maybe_unused]] int blocksize,
    [[maybe_unused]] char *buf,
    void *priv_data)
{
    if (IS_DOT_DOTDOT(dirent->name, strnlen(dirent->name, dirent->name_len)))
    {
        return 0;
    }
    errcode_t ret = 0;
    extract_ctx *ctx = (extract_ctx *)priv_data;
    struct ext2_inode inode;
    char *name = nullptr;

    ret = ext2fs_read_inode(ctx->fs, dirent->inode, &inode);
    if (ret)
    {
        com_err("ext2fs_read_inode", ret, NULL);
        return 0;
    }

    ret = ext2fs_get_pathname(ctx->fs, ctx->parent_ino, dirent->inode, &name);
    if (ret)
    {
        com_err("ext2fs_get_pathname", ret, NULL);
        return 0;
    }

    // write configs
    auto config = config_ctx{
        .path = name,
        .uid = inode.i_uid,
        .gid = inode.i_gid,
        .mode = inode.i_mode,
        .is_symlink = LINUX_S_ISLNK(inode.i_mode),
    };
    process_file_ex(dirent->inode, ctx, &config);
    if (config.is_symlink)
        config.symlink = process_readlink(&inode, ctx);

    extract_config.configs.push_back(config);

    switch (inode.i_mode & LINUX_S_IFMT)
    {
    case LINUX_S_IFDIR:
        extract_directory(dirent->inode, &inode, name, ctx);
        process_directory(dirent->inode, priv_data);
        break;
    case LINUX_S_IFREG:
        extract_regular_file(dirent->inode, &inode, name, ctx);
        break;
    case LINUX_S_IFLNK:
        extract_link(name, config.symlink);
        break;
    case LINUX_S_IFCHR:
    case LINUX_S_IFBLK:
    case LINUX_S_IFIFO:
    case LINUX_S_IFSOCK:
        std::cerr << fmt::format("Not support yet for special node: Mode: {:#o}, Name: {}", inode.i_mode, name) << std::endl;
        break;

    default:
        std::cerr << fmt::format("Unknow extract mode: {:#o} at inode {:#x}", inode.i_mode, dirent->inode) << std::endl;
        break;
    }

    progress_plus();

    if (name)
        free(name);

    return 0;
}

int process_directory(ext2_ino_t dir_ino, void *ctx)
{
    struct ext2_inode inode;
    errcode_t err = 0;
    extract_ctx *ectx = (extract_ctx *)ctx;
    extract_ctx nctx;

    nctx.fs = ectx->fs;
    nctx.parent_ino = dir_ino;

    err = ext2fs_read_inode(ectx->fs, dir_ino, &inode);
    if (err)
    {
        com_err("ext2fs_read_inode", err, NULL);
        return 0;
    }

    if (dir_ino == EXT2_ROOT_INO)
    {
        auto config = config_ctx{
            .path = "/",
            .uid = inode.i_uid,
            .gid = inode.i_gid,
            .mode = inode.i_mode,
        };
        process_file_ex(dir_ino, ectx, &config);
        extract_config.configs.push_back(config);
    }

    if (LINUX_S_ISDIR(inode.i_mode))
    {
        err = ext2fs_dir_iterate(ectx->fs, dir_ino, 0, NULL, process_file, &nctx);
        if (err)
        {
            com_err("ext2fs_dir_iterate", err, NULL);
        }
    }

    return 0;
}

/*
    Count files
*/
static int count_callback(ext2_dir_entry *dirent, int, int, char *, void *priv_data)
{
    CountContext *ctx = static_cast<CountContext *>(priv_data);

    if (IS_DOT_DOTDOT(dirent->name, strnlen(dirent->name, dirent->name_len)))
    {
        return 0;
    }

    ctx->count++;

    struct ext2_inode inode;
    errcode_t ret = ext2fs_read_inode(ctx->fs, dirent->inode, &inode);
    if (ret)
    {
        return 0;
    }

    if (LINUX_S_ISDIR(inode.i_mode))
    {
        CountContext sub_ctx = {0, ctx->fs};
        count_files_recursive(ctx->fs, dirent->inode, &sub_ctx);
        ctx->count += sub_ctx.count;
    }

    return 0;
}

uint32_t count_files_recursive(ext2_filsys fs, ext2_ino_t dir_ino, CountContext *ctx)
{
    bool cleanup = false;
    if (!ctx)
    {
        ctx = new CountContext{0, fs};
        cleanup = true;
    }

    struct ext2_inode inode;
    errcode_t ret = ext2fs_read_inode(fs, dir_ino, &inode);
    if (ret)
    {
        com_err("ext2fs_read_inode", ret, NULL);
        if (cleanup)
            delete ctx;
        return 0;
    }

    if (LINUX_S_ISDIR(inode.i_mode))
    {
        ret = ext2fs_dir_iterate(fs, dir_ino, 0, NULL, count_callback, ctx);
        if (ret)
        {
            com_err("ext2fs_dir_iterate", ret, NULL);
        }
    }

    uint32_t result = ctx->count;
    if (cleanup)
    {
        delete ctx;
    }
    return result;
}