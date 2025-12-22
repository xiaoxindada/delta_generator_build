set(core_headers "${CMAKE_SOURCE_DIR}/src/core/include" CACHE STRING "" FORCE)
set(fmtlib_headers "${CMAKE_SOURCE_DIR}/src/fmtlib/include" CACHE STRING "" FORCE)
set(zlib_headers "${CMAKE_SOURCE_DIR}/src/zlib" CACHE STRING "" FORCE)
set(libbase_headers "${CMAKE_SOURCE_DIR}/src/libbase/include" CACHE STRING "" FORCE)
set(liblz4_headers "${CMAKE_SOURCE_DIR}/src/lz4/lib" CACHE STRING "" FORCE)
set(liblzma_headers "${CMAKE_SOURCE_DIR}/src/lzma/C" CACHE STRING "" FORCE)
set(libzstd_headers 
    "${CMAKE_SOURCE_DIR}/src/zstd/lib" 
    "${CMAKE_SOURCE_DIR}/src/zstd/lib/common"
CACHE STRING "" FORCE)

set(avb_headers "${CMAKE_SOURCE_DIR}/src/avb" CACHE STRING "" FORCE)
set(libfec_headers 
    "${CMAKE_SOURCE_DIR}/src/extras/libfec/include"
    "${CMAKE_SOURCE_DIR}/src/extras/libfec"
    "${CMAKE_SOURCE_DIR}/src/fec"
 CACHE STRING "" FORCE)
set(libcutils_headers "${CMAKE_SOURCE_DIR}/src/core/libcutils/include" CACHE STRING "" FORCE)
set(libutils_headers "${CMAKE_SOURCE_DIR}/src/core/libutils/include" CACHE STRING "" FORCE)
set(libchrome_headers "${CMAKE_SOURCE_DIR}/src/libchrome" CACHE STRING "" FORCE)
set(boringssl_headers "${CMAKE_SOURCE_DIR}/src/boringssl/include" CACHE STRING "" FORCE)

set(liblog_headers
    "${CMAKE_SOURCE_DIR}/src/logging/liblog/include"
    "${CMAKE_SOURCE_DIR}/src/logging/liblog/include_vndk"
    CACHE STRING "" FORCE
)
set(liblogwrap_headers "${CMAKE_SOURCE_DIR}/src/logging/logwrapper/include" CACHE STRING "" FORCE)
set(libselinux_headers 
    "${CMAKE_SOURCE_DIR}/src/selinux/libselinux/include" 
    "${CMAKE_SOURCE_DIR}/src/selinux/libsepol/include" 
CACHE STRING "" FORCE)
set(libpcre2_headers "${CMAKE_SOURCE_DIR}/src/pcre/include" CACHE STRING "" FORCE)
set(libprocessgroup_headers
    "${CMAKE_SOURCE_DIR}/src/core/libprocessgroup"
    "${CMAKE_SOURCE_DIR}/src/core/libprocessgroup/include"
    "${CMAKE_SOURCE_DIR}/src/core/libprocessgroup/cgrouprc/include"
    CACHE STRING "" FORCE)
set(libprocessgroup_util_headers "${CMAKE_SOURCE_DIR}/src/core/libprocessgroup/util/include" CACHE STRING "" FORCE)
set(libsparse_headers "${CMAKE_SOURCE_DIR}/src/core/libsparse/include" CACHE STRING "" FORCE)
set(libgtest_prod_headers "${CMAKE_SOURCE_DIR}/src/googletest/googletest/include" CACHE STRING "" FORCE)
set(libgmock_headers "${CMAKE_SOURCE_DIR}/src/googletest/googlemock/include" CACHE STRING "" FORCE)
set(libfs_mgr_headers "${CMAKE_SOURCE_DIR}/src/core/libfs_mgr/include" CACHE STRING "" FORCE)
set(libfscrypt_headers "${CMAKE_SOURCE_DIR}/src/libfscrypt/include" CACHE STRING "" FORCE)
set(libmodpb64_headers "${CMAKE_SOURCE_DIR}/src/modp_b64" CACHE STRING "" FORCE)
set(libevent_headers 
    "${CMAKE_SOURCE_DIR}/src/libevent/include"
    "${CMAKE_SOURCE_DIR}/src/libevent/compat"
    CACHE STRING "" FORCE
)
set(e2fsprogs_lib_headers 
    "${CMAKE_SOURCE_DIR}/src/e2fsprogs/e2fsck"
    "${CMAKE_SOURCE_DIR}/src/e2fsprogs/lib"
    "${CMAKE_SOURCE_DIR}/src/e2fsprogs/lib/blkid"
    "${CMAKE_SOURCE_DIR}/src/e2fsprogs/lib/e2p"
    "${CMAKE_SOURCE_DIR}/src/e2fsprogs/lib/et"
    "${CMAKE_SOURCE_DIR}/src/e2fsprogs/lib/ext2fs"
    "${CMAKE_SOURCE_DIR}/src/e2fsprogs/lib/ss"
    "${CMAKE_SOURCE_DIR}/src/e2fsprogs/lib/support"
    "${CMAKE_SOURCE_DIR}/src/e2fsprogs/lib/uuid"
CACHE STRING "" FORCE)
set(libext4_utils_headers 
    "${CMAKE_SOURCE_DIR}/src/extars/ext4_utils"
    "${CMAKE_SOURCE_DIR}/src/extras/ext4_utils/include"
    CACHE STRING "" FORCE)
