// Copyright 2014 Samsung Electronics Co, Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WRT_SERIVCE_NODE_NATIVE_NODE_H_
#define WRT_SERIVCE_NODE_NATIVE_NODE_H_

#include <map>
#include <memory>
#include <vector>
#include <string>

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>

#include "extension.h"

namespace wrt {
namespace service {

class ExtensionNode: public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object> target);
  static v8::Handle<v8::Value> NewInstance(const std::string& name);
 private:
  class ChunkData {
   public:
    ChunkData(): data_(nullptr), length_(0) {}
    ~ChunkData() {
      if (data_) std::free(data_);
    }

    ChunkData& operator=(const ChunkData&) = delete;
    ChunkData(const ChunkData&) = delete;

    uint8_t* data() const { return data_; }
    size_t length() const { return length_; }
    void set_data(uint8_t* data) { data_ = data; }
    void set_length(const size_t length) { length_ = length; }
   private:
    uint8_t* data_;
    size_t length_;
  };

  ExtensionNode(const std::string& name);
  virtual ~ExtensionNode();

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void LoadInstance(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void PostMessage(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SendSyncMessage(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetMessageListener(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void PostData(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SendSyncData(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetDataListener(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void ReceiveChunkData(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void GetProp(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info);
  static v8::Persistent<v8::Function> constructor_;

  void SyncReplyCallback(const std::string& reply,
                         uint8_t* buffer, size_t len,
                         bool is_data,
                         const v8::FunctionCallbackInfo<v8::Value>& args);
  void PostMessageToJSCallback(const std::string& msg,
                               uint8_t* buffer, size_t len,
                               bool is_data);

  std::string name_;
  // wrt-service allows single instance per each extension
  ExtensionInstance* instance_;
  // FIXME: message_listener should be member of extension instance if allow multi-instance.
  v8::Persistent<v8::Function> message_listener_;
  v8::Persistent<v8::Function> data_listener_;
  std::map<int, std::unique_ptr<std::vector<uint8_t>>> data_chunk_storage_;
};

class NativeNode {
 public:
  static void Init(v8::Handle<v8::Object> target);

  static void Initialize(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetExtensions(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void UpdateRuntimeVariables(const v8::FunctionCallbackInfo<v8::Value>& args);

 private:
  static bool initialized_;
  static v8::Persistent<v8::Array> extensions_;
};

} // namespace service
} // namespace wrt

#endif // WRT_SERIVCE_NODE_NATIVE_NODE_H_
