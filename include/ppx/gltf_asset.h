// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ppx_gltf_asset_h
#define ppx_gltf_asset_h

#include "ppx/config.h"
#include "ppx/geometry.h"

namespace ppx {
namespace gltf {

//! @class Scene
//!
//!
class Scene
{
public:
    Scene();
    ~Scene();

private:
};

//! @class Asset
//!
//!
class Asset
{
public:
    Asset();
    ~Asset();

    Scene& GetMainScene();

    static Result LoadFile(const std::filesystem::path& path, Asset* pAsset);
    static Asset  LoadFile(const std::filesystem::path& path);

private:
    std::uint32_t mainSceneIndex;
    std::vector<Scene> mScenes;
};

} // namespace gltf
} // namespace ppx

#endif // ppx_gltf_asset_h
