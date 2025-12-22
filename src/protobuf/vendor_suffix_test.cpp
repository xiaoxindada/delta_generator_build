#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <link.h>

#include <string>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "google/protobuf/stubs/common.h"

namespace {

using ::testing::UnorderedElementsAre;

constexpr int kMajor = GOOGLE_PROTOBUF_VERSION / 1000000;
constexpr int kMinor = GOOGLE_PROTOBUF_VERSION / 1000 % 1000;
constexpr int kPatch = GOOGLE_PROTOBUF_VERSION % 1000;

std::string GetSuffix() {
  std::string suffix = GOOGLE_PROTOBUF_VERSION_SUFFIX;
  if (suffix != "") {
    return absl::StrCat("-", suffix);
  }
  return "";
}

TEST(ProtobufLibraryVendorSuffixTest, IteratePhdr) {
  std::vector<std::string> libs;
  dl_iterate_phdr(
      [](dl_phdr_info *info, size_t, void *data) -> int {
        auto *local_libs = static_cast<decltype(&libs)>(data);
        std::string name = info->dlpi_name;
        size_t soname_start = name.find("libprotobuf-cpp");
        if (soname_start != name.npos) {
          local_libs->push_back(name.substr(soname_start));
        }
        return 0;
      },
      &libs);

  EXPECT_THAT(libs, UnorderedElementsAre(
                        absl::StrFormat("libprotobuf-cpp-full-%d.%d.%d%s.so",
                                        kMajor, kMinor, kPatch, GetSuffix()),
                        absl::StrFormat("libprotobuf-cpp-lite-%d.%d.%d%s.so",
                                        kMajor, kMinor, kPatch, GetSuffix())));
}

} // namespace
