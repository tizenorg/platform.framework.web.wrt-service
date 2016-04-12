/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "native-node.h"

#include <v8.h>
#include <node.h>
#include <dlog.h>

#include "extension-manager.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "WRT_SERVICE"

using namespace v8;
using namespace node;

namespace wrt {
namespace service {

namespace {

// ChunkID generator for postData and sendDataSync methods
int GetNextChunkID() {
    static int id = 0;
    return ++id;
}

}  // namespace

Persistent<Function> ExtensionNode::constructor_;
bool NativeNode::initialized_ = false;
Persistent<Array> NativeNode::extensions_;

void ExtensionNode::Init(Handle<Object> /*target*/) {
  Isolate* isolate = Isolate::GetCurrent();
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "Extension"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  #define DEFINE_NODE_FUNCTION(s,f) \
    tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, #s), \
        FunctionTemplate::New(isolate, f)->GetFunction());
  #define DEFINE_NODE_GETTER(s,f) \
    tpl->InstanceTemplate()->SetAccessor(String::NewFromUtf8(isolate, #s), f);

  DEFINE_NODE_FUNCTION(loadInstance, LoadInstance);
  DEFINE_NODE_FUNCTION(postMessage, PostMessage);
  DEFINE_NODE_FUNCTION(sendSyncMessage, SendSyncMessage);
  DEFINE_NODE_FUNCTION(setMessageListener, SetMessageListener);
  DEFINE_NODE_FUNCTION(postData, PostData);
  DEFINE_NODE_FUNCTION(sendSyncData, SendSyncData);
  DEFINE_NODE_FUNCTION(setDataListener, SetDataListener);
  DEFINE_NODE_FUNCTION(receiveChunkData, ReceiveChunkData);

  DEFINE_NODE_GETTER(name, GetProp);
  DEFINE_NODE_GETTER(entry_points, GetProp);
  DEFINE_NODE_GETTER(jsapi, GetProp);
  DEFINE_NODE_GETTER(use_trampoline, GetProp);

  #undef DEFINE_NODE_FUNCTION
  #undef DEFINE_NODE_GETTER

  constructor_.Reset(isolate, tpl->GetFunction());
}

ExtensionNode::ExtensionNode(const std::string& name)
    : name_(name),
      instance_(NULL) {
}

ExtensionNode::~ExtensionNode() {
}


void ExtensionNode::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (args[0]->IsString()) {
    String::Utf8Value name(args[0]->ToString());
    ExtensionNode* obj = new ExtensionNode(std::string(*name));
    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    return;;
  }
}

Handle<Value> ExtensionNode::NewInstance(const std::string& name) {
  Isolate* isolate = Isolate::GetCurrent();
  EscapableHandleScope scope(isolate);

  const unsigned argc = 1;
  Handle<Value> argv[argc] = { String::NewFromUtf8(isolate, name.c_str()) };
  Local<Function> cons = Local<Function>::New(isolate, constructor_);
  Local<Object> instance = cons->NewInstance(argc, argv);

  return scope.Escape(instance);
}

void ExtensionNode::LoadInstance(const FunctionCallbackInfo<Value>& args) {
  ExtensionNode* obj = ObjectWrap::Unwrap<ExtensionNode>(args.This());

  ExtensionManager* manager = ExtensionManager::GetInstance();
  ExtensionMap& map = manager->extensions();
  auto iter = map.find(obj->name_);
  if (iter != map.end()) {
    obj->instance_ = iter->second->CreateInstance();
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    obj->instance_->set_post_message_listener(
        std::bind(&ExtensionNode::PostMessageToJSCallback,
                  obj, _1, _2, _3, _4));
  }
}

void ExtensionNode::PostMessage(const FunctionCallbackInfo<Value>& args) {
  ReturnValue<Value> result(args.GetReturnValue());

  if (args.Length() < 1 || !args[0]->IsString()) {
    result.Set(false);
    return;
  }

  ExtensionNode* obj = ObjectWrap::Unwrap<ExtensionNode>(args.This());
  if (obj->instance_ == NULL) {
    result.Set(false);
    return;
  }

  String::Utf8Value msg(args[0]->ToString());
  obj->instance_->HandleMessage(std::string(*msg));

  result.Set(true);
}

void ExtensionNode::SendSyncMessage(const FunctionCallbackInfo<Value>& args) {
  ReturnValue<Value> result(args.GetReturnValue());

  if (args.Length() < 1 || !args[0]->IsString()) {
    result.Set(false);
    return;
  }

  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  ExtensionNode* obj = ObjectWrap::Unwrap<ExtensionNode>(args.This());
  if (obj->instance_ == NULL) {
    result.Set(false);
    return;
  }

  using std::placeholders::_1;
  using std::placeholders::_2;
  using std::placeholders::_3;
  using std::placeholders::_4;
  obj->instance_->set_send_sync_message_listener(
      std::bind(&ExtensionNode::SyncReplyCallback,
                obj, _1, _2, _3, _4, args));

  String::Utf8Value msg(args[0]->ToString());
  obj->instance_->HandleSyncMessage(std::string(*msg));
}

void ExtensionNode::SetMessageListener(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);
  ReturnValue<Value> result(args.GetReturnValue());
  if (args.Length() < 1 || !args[0]->IsFunction()) {
    result.Set(false);
    return;
  }

