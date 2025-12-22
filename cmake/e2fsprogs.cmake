set(e2fsprogs_dir "${CMAKE_SOURCE_DIR}/src/e2fsprogs")
set(contrib_dir "${e2fsprogs_dir}/contrib")

set(e2fsprogs_cflags
        "-Wall"
        # Some warnings that Android's build system enables by default are not
        # supported by upstream e2fsprogs.  When such a warning shows up
        # disable it below.  Please don't disable warnings that upstream
        # e2fsprogs is supposed to support; for those fix the code instead.
        "-Wno-pointer-arith"
        "-Wno-sign-compare"
        "-Wno-type-limits"
        "-Wno-typedef-redefinition"
        "-Wno-unused-parameter"
        "-Wno-unused-but-set-variable"
        "-Wno-macro-redefined"
        "-Wno-sign-compare" #Better keep compare
)

set(libext2_com_err_srcs_dir "${e2fsprogs_dir}/lib/et")
set(libext2_com_err_srcs
        "${libext2_com_err_srcs_dir}/error_message.c"
        "${libext2_com_err_srcs_dir}/et_name.c"
        "${libext2_com_err_srcs_dir}/init_et.c"
        "${libext2_com_err_srcs_dir}/com_err.c"
        "${libext2_com_err_srcs_dir}/com_right.c"
)

add_library(ext2_com_err STATIC ${libext2_com_err_srcs})
target_compile_options(ext2_com_err PRIVATE ${e2fsprogs_cflags})
target_include_directories(ext2_com_err PUBLIC
    ${e2fsprogs_lib_headers}
)

set(libext2_uuid_srcs_dir "${e2fsprogs_dir}/lib/uuid")
set(libext2_uuid_srcs
        "${libext2_uuid_srcs_dir}/clear.c"
        "${libext2_uuid_srcs_dir}/compare.c"
        "${libext2_uuid_srcs_dir}/copy.c"
        "${libext2_uuid_srcs_dir}/gen_uuid.c"
        "${libext2_uuid_srcs_dir}/isnull.c"
        "${libext2_uuid_srcs_dir}/pack.c"
        "${libext2_uuid_srcs_dir}/parse.c"
        "${libext2_uuid_srcs_dir}/unpack.c"
        "${libext2_uuid_srcs_dir}/unparse.c"
        "${libext2_uuid_srcs_dir}/uuid_time.c"
)

add_library(ext2_uuid STATIC ${libext2_uuid_srcs})
target_compile_options(ext2_uuid PRIVATE ${e2fsprogs_cflags})

target_include_directories(ext2_uuid PUBLIC
    ${e2fsprogs_lib_headers}
)

