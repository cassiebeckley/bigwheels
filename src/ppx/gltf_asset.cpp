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

#include "ppx/gltf_asset.h"
#include "ppx/timer.h"
#include "ppx/gltf_asset.h"

#include <nlohmann/json.hpp>

namespace ppx {
namespace gltf {

using json = nlohmann::json;

Scene::Scene()
{
}

Scene::~Scene()
{
}

Asset::Asset()
{
}

Asset::~Asset()
{
}

Scene& Asset::GetMainScene()
{
    return mScenes[0];
}

Result Asset::LoadFile(const std::filesystem::path& path, Asset* pAsset)
{
    if (IsNull(pAsset)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    Timer timer;
    PPX_ASSERT_MSG(timer.Start() == ppx::TIMER_RESULT_SUCCESS, "timer start failed");
    double fnStartTime = timer.SecondsSinceStart();

    if (!std::filesystem::exists(path)) {
        return ppx::ERROR_PATH_DOES_NOT_EXIST;
    }

    // TODO: gltf stores all binary data as little-endian. In order to make this work correctly on
    // big-endian systems we'll need to detect endianness at compile time and convert if necessary.

    // TODO: investigate making this a zero-copy parser if extra performance is needed
    std::ifstream f(path);
    if (!f) {
        return ppx::ERROR_GLTF_FILE_LOAD_FAILED;
    }

    json assetJSON;
    uint32_t glbBufferLength = 0;
    char *glbBuffer = nullptr;

    PPX_LOG_DEBUG("trying to load: " << path << " first character: " << f.peek());

    if (f.peek() == '{') {
        // glTF JSON file
        // TODO: handle whitespace at beginning of file?
        assetJSON = json::parse(f);
        if (assetJSON.is_discarded()) {
          return ppx::ERROR_GLTF_FILE_INVALID_FORMAT;
        }
    }
    else {
        // glb binary file
        uint32_t header;
        uint32_t version;
        uint32_t length; // TODO: could use this for zero-copy; malloc a buffer of this size and read the file into it on a streaming basis

        f.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (header != 0x46546C67) {
            return ppx::ERROR_GLTF_FILE_INVALID_FORMAT;
        }

        f.read(reinterpret_cast<char*>(&version), sizeof(version));

        if (version != 2) {
            // We're only handling glTF 2.0 for now
            return ppx::ERROR_GLTF_FILE_INVALID_FORMAT;
        }

        f.read(reinterpret_cast<char*>(&length), sizeof(length));

        PPX_LOG_DEBUG("glTF binary file: " << (char)header << " " << version << " " << length);

        int chunkCount = 0;
        while (f.peek() != -1) {
            if (chunkCount++ >= 2) {
              // There must be only two chunks
              return ppx::ERROR_GLTF_FILE_INVALID_FORMAT;
            }
            uint32_t chunkLength;
            uint32_t chunkType;

            f.read(reinterpret_cast<char*>(&chunkLength), sizeof(chunkLength));
            f.read(reinterpret_cast<char*>(&chunkType), sizeof(chunkType));

            PPX_LOG_DEBUG("chunk of length " << chunkLength);

            char* chunkData = new char[chunkLength];

            f.read(chunkData, chunkLength);

            switch (chunkType) {
                case 0x4E4F534A: // JSON
                    PPX_LOG_DEBUG("handling as JSON");
                    if (!assetJSON.is_null()) {
                      // There can only be one JSON chunk
                      return ppx::ERROR_GLTF_FILE_INVALID_FORMAT;
                    }

                    assetJSON = json::parse(chunkData, chunkData + chunkLength);
                    if (assetJSON.is_discarded()) {
                      return ppx::ERROR_GLTF_FILE_INVALID_FORMAT;
                    }

                    delete[] chunkData;
                    break;
                case 0x004E4942: // BIN
                    PPX_LOG_DEBUG("handling as BIN");
                    if (assetJSON.is_null()) {
                      // The BIN chunk *must* be second
                      return ppx::ERROR_GLTF_FILE_INVALID_FORMAT;
                    }
                    glbBufferLength = chunkLength;
                    glbBuffer = chunkData;
                    break;
                default:
                    return ppx::ERROR_GLTF_FILE_INVALID_FORMAT;
            }
        }

        if (chunkCount == 0) {
          return ppx::ERROR_GLTF_FILE_NO_DATA;
        }
    }

    if (assetJSON["asset"]["version"] != "2.0") {
      // We only support glTF 2.0
      return ppx::ERROR_GLTF_FILE_INVALID_FORMAT;
    }

    // extract necessary data from JSON
    Asset asset;
    asset.mainSceneIndex = assetJSON["scene"];

    for (auto& sceneJSON : assetJSON["scenes"]) {
      PPX_LOG_DEBUG("Loading scene " << sceneJSON["name"]);
      for (auto& nodeId : sceneJSON["nodes"]) {
        auto& node = assetJSON["nodes"][nodeId.get<size_t>()];

        // TODO: get mesh and translation from node, add them to scene
      }
    }

    if (glbBuffer != nullptr) {
      delete[] glbBuffer;
    }

    double fnEndTime = timer.SecondsSinceStart();
    float  fnElapsed = static_cast<float>(fnEndTime - fnStartTime);
    PPX_LOG_INFO("Created asset from GLTF file: " << path << " (" << FloatString(fnElapsed) << " seconds)");

    return ppx::SUCCESS;
}

Asset Asset::LoadFile(const std::filesystem::path& path)
{
    Asset asset;
    PPX_CHECKED_CALL(LoadFile(path, &asset));
    return asset;
}

} // namespace gltf
} // namespace ppx
