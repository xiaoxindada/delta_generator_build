# Android platform profiling

[TOC]

## General Tips

Here are some tips for Android platform developers, who build and flash system images on rooted
devices:
1. After running `adb root`, simpleperf can be used to profile any process or system wide.
2. It is recommended to use the latest simpleperf available in AOSP main, if you are not working
on the current main branch. Scripts are in `system/extras/simpleperf/scripts`, binaries are in
`system/extras/simpleperf/scripts/bin/android`.
3. It is recommended to use `app_profiler.py` for recording, and `report_html.py` for reporting.
Below is an example.

```sh
# Record surfaceflinger process for 10 seconds with dwarf based call graph. More examples are in
# scripts reference in the doc.
$ ./app_profiler.py -np surfaceflinger -r "-g --duration 10"

# Generate html report.
$ ./report_html.py
```

4. Since Android >= O has symbols for system libraries on device, we don't need to use unstripped
binaries in `$ANDROID_PRODUCT_OUT/symbols` to report call graphs. However, they are needed to add
source code and disassembly (with line numbers) in the report. Below is an example.

```sh
# Doing recording with app_profiler.py or simpleperf on device, and generates perf.data on host.
$ ./app_profiler.py -np surfaceflinger -r "--call-graph fp --duration 10"

# Collect unstripped binaries from $ANDROID_PRODUCT_OUT/symbols to binary_cache/.
$ ./binary_cache_builder.py -lib $ANDROID_PRODUCT_OUT/symbols

# Collect unstripped binaries from symbol file downloaded from builder server to binary_cache/.
$ unzip comet-symbols-12488474.zip
$ ./binary_cache_builder.py -lib out

# To verify that the binaries in binary_cache/ include debug sections, you can perform a manual
# check.

# Generate an HTML report with source code and disassembly.
# Disassembling all binaries can be slow, so you can use the --binary_filter
# option to disassemble only specific binaries, like surfaceflinger.so in this example.
$ ./report_html.py --add_source_code --source_dirs $ANDROID_BUILD_TOP --add_disassembly \
  --binary_filter surfaceflinger.so
```

