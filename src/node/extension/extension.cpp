// Copyright 2014 Samsung Electronics Co, Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extension.h"

#include <string.h>
#include <dlfcn.h>

#include <dlog.h>

#include "extension-adapter.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "WRT_SERVICE"

namespace wrt {
namespace service {

Extension::Extension(const std::string& path, RuntimeVariableDelegate* delegate)
  : initialized_(false),
    library_path_(path),
    xw_extension_(0),
    use_trampoline_(true),
    rv_delegate_(delegate),
    created_instance_callback_(NULL),
    destroyed_instance_callback_(NULL),
    shutdown_callback_(NULL),
    handle_msg_callback_(NULL),
    handle_sync_msg_callback_(NULL) {
}

Extension::Extension(const std::string& path,
                     const std::string& name,
                     const std::vector<std::string>& entry_points,
                     RuntimeVariableDelegate* delegate)
  : initialized_(false),
    library_path_(path),
    xw_extension_(0),
    name_(name),
    entry_points_(entry_points),
    use_trampoline_(true),
    rv_delegate_(delegate),
    created_instance_callback_(NULL),
    destroyed_instance_callback_(NULL),
    shutdown_callback_(NULL),
    handle_msg_callback_(NULL),
    handle_sync_msg_callback_(NULL) {
}

Extension::~Extension() {
  if (!initialized_)
    return;

  if (shutdown_callback_)
    shutdown_callback_(xw_extension_);
  ExtensionAdapter::GetInstance()->UnregisterExtension(this);
}

bool Extension::Initialize() {
  if (initialized_)
    return true;

  SLOGD("Extension Module library : [%s]", library_path_.c_str());

  void* handle = dlopen(library_path_.c_str(), RTLD_LAZY);
  if (!handle) {
    LOGE("Error loading extension '%s' : %s", library_path_.c_str(), dlerror());
    return false;
  }

  XW_Initialize_Func initialize = reinterpret_cast<XW_Initialize_Func>(
      dlsym(handle, "XW_Initialize"));
  if (!initialize) {
    LOGE("Error loading extension '%s' : couldn't get XW_Initialize function", library_path_.c_str());
    dlclose(handle);
    return false;
  }

  ExtensionAdapter* adapter = ExtensionAdapter::GetInstance();
  xw_extension_ = adapter->GetNextXWExtension();
  adapter->RegisterExtension(this);

  int ret = initialize(xw_extension_, ExtensionAdapter::GetInterface);
  if (ret != XW_OK) {
    LOGE("Error loading extension '%s' : XW_Initialize function returned error value.", library_path_.c_str());
    dlclose(handle);
    return false;
  }

  initialized_ = true;
  return true;
}

ExtensionInstance* Extension::CreateInstance() {
  Initialize();
  ExtensionAdapter* adapter = ExtensionAdapter::GetInstance();
  XW_Instance xw_instance = adapter->GetNextXWInstance();
  return new ExtensionInstance(this, xw_instance);
}

void Extension::GetRuntimeVariable(const char* key, char* value, size_t value_len) {
  if (rv_delegate_) {
    rv_delegate_->GetRuntimeVariable(key, value, value_len);
  }
}

int Extension::CheckAPIAccessControl(const char* /*api_name*/) {
  // TODO
  return XW_OK;
}

int Extension::RegisterPermissions(const char* /*perm_table*/) {
  // TODO
  return XW_OK;
}

ExtensionInstance::ExtensionInstance(Extension* extension, XW_Instance xw_instance)
  : extension_(extension),
    xw_instance_(xw_instance),
    instance_data_(NULL) {
  ExtensionAdapter::GetInstance()->RegisterInstance(this);
  XW_CreatedInstanceCallback callback = extension_->created_instance_callback_;
  if (callback)
    callback(xw_instance_);
}

ExtensionInstance::~ExtensionInstance() {
  XW_DestroyedInstanceCallback callback = extension_->destroyed_instance_callback_;
  if (callback)
    callback(xw_instance_);
  ExtensionAdapter::GetInstance()->UnregisterInstance(this);
}

void ExtensionInstance::HandleMessage(const std::string& msg) {
  XW_HandleMessageCallback callback = extension_->handle_msg_callback_;
  if (callback)
    callback(xw_instance_, msg.c_str());
}

void ExtensionInstance::HandleSyncMessage(const std::string& msg) {
  XW_HandleSyncMessageCallback callback = extension_->handle_sync_msg_callback_;
  if (callback) {
    callback(xw_instance_, msg.c_str());
  }
}

void ExtensionInstance::HandleData(const std::string& msg,
                                   uint8_t* buffer, size_t len) {
  XW_HandleDataCallback callback = extension_->handle_data_callback_;
  if (callback) {
    callback(xw_instance_, msg.c_str(), buffer, len);
  }
}

void ExtensionInstance::HandleSyncData(const std::string& msg,
                                       uint8_t* buffer, size_t len) {
  XW_HandleDataCallback callback = extension_->handle_sync_data_callback_;
  if (callback) {
    callback(xw_instance_, msg.c_str(), buffer, len);
  }
}

void ExtensionInstance::PostMessageToJS(const std::string& msg) {
  if (post_message_callback_) {
    post_message_callback_(msg, NULL, 0, false);
  }
}

void ExtensionInstance::PostMessageToJS(const std::string& msg,
                                        uint8_t* buf, size_t len) {
  if (post_message_callback_) {
    post_message_callback_(msg, buf, len, true);
  }
}

void ExtensionInstance::SyncReplyToJS(const std::string& reply) {
  if (send_sync_reply_callback_) {
    send_sync_reply_callback_(reply, NULL, 0, false);
  }
}

void ExtensionInstance::SyncReplyToJS(const std::string& reply,
                                      uint8_t* buf, size_t len) {
  if (send_sync_reply_callback_) {
    send_sync_reply_callback_(reply, buf, len, true);
  }
}

} // namespace service
} // namespace wrt

