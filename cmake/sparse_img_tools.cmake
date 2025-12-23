set(libsparse_dir "${CMAKE_SOURCE_DIR}/src/core/libsparse")

set(simg2img_srcs
    "${libsparse_dir}/simg2img.cpp"
    "${libsparse_dir}/sparse_crc32.cpp"
)

set(img2simg_srcs
    "${libsparse_dir}/img2simg.cpp"
)

add_executable(simg2img ${simg2img_srcs})
target_compile_options(simg2img PRIVATE "-Wall")
target_include_directories(simg2img PRIVATE
    ${libsparse_headers}
    ${zlib_headers}
    ${libbase_headers}
)
target_link_libraries(simg2img PRIVATE
    sparse
    zlib
    base
)

add_executable(img2simg ${img2simg_srcs})
target_compile_options(img2simg PRIVATE "-Wall")
target_include_directories(img2simg PRIVATE
    ${libsparse_headers}
    ${zlib_headers}
    ${libbase_headers}
)
target_link_libraries(img2simg PRIVATE
    sparse
    zlib
    base
)