set(libext2fs_srcs_dir "${e2fsprogs_dir}/lib/ext2fs")
set(libext2fs_srcs
    "${libext2fs_srcs_dir}/ext2_err.c"
    "${libext2fs_srcs_dir}/alloc.c"
    "${libext2fs_srcs_dir}/alloc_sb.c"
    "${libext2fs_srcs_dir}/alloc_stats.c"
    "${libext2fs_srcs_dir}/alloc_tables.c"
    "${libext2fs_srcs_dir}/atexit.c"
    "${libext2fs_srcs_dir}/badblocks.c"
    "${libext2fs_srcs_dir}/bb_inode.c"
    "${libext2fs_srcs_dir}/bitmaps.c"
    "${libext2fs_srcs_dir}/bitops.c"
    "${libext2fs_srcs_dir}/blkmap64_ba.c"
    "${libext2fs_srcs_dir}/blkmap64_rb.c"
    "${libext2fs_srcs_dir}/blknum.c"
    "${libext2fs_srcs_dir}/block.c"
    "${libext2fs_srcs_dir}/bmap.c"
    "${libext2fs_srcs_dir}/check_desc.c"
    "${libext2fs_srcs_dir}/crc16.c"
    "${libext2fs_srcs_dir}/crc32c.c"
    "${libext2fs_srcs_dir}/csum.c"
    "${libext2fs_srcs_dir}/closefs.c"
    "${libext2fs_srcs_dir}/dblist.c"
    "${libext2fs_srcs_dir}/dblist_dir.c"
    "${libext2fs_srcs_dir}/digest_encode.c"
    "${libext2fs_srcs_dir}/dirblock.c"
    "${libext2fs_srcs_dir}/dirhash.c"
    "${libext2fs_srcs_dir}/dir_iterate.c"
    "${libext2fs_srcs_dir}/dupfs.c"
    "${libext2fs_srcs_dir}/expanddir.c"
    "${libext2fs_srcs_dir}/ext_attr.c"
    "${libext2fs_srcs_dir}/extent.c"
    "${libext2fs_srcs_dir}/fallocate.c"
    "${libext2fs_srcs_dir}/fileio.c"
    "${libext2fs_srcs_dir}/finddev.c"
    "${libext2fs_srcs_dir}/flushb.c"
    "${libext2fs_srcs_dir}/freefs.c"
    "${libext2fs_srcs_dir}/gen_bitmap.c"
    "${libext2fs_srcs_dir}/gen_bitmap64.c"
    "${libext2fs_srcs_dir}/get_num_dirs.c"
    "${libext2fs_srcs_dir}/get_pathname.c"
    "${libext2fs_srcs_dir}/getenv.c"
    "${libext2fs_srcs_dir}/getsize.c"
    "${libext2fs_srcs_dir}/getsectsize.c"
    "${libext2fs_srcs_dir}/hashmap.c"
    "${libext2fs_srcs_dir}/i_block.c"
    "${libext2fs_srcs_dir}/icount.c"
    "${libext2fs_srcs_dir}/imager.c"
    "${libext2fs_srcs_dir}/ind_block.c"
    "${libext2fs_srcs_dir}/initialize.c"
    "${libext2fs_srcs_dir}/inline.c"
    "${libext2fs_srcs_dir}/inline_data.c"
    "${libext2fs_srcs_dir}/inode.c"
    "${libext2fs_srcs_dir}/io_manager.c"
    "${libext2fs_srcs_dir}/ismounted.c"
    "${libext2fs_srcs_dir}/link.c"
    "${libext2fs_srcs_dir}/llseek.c"
    "${libext2fs_srcs_dir}/lookup.c"
    "${libext2fs_srcs_dir}/mmp.c"
    "${libext2fs_srcs_dir}/mkdir.c"
    "${libext2fs_srcs_dir}/mkjournal.c"
    "${libext2fs_srcs_dir}/namei.c"
    "${libext2fs_srcs_dir}/native.c"
    "${libext2fs_srcs_dir}/newdir.c"
    "${libext2fs_srcs_dir}/nls_utf8.c"
    "${libext2fs_srcs_dir}/openfs.c"
    "${libext2fs_srcs_dir}/orphan.c"
    "${libext2fs_srcs_dir}/progress.c"
    "${libext2fs_srcs_dir}/punch.c"
    "${libext2fs_srcs_dir}/qcow2.c"
    "${libext2fs_srcs_dir}/rbtree.c"
    "${libext2fs_srcs_dir}/read_bb.c"
    "${libext2fs_srcs_dir}/read_bb_file.c"
    "${libext2fs_srcs_dir}/res_gdt.c"
    "${libext2fs_srcs_dir}/rw_bitmaps.c"
    "${libext2fs_srcs_dir}/sha256.c"
    "${libext2fs_srcs_dir}/sha512.c"
    "${libext2fs_srcs_dir}/swapfs.c"
    "${libext2fs_srcs_dir}/symlink.c"
    "${libext2fs_srcs_dir}/undo_io.c"
    "${libext2fs_srcs_dir}/sparse_io.c"
    "${libext2fs_srcs_dir}/unlink.c"
    "${libext2fs_srcs_dir}/valid_blk.c"
    "${libext2fs_srcs_dir}/version.c"
    # get rid of this?!
    "${libext2fs_srcs_dir}/test_io.c" # Maybe unused
)

if(WIN32)
    list(APPEND libext2fs_srcs "${libext2fs_srcs_dir}/windows_io.c")
else()
    list(APPEND libext2fs_srcs "${libext2fs_srcs_dir}/unix_io.c")
endif()

add_library(ext2fs STATIC ${libext2fs_srcs})
target_compile_options(ext2fs PRIVATE ${e2fsprogs_cflags})
target_include_directories(ext2fs PUBLIC
    ${libsparse_headers}
    ${e2fsprogs_lib_headers}
)
target_link_libraries(ext2fs PUBLIC 
    sparse
    ext2_com_err
    ext2_uuid
    zlib
)

