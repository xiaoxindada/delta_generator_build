set(protobuf_dir "${CMAKE_SOURCE_DIR}/src/protobuf")

set(protobuf_cflags
        "-Wall"
        "-Wno-missing-field-initializers"
        "-Wno-unused-function"
        "-Wno-unused-parameter"
        "-Wno-error=user-defined-warnings"
        "-Wno-deprecated-enum-enum-conversion"
        "-Wno-unused-private-field"
)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    list(APPEND protobuf_cflags "-frtti")
endif()

set(absl_libs 
    # lite
    # "absl_base_dynamic_annotations"
    # "absl_container_btree"
    # "absl_container_flat_hash_map"
    # "absl_container_flat_hash_set"
    # "absl_log_absl_check"
    # "absl_log_absl_log"
    # "absl_strings"
    # "absl_strings_cord"
    # "absl_strings_str_format"
    # "absl_synchronization"
    # "absl_types_optional"
    # "absl_types_span"

    # full
    # "absl_cleanup"
    # "absl_functional_function_ref"
    # "absl_log_die_if_null"
    # "absl_time"
    # "absl_types_variant"

    absl_base
    absl_log
    absl_strings
    absl_synchronization
    absl_low_level_hash
    absl_raw_hash_set
    absl_status
    absl_statusor
    absl_cleanup
    absl_log_internal_check_op
    absl_time
)

set(protobuf_cpp_lite_srcs
        "${protobuf_dir}/src/google/protobuf/any_lite.cc"
        "${protobuf_dir}/src/google/protobuf/arena.cc"
        "${protobuf_dir}/src/google/protobuf/arena_align.cc"
        "${protobuf_dir}/src/google/protobuf/arenastring.cc"
        "${protobuf_dir}/src/google/protobuf/arenaz_sampler.cc"
        "${protobuf_dir}/src/google/protobuf/extension_set.cc"
        "${protobuf_dir}/src/google/protobuf/generated_enum_util.cc"
        "${protobuf_dir}/src/google/protobuf/generated_message_tctable_lite.cc"
        "${protobuf_dir}/src/google/protobuf/generated_message_util.cc"
        "${protobuf_dir}/src/google/protobuf/implicit_weak_message.cc"
        "${protobuf_dir}/src/google/protobuf/inlined_string_field.cc"
        "${protobuf_dir}/src/google/protobuf/io/coded_stream.cc"
        "${protobuf_dir}/src/google/protobuf/io/io_win32.cc"
        "${protobuf_dir}/src/google/protobuf/io/zero_copy_sink.cc"
        "${protobuf_dir}/src/google/protobuf/io/zero_copy_stream.cc"
        "${protobuf_dir}/src/google/protobuf/io/zero_copy_stream_impl.cc"
        "${protobuf_dir}/src/google/protobuf/io/zero_copy_stream_impl_lite.cc"
        "${protobuf_dir}/src/google/protobuf/map.cc"
        "${protobuf_dir}/src/google/protobuf/message_lite.cc"
        "${protobuf_dir}/src/google/protobuf/parse_context.cc"
        "${protobuf_dir}/src/google/protobuf/port.cc"
        "${protobuf_dir}/src/google/protobuf/raw_ptr.cc"
        "${protobuf_dir}/src/google/protobuf/repeated_field.cc"
        "${protobuf_dir}/src/google/protobuf/repeated_ptr_field.cc"
        "${protobuf_dir}/src/google/protobuf/stubs/common.cc"
        "${protobuf_dir}/src/google/protobuf/wire_format_lite.cc"
        "${protobuf_dir}/third_party/utf8_range/naive.c"
        "${protobuf_dir}/third_party/utf8_range/range2-neon.c"
        "${protobuf_dir}/third_party/utf8_range/range2-sse.c"
        "${protobuf_dir}/third_party/utf8_range/utf8_validity.cc"
)

