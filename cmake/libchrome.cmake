set(target_name "chrome")
set(libchrome_srcs_dir "${CMAKE_SOURCE_DIR}/src/libchrome")

set(libchromeCommonSrc
    "${libchrome_srcs_dir}/base/at_exit.cc"
    "${libchrome_srcs_dir}/base/barrier_closure.cc"
    "${libchrome_srcs_dir}/base/base64.cc"
    "${libchrome_srcs_dir}/base/base64url.cc"
    "${libchrome_srcs_dir}/base/base_paths.cc"
    "${libchrome_srcs_dir}/base/base_paths_posix.cc"
    "${libchrome_srcs_dir}/base/base_switches.cc"
    "${libchrome_srcs_dir}/base/big_endian.cc"
    "${libchrome_srcs_dir}/base/build_time.cc"
    "${libchrome_srcs_dir}/base/callback_helpers.cc"
    "${libchrome_srcs_dir}/base/callback_internal.cc"
    "${libchrome_srcs_dir}/base/command_line.cc"
    "${libchrome_srcs_dir}/base/cpu.cc"
    "${libchrome_srcs_dir}/base/debug/activity_tracker.cc"
    "${libchrome_srcs_dir}/base/debug/alias.cc"
    "${libchrome_srcs_dir}/base/debug/crash_logging.cc"
    "${libchrome_srcs_dir}/base/debug/debugger.cc"
    "${libchrome_srcs_dir}/base/debug/debugger_posix.cc"
    "${libchrome_srcs_dir}/base/debug/dump_without_crashing.cc"
    "${libchrome_srcs_dir}/base/debug/proc_maps_linux.cc"
    "${libchrome_srcs_dir}/base/debug/profiler.cc"
    "${libchrome_srcs_dir}/base/debug/stack_trace.cc"
    "${libchrome_srcs_dir}/base/debug/task_annotator.cc"
    "${libchrome_srcs_dir}/base/environment.cc"
    "${libchrome_srcs_dir}/base/feature_list.cc"
    "${libchrome_srcs_dir}/base/files/file.cc"
    "${libchrome_srcs_dir}/base/files/file_descriptor_watcher_posix.cc"
    "${libchrome_srcs_dir}/base/files/file_enumerator.cc"
    "${libchrome_srcs_dir}/base/files/file_enumerator_posix.cc"
    "${libchrome_srcs_dir}/base/files/file_path.cc"
    "${libchrome_srcs_dir}/base/files/file_path_constants.cc"
    "${libchrome_srcs_dir}/base/files/file_path_watcher.cc"
    "${libchrome_srcs_dir}/base/files/file_posix.cc"
    "${libchrome_srcs_dir}/base/files/file_tracing.cc"
    "${libchrome_srcs_dir}/base/files/file_util.cc"
    "${libchrome_srcs_dir}/base/files/file_util_posix.cc"
    "${libchrome_srcs_dir}/base/files/important_file_writer.cc"
    "${libchrome_srcs_dir}/base/files/memory_mapped_file.cc"
    "${libchrome_srcs_dir}/base/files/memory_mapped_file_posix.cc"
    "${libchrome_srcs_dir}/base/files/scoped_file.cc"
    "${libchrome_srcs_dir}/base/files/scoped_temp_dir.cc"
    "${libchrome_srcs_dir}/base/guid.cc"
    "${libchrome_srcs_dir}/base/hash.cc"
    "${libchrome_srcs_dir}/base/json/json_file_value_serializer.cc"
    "${libchrome_srcs_dir}/base/json/json_parser.cc"
    "${libchrome_srcs_dir}/base/json/json_reader.cc"
    "${libchrome_srcs_dir}/base/json/json_string_value_serializer.cc"
    "${libchrome_srcs_dir}/base/json/json_value_converter.cc"
    "${libchrome_srcs_dir}/base/json/json_writer.cc"
    "${libchrome_srcs_dir}/base/json/string_escape.cc"
    "${libchrome_srcs_dir}/base/lazy_instance_helpers.cc"
    "${libchrome_srcs_dir}/base/location.cc"
    "${libchrome_srcs_dir}/base/logging.cc"
    "${libchrome_srcs_dir}/base/md5.cc"
    "${libchrome_srcs_dir}/base/memory/aligned_memory.cc"
    "${libchrome_srcs_dir}/base/memory/platform_shared_memory_region.cc"
    "${libchrome_srcs_dir}/base/memory/read_only_shared_memory_region.cc"
    "${libchrome_srcs_dir}/base/memory/ref_counted.cc"
    "${libchrome_srcs_dir}/base/memory/ref_counted_memory.cc"
    "${libchrome_srcs_dir}/base/memory/shared_memory_handle.cc"
    "${libchrome_srcs_dir}/base/memory/shared_memory_helper.cc"
    "${libchrome_srcs_dir}/base/memory/shared_memory_mapping.cc"
    "${libchrome_srcs_dir}/base/memory/unsafe_shared_memory_region.cc"
    "${libchrome_srcs_dir}/base/memory/weak_ptr.cc"
    "${libchrome_srcs_dir}/base/memory/writable_shared_memory_region.cc"
    "${libchrome_srcs_dir}/base/message_loop/incoming_task_queue.cc"
    "${libchrome_srcs_dir}/base/message_loop/message_loop.cc"
    "${libchrome_srcs_dir}/base/message_loop/message_loop_current.cc"
    "${libchrome_srcs_dir}/base/message_loop/message_loop_task_runner.cc"
    "${libchrome_srcs_dir}/base/message_loop/message_pump.cc"
    "${libchrome_srcs_dir}/base/message_loop/message_pump_default.cc"
    "${libchrome_srcs_dir}/base/message_loop/message_pump_libevent.cc"
    "${libchrome_srcs_dir}/base/message_loop/watchable_io_message_pump_posix.cc"
    "${libchrome_srcs_dir}/base/metrics/bucket_ranges.cc"
    "${libchrome_srcs_dir}/base/metrics/dummy_histogram.cc"
    "${libchrome_srcs_dir}/base/metrics/field_trial.cc"
    "${libchrome_srcs_dir}/base/metrics/field_trial_param_associator.cc"
    "${libchrome_srcs_dir}/base/metrics/histogram.cc"
    "${libchrome_srcs_dir}/base/metrics/histogram_base.cc"
    "${libchrome_srcs_dir}/base/metrics/histogram_functions.cc"
    "${libchrome_srcs_dir}/base/metrics/histogram_samples.cc"
    "${libchrome_srcs_dir}/base/metrics/histogram_snapshot_manager.cc"
    "${libchrome_srcs_dir}/base/metrics/metrics_hashes.cc"
    "${libchrome_srcs_dir}/base/metrics/persistent_histogram_allocator.cc"
    "${libchrome_srcs_dir}/base/metrics/persistent_memory_allocator.cc"
    "${libchrome_srcs_dir}/base/metrics/persistent_sample_map.cc"
    "${libchrome_srcs_dir}/base/metrics/sample_map.cc"
    "${libchrome_srcs_dir}/base/metrics/sample_vector.cc"
    "${libchrome_srcs_dir}/base/metrics/sparse_histogram.cc"
    "${libchrome_srcs_dir}/base/metrics/statistics_recorder.cc"
    "${libchrome_srcs_dir}/base/native_library.cc"
    "${libchrome_srcs_dir}/base/native_library_posix.cc"
    "${libchrome_srcs_dir}/base/observer_list_threadsafe.cc"
    "${libchrome_srcs_dir}/base/path_service.cc"
    "${libchrome_srcs_dir}/base/pending_task.cc"
    "${libchrome_srcs_dir}/base/pickle.cc"
    "${libchrome_srcs_dir}/base/posix/file_descriptor_shuffle.cc"
    "${libchrome_srcs_dir}/base/posix/global_descriptors.cc"
    "${libchrome_srcs_dir}/base/posix/safe_strerror.cc"
    "${libchrome_srcs_dir}/base/process/kill.cc"
    "${libchrome_srcs_dir}/base/process/kill_posix.cc"
    "${libchrome_srcs_dir}/base/process/launch.cc"
    "${libchrome_srcs_dir}/base/process/launch_posix.cc"
    "${libchrome_srcs_dir}/base/process/memory.cc"
    "${libchrome_srcs_dir}/base/process/process_handle.cc"
    "${libchrome_srcs_dir}/base/process/process_handle_posix.cc"
    "${libchrome_srcs_dir}/base/process/process_iterator.cc"
    "${libchrome_srcs_dir}/base/process/process_metrics.cc"
    "${libchrome_srcs_dir}/base/process/process_metrics_posix.cc"
    "${libchrome_srcs_dir}/base/process/process_posix.cc"
    "${libchrome_srcs_dir}/base/rand_util.cc"
    "${libchrome_srcs_dir}/base/rand_util_posix.cc"
    "${libchrome_srcs_dir}/base/run_loop.cc"
    "${libchrome_srcs_dir}/base/scoped_native_library.cc"
    "${libchrome_srcs_dir}/base/sequence_checker_impl.cc"
    "${libchrome_srcs_dir}/base/sequence_token.cc"
    "${libchrome_srcs_dir}/base/sequenced_task_runner.cc"
    "${libchrome_srcs_dir}/base/sha1.cc"
    "${libchrome_srcs_dir}/base/strings/nullable_string16.cc"
    "${libchrome_srcs_dir}/base/strings/pattern.cc"
    "${libchrome_srcs_dir}/base/strings/safe_sprintf.cc"
    "${libchrome_srcs_dir}/base/strings/strcat.cc"
    "${libchrome_srcs_dir}/base/strings/string16.cc"
    "${libchrome_srcs_dir}/base/strings/string_number_conversions.cc"
    "${libchrome_srcs_dir}/base/strings/string_piece.cc"
    "${libchrome_srcs_dir}/base/strings/string_split.cc"
    "${libchrome_srcs_dir}/base/strings/string_util.cc"
    "${libchrome_srcs_dir}/base/strings/string_util_constants.cc"
    "${libchrome_srcs_dir}/base/strings/stringprintf.cc"
    "${libchrome_srcs_dir}/base/strings/utf_string_conversion_utils.cc"
    "${libchrome_srcs_dir}/base/strings/utf_string_conversions.cc"
    "${libchrome_srcs_dir}/base/sync_socket_posix.cc"
    "${libchrome_srcs_dir}/base/synchronization/atomic_flag.cc"
    "${libchrome_srcs_dir}/base/synchronization/condition_variable_posix.cc"
    "${libchrome_srcs_dir}/base/synchronization/lock.cc"
    "${libchrome_srcs_dir}/base/synchronization/lock_impl_posix.cc"
    "${libchrome_srcs_dir}/base/synchronization/waitable_event_posix.cc"
    "${libchrome_srcs_dir}/base/sys_info.cc"
    "${libchrome_srcs_dir}/base/sys_info_posix.cc"
    "${libchrome_srcs_dir}/base/task/cancelable_task_tracker.cc"
    "${libchrome_srcs_dir}/base/task_runner.cc"
    "${libchrome_srcs_dir}/base/task_scheduler/scheduler_lock_impl.cc"
    "${libchrome_srcs_dir}/base/task_scheduler/scoped_set_task_priority_for_current_thread.cc"
    "${libchrome_srcs_dir}/base/task_scheduler/sequence.cc"
    "${libchrome_srcs_dir}/base/task_scheduler/sequence_sort_key.cc"
    "${libchrome_srcs_dir}/base/task_scheduler/task.cc"
    "${libchrome_srcs_dir}/base/task_scheduler/task_traits.cc"
    "${libchrome_srcs_dir}/base/third_party/dynamic_annotations/dynamic_annotations.c"
    "${libchrome_srcs_dir}/base/third_party/icu/icu_utf.cc"
    "${libchrome_srcs_dir}/base/third_party/nspr/prtime.cc"
    "${libchrome_srcs_dir}/base/threading/platform_thread_posix.cc"
    "${libchrome_srcs_dir}/base/threading/post_task_and_reply_impl.cc"
    "${libchrome_srcs_dir}/base/threading/scoped_blocking_call.cc"
    "${libchrome_srcs_dir}/base/threading/sequence_local_storage_map.cc"
    "${libchrome_srcs_dir}/base/threading/sequence_local_storage_slot.cc"
    "${libchrome_srcs_dir}/base/threading/sequenced_task_runner_handle.cc"
    "${libchrome_srcs_dir}/base/threading/simple_thread.cc"
    "${libchrome_srcs_dir}/base/threading/thread.cc"
    "${libchrome_srcs_dir}/base/threading/thread_checker_impl.cc"
    "${libchrome_srcs_dir}/base/threading/thread_collision_warner.cc"
    "${libchrome_srcs_dir}/base/threading/thread_id_name_manager.cc"
    "${libchrome_srcs_dir}/base/threading/thread_local_storage.cc"
    "${libchrome_srcs_dir}/base/threading/thread_local_storage_posix.cc"
    "${libchrome_srcs_dir}/base/threading/thread_restrictions.cc"
    "${libchrome_srcs_dir}/base/threading/thread_task_runner_handle.cc"
    "${libchrome_srcs_dir}/base/time/clock.cc"
    "${libchrome_srcs_dir}/base/time/default_clock.cc"
    "${libchrome_srcs_dir}/base/time/default_tick_clock.cc"
    "${libchrome_srcs_dir}/base/time/tick_clock.cc"
    "${libchrome_srcs_dir}/base/time/time.cc"
    "${libchrome_srcs_dir}/base/time/time_conversion_posix.cc"
    "${libchrome_srcs_dir}/base/time/time_exploded_posix.cc"
    "${libchrome_srcs_dir}/base/time/time_now_posix.cc"
    "${libchrome_srcs_dir}/base/time/time_override.cc"
    "${libchrome_srcs_dir}/base/timer/elapsed_timer.cc"
    "${libchrome_srcs_dir}/base/timer/timer.cc"
    "${libchrome_srcs_dir}/base/token.cc"
    "${libchrome_srcs_dir}/base/unguessable_token.cc"
    "${libchrome_srcs_dir}/base/value_iterators.cc"
    "${libchrome_srcs_dir}/base/values.cc"
    "${libchrome_srcs_dir}/base/version.cc"
    "${libchrome_srcs_dir}/base/vlog.cc"
    "${libchrome_srcs_dir}/device/bluetooth/bluetooth_advertisement.cc"
    "${libchrome_srcs_dir}/device/bluetooth/bluetooth_uuid.cc"
    "${libchrome_srcs_dir}/device/bluetooth/bluez/bluetooth_service_attribute_value_bluez.cc"
    "${libchrome_srcs_dir}/ui/gfx/geometry/insets.cc"
    "${libchrome_srcs_dir}/ui/gfx/geometry/insets_f.cc"
    "${libchrome_srcs_dir}/ui/gfx/geometry/point.cc"
    "${libchrome_srcs_dir}/ui/gfx/geometry/point_conversions.cc"
    "${libchrome_srcs_dir}/ui/gfx/geometry/point_f.cc"
    "${libchrome_srcs_dir}/ui/gfx/geometry/rect.cc"
    "${libchrome_srcs_dir}/ui/gfx/geometry/rect_f.cc"
    "${libchrome_srcs_dir}/ui/gfx/geometry/size.cc"
    "${libchrome_srcs_dir}/ui/gfx/geometry/size_conversions.cc"
    "${libchrome_srcs_dir}/ui/gfx/geometry/size_f.cc"
    "${libchrome_srcs_dir}/ui/gfx/geometry/vector2d.cc"
    "${libchrome_srcs_dir}/ui/gfx/geometry/vector2d_f.cc"
    "${libchrome_srcs_dir}/ui/gfx/range/range.cc"
    "${libchrome_srcs_dir}/ui/gfx/range/range_f.cc"
)

