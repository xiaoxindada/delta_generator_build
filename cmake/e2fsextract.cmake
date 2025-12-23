set(e2fsextarct_srcs_dir "${CMAKE_SOURCE_DIR}/src/e2fsextract")
set(mman_win32_srcs_dir "${CMAKE_SOURCE_DIR}/src/mman-win32")

if(WIN32) # mmap support for windows
set(mman_win32_srcs
        "${mman_win32_srcs_dir}/mman.c"
)
add_library(mman-win32 STATIC ${mman_win32_srcs})
target_compile_options(mman-win32 PRIVATE "-Wall")
target_include_directories(mman-win32 PUBLIC ${mman_win32_srcs_dir})
endif()

# program version
set(VERSION 1)
set(PATCHLEVEL 0)
configure_file("${e2fsextarct_srcs_dir}/version.h.in" "${e2fsextarct_srcs_dir}/version.h" )

set(e2fsextarct_srcs
        "${e2fsextarct_srcs_dir}/main.cc"
        "${e2fsextarct_srcs_dir}/process.cc"
        "${e2fsextarct_srcs_dir}/extract.cc"
        "${e2fsextarct_srcs_dir}/progress.cc"
)

add_executable(e2fsextract ${e2fsextarct_srcs})
target_compile_options(e2fsextract PRIVATE 
        ${e2fsprogs_cflags}
        "-D_FILE_OFFSET_BITS=64"
	"-D_LARGEFILE_SOURCE"
	"-D_LARGEFILE64_SOURCE"
        $<$<BOOL:${WIN32}>:"-DWINDOWS_IO_MANAGER_USE_MMAP_READ">
)

target_include_directories(e2fsextract PRIVATE
        ${fmtlib_headers}
        ${e2fsprogs_lib_headers}
        ${e2fsextarct_srcs_dir}
)

target_link_libraries(e2fsextract PRIVATE
        ext2_com_err
        ext2fs
        fmtlib
        sparse
        base
        $<$<BOOL:${WIN32}>:mman-win32>
)