set(libcrypto_utils_headers 
    "${CMAKE_SOURCE_DIR}/src/core/libcrypto_utils" 
    "${CMAKE_SOURCE_DIR}/src/core/libcrypto_utils/include"
    CACHE STRING "" FORCE)
set(libsquashfs_utils_headers 
    "${CMAKE_SOURCE_DIR}/src/extras/squashfs_utils"
    "${CMAKE_SOURCE_DIR}/src/extras/squashfs_utils/include"
    CACHE STRING "" FORCE)
set(squashfs_headers "${CMAKE_SOURCE_DIR}/src/squashfs-tools/squashfs-tools" CACHE STRING "" FORCE)
set(bootimg_headers "${CMAKE_SOURCE_DIR}/src/mkbootimg/include/bootimg" CACHE STRING "" FORCE)
set(libdivsufsort_headers 
    "${CMAKE_SOURCE_DIR}/src/libdivsufsort/include"
    "${CMAKE_SOURCE_DIR}/src/libdivsufsort/android_include"
    CACHE STRING "" FORCE)
set(libbrotli_headers "${CMAKE_SOURCE_DIR}/src/brotli/c/include" CACHE STRING "" FORCE)
set(libbrillo_headers "${CMAKE_SOURCE_DIR}/src/libbrillo" CACHE STRING "" FORCE)
set(libbz_headers "${CMAKE_SOURCE_DIR}/src/bzip2" CACHE STRING "" FORCE)
set(libxz_headers 
    "${CMAKE_SOURCE_DIR}/src/xz-embedded/userspace"
    "${CMAKE_SOURCE_DIR}/src/xz-embedded/linux/include/linux"
    CACHE STRING "" FORCE)
set(libsparse_headers "${CMAKE_SOURCE_DIR}/src/core/libsparse/include" CACHE STRING "" FORCE)

set(protobuf_headers 
    "${CMAKE_SOURCE_DIR}/src/protobuf/src"
    "${CMAKE_SOURCE_DIR}/src/protobuf/third_party"
    "${CMAKE_SOURCE_DIR}/src/protobuf/third_party/utf8_range"
    "${CMAKE_SOURCE_DIR}/src/abseil-cpp"
    CACHE STRING "" FORCE)
set(absl_headers
    "${CMAKE_SOURCE_DIR}/src/abseil-cpp"
    CACHE STRING "" FORCE)
set(libzucchini_headers
    "${CMAKE_SOURCE_DIR}/src/zucchini/aosp/include/components"
    "${CMAKE_SOURCE_DIR}/src/zucchini/aosp/include"
    CACHE STRING "" FORCE)

set(libbsdiff_headers "${CMAKE_SOURCE_DIR}/src/bsdiff/include" CACHE STRING "" FORCE)
set(libjsoncpp_headers "${CMAKE_SOURCE_DIR}/src/jsoncpp/include" CACHE STRING "" FORCE)
set(libavb_headers 
    "${CMAKE_SOURCE_DIR}/src/avb"
    "${CMAKE_SOURCE_DIR}/src/avb/libavb"
    CACHE STRING "" FORCE)
set(gsid_headers "${CMAKE_SOURCE_DIR}/src/gsid/include" CACHE STRING "" FORCE)
set(puffin_headers "${CMAKE_SOURCE_DIR}/src/puffin/src/include" CACHE STRING "" FORCE)
set(update_engine_headers "${CMAKE_SOURCE_DIR}/src/update_engine" CACHE STRING "" FORCE)
set(libsnapshot_cow_headers "${CMAKE_SOURCE_DIR}/src/core/fs_mgr/ibsnapshot/libsnapshot_cow/include" CACHE STRING "" FORCE)
set(libziparchive_headers 
    "${CMAKE_SOURCE_DIR}/src/libziparchive/include"
    "${CMAKE_SOURCE_DIR}/src/libziparchive/incfs_support/include"
    CACHE STRING "" FORCE
)
set(libverity_tree_headers "${CMAKE_SOURCE_DIR}/src/extras/verity/include")
set(liberofs_headers 
    "${CMAKE_SOURCE_DIR}/src/erofs-utils/include"
    "${CMAKE_SOURCE_DIR}/src/erofs-utils/lib"
    CACHE STRING "" FORCE
)
set(libgflags_headers
    "${CMAKE_SOURCE_DIR}/src/gflags"
    "${CMAKE_SOURCE_DIR}/src/gflags/android"
    CACHE STRING "" FORCE
)