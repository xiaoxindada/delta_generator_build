set(target_name "ext4_utils")

set(libext4_utils_dir "${CMAKE_SOURCE_DIR}/src/extras/ext4_utils")

set(cflags
    "-fno-strict-aliasing"
    "-D_FILE_OFFSET_BITS=64"
)

set(libext4_utils_srcs
    "${libext4_utils_dir}/ext4_utils.cpp"
    "${libext4_utils_dir}/wipe.cpp"
    "${libext4_utils_dir}/ext4_sb.cpp"
)

add_library(${target_name} STATIC ${libext4_utils_srcs})
target_compile_options(${target_name} PRIVATE ${cflags})
target_include_directories(${target_name} PUBLIC
    ${libbase_headers}
    ${libcutils_headers}
    ${libext4_utils_headers}
    ${e2fsprogs_lib_headers}
)
target_link_libraries(${target_name} PUBLIC 
    base
    cutils
    ext2_uuid
    zlib
)