set(libext2_e2p_dir "${e2fsprogs_dir}/lib/e2p")
set(libext2_e2p_srcs
        "${libext2_e2p_dir}/encoding.c"
        "${libext2_e2p_dir}/errcode.c"
        "${libext2_e2p_dir}/feature.c"
        "${libext2_e2p_dir}/fgetflags.c"
        "${libext2_e2p_dir}/fsetflags.c"
        "${libext2_e2p_dir}/fgetproject.c"
        "${libext2_e2p_dir}/fsetproject.c"
        "${libext2_e2p_dir}/fgetversion.c"
        "${libext2_e2p_dir}/fsetversion.c"
        "${libext2_e2p_dir}/getflags.c"
        "${libext2_e2p_dir}/getversion.c"
        "${libext2_e2p_dir}/hashstr.c"
        "${libext2_e2p_dir}/iod.c"
        "${libext2_e2p_dir}/ljs.c"
        "${libext2_e2p_dir}/ls.c"
        "${libext2_e2p_dir}/mntopts.c"
        "${libext2_e2p_dir}/parse_num.c"
        "${libext2_e2p_dir}/pe.c"
        "${libext2_e2p_dir}/pf.c"
        "${libext2_e2p_dir}/ps.c"
        "${libext2_e2p_dir}/setflags.c"
        "${libext2_e2p_dir}/setversion.c"
        "${libext2_e2p_dir}/uuid.c"
        "${libext2_e2p_dir}/ostype.c"
        "${libext2_e2p_dir}/percent.c"
)

add_library(ext2_e2p STATIC ${libext2_e2p_srcs})
target_compile_options(ext2_e2p PRIVATE ${e2fsprogs_cflags})
target_include_directories(ext2_e2p PUBLIC
    ${e2fsprogs_lib_headers}
)

set(libext2_support_srcs_dir "${e2fsprogs_dir}/lib/support")
set(libext2_support_srcs
    "${libext2_support_srcs_dir}/cstring.c"
)

set(libext2_ss_dir "${e2fsprogs_dir}/lib/ss")
set(libext2_ss_srcs
        "${libext2_ss_dir}/ss_err.c"
        "${libext2_ss_dir}/std_rqs.c"
        "${libext2_ss_dir}/invocation.c"
        "${libext2_ss_dir}/help.c"
        "${libext2_ss_dir}/execute_cmd.c"
        "${libext2_ss_dir}/listen.c"
        "${libext2_ss_dir}/parse.c"
        "${libext2_ss_dir}/error.c"
        "${libext2_ss_dir}/prompt.c"
        "${libext2_ss_dir}/request_tbl.c"
        "${libext2_ss_dir}/list_rqs.c"
        "${libext2_ss_dir}/pager.c"
        "${libext2_ss_dir}/requests.c"
        "${libext2_ss_dir}/data.c"
        "${libext2_ss_dir}/get_readline.c"
)

add_library(ext2_ss STATIC ${libext2_ss_srcs})
target_compile_options(ext2_ss PRIVATE ${e2fsprogs_cflags})
target_include_directories(ext2_ss PUBLIC
    ${e2fsprogs_lib_headers}
)

set(libext2_blkid_dir "${e2fsprogs_dir}/lib/blkid")
set(libext2_blkid_srcs
        "${libext2_blkid_dir}/cache.c"
        "${libext2_blkid_dir}/dev.c"
        "${libext2_blkid_dir}/devname.c"
        "${libext2_blkid_dir}/devno.c"
        "${libext2_blkid_dir}/getsize.c"
        "${libext2_blkid_dir}/llseek.c"
        "${libext2_blkid_dir}/probe.c"
        "${libext2_blkid_dir}/read.c"
        "${libext2_blkid_dir}/resolve.c"
        "${libext2_blkid_dir}/save.c"
        "${libext2_blkid_dir}/tag.c"
        "${libext2_blkid_dir}/version.c"
)

add_library(ext2_blkid STATIC ${libext2_blkid_srcs})
target_compile_options(ext2_blkid PRIVATE ${e2fsprogs_cflags})
target_include_directories(ext2_blkid PUBLIC
    ${e2fsprogs_lib_headers}
)
target_link_libraries(ext2_blkid PUBLIC 
    ext2_uuid
)

set(libext2_support_srcs_dir "${e2fsprogs_dir}/lib/support")
set(libext2_support_srcs
    "${libext2_support_srcs_dir}/cstring.c"
)

add_library(ext2_support STATIC ${libext2_support_srcs})
target_compile_options(ext2_support PRIVATE ${e2fsprogs_cflags})
target_include_directories(ext2_support PUBLIC
    ${e2fsprogs_lib_headers}
)

set(libext2_profile_srcs
        "${libext2_support_srcs_dir}/prof_err.c"
        "${libext2_support_srcs_dir}/profile.c"
)

