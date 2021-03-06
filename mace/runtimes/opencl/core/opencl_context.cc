// Copyright 2018 The MACE Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mace/runtimes/opencl/core/opencl_context.h"

#include <sys/stat.h>

#include "mace/port/env.h"

namespace mace {

namespace {

const char *kPrecompiledProgramFileName = "mace_cl_compiled_program.bin";

std::string FindFirstExistPath(const std::vector<std::string> &paths) {
  std::string result;
  struct stat st;
  for (auto path : paths) {
    if (stat(path.c_str(), &st) == 0) {
      if (S_ISREG(st.st_mode)) {
        result = path;
        break;
      }
    }
  }
  return result;
}
}  // namespace

OpenclContext::OpenclContext(
    const std::string &storage_path,
    const std::string &opencl_cache_full_path,
    const OpenCLCacheReusePolicy &opencl_cache_reuse_policy,
    const std::vector<std::string> &opencl_binary_paths,
    const std::string &opencl_parameter_path,
    const unsigned char *opencl_binary_ptr,
    const size_t opencl_binary_size,
    const unsigned char *opencl_parameter_ptr,
    const size_t opencl_parameter_size)
    : opencl_tuner_(new Tuner<uint32_t>(opencl_parameter_path,
                                        opencl_parameter_ptr,
                                        opencl_parameter_size)),
      opencl_cache_reuse_policy_(opencl_cache_reuse_policy) {
  if (opencl_cache_full_path.size() > 0) {
    opencl_cache_storage_.reset(new FileStorage(opencl_cache_full_path));
  } else {
    if (!storage_path.empty()) {
      storage_factory_.reset(new FileStorageFactory(storage_path));
      opencl_cache_storage_ =
          storage_factory_->CreateStorage(kPrecompiledProgramFileName);
    }
  }

  if (opencl_binary_ptr != nullptr) {
    opencl_binary_storage_.reset(
        new ReadOnlyByteStreamStorage(opencl_binary_ptr, opencl_binary_size));
  } else {
    std::string precompiled_binary_path =
        FindFirstExistPath(opencl_binary_paths);
    if (!precompiled_binary_path.empty()) {
      opencl_binary_storage_.reset(
          new FileStorage(precompiled_binary_path));
    }
  }
}

OpenclContext::~OpenclContext() = default;

std::shared_ptr<KVStorage> OpenclContext::opencl_binary_storage() {
  return opencl_binary_storage_;
}

std::shared_ptr<KVStorage> OpenclContext::opencl_cache_storage() {
  return opencl_cache_storage_;
}

OpenCLCacheReusePolicy OpenclContext::opencl_cache_reuse_policy() {
  return opencl_cache_reuse_policy_;
}

std::shared_ptr<Tuner<uint32_t>>
OpenclContext::opencl_tuner() {
  return opencl_tuner_;
}

}  // namespace mace
