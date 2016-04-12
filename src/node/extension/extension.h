// Copyright 2014 Samsung Electronics Co, Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WRT_SERVICE_NODE_EXTENSION_H_
#define WRT_SERVICE_NODE_EXTENSION_H_

#include <string>
#include <vector>
#include <map>
#include <functional>

#include "XW_Extension.h"
#include "XW_Extension_SyncMessage.h"
#include "XW_Extension_Data.h"

namespace wrt {
namespace service {

class ExtensionAdapter;
class ExtensionInstance;

class Extension {
 public:
  class RuntimeVariableDelegate {
   public:
    virtual void GetRuntimeVariable(const char* key, char* value, size_t value_len) = 0;
  };

  Extension(const std::string& path, RuntimeVariableDelegate* delegate);
  Extension(const std::string& path,
            const std::string& name,
            const std::vector<std::string>& entry_points,
            RuntimeVariableDelegate* delegate);
  virtual ~Extension();

  bool Initialize();
  ExtensionInstance* CreateInstance();

  XW_Extension xw_extension() {
    return xw_extension_;
  }

  std::string name() {
    return name_;
  }

  std::string javascript_api() {
    Initialize();
    return javascript_api_;
  }

  std::vector<std::string>& entry_points() {
    return entry_points_;
  }

  bool use_trampoline() {
    return use_trampoline_;
  }

  void set_name(const std::string& name) {
    name_ = name;
  }

  void set_javascript_api(const std::string& javascript_api) {
    javascript_api_ = javascript_api;
  }

  void set_use_trampoline(bool use_trampoline) {
    use_trampoline_ = use_trampoline;
  }

 private:
  friend class ExtensionAdapter;
  friend class ExtensionInstance;

  void GetRuntimeVariable(const char* key, char* value, size_t value_len);
  int CheckAPIAccessControl(const char* api_name);
  int RegisterPermissions(const char* perm_table);

  bool initialized_;
  std::string library_path_;

  XW_Extension xw_extension_;
  std::string name_;
  std::string javascript_api_;
  std::vector<std::string> entry_points_;
  bool use_trampoline_;
  RuntimeVariableDelegate* rv_delegate_;

  XW_CreatedInstanceCallback created_instance_callback_;
  XW_DestroyedInstanceCallback destroyed_instance_callback_;
  XW_ShutdownCallback shutdown_callback_;
  XW_HandleMessageCallback handle_msg_callback_;
  XW_HandleSyncMessageCallback handle_sync_msg_callback_;
  XW_HandleDataCallback handle_data_callback_;
  XW_HandleDataCallback handle_sync_data_callback_;
};

class ExtensionInstance {
 public:
  typedef std::function<void(const std::string&, uint8_t*, size_t, bool)>
      MessageCallback;

  ExtensionInstance(Extension* extension, XW_Instance xw_instance);
  virtual ~ExtensionInstance();

  void HandleMessage(const std::string& msg);
  void HandleSyncMessage(const std::string& msg);

  void HandleData(const std::string& msg, uint8_t* buffer, size_t len);
  void HandleSyncData(const std::string& msg, uint8_t* buffer, size_t len);

  XW_Instance xw_instance() {
    return xw_instance_;
  }

  void set_post_message_listener(MessageCallback callback) {
    post_message_callback_ = callback;
  }

  void set_send_sync_message_listener(MessageCallback callback) {
    send_sync_reply_callback_ = callback;
  }

 private:
  friend class ExtensionAdapter;

  void PostMessageToJS(const std::string& msg);
  void PostMessageToJS(const std::string& msg, uint8_t* buf, size_t len);
  void SyncReplyToJS(const std::string& reply);
  void SyncReplyToJS(const std::string& reply, uint8_t* buf, size_t len);

  Extension* extension_;
  XW_Instance xw_instance_;
  void* instance_data_;

  MessageCallback post_message_callback_;
  MessageCallback send_sync_reply_callback_;
};

} // namespace service
} // namespace wrt

#endif // WRT_SERVICE_NODE_EXTENSION_H_