add_library(ext2_profile STATIC ${libext2_profile_srcs})
target_compile_options(ext2_profile PRIVATE ${e2fsprogs_cflags})
target_include_directories(ext2_profile PUBLIC
    ${e2fsprogs_lib_headers}
)

set(libext2_quota_srcs
        "${libext2_support_srcs_dir}/devname.c"
        "${libext2_support_srcs_dir}/dict.c"
        "${libext2_support_srcs_dir}/mkquota.c"
        "${libext2_support_srcs_dir}/parse_qtype.c"
        "${libext2_support_srcs_dir}/plausible.c"
        "${libext2_support_srcs_dir}/profile.c"
        "${libext2_support_srcs_dir}/profile_helpers.c"
        "${libext2_support_srcs_dir}/prof_err.c"
        "${libext2_support_srcs_dir}/quotaio.c"
        "${libext2_support_srcs_dir}/quotaio_tree.c"
        "${libext2_support_srcs_dir}/quotaio_v2.c"
)

add_library(ext2_quota STATIC ${libext2_quota_srcs})
target_compile_options(ext2_quota PRIVATE ${e2fsprogs_cflags})
target_include_directories(ext2_quota PUBLIC
    ${e2fsprogs_lib_headers}
)
target_link_libraries(ext2_quota PUBLIC 
    ext2fs
    ext2_blkid
    ext2_com_err
)

set(libext2_misc_srcs_dir "${e2fsprogs_dir}/misc")
set(libext2_misc_srcs
        "${libext2_misc_srcs_dir}/create_inode.c"
        "${libext2_misc_srcs_dir}/create_inode_libarchive.c"
)

add_library(ext2_misc STATIC ${libext2_misc_srcs})
target_compile_options(ext2_misc PRIVATE ${e2fsprogs_cflags})
target_include_directories(ext2_misc PUBLIC
    ${e2fsprogs_lib_headers}
    ${libext2_misc_srcs_dir}
)
target_link_libraries(ext2_misc PUBLIC 
    ext2fs
    ext2_blkid
    ext2_com_err
)

set(mke2fs_srcs
        "${libext2_misc_srcs_dir}/mke2fs.c"
        "${libext2_misc_srcs_dir}/util.c"
        "${libext2_misc_srcs_dir}/mk_hugefiles.c"
        "${libext2_misc_srcs_dir}/default_profile.c"
)

add_executable(mke2fs ${mke2fs_srcs})
target_compile_options(mke2fs PRIVATE ${e2fsprogs_cflags})
target_include_directories(mke2fs PUBLIC
    ${e2fsprogs_lib_headers}
)
target_link_libraries(mke2fs PRIVATE 
    ext2_blkid
    ext2_misc
    ext2_uuid
    ext2_quota
    ext2_com_err
    ext2_e2p
    ext2fs
    sparse
    base
    zlib
)

set(e2fsdroid_srcs_dir "${contrib_dir}/android")
set(e2fsdroid_srcs
    "${e2fsdroid_srcs_dir}/e2fsdroid.c"
    "${e2fsdroid_srcs_dir}/block_range.c"
    "${e2fsdroid_srcs_dir}/fsmap.c"
    "${e2fsdroid_srcs_dir}/block_list.c"
    "${e2fsdroid_srcs_dir}/base_fs.c"
    "${e2fsdroid_srcs_dir}/perms.c"
    "${e2fsdroid_srcs_dir}/basefs_allocator.c"
)

add_executable(e2fsdroid ${e2fsdroid_srcs})
target_compile_options(e2fsdroid PRIVATE ${e2fsprogs_cflags})
target_include_directories(e2fsdroid PUBLIC
    ${e2fsprogs_lib_headers}
)
target_link_libraries(e2fsdroid PRIVATE
        ext2_com_err
        ext2_misc
        ext2fs
        sparse
        zlib
        cutils
        base
        selinux
        crypto
        log
)