  ExtensionNode* obj = ObjectWrap::Unwrap<ExtensionNode>(args.This());
  obj->message_listener_.Reset(isolate, Handle<Function>::Cast(args[0]));

  result.Set(true);
}

void ExtensionNode::PostData(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);
  ReturnValue<Value> result(args.GetReturnValue());

  if (args.Length() < 1 || !args[0]->IsString()) {
    result.Set(false);
    return;
  }

  ExtensionNode* obj = ObjectWrap::Unwrap<ExtensionNode>(args.This());
  if (obj->instance_ == NULL) {
    result.Set(false);
    return;
  }

  String::Utf8Value msg(args[0]->ToString());

  std::vector<uint8_t> chunk;
  if (args.Length() > 1) {
    if (args[1]->IsArray()) {
      Handle<Array> array = Handle<Array>::Cast(args[1]);
      for (std::size_t i = 0; i < array->Length(); ++i) {
        uint8_t v = static_cast<uint8_t>(array->Get(i)->Uint32Value());
        chunk.push_back(v);
      }
    } else if (args[1]->IsString()) {
      String::Utf8Value chunk_str(args[1]->ToString());
      chunk.resize(chunk_str.length());
      strncpy(reinterpret_cast<char*>(chunk.data()),
              *chunk_str, chunk_str.length());
    }
  }

  obj->instance_->HandleData(std::string(*msg), chunk.data(), chunk.size());

  result.Set(true);
}

void ExtensionNode::SendSyncData(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);
  ReturnValue<Value> result(args.GetReturnValue());

  if (args.Length() < 1 || !args[0]->IsString()) {
    result.Set(false);
    return;
  }

  ExtensionNode* obj = ObjectWrap::Unwrap<ExtensionNode>(args.This());
  if (obj->instance_ == NULL) {
    result.Set(false);
    return;
  }

  String::Utf8Value msg(args[0]->ToString());

  std::vector<uint8_t> chunk;
  if (args.Length() > 1) {
    if (args[1]->IsArray()) {
      Handle<Array> array = Handle<Array>::Cast(args[1]);
      for (std::size_t i = 0; i < array->Length(); ++i) {
        uint8_t v = static_cast<uint8_t>(array->Get(i)->Uint32Value());
        chunk.push_back(v);
      }
    } else if (args[1]->IsString()) {
      String::Utf8Value chunk_str(args[1]->ToString());
      chunk.resize(chunk_str.length());
      strncpy(reinterpret_cast<char*>(chunk.data()),
              *chunk_str, chunk_str.length());
    }
  }

  using std::placeholders::_1;
  using std::placeholders::_2;
  using std::placeholders::_3;
  using std::placeholders::_4;
  obj->instance_->set_send_sync_message_listener(
      std::bind(&ExtensionNode::SyncReplyCallback,
                obj, _1, _2, _3, _4, args));

  obj->instance_->HandleSyncData(std::string(*msg), chunk.data(), chunk.size());
}

void ExtensionNode::SetDataListener(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope handle_scope(isolate);

  ReturnValue<Value> result(args.GetReturnValue());
  if (args.Length() < 1 || !args[0]->IsFunction()) {
    result.Set(false);
    return;
  }

  ExtensionNode* obj = ObjectWrap::Unwrap<ExtensionNode>(args.This());
  obj->data_listener_.Reset(isolate, Handle<Function>::Cast(args[0]));

  result.Set(true);
}

