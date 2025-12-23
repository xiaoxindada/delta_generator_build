cmake_minimum_required(VERSION 3.20)

project(android_tools_build LANGUAGES C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    message("This is Android cross compile")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message("This is use clang build")
    add_compile_options("-fcolor-diagnostics")
    add_link_options("-fuse-ld=lld")
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        link_libraries(c++ c++abi)
    endif()
endif()

add_link_options("-static")
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options("-flto" "-Os" "-ffunction-sections" "-fdata-sections")
    add_link_options("-flto" "-Wl,--gc-sections" "-s")
endif()

# check x86_64 aprotoc
function(check_x86_64_aprotoc find_dir)
find_program(have_prebuilt_aprotoc "aprotoc" "${find_dir}")
if(NOT DEFINED MY_TARGET)
    set(MY_TARGET " ")
endif()
if(NOT have_prebuilt_aprotoc)
    if(NOT ${MY_TARGET} MATCHES "aprotoc")
        message(FATAL_ERROR 
        "Not found prebuilt/x86_64/aprotoc, please to build x86_64 aprotoc\n \
        use command: cmake xxx -DMY_TARGET=\"aprotoc\" to build \n \
        compiled copy to ${find_dir}
        ")
    endif()
endif()
message("MY_TARGET: ${MY_TARGET}")    
endfunction()

set(prebuilt_protoc "${CMAKE_SOURCE_DIR}/prebuilt/x86_64/aprotoc")
check_x86_64_aprotoc("${CMAKE_SOURCE_DIR}/prebuilt/x86_64")

# fix absl dll test link dir not defined 
link_directories(${CMAKE_CURRENT_BINARY_DIR})

# include headers
include("cmake/include.cmake")

# libs
include("cmake/googletest.cmake")
include("cmake/fmtlib.cmake")
include("cmake/zlib.cmake")
include("cmake/liblog.cmake")
include("cmake/libbase.cmake")
include("cmake/libcutils.cmake")
include("cmake/libjsoncpp.cmake")
include("cmake/libjsonpbparse.cmake")
include("cmake/libsparse.cmake")
include("cmake/libprocessgroup.cmake")
include("cmake/boringssl.cmake")
include("cmake/libchrome.cmake")
include("cmake/libzstd.cmake")
include("cmake/libbz.cmake")
include("cmake/libxz.cmake")
include("cmake/liblz4.cmake")
include("cmake/liblzma.cmake")
include("cmake/libziparchive.cmake")
include("cmake/libmodpb64.cmake")
include("cmake/libbrotli.cmake")
include("cmake/libbrillo.cmake")
include("cmake/pcre2.cmake")
include("cmake/libselinux.cmake")
include("cmake/libcrypto_utils.cmake")
include("cmake/libavb.cmake")
include("cmake/libevent.cmake")
include("cmake/libext4_utils.cmake")
include("cmake/libsquashfs_utils.cmake")
include("cmake/libfec_rs.cmake")
include("cmake/libfec.cmake")
include("cmake/libgflags.cmake")
include("cmake/libdivsufsort.cmake")
include("cmake/libbsdiff.cmake")
include("cmake/libzucchini.cmake")
include("cmake/puffin.cmake")
include("cmake/fs_mgr.cmake")
include("cmake/libverity_tree.cmake")

# protobuf
include("cmake/absl.cmake")
include("cmake/protobuf.cmake")

# update_engine
include("cmake/update_engine.cmake")

# e2fsprogs
include("cmake/e2fsprogs.cmake")
include("cmake/e2fsextract.cmake")

# erofs
include("cmake/erofs.cmake")
include("cmake/erofs_extract.cmake")

# mkbootfs
include("cmake/mkbootfs.cmake")

# dtc
include("cmake/dtc.cmake")

# partition_tools
include("cmake/partition_tools.cmake")

set(install_mkbootfs_targets mkbootfs)
set(install_e2fsprogs_targets mke2fs e2fsdroid e2fsextract e2fsck resize debugfs)
set(install_erofs_targets mkfs.erofs extract.erofs fsck.erofs dump.erofs)
set(install_update_engine_targets delta_generator)
set(install_dtc_targets dtc mkdtimg fdtget fdtput fdtdump fdtoverlay)
set(install_partition_tools_targets lpmake lpunpack lpadd lpflash lpdump)
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    list(REMOVE_ITEM install_partition_tools_targets lpdump)
endif()

install(
    TARGETS ${install_mkbootfs_targets}
    RUNTIME DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/bin/mkbootfs_prog"
)
install(
    TARGETS ${install_e2fsprogs_targets}
    RUNTIME DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/bin/e2fsprogs"
)
install(
    TARGETS ${install_erofs_targets}
    RUNTIME DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/bin/erofs"
)
install(
    TARGETS ${install_update_engine_targets}
    RUNTIME DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/bin/update_engine"
)
install(
    TARGETS ${install_dtc_targets}
    RUNTIME DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/bin/dtc_progs"
)
install(
    TARGETS ${install_partition_tools_targets}
    RUNTIME DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/bin/partition_tools"
)