set(mkbootfs_dir "${CMAKE_SOURCE_DIR}/src/core/mkbootfs")

set(cflags
    "-Wall"
)

set(mkbootfs_srcs
    "${mkbootfs_dir}/mkbootfs.cpp"
)

set(mkbootimg_srcs
    "${mkbootfs_dir}/mkbootfs.cpp"
)


add_executable(mkbootfs ${mkbootfs_srcs})
target_compile_options(mkbootfs PRIVATE ${cflags})
target_include_directories(mkbootfs PUBLIC
    ${libbase_headers}
    ${libcutils_headers}
    ${liblog_headers}
)
target_link_libraries(mkbootfs PRIVATE
    base
    cutils
    log
)