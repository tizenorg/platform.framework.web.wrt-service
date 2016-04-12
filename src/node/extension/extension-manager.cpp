// Copyright 2014 Samsung Electronics Co, Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extension-manager.h"
#include "picojson.h"

#include <glob.h>
#include <dlog.h>
#include <memory>
#include <iostream>
#include <fstream>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "WRT_SERVICE"

namespace wrt {
namespace service {

namespace {
#ifdef DEVICE_ARCH_64_BIT
const char kExtensionDir[] = "/usr/lib64/tizen-extensions-crosswalk";
#else
const char kExtensionDir[] = "/usr/lib/tizen-extensions-crosswalk";
#endif
const char kExtensionPrefix[] = "lib";
const char kExtensionSuffix[] = ".so";
const char kExtensionMetadataSuffix[] = ".json";
}

ExtensionManager::ExtensionManager() {
}

ExtensionManager::~ExtensionManager() {
}

ExtensionManager* ExtensionManager::GetInstance() {
  static ExtensionManager self;
  return &self;
}

void ExtensionManager::RegisterExtensionsByMetadata(const std::string& metafile_path) {
  std::ifstream metafile(metafile_path.c_str());
  if (!metafile.is_open()) {
    SLOGE("Can't open plugin metadata file");
    return;
  }

  picojson::value metadata;
  metafile >> metadata;
  if (metadata.is<picojson::array>()) {
    auto& plugins = metadata.get<picojson::array>();
    for (auto plugin = plugins.begin(); plugin != plugins.end(); ++plugin) {
      if (!plugin->is<picojson::object>())
        continue;

      std::string name = plugin->get("name").to_str();
      std::string lib = plugin->get("lib").to_str();
      if (lib.at(0) != '/') {
        lib = std::string(kExtensionDir) + "/" + lib;
      }
      std::vector<std::string> entries;
      auto& entry_points_value = plugin->get("entry_points");
      if (entry_points_value.is<picojson::array>()) {
        auto& entry_points = entry_points_value.get<picojson::array>();
        for (auto entry = entry_points.begin(); entry != entry_points.end();
             ++entry) {
          entries.push_back(entry->to_str());
        }
      } else {
        SLOGE("there is no entry points");
      }
      Extension* extension = new Extension(lib, name, entries, this);
      RegisterExtension(extension);
    }
  } else {
    SLOGE("%s is not plugin metadata", metafile_path.c_str());
  }
  metafile.close();
}

void ExtensionManager::RegisterExtensionsByMetadata() {
  std::string extension_path(kExtensionDir);
  extension_path.append("/");
  extension_path.append("*");
  extension_path.append(kExtensionMetadataSuffix);

  SLOGD("Register Extension directory path : [%s]", extension_path.c_str());

  glob_t glob_result;
  glob(extension_path.c_str(), GLOB_TILDE, NULL, &glob_result);
  for (unsigned int i = 0; i < glob_result.gl_pathc; ++i) {
    RegisterExtensionsByMetadata(glob_result.gl_pathv[i]);
  }
  if (glob_result.gl_pathc == 0) {
    RegisterExtensionsInDirectory();
  }
}

void ExtensionManager::RegisterExtensionsInDirectory() {
  std::string extension_path(kExtensionDir);
  extension_path.append("/");
  extension_path.append(kExtensionPrefix);
  extension_path.append("*");
  extension_path.append(kExtensionSuffix);

  glob_t glob_result;
  glob(extension_path.c_str(), GLOB_TILDE, NULL, &glob_result);
  for (unsigned int i = 0; i < glob_result.gl_pathc; ++i) {
    Extension* extension = new Extension(glob_result.gl_pathv[i], this);
    if (extension->Initialize()) {
      RegisterExtension(extension);
    }
  }
}

void ExtensionManager::AddRuntimeVariable(const std::string& key, const std::string& value) {
  runtime_variables_.insert(std::make_pair(key, value));
}

void ExtensionManager::GetRuntimeVariable(const char* key, char* value, size_t value_len) {
  auto it = runtime_variables_.find(key);
  if (it != runtime_variables_.end()) {
    strncpy(value, it->second.c_str(), value_len);
  }
}

void ExtensionManager::ClearRuntimeVariables(void) {
  runtime_variables_.clear();
}

bool ExtensionManager::RegisterExtension(Extension* extension) {
  if (!extension)
    return false;

  std::string name = extension->name();

  if (extension_symbols_.find(name) != extension_symbols_.end()) {
    LOGW("Ignoring extension with name already registred. '%s'", name.c_str());
    return false;
  }

  std::vector<std::string>& entry_points = extension->entry_points();
  std::vector<std::string>::iterator iter;
  for (iter = entry_points.begin(); iter != entry_points.end(); ++iter) {
    if (extension_symbols_.find(*iter) != extension_symbols_.end()) {
      LOGW("Ignoring extension with entry_point already registred. '%s'", (*iter).c_str());
      return false;
    }
  }

  for (iter = entry_points.begin(); iter != entry_points.end(); ++iter) {
    extension_symbols_.insert(*iter);
  }

  extension_symbols_.insert(name);
  extensions_[name] = extension;

  return true;
}


} // namespace service
} // namespace wrt
