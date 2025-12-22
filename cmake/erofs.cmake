set(erofs_utils_dir "${CMAKE_SOURCE_DIR}/src/erofs-utils")

set(cflags
        "-Wall"
        "-Wno-error=#warnings"
        "-Wno-ignored-qualifiers"
        "-Wno-pointer-arith"
        "-Wno-unused-parameter"
        "-Wno-unused-function"
        "-DHAVE_FALLOCATE"
        "-DHAVE_LINUX_TYPES_H"
        "-DHAVE_LIBSELINUX"
        "-DHAVE_LIBUUID"
        "-DLZ4_ENABLED"
        "-DLZ4HC_ENABLED"
        "-DWITH_ANDROID"
        "-DHAVE_MEMRCHR"
        "-DHAVE_SYS_IOCTL_H"
        "-DHAVE_LLISTXATTR"
        "-DHAVE_LGETXATTR"
        "-D_FILE_OFFSET_BITS=64"
        "-DEROFS_MAX_BLOCK_SIZE=16384"
        "-DHAVE_UTIMENSAT"
        "-DHAVE_UNISTD_H"
        "-DHAVE_SYSCONF"
        "-DEROFS_MT_ENABLED"
)

set(common_libs 
    base
    cutils
    ext2_uuid
    log
    lz4
    selinux
)

function(generate_version_header input_file output_file)
    if(NOT EXISTS ${input_file})
        message(FATAL_ERROR "Input file ${input_file} does not exist")
    endif()

    file(STRINGS ${input_file} FIRST_LINE LIMIT_COUNT 1)
    
    if(FIRST_LINE)
        string(STRIP "${FIRST_LINE}" CLEAN_VERSION)
        set(VERSION_DEFINE "#define PACKAGE_VERSION \"${CLEAN_VERSION}\"")
        file(WRITE ${output_file} "${VERSION_DEFINE}\n")
        message(STATUS "Generated ${output_file} with version: ${CLEAN_VERSION}")
    endif()
endfunction()


file(GLOB liberofs_srcs "${erofs_utils_dir}/lib/*.c")
list(REMOVE_ITEM liberofs_srcs
    "${erofs_utils_dir}/lib/compressor_libdeflate.c"
    "${erofs_utils_dir}/lib/compressor_libzstd.c"
)

add_library(erofs STATIC ${liberofs_srcs})
generate_version_header("${erofs_utils_dir}/VERSION" "${erofs_utils_dir}/erofs-utils-version.h")
target_compile_options(erofs PRIVATE ${cflags} "-include${erofs_utils_dir}/erofs-utils-version.h")
target_include_directories(erofs PUBLIC
    ${libbase_headers}
    ${libcutils_headers}
    ${liblog_headers}
    ${liblz4_headers}
    ${libselinux_headers}
    ${e2fsprogs_lib_headers}
    ${liberofs_headers}
)
target_link_libraries(erofs PUBLIC
    ${common_libs}
)

file(GLOB_RECURSE mkfs_srcs "${erofs_utils_dir}/mkfs/*.c")
add_executable(mkfs.erofs ${mkfs_srcs})
target_compile_options(mkfs.erofs PRIVATE ${cflags})
target_include_directories(mkfs.erofs PUBLIC
    ${liberofs_headers}
    ${libselinux_headers}
)
target_link_libraries(mkfs.erofs PRIVATE 
    ${common_libs}
    erofs
)

file(GLOB_RECURSE dump_srcs "${erofs_utils_dir}/dump/*.c")
add_executable(dump.erofs ${dump_srcs})
target_compile_options(dump.erofs PRIVATE ${cflags})
target_include_directories(dump.erofs PUBLIC
    ${liberofs_headers}
)
target_link_libraries(dump.erofs PRIVATE 
    ${common_libs}
    erofs
)

file(GLOB_RECURSE fsck_srcs "${erofs_utils_dir}/fsck/*.c")
add_executable(fsck.erofs ${fsck_srcs})
target_compile_options(fsck.erofs PRIVATE ${cflags})
target_include_directories(fsck.erofs PUBLIC
    ${liberofs_headers}
)
target_link_libraries(fsck.erofs PRIVATE
    ${common_libs}
    erofs
)