void ExtensionNode::ReceiveChunkData(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::ReturnValue<v8::Value> result(args.GetReturnValue());
  if (args.Length() < 1 || !args[0]->IsNumber()) {
    result.Set(false);
    return;
  }

  ExtensionNode* obj = ObjectWrap::Unwrap<ExtensionNode>(args.This());

  int chunk_id = args[0]->Int32Value();
  std::string chunk_type("octet");
  if (args.Length() > 1 && args[1]->IsString()) {
    v8::String::Utf8Value value(args[1]->ToString());
    chunk_type = std::string(*value);
  }

  auto it = obj->data_chunk_storage_.find(chunk_id);
  if (it != obj->data_chunk_storage_.end()) {
    auto chunk = it->second.get();
    if (chunk_type == "string") {
      chunk->push_back(0); // add null terminator
      result.Set(
          v8::String::NewFromUtf8(
              isolate,
              reinterpret_cast<const char*>(chunk->data())));
    } else {
      v8::Local<v8::Array> arr = v8::Array::New(isolate, chunk->size());
      for (std::size_t i = 0; i < chunk->size(); ++i) {
        arr->Set(i, v8::Number::New(isolate, chunk->data()[i]));
      }
      result.Set(arr);
    }
    obj->data_chunk_storage_.erase(it);
  }
}

void ExtensionNode::GetProp(Local<String> property, const v8::PropertyCallbackInfo<v8::Value>& info) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  ExtensionNode* obj = ObjectWrap::Unwrap<ExtensionNode>(info.Holder());
  if (property->Equals(String::NewFromUtf8(isolate, "name"))) {
    info.GetReturnValue().Set(String::NewFromUtf8(isolate, (obj->name_).c_str()));
    return;
  }

  ExtensionManager* manager = ExtensionManager::GetInstance();
  ExtensionMap& map = manager->extensions();
  auto iter = map.find(obj->name_);

  if (property->Equals(String::NewFromUtf8(isolate, "jsapi"))) {
    if (iter != map.end()) {
      info.GetReturnValue().Set(String::NewFromUtf8(isolate, iter->second->javascript_api().c_str()));
      return;
    } else {
      return;
    }
  }
  else if (property->Equals(String::NewFromUtf8(isolate, "use_trampoline"))) {
    if (iter != map.end()) {
      info.GetReturnValue().Set(Boolean::New(isolate, iter->second->use_trampoline()));
      return;
    } else {
      return;
    }
  }
  else if (property->Equals(String::NewFromUtf8(isolate, "entry_points"))) {
    if (iter != map.end()) {
      std::vector<std::string>& entry_points = iter->second->entry_points();
      Handle<Array> array = Array::New(isolate, entry_points.size());
      int idx = 0;
      for (auto it = entry_points.begin(); it != entry_points.end(); ++it) {
        array->Set(idx++, String::NewFromUtf8(isolate, it->c_str()));
      }
      info.GetReturnValue().Set(array);
      return;
    } else {
      info.GetReturnValue().Set(Array::New(isolate, 0));
      return;
    }
  }
}

void ExtensionNode::SyncReplyCallback(const std::string& reply,
                                      uint8_t* buffer, size_t len,
                                      bool is_data,
                                      const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  ReturnValue<Value> result(args.GetReturnValue());

  if (is_data) {
    Local<Object> obj = Object::New(isolate);
    obj->Set(String::NewFromUtf8(isolate, "reply"),
             String::NewFromUtf8(isolate, reply.c_str()));
    if (buffer && len > 0) {
      int chunk_id = GetNextChunkID();
      std::unique_ptr<std::vector<uint8_t>>
        received_chunk_ptr(new std::vector<uint8_t>(buffer, buffer + len));
      data_chunk_storage_.insert(
          std::make_pair(chunk_id, std::move(received_chunk_ptr)));
      // buffer should freed in ExtensionNode
      delete[] buffer;
      obj->Set(String::NewFromUtf8(isolate, "chunk_id"),
         Integer::New(isolate, chunk_id));
    } else {
      obj->Set(String::NewFromUtf8(isolate, "chunk_id"), Undefined(isolate));
    }
    result.Set(obj);
  } else {
    result.Set(String::NewFromUtf8(isolate, reply.c_str()));
  }
}