set(protobuf_cpp_full_srcs
        ${protobuf_cpp_lite_srcs}
        "${protobuf_dir}/src/google/protobuf/any.cc"
        "${protobuf_dir}/src/google/protobuf/any.pb.cc"
        "${protobuf_dir}/src/google/protobuf/api.pb.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/importer.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/parser.cc"
        "${protobuf_dir}/src/google/protobuf/cpp_features.pb.cc"
        "${protobuf_dir}/src/google/protobuf/descriptor.cc"
        "${protobuf_dir}/src/google/protobuf/descriptor.pb.cc"
        "${protobuf_dir}/src/google/protobuf/descriptor_database.cc"
        "${protobuf_dir}/src/google/protobuf/duration.pb.cc"
        "${protobuf_dir}/src/google/protobuf/dynamic_message.cc"
        "${protobuf_dir}/src/google/protobuf/empty.pb.cc"
        "${protobuf_dir}/src/google/protobuf/extension_set_heavy.cc"
        "${protobuf_dir}/src/google/protobuf/feature_resolver.cc"
        "${protobuf_dir}/src/google/protobuf/field_mask.pb.cc"
        "${protobuf_dir}/src/google/protobuf/generated_message_bases.cc"
        "${protobuf_dir}/src/google/protobuf/generated_message_reflection.cc"
        "${protobuf_dir}/src/google/protobuf/generated_message_tctable_full.cc"
        "${protobuf_dir}/src/google/protobuf/generated_message_tctable_gen.cc"
        "${protobuf_dir}/src/google/protobuf/internal_message_util.cc"
        "${protobuf_dir}/src/google/protobuf/io/gzip_stream.cc"
        "${protobuf_dir}/src/google/protobuf/io/printer.cc"
        "${protobuf_dir}/src/google/protobuf/io/strtod.cc"
        "${protobuf_dir}/src/google/protobuf/io/tokenizer.cc"
        "${protobuf_dir}/src/google/protobuf/json/internal/lexer.cc"
        "${protobuf_dir}/src/google/protobuf/json/internal/message_path.cc"
        "${protobuf_dir}/src/google/protobuf/json/internal/parser.cc"
        "${protobuf_dir}/src/google/protobuf/json/internal/unparser.cc"
        "${protobuf_dir}/src/google/protobuf/json/internal/untyped_message.cc"
        "${protobuf_dir}/src/google/protobuf/json/internal/writer.cc"
        "${protobuf_dir}/src/google/protobuf/json/internal/zero_copy_buffered_stream.cc"
        "${protobuf_dir}/src/google/protobuf/json/json.cc"
        "${protobuf_dir}/src/google/protobuf/map_field.cc"
        "${protobuf_dir}/src/google/protobuf/message.cc"
        "${protobuf_dir}/src/google/protobuf/reflection_mode.cc"
        "${protobuf_dir}/src/google/protobuf/reflection_ops.cc"
        "${protobuf_dir}/src/google/protobuf/service.cc"
        "${protobuf_dir}/src/google/protobuf/source_context.pb.cc"
        "${protobuf_dir}/src/google/protobuf/struct.pb.cc"
        "${protobuf_dir}/src/google/protobuf/text_format.cc"
        "${protobuf_dir}/src/google/protobuf/timestamp.pb.cc"
        "${protobuf_dir}/src/google/protobuf/type.pb.cc"
        "${protobuf_dir}/src/google/protobuf/unknown_field_set.cc"
        "${protobuf_dir}/src/google/protobuf/util/delimited_message_util.cc"
        "${protobuf_dir}/src/google/protobuf/util/field_comparator.cc"
        "${protobuf_dir}/src/google/protobuf/util/field_mask_util.cc"
        "${protobuf_dir}/src/google/protobuf/util/message_differencer.cc"
        "${protobuf_dir}/src/google/protobuf/util/time_util.cc"
        "${protobuf_dir}/src/google/protobuf/util/type_resolver_util.cc"
        "${protobuf_dir}/src/google/protobuf/wire_format.cc"
        "${protobuf_dir}/src/google/protobuf/wrappers.pb.cc"
)

