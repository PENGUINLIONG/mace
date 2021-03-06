// Copyright 2020 The MACE Authors. All Rights Reserved.
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

#ifndef MACE_LIBMACE_ENGINES_ENGINE_REGISTRY_H_
#define MACE_LIBMACE_ENGINES_ENGINE_REGISTRY_H_

#include <memory>

#include "mace/libmace/engines/base_engine.h"

namespace mace {

std::unique_ptr<BaseEngine> SmartCreateEngine(const MaceEngineConfig &config);

}  // namespace mace

#endif  // MACE_LIBMACE_ENGINES_ENGINE_REGISTRY_H_