set(libchromeLinuxSrc
    "${libchrome_srcs_dir}/base/files/file_path_watcher_linux.cc"
    "${libchrome_srcs_dir}/base/files/file_util_linux.cc"
    "${libchrome_srcs_dir}/base/memory/shared_memory_posix.cc"
    "${libchrome_srcs_dir}/base/posix/unix_domain_socket.cc"
    "${libchrome_srcs_dir}/base/process/internal_linux.cc"
    "${libchrome_srcs_dir}/base/process/memory_linux.cc"
    "${libchrome_srcs_dir}/base/process/process_handle_linux.cc"
    "${libchrome_srcs_dir}/base/process/process_info_linux.cc"
    "${libchrome_srcs_dir}/base/process/process_iterator_linux.cc"
    "${libchrome_srcs_dir}/base/process/process_metrics_linux.cc"
    "${libchrome_srcs_dir}/base/strings/sys_string_conversions_posix.cc"
    "${libchrome_srcs_dir}/base/sys_info_linux.cc"
    "${libchrome_srcs_dir}/base/threading/platform_thread_internal_posix.cc"
    "${libchrome_srcs_dir}/base/threading/platform_thread_linux.cc"
)

set(libchromeMuslSrc
    "${libchrome_srcs_dir}/base/debug/stack_trace_posix.cc"
    "${libchrome_srcs_dir}/base/memory/platform_shared_memory_region_posix.cc"
    "${libchrome_srcs_dir}/base/memory/shared_memory_handle_posix.cc"
)