set(protoc_srcs
        "${protobuf_dir}/src/google/protobuf/compiler/allowlists/editions.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/allowlists/empty_package.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/allowlists/open_enum.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/allowlists/unused_imports.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/allowlists/weak_imports.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/code_generator.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/command_line_interface.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/cpp/enum.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/cpp/extension.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/cpp/field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/cpp/field_generators/cord_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/cpp/field_generators/enum_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/cpp/field_generators/map_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/cpp/field_generators/message_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/cpp/field_generators/primitive_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/cpp/field_generators/string_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/cpp/file.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/cpp/generator.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/cpp/helpers.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/cpp/message.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/cpp/padding_optimizer.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/cpp/parse_function_generator.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/cpp/service.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/cpp/tracker.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/csharp/csharp_doc_comment.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/csharp/csharp_enum.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/csharp/csharp_enum_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/csharp/csharp_field_base.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/csharp/csharp_generator.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/csharp/csharp_helpers.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/csharp/csharp_map_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/csharp/csharp_message.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/csharp/csharp_message_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/csharp/csharp_primitive_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/csharp/csharp_reflection_class.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/csharp/csharp_repeated_enum_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/csharp/csharp_repeated_message_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/csharp/csharp_repeated_primitive_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/csharp/csharp_source_generator_base.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/csharp/csharp_wrapper_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/csharp/names.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/context.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/doc_comment.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/enum.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/enum_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/enum_field_lite.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/enum_lite.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/extension.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/extension_lite.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/file.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/generator.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/generator_factory.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/helpers.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/java_features.pb.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/kotlin_generator.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/map_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/map_field_lite.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/message.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/message_builder.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/message_builder_lite.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/message_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/message_field_lite.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/message_lite.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/message_serialization.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/name_resolver.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/names.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/primitive_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/primitive_field_lite.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/service.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/shared_code_generator.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/string_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/java/string_field_lite.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/objectivec/enum.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/objectivec/enum_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/objectivec/extension.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/objectivec/field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/objectivec/file.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/objectivec/generator.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/objectivec/helpers.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/objectivec/import_writer.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/objectivec/line_consumer.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/objectivec/map_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/objectivec/message.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/objectivec/message_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/objectivec/names.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/objectivec/oneof.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/objectivec/primitive_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/objectivec/text_format_decode_data.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/php/names.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/php/php_generator.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/plugin.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/plugin.pb.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/python/generator.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/python/helpers.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/python/pyi_generator.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/retention.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/ruby/ruby_generator.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/rust/accessors/accessors.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/rust/accessors/singular_message.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/rust/accessors/singular_scalar.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/rust/accessors/singular_string.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/rust/accessors/unsupported_field.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/rust/context.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/rust/generator.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/rust/message.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/rust/naming.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/rust/oneof.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/rust/relative_path.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/subprocess.cc"
        "${protobuf_dir}/src/google/protobuf/compiler/zip_writer.cc"
)

set(aprotoc_srcs
    "${protobuf_dir}/src/google/protobuf/compiler/main.cc"
)

add_library(protobuf-cpp-lite STATIC ${protobuf_cpp_lite_srcs})
target_compile_options(protobuf-cpp-lite PRIVATE ${protobuf_cflags})
target_include_directories(protobuf-cpp-lite PUBLIC
    ${protobuf_dir}
    ${protobuf_headers}
    ${absl_headers}
)
target_link_libraries(protobuf-cpp-lite PUBLIC
    ${absl_libs}
    log
)

add_library(protobuf-cpp-full STATIC ${protobuf_cpp_full_srcs})
target_compile_options(protobuf-cpp-full PRIVATE ${protobuf_cflags} "-DHAVE_ZLIB=1")
target_include_directories(protobuf-cpp-full PUBLIC
    ${protobuf_dir}
    ${protobuf_headers}
    ${absl_headers}
)
target_link_libraries(protobuf-cpp-full PUBLIC
    ${absl_libs}
    log
    zlib
)

add_library(protoc STATIC
    ${protobuf_cpp_lite_srcs} 
    ${protobuf_cpp_full_srcs} 
    ${protoc_srcs}
 )
target_compile_options(protoc PRIVATE ${protobuf_cflags} "-DHAVE_ZLIB=1")
target_include_directories(protoc PUBLIC
    ${protobuf_dir}
    ${protobuf_headers}
    ${absl_headers}
)
target_link_libraries(protoc PUBLIC
    ${absl_libs}
    log
    zlib
)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
add_executable(aprotoc ${aprotoc_srcs})
target_compile_options(aprotoc PRIVATE ${protobuf_cflags} "-DHAVE_ZLIB=1")
target_link_options(aprotoc PRIVATE "-static")
target_include_directories(aprotoc PUBLIC
    ${protobuf_dir}
    ${protobuf_headers}
    ${absl_headers}
)
target_link_libraries(aprotoc PRIVATE
    absl_log_initialize
    protoc
    zlib
)
endif()