/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <record_file.h>
#include "command.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "report_lib_interface.cpp"
#include "test_util.h"

using namespace simpleperf;
using namespace std;
using namespace android;

class SimplePerfReportFuzzer {
 public:
  SimplePerfReportFuzzer(const uint8_t* data, size_t size) : mFdp(data, size) {
    /**
     * Use maximum of 80% of buffer to write in FD and save at least 20% for fuzzing other APIs
     */
    const int32_t dataSize = mFdp.ConsumeIntegralInRange<int32_t>(0, (size * 80) / 100);
    std::vector<uint8_t> dataPointer = mFdp.ConsumeBytes<uint8_t>(dataSize);
    android::base::WriteFully(mTempfile.fd, dataPointer.data(), dataPointer.size());
    android::base::WriteFully(mTempfileWholeData.fd, data, size);
    RegisterDumpRecordCommand();
  }
  void process();

 private:
  FuzzedDataProvider mFdp;
  TemporaryFile mTempfile;
  TemporaryFile mTempfileWholeData;
  void TestPerfDataReader(const char* perf_data_path);
  void TestDumpCmd(const char* perf_data_path);
  void TestReportLib(const char* perf_data_path);
};

void SimplePerfReportFuzzer::process() {
  TestPerfDataReader(mTempfile.path);
  TestDumpCmd(mTempfile.path);
  // It is better to use whole data as input to report lib. Because the init corpuses are real
  // recording files.
  TestReportLib(mTempfileWholeData.path);
}

void SimplePerfReportFuzzer::TestPerfDataReader(const char* perf_data_path) {
  std::unique_ptr<RecordFileReader> reader = RecordFileReader::CreateInstance(perf_data_path);
  if (!reader.get()) {
    return;
  }
  while (mFdp.remaining_bytes()) {
    auto InvokeReader = mFdp.PickValueInArray<const std::function<void()>>({
        [&]() { reader->ReadCmdlineFeature(); },
        [&]() { reader->ReadBuildIdFeature(); },
        [&]() { reader->ReadFeatureString(mFdp.ConsumeIntegral<int32_t>() /* feature */); },
        [&]() {
          vector<uint8_t> buf;
          bool error;
          reader->ReadAuxData(mFdp.ConsumeIntegral<uint32_t>() /* cpu */,
                              mFdp.ConsumeIntegral<uint64_t>() /* aux_offset */,
                              mFdp.ConsumeIntegral<size_t>() /* size */, buf, error);
        },
        [&]() { reader->ReadDebugUnwindFeature(); },
        [&]() { reader->DataSection(); },
        [&]() {
          ThreadTree thread_tree;
          reader->LoadBuildIdAndFileFeatures(thread_tree);
        },
    });
    InvokeReader();
  }
  reader->Close();
}

void SimplePerfReportFuzzer::TestDumpCmd(const char* perf_data_path) {
  std::unique_ptr<Command> dump_cmd = CreateCommandInstance("dump");
  CaptureStdout capture;
  capture.Start();
  dump_cmd->Run({"-i", perf_data_path, "--dump-etm", "raw,packet,element"});
}

void SimplePerfReportFuzzer::TestReportLib(const char* perf_data_path) {
  ReportLib report_lib;
  if (!report_lib.SetRecordFile(perf_data_path)) {
    return;
  }
  vector<char> raw_data;
  while (true) {
    Sample* sample = report_lib.GetNextSample();
    if (sample == nullptr) {
      break;
    }
    const char* tracing_data = report_lib.GetTracingDataOfCurrentSample();
    Event* event = report_lib.GetEventOfCurrentSample();
    if (event == nullptr) {
      break;
    }
    if (event->tracing_data_format.size != 0) {
      // Test if we can read tracing data.
      raw_data.assign(tracing_data, tracing_data + event->tracing_data_format.size);
    }
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  SimplePerfReportFuzzer simplePerfReportFuzzer(data, size);
  simplePerfReportFuzzer.process();
  return 0;
}