set(libchromeGlibcSrc
    "${libchrome_srcs_dir}/base/allocator/allocator_shim.cc"
    "${libchrome_srcs_dir}/base/allocator/allocator_shim_default_dispatch_to_glibc.cc"
    "${libchrome_srcs_dir}/base/debug/stack_trace_posix.cc"
    "${libchrome_srcs_dir}/base/memory/platform_shared_memory_region_posix.cc"
    "${libchrome_srcs_dir}/base/memory/shared_memory_handle_posix.cc"
)

set(libchromeLinuxBionicSrc # NDK
    "${libchrome_srcs_dir}/base/debug/stack_trace_android.cc"
    "${libchrome_srcs_dir}/base/memory/platform_shared_memory_region_posix.cc"
    "${libchrome_srcs_dir}/base/memory/shared_memory_handle_posix.cc"
)

set(libchromeAndroidSrc
    "${libchrome_srcs_dir}/base/android/android_hardware_buffer_compat.cc"
    "${libchrome_srcs_dir}/base/android/build_info.cc"
    "${libchrome_srcs_dir}/base/android/content_uri_utils.cc"
    "${libchrome_srcs_dir}/base/android/java_exception_reporter.cc"
    "${libchrome_srcs_dir}/base/android/jni_android.cc"
    "${libchrome_srcs_dir}/base/android/jni_array.cc"
    "${libchrome_srcs_dir}/base/android/jni_string.cc"
    "${libchrome_srcs_dir}/base/android/path_utils.cc"
    "${libchrome_srcs_dir}/base/android/scoped_java_ref.cc"
    "${libchrome_srcs_dir}/base/android/scoped_hardware_buffer_handle.cc"
    "${libchrome_srcs_dir}/base/android/sys_utils.cc"
    "${libchrome_srcs_dir}/base/base_paths_android.cc"
    "${libchrome_srcs_dir}/base/debug/stack_trace_android.cc"
    "${libchrome_srcs_dir}/base/memory/platform_shared_memory_region_android.cc"
    "${libchrome_srcs_dir}/base/memory/shared_memory_android.cc"
    "${libchrome_srcs_dir}/base/memory/shared_memory_handle_android.cc"
    "${libchrome_srcs_dir}/base/os_compat_android.cc"
    "${libchrome_srcs_dir}/base/sys_info_android.cc"
    "${libchrome_srcs_dir}/base/time/time_android.cc"
)