For a comprehensive guide to displaying source code and disassembly, see
[Show Annotated Source Code and Disassembly](README.md#show-annotated-source-code-and-disassembly).


## Start simpleperf from system_server process

Sometimes we want to profile a process/system-wide when a special situation happens. In this case,
we can add code starting simpleperf at the point where the situation is detected.

1. Disable selinux by `adb shell setenforce 0`. Because selinux only allows simpleperf running
   in shell or debuggable/profileable apps.

2. Add below code at the point where the special situation is detected.

```java
try {
  // for capability check
  Os.prctl(OsConstants.PR_CAP_AMBIENT, OsConstants.PR_CAP_AMBIENT_RAISE,
           OsConstants.CAP_SYS_PTRACE, 0, 0);
  // Write to /data instead of /data/local/tmp. Because /data can be written by system user.
  Runtime.getRuntime().exec("/system/bin/simpleperf record -g -p " + String.valueOf(Process.myPid())
            + " -o /data/perf.data --duration 30 --log-to-android-buffer --log verbose");
} catch (Exception e) {
  Slog.e(TAG, "error while running simpleperf");
  e.printStackTrace();
}
```

## Hardware PMU counter limit

When monitoring instruction and cache related perf events (in hw/cache/raw/pmu category of list cmd),
these events are mapped to PMU counters on each cpu core. But each core only has a limited number
of PMU counters. If number of events > number of PMU counters, then the counters are multiplexed
among events, which probably isn't what we want. We can use `simpleperf stat --print-hw-counter` to
show hardware counters (per core) available on the device.

On Pixel devices, the number of PMU counters on each core is usually 7, of which 4 of them are used
by the kernel to monitor memory latency. So only 3 counters are available. It's fine to monitor up
to 3 PMU events at the same time. To monitor more than 3 events, the `--use-devfreq-counters` option
can be used to borrow from the counters used by the kernel.

## Get boot-time profile

On userdebug/eng devices, we can get boot-time profile via simpleperf.

Step 1. Customize the configuration if needed. By default, simpleperf tracks all processes
except for itself, starts at `early-init`, and stops when `sys.boot_completed` is set.
You can customize it by changing the trigger or command line flags in
`system/extras/simpleperf/simpleperf.rc`.

Step 2. Add `androidboot.simpleperf.boot_record=1` to the kernel command line.
For example, on Pixel devices, you can do
```
$ fastboot oem cmdline add androidboot.simpleperf.boot_record=1
```

Step 3. Reboot the device. When booting, init finds that the kernel command line flag is set,
so it forks a background process to run simpleperf to record boot-time profile.
init starts simpleperf at `early-init` stage, which is very soon after second-stage init starts.

Step 4. After boot, the boot-time profile is stored in /tmp/boot_perf.data. Then we can pull
the profile to host to report.

```
$ adb shell ls /tmp/boot_perf.data
/tmp/boot_perf.data
```

Following is a boot-time profile example. From timestamp, the first sample is generated at about
4.5s after booting.

![boot_time_profile](pictures/boot_time_profile.png)

## Use tracepoint/kprobe/uprobe events to get callstacks for certain functions

Simpleperf supports tracepoint events, kprobe events and uprobe events. tracepoint events are
predefined locations in the kernel source code that act as hooks for tracing. kprobe events allows
adding dynamic tracepoints for kernel functions. uprobe events allows adding dynamic tracepoints
for userspace binary functions.

Through simpleperf, we can get callstacks when certain tracepoint, kprobe or uprobe events happen.
This can help us understand why these events happen. Following are some examples.

```sh
# We need `adb root` to monitor tracepoint/kprobe/uprobe events.
(host) $ adb root
# List all available tracepoint events.
(device) $ simpleperf list tracepoint
# Show options for kprobe/uprobe events.
(device) $ simpleperf record --help
--kprobe kprobe_event1,kprobe_event2,...
             Add kprobe events during recording. The kprobe_event format is in
             Documentation/trace/kprobetrace.rst in the kernel. Examples:
               'p:myprobe do_sys_openat2 $arg2:string'   - add event kprobes:myprobe
               'r:myretprobe do_sys_openat2 $retval:s64' - add event kprobes:myretprobe
--uprobe uprobe_event1,uprobe_event2,...
             Add uprobe events during recording. The uprobe_event format is in
             Documentation/trace/uprobetracer.rst in the kernel. Examples:
               'p:myprobe /system/lib64/libc.so:0x1000'
                   - add event uprobes:myprobe
               'r:myretprobe /system/lib64/libc.so:0x1000'
                   - add event uprobes:myretprobe
-e event1[:modifier1],event2[:modifier2],...
             Select a list of events to record. An event can be:
               1) an event name listed in `simpleperf list`;
               2) a raw PMU event in rN format. N is a hex number.
                  For example, r1b selects event number 0x1b.
               3) a kprobe event added by --kprobe option.
               4) a uprobe event added by --uprobe option.
             Modifiers can be added to define how the event should be
             monitored. Possible modifiers are:
                u - monitor user space events only
                k - monitor kernel space events only

# To use tracepoint events, use -e to monitor them.
# Example: Trace sched_process_exec event for system wide for 10 seconds, recording callstack and
# field values (field values provide details of the event).
(device) simpleperf record -e sched:sched_process_exec -g --duration 10 -a
# The callstacks can be viewed by profiler UIs in `view_the_profile.md`.
(host)$ report_html.py -i perf.data
# The field values call be viewed by `report_sample.py` or `simpleperf dump`.
(host)$ report_sample.py -i perf.data --show_tracing_data

# To use kprobe events, use --kprobe to add kprobe events, and use -e to monitor them.
# Example: Trace each sys_open syscall for command `sleep 1`, recording callstack, file path and
# return value.
(device)$ simpleperf record --kprobe \
  'p:open do_sys_openat2 $arg2:string,r:open_ret do_sys_openat2 $retval:s64' \
  -e kprobes:open,kprobes:open_ret -g -m 4096 sleep 1
# The callstacks can be viewed by profiler UIs in `view_the_profile.md`.
(host)$ report_html.py -i perf.data
# The field values (file path and return value) call be viewed by `report_sample.py` or
# `simpleperf dump`.
(host)$ report_sample.py -i perf.data --show_tracing_data

# To use uprobe events, use --uprobe to add uprobe events, and use -e to monitor them.
# uprobe events needs one manual step to convert symbol names to virtual addresses in ELF file.
# Hopefully we can automate it in simpleperf in the future.
# Example: Trace pthread_mutex_lock and pthread_mutex_unlock for command `sleep 1`, recording
# callstack.
(device) $ readelf -sW /system/lib64/libc.so | grep pthread_mutex
   222: 0000000000084db0   232 FUNC    GLOBAL DEFAULT   15 pthread_mutex_lock
  1031: 0000000000085260   316 FUNC    GLOBAL DEFAULT   15 pthread_mutex_unlock
(device) $ /system/bin/simpleperf record --uprobe \
  "p:pthread_mutex_lock /system/lib64/libc.so:0x84db0,p:pthread_mutex_unlock /system/lib64/libc.so:0x85260"
  -e uprobes:pthread_mutex_lock,uprobes:pthread_mutex_unlock -g -m 4096 sleep 1
# The callstacks can be viewed by profiler UIs in `view_the_profile.md`.
(host) gecko_profile_generator.py -i perf.data | gzip > gecko-profile.json.gz
```