void ExtensionNode::PostMessageToJSCallback(const std::string& msg,
                                            uint8_t* buffer, size_t len,
                                            bool is_data) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (is_data) {
    if (!data_listener_.IsEmpty()) {
      Local<Value> args[] = {
          Local<Value>::New(isolate, String::NewFromUtf8(isolate, msg.c_str())),
          Undefined(isolate) };

      if (buffer && len > 0) {
        int chunk_id = GetNextChunkID();
        std::unique_ptr<std::vector<uint8_t>>
          received_chunk_ptr(new std::vector<uint8_t>(buffer, buffer + len));
        data_chunk_storage_.insert(
            std::make_pair(chunk_id, std::move(received_chunk_ptr)));
        // buffer should freed in ExtensionNode
        delete[] buffer;
        args[1] = Local<Value>::New(isolate, Integer::New(isolate, chunk_id));
      }
      Local<Function> func = Local<Function>::New(isolate, data_listener_);
      func->Call(func, 2, args);
    }
  } else {
    if (!message_listener_.IsEmpty()) {
      Local<Value> args[] = {
          Local<Value>::New(isolate, String::NewFromUtf8(isolate,msg.c_str())) };
      Local<Function> func = Local<Function>::New(isolate, message_listener_);
      func->Call(func, 1, args);
    }
  }
}

void NativeNode::Init(Handle<Object> target) {
  NODE_SET_METHOD(target, "initialize", Initialize);
  NODE_SET_METHOD(target, "getExtensions", GetExtensions);
  NODE_SET_METHOD(target, "updateRuntimeVariables", UpdateRuntimeVariables);
}

void NativeNode::Initialize(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  ExtensionManager* manager = ExtensionManager::GetInstance();

  // If the runtime variable is passed, update it.
  UpdateRuntimeVariables(args);

  manager->RegisterExtensionsByMetadata();

  // set trampoline flag
  ExtensionMap& map = manager->extensions();
  for (auto it1 = map.begin(); it1 != map.end(); ++it1 ) {
    std::string part;
    unsigned int pos = 0;
    while (true) {
      pos = it1->first.find(".", pos);
      if (pos == std::string::npos)
        part = it1->first;
      else
        part = it1->first.substr(0, pos);

      auto it2 = map.find(part);
      if (it2 != map.end()) {
        it2->second->set_use_trampoline(false);
        break;
      }

      if (pos == std::string::npos)
        break;
      pos++;
    }
  }

  initialized_ = true;

}

void NativeNode::UpdateRuntimeVariables(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (args.Length() > 0 && args[0]->IsObject()) {
    ExtensionManager* manager = ExtensionManager::GetInstance();

    manager->ClearRuntimeVariables();

    Local<Object> props = args[0]->ToObject();
    Local<Array> keys = props->GetPropertyNames();

    for (unsigned int i=0; i < keys->Length(); i++) {
      Handle<String> key = keys->Get(Integer::New(isolate, i))->ToString();
      Handle<Value> value = props->Get(key);

      if (value->IsString()) {
        String::Utf8Value key_utf8(key);
        String::Utf8Value value_utf8(value->ToString());
        manager->AddRuntimeVariable(std::string(*key_utf8), std::string(*value_utf8));
      }
    }

  }

  return;
}

void NativeNode::GetExtensions(const FunctionCallbackInfo<Value>& args) {
  if (!initialized_)
    return;

  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  ExtensionManager* manager = ExtensionManager::GetInstance();
  ExtensionMap& map = manager->extensions();

  if (extensions_.IsEmpty()) {
    Handle<Array> extensions = Array::New(isolate, map.size());
    extensions_.Reset(isolate, extensions);
    int idx = 0;
    for (auto iter = map.begin(); iter != map.end(); ++iter) {
      extensions->Set(idx++, ExtensionNode::NewInstance(iter->first));
    }
  }

  args.GetReturnValue().Set(extensions_);
}

} // namespace service
} // namespace wrt

extern "C" {
    void static NodeInit(Handle<Object> target) {
        wrt::service::ExtensionNode::Init(target);
        wrt::service::NativeNode::Init(target);
    }

    NODE_MODULE(native, NodeInit);
}