set(e2fsck_srcs_dir "${e2fsprogs_dir}/e2fsck")
set(e2fsck_srcs
        "${e2fsck_srcs_dir}/e2fsck.c"
        "${e2fsck_srcs_dir}/super.c"
        "${e2fsck_srcs_dir}/pass1.c"
        "${e2fsck_srcs_dir}/pass1b.c"
        "${e2fsck_srcs_dir}/pass2.c"
        "${e2fsck_srcs_dir}/pass3.c"
        "${e2fsck_srcs_dir}/pass4.c"
        "${e2fsck_srcs_dir}/pass5.c"
        "${e2fsck_srcs_dir}/logfile.c"
        "${e2fsck_srcs_dir}/journal.c"
        "${e2fsck_srcs_dir}/recovery.c"
        "${e2fsck_srcs_dir}/revoke.c"
        "${e2fsck_srcs_dir}/badblocks.c"
        "${e2fsck_srcs_dir}/util.c"
        "${e2fsck_srcs_dir}/unix.c"
        "${e2fsck_srcs_dir}/dirinfo.c"
        "${e2fsck_srcs_dir}/dx_dirinfo.c"
        "${e2fsck_srcs_dir}/ehandler.c"
        "${e2fsck_srcs_dir}/problem.c"
        "${e2fsck_srcs_dir}/message.c"
        "${e2fsck_srcs_dir}/ea_refcount.c"
        "${e2fsck_srcs_dir}/quota.c"
        "${e2fsck_srcs_dir}/rehash.c"
        "${e2fsck_srcs_dir}/region.c"
        "${e2fsck_srcs_dir}/sigcatcher.c"
        "${e2fsck_srcs_dir}/readahead.c"
        "${e2fsck_srcs_dir}/extents.c"
        "${e2fsck_srcs_dir}/encrypted_files.c"
)

add_executable(e2fsck ${e2fsck_srcs})
target_compile_options(e2fsck PRIVATE ${e2fsprogs_cflags})
target_include_directories(e2fsck PUBLIC
    ${e2fsprogs_lib_headers}
)
target_link_libraries(e2fsck PRIVATE
    ext2fs
    ext2_blkid
    ext2_com_err
    ext2_uuid
    ext2_quota
    ext2_e2p
)

set(resize_srcs_dir "${e2fsprogs_dir}/resize")
set(resize_srcs
        "${resize_srcs_dir}/extent.c"
        "${resize_srcs_dir}/resize2fs.c"
        "${resize_srcs_dir}/main.c"
        "${resize_srcs_dir}/online.c"
        "${resize_srcs_dir}/sim_progress.c"
        "${resize_srcs_dir}/resource_track.c"
)

add_executable(resize ${resize_srcs})
target_compile_options(resize PRIVATE ${e2fsprogs_cflags})
target_include_directories(resize PUBLIC
    ${e2fsprogs_lib_headers}
)
target_link_libraries(resize PRIVATE
    ext2fs
    ext2_com_err
    ext2_e2p
    ext2_uuid
    ext2_blkid
)

set(debugfs_srcs_dir "${e2fsprogs_dir}/debugfs")
set(debugfs_srcs
        "${debugfs_srcs_dir}/debug_cmds.c"
        "${debugfs_srcs_dir}/debugfs.c"
        "${debugfs_srcs_dir}/do_orphan.c"
        "${debugfs_srcs_dir}/util.c"
        "${debugfs_srcs_dir}/ncheck.c"
        "${debugfs_srcs_dir}/icheck.c"
        "${debugfs_srcs_dir}/ls.c"
        "${debugfs_srcs_dir}/lsdel.c"
        "${debugfs_srcs_dir}/dump.c"
        "${debugfs_srcs_dir}/set_fields.c"
        "${debugfs_srcs_dir}/logdump.c"
        "${debugfs_srcs_dir}/htree.c"
        "${debugfs_srcs_dir}/unused.c"
        "${debugfs_srcs_dir}/e2freefrag.c"
        "${debugfs_srcs_dir}/filefrag.c"
        "${debugfs_srcs_dir}/extent_cmds.c"
        "${debugfs_srcs_dir}/extent_inode.c"
        "${debugfs_srcs_dir}/zap.c"
        "${debugfs_srcs_dir}/quota.c"
        "${debugfs_srcs_dir}/xattrs.c"
        "${debugfs_srcs_dir}/journal.c"
        "${debugfs_srcs_dir}/revoke.c"
        "${debugfs_srcs_dir}/recovery.c"
        "${debugfs_srcs_dir}/do_journal.c"
)

add_executable(debugfs ${debugfs_srcs})
target_compile_options(debugfs PRIVATE ${e2fsprogs_cflags} "-DDEBUGFS")
target_include_directories(debugfs PUBLIC
    ${e2fsprogs_lib_headers}
)
target_link_libraries(debugfs PRIVATE
    ext2_misc
    ext2fs
    ext2_blkid
    ext2_uuid
    ext2_ss
    ext2_quota
    ext2_com_err
    ext2_e2p
    ext2_support
)