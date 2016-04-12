// Copyright 2014 Samsung Electronics Co, Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WRT_SERVICE_NODE_EXTENSION_MANAGER_H_
#define WRT_SERVICE_NODE_EXTENSION_MANAGER_H_

#include <string>
#include <set>
#include <map>
#include <memory>

#include "XW_Extension.h"
#include "XW_Extension_SyncMessage.h"
#include "extension.h"

namespace wrt {
namespace service {

typedef std::map<std::string, Extension*> ExtensionMap;
typedef std::map<std::string, std::string> StringMap;

class ExtensionManager : public Extension::RuntimeVariableDelegate {
 public:
  static ExtensionManager* GetInstance();

  void RegisterExtensionsInDirectory();
  void RegisterExtensionsByMetadata();
  void RegisterExtensionsByMetadata(const std::string& metafile_path);

  void AddRuntimeVariable(const std::string& key, const std::string& value);

  void GetRuntimeVariable(const char* key, char* value, size_t value_len);

  void ClearRuntimeVariables(void);

  ExtensionMap& extensions() {
    return extensions_;
  }

 private:
  ExtensionManager();
  virtual ~ExtensionManager();

  bool RegisterExtension(Extension* extension);

  ExtensionMap extensions_;

  std::set<std::string> extension_symbols_;
  StringMap runtime_variables_;
};

} // namespace service
} // namespace wrt

#endif // WRT_SERVICE_NODE_EXTENSION_MANAGER_H_
