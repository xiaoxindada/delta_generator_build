set(target_name "extract.erofs")
set(erofs_extract_srcs_dir "${CMAKE_SOURCE_DIR}/src/erofs_extract")

file(GLOB erofs_extract_srcs "${erofs_extract_srcs_dir}/*.cpp")

add_executable(${target_name} ${erofs_extract_srcs})
target_include_directories(${target_name} PRIVATE
        "${erofs_extract_srcs_dir}/include"
        ${liberofs_headers}
)
target_link_libraries(${target_name} PRIVATE
    ${erofs_common_libs}
    erofs
)
target_compile_options(${target_name} PRIVATE ${erofs_cflags} "-Wno-unused-result")
set(ENV{TZ} UTF-8)
execute_process(
	COMMAND date "+%y%m%d%H%M"
	OUTPUT_VARIABLE EXTRACT_BUILD_TIME
	OUTPUT_STRIP_TRAILING_WHITESPACE
)
target_compile_definitions(${target_name} PRIVATE "-DEXTRACT_BUILD_TIME=\"-${EXTRACT_BUILD_TIME}\"")
MESSAGE(STATUS "[extract] build time is ${EXTRACT_BUILD_TIME}")