set(target_srcs
    ${libchromeCommonSrc}
)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(HAVE_BIONIC 1)
else()
    set(HAVE_GLIBC 1)
endif()

message(STATUS "HAVE_BIONIC: ${HAVE_BIONIC}")
message(STATUS "HAVE_GLIBC: ${HAVE_GLIBC}")
message(STATUS "HAVE_MUSL: ${HAVE_MUSL}")

if(HAVE_BIONIC)
    message(STATUS "libchrome use bionic sources")
    list(REMOVE_ITEM libchromeLinuxSrc 
        "${libchrome_srcs_dir}/base/process/memory_linux.cc"
    )
    list(APPEND target_srcs
        ${libchromeLinuxBionicSrc}
    )
endif()

if(HAVE_GLIBC)
    message(STATUS "libchrome use glibc sources")    
    list(REMOVE_ITEM libchromeGlibcSrc 
        "${libchrome_srcs_dir}/base/allocator/allocator_shim.cc"
    )
    list(APPEND target_srcs
        ${libchromeGlibcSrc}
    )
endif()

if(HAVE_MUSL)
    message(STATUS "libchrome use musl sources")
    list(APPEND target_srcs
        ${libchromeMuslSrc}
    )
endif()


set(libchrome_defaults_cflags
        "-Wall"
        "-Wno-deprecated-declarations"
        "-Wno-implicit-fallthrough"
        "-Wno-implicit-int-float-conversion"
        # memory_mapped_file.cc:80, json_parser.cc:264,
        # sys_string_conversions_posix.cc:122, and
        # icu_utf.cc:161,165 have -Wno-implicit-fallthrough.
        "-Wno-missing-field-initializers"
        "-Wno-unused-parameter"      
)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    # Cross-compilation fails to pass Android resources
    # list(APPEND target_srcs
    #     ${libchromeAndroidSrc}
    # )
    list(APPEND libchrome_defaults_cflags 
        "-U__ANDROID__"
    )
else()
    list(APPEND libchrome_defaults_cflags 
        "-D__ANDROID_HOST__"
        "-DDONT_EMBED_BUILD_METADATA"
    )
endif()

add_library(${target_name} STATIC ${target_srcs} ${libchromeLinuxSrc})
target_compile_options(${target_name} PUBLIC ${libchrome_defaults_cflags})
target_include_directories(${target_name} PUBLIC
    ${libchrome_srcs_dir}
    ${libbase_headers}
    ${libevent_headers}
    ${libmodpb64_headers}
    ${libgtest_prod_headers}
    ${libcutils_headers}
    ${liblog_headers}
)
target_link_libraries(${target_name} PUBLIC
    base
    event
    modpb64
    gtest
    cutils
    log
)