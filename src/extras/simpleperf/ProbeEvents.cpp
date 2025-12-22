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

#include "ProbeEvents.h"

#include <inttypes.h>

#include <filesystem>
#include <memory>
#include <string>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>

#include "RegEx.h"
#include "environment.h"
#include "event_selection_set.h"
#include "event_type.h"
#include "utils.h"

namespace simpleperf {

using android::base::ParseInt;
using android::base::ParseUint;
using android::base::Split;
using android::base::StringPrintf;
using android::base::unique_fd;
using android::base::WriteStringToFd;

static const std::string kKprobeEventPrefix = "kprobes:";

bool ProbeEvents::ParseProbeEventName(ProbeEventType type, const std::string& probe_cmd,
                                      ProbeEvent* event) {
  // kprobe_cmd is in formats described in <kernel>/Documentation/trace/kprobetrace.rst:
  //   p[:[GRP/]EVENT] [MOD:]SYM[+offs]|MEMADDR [FETCHARGS]
  //   r[MAXACTIVE][:[GRP/]EVENT] [MOD:]SYM[+offs] [FETCHARGS]
  // uprobe_cmd is in formats described in <kernel>/Documentation/trace/uprobetracer.rst:
  //   p[:[GRP/][EVENT]] PATH:OFFSET [FETCHARGS] : Set a uprobe
  //   r[:[GRP/][EVENT]] PATH:OFFSET [FETCHARGS] : Set a return uprobe (uretprobe)
  std::vector<std::string> args = Split(probe_cmd, " ");
  if (args.size() < 2) {
    return false;
  }

  event->type = type;
  event->group_name = type == ProbeEventType::kKprobe ? "kprobes" : "uprobes";

  // Parse given name.
  auto name_reg = RegEx::Create(R"(:([a-zA-Z_][\w_]*/)?([a-zA-Z_][\w_]*))");
  auto match = name_reg->SearchAll(args[0]);
  if (match->IsValid()) {
    if (match->GetField(1).length() > 0) {
      event->group_name = match->GetField(1);
      event->group_name.pop_back();
    }
    event->event_name = match->GetField(2);
    return true;
  }

  if (type == ProbeEventType::kKprobe) {
    // Generate name from MEMADDR.
    char probe_type = args[0][0];
    uint64_t kaddr;
    if (ParseUint(args[1], &kaddr)) {
      event->event_name = StringPrintf("%c_0x%" PRIx64, probe_type, kaddr);
      return true;
    }

    // Generate name from [MOD:]SYM[+offs].
    std::string symbol;
    int64_t offset;
    size_t split_pos = args[1].find_first_of("+-");
    if (split_pos == std::string::npos) {
      symbol = args[1];
      offset = 0;
    } else {
      symbol = args[1].substr(0, split_pos);
      if (!ParseInt(args[1].substr(split_pos), &offset) || offset < 0) {
        return false;
      }
    }
    std::string s = StringPrintf("%c_%s_%" PRId64, probe_type, symbol.c_str(), offset);
    event->event_name = RegEx::Create(R"(\.|:)")->Replace(s, "_").value();
    return true;
  } else {
    // Generate name from PATH:OFFSET.
    uint64_t offset;
    std::vector<std::string> target = Split(args[1], ":");
    if (target.size() == 2 && ParseUint(target[1], &offset)) {
      std::filesystem::path path(target[0]);
      std::string filename = path.filename().string();
      auto pos = filename.find_first_of(".-_");
      if (pos != std::string::npos) {
        filename = filename.substr(0, pos);
      }
      // 'p' is used in the event name even if it's a uretprobe.
      event->event_name = StringPrintf("p_%s_0x%" PRIx64, filename.c_str(), offset);
      return true;
    }

    return false;
  }
}

ProbeEvents::~ProbeEvents() {
  if (!IsEmpty()) {
    // Probe events can be deleted only when no perf event file is using them.
    event_selection_set_.CloseEventFiles();
    Clear();
  }
}

bool ProbeEvents::IsProbeSupported(ProbeEventType type) {
  const char* file_name = type == ProbeEventType::kKprobe ? "kprobe_events" : "uprobe_events";
  auto& path = GetProbeControlPath(type);
  if (!path.has_value()) {
    path = "";
    if (const char* tracefs_dir = GetTraceFsDir(); tracefs_dir != nullptr) {
      std::string probe_event_path = std::string(tracefs_dir) + "/" + file_name;
      if (IsRegularFile(probe_event_path)) {
        path = std::move(probe_event_path);
      }
    }
  }
  return !path.value().empty();
}

bool ProbeEvents::AddProbe(ProbeEventType type, const std::string& probe_cmd) {
  ProbeEvent event;
  if (!IsProbeSupported(type)) {
    LOG(ERROR) << "probe events isn't supported by the kernel.";
    return false;
  }
  if (!ParseProbeEventName(type, probe_cmd, &event)) {
    LOG(ERROR) << "invalid probe cmd: " << probe_cmd;
    return false;
  }
  if (!WriteProbeCmd(type, probe_cmd)) {
    return false;
  }
  probe_events_.emplace_back(std::move(event));
  return true;
}

bool ProbeEvents::IsKprobeEvent(const std::string& event_name) {
  return android::base::StartsWith(event_name, kKprobeEventPrefix);
}

bool ProbeEvents::CreateProbeEventIfNotExist(const std::string& event_name) {
  // uprobes aren't supported in this function because we can't identify the binary from an event
  // name.
  if (!IsKprobeEvent(event_name) ||
      (EventTypeManager::Instance().FindType(event_name) != nullptr)) {
    // No need to create a probe event.
    return true;
  }
  std::string function_name = event_name.substr(kKprobeEventPrefix.size());
  return AddProbe(ProbeEventType::kKprobe,
                  StringPrintf("p:%s %s", function_name.c_str(), function_name.c_str()));
}

void ProbeEvents::Clear() {
  for (const auto& probe_event : probe_events_) {
    if (!WriteProbeCmd(probe_event.type,
                       "-:" + probe_event.group_name + "/" + probe_event.event_name)) {
      LOG(WARNING) << "failed to delete probe event " << probe_event.group_name << ":"
                   << probe_event.event_name;
    }
    EventTypeManager::Instance().RemoveProbeType(probe_event.group_name + ":" +
                                                 probe_event.event_name);
  }
  probe_events_.clear();
}

bool ProbeEvents::WriteProbeCmd(ProbeEventType type, const std::string& probe_cmd) {
  const std::string& file_path = GetProbeControlPath(type).value();
  unique_fd fd(open(file_path.c_str(), O_APPEND | O_WRONLY | O_CLOEXEC));
  if (!fd.ok()) {
    PLOG(ERROR) << "failed to open " << file_path;
    return false;
  }
  if (!WriteStringToFd(probe_cmd, fd)) {
    PLOG(ERROR) << "failed to write '" << probe_cmd << "' to " << file_path;
    return false;
  }
  return true;
}

std::optional<std::string>& ProbeEvents::GetProbeControlPath(ProbeEventType type) {
  switch (type) {
    case ProbeEventType::kKprobe:
      return kprobe_control_path_;
    case ProbeEventType::kUprobe:
      return uprobe_control_path_;
  }
}

}  // namespace simpleperf
