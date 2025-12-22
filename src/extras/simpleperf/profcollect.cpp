/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <time.h>

#include <android-base/file.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include <wakelock/wakelock.h>
#include <include/simpleperf_profcollect.hpp>

#include "ETMRecorder.h"
#include "android-base/logging.h"
#include "command.h"
#include "event_attr.h"
#include "event_fd.h"
#include "event_selection_set.h"
#include "event_type.h"
#include "utils.h"

using namespace simpleperf;

namespace {

class CommandRegister {
 public:
  CommandRegister() {
    RegisterRecordCommand();
    RegisterInjectCommand();
  }
};

CommandRegister command_register;

}  // namespace

bool IsETMDriverAvailable() {
  bool result = ETMRecorder::GetInstance().IsETMDriverAvailable();
  LOG(INFO) << "HasDriverSupport result " << result;
  return result;
}

bool IsETMDeviceAvailable() {
  auto result = ETMRecorder::GetInstance().CheckEtmSupport(false);
  if (!result.ok()) {
    LOG(INFO) << "HasDeviceSupport check failed: " << result.error();
    return false;
  }
  const EventType* type = FindEventTypeByName("cs-etm", false);
  if (type == nullptr) {
    LOG(INFO) << "HasDeviceSupport check failed: no etm event";
    return false;
  }
  bool ret = IsEventAttrSupported(CreateDefaultPerfEventAttr(*type), type->name);
  LOG(INFO) << "HasDeviceSupport result " << ret;
  return ret;
}

bool IsLBRAvailable() {
  return IsBranchSamplingSupported();
}

static std::vector<std::string> ConvertArgs(const char** args, int arg_count) {
  std::vector<std::string> cmd_args(arg_count);
  for (int i = 0; i < arg_count; ++i) {
    cmd_args[i] = args[i];
  }
  return cmd_args;
}

bool RunRecordCmd(const char** args, int arg_count) {
  std::vector<std::string> cmd_args = ConvertArgs(args, arg_count);
  // If TRBE is available, only record ETM data on cpus having TRBE.
  auto check_etm_event = [](const std::string& s) { return s.find("cs-etm") != s.npos; };
  if (std::find_if(cmd_args.begin(), cmd_args.end(), check_etm_event) != cmd_args.end()) {
    const std::set<int>& cpus_having_trbe = ETMRecorder::GetInstance().GetCPUsHavingTRBESink();
    if (!cpus_having_trbe.empty()) {
      cmd_args.push_back("--cpu");
      cmd_args.push_back(ToCpuString(cpus_having_trbe));
    }
  }

  LOG(INFO) << "Record " << android::base::Join(cmd_args, " ");
  // The kernel may panic when trying to hibernate or hotplug CPUs while collecting
  // ETM data. So get wakelock to keep the CPUs on.
  auto wakelock = android::wakelock::WakeLock::tryGet("profcollectd");
  if (!wakelock) {
    LOG(ERROR) << "Record failed: Failed to request wakelock.";
    return false;
  }
  bool result = CreateCommandInstance("record")->Run(cmd_args);
  LOG(INFO) << "Record result " << result;
  return result;
}

bool RunInjectCmd(const char** args, int arg_count) {
  std::vector<std::string> cmd_args = ConvertArgs(args, arg_count);
  LOG(INFO) << "Inject " << android::base::Join(cmd_args, " ");
  bool result = CreateCommandInstance("inject")->Run(cmd_args);
  LOG(INFO) << "Inject result " << result;
  return result;
}

static android::base::unique_fd log_fd;
static android::base::LogFunction saved_log_func;

static void FileLogger(android::base::LogId id, android::base::LogSeverity severity,
                       const char* tag, const char* file, unsigned int line, const char* message) {
  if (log_fd.ok()) {
    char severity_char = android::base::kSeverityChars[severity];
    struct tm now;
    time_t t = time(nullptr);
    localtime_r(&t, &now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%m-%d %H:%M:%S", &now);
    std::string s = android::base::StringPrintf("%s %c %s %s:%u] %s\n", tag, severity_char,
                                                timestamp, file, line, message);
    WriteStringToFd(s, log_fd);
  }
  saved_log_func(id, severity, tag, file, line, message);
}

void SetLogFile(const char* filename) {
  int fd = TEMP_FAILURE_RETRY(open(filename, O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, 0600));
  if (fd == -1) {
    PLOG(ERROR) << "failed to open " << filename;
    return;
  }
  log_fd.reset(fd);
  saved_log_func = SetLogger(FileLogger);
}

void ResetLogFile() {
  log_fd.reset();
  SetLogger(std::move(saved_log_func));
}
