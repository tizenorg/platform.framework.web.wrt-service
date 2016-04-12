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

#include <string.h>
#include <glib.h>
#include <dlog.h>
#include <v8.h>
#include <node.h>

#include "tizen-appfw.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "WRT_SERVICE"

using namespace node;
using namespace v8;

namespace wrt {
namespace service {

static int aulHandler(aul_type type, bundle* bd, void* data) {
  TizenAppFW* appfw = static_cast<TizenAppFW*>(data);

  if (appfw == NULL)
    return 0;

  switch(type) {
    case AUL_START:
    {
      int len;
      char* encoded_bundle;
      bundle_encode(bd, (bundle_raw**)&encoded_bundle, &len);
      appfw->OnService(encoded_bundle);
      free(encoded_bundle);
      break;
    }
    case AUL_TERMINATE:
      appfw->OnTerminate();
      break;
    default:
      LOGW("Unhandled aul event. type=%d", type);
      break;
  }
  return 0;
}

TizenAppFW& TizenAppFW::GetInstance() {
  static TizenAppFW instance;
  return instance;
}

TizenAppFW::TizenAppFW():initialized_(false) {
}

TizenAppFW::~TizenAppFW() {
}

void TizenAppFW::Init(int argc, char* argv[]) {
  if (initialized_)
    return;
  initialized_ = true;
  aul_launch_init(aulHandler, this);
  aul_launch_argv_handler(argc, argv);
}

void TizenAppFW::OnService(const char* bundle) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Handle<v8::Function> service_handler =
      v8::Local<v8::Function>::New(isolate, service_handler_);

  if (service_handler->IsFunction()) {
    Handle<String> v = String::NewFromUtf8(isolate, bundle);
    Handle<Value> args[1] = {v};
    service_handler->Call(service_handler, 1, args);
  }
}

void TizenAppFW::OnTerminate() {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Handle<v8::Function> terminate_handler =
      v8::Local<v8::Function>::New(isolate, terminate_handler_);

  if ( terminate_handler->IsFunction())
    terminate_handler->Call(terminate_handler, 0, NULL);
}

void TizenAppFW::set_service_handler(Handle<Function> handler) {
  service_handler_.Reset(Isolate::GetCurrent(), handler);
}

v8::Handle<v8::Function> TizenAppFW::service_handler() {
  return v8::Local<Function>::New(Isolate::GetCurrent(), service_handler_);
}

void TizenAppFW::set_terminate_handler(Handle<Function> handler) {
  terminate_handler_.Reset(Isolate::GetCurrent(), handler);
}

v8::Handle<v8::Function> TizenAppFW::terminate_handler() {
  return v8::Local<Function>::New(Isolate::GetCurrent(), terminate_handler_);
}

static void onServiceGetter(Local<String> /*property*/,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  Handle<Value> handler = TizenAppFW::GetInstance().service_handler();
  info.GetReturnValue().Set(handler);
}

static void onServiceSetter(Local<String> /*property*/, Local<Value> value,
    const v8::PropertyCallbackInfo<void>& /*info*/) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (value->IsFunction())
    TizenAppFW::GetInstance().set_service_handler(Handle<Function>::Cast(value));
}

static void onTerminateGetter(Local<String> /*property*/,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  Handle<Value> handler = TizenAppFW::GetInstance().terminate_handler();
  info.GetReturnValue().Set(handler);
}

static void onTerminateSetter(Local<String> /*property*/, Local<Value> value,
    const v8::PropertyCallbackInfo<void>& /*info*/) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (value->IsFunction())
    TizenAppFW::GetInstance().set_terminate_handler(Handle<Function>::Cast(value));
}

static void appfwInit(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (args.Length() < 1)
    return;

  if (!args[0]->IsArray())
    return;

  Local<Array> array_args = Local<Array>::Cast(args[0]);
  int argc = array_args->Length();
  {
    char *argv[argc];
    for (int i=0; i< argc; i++) {
      Local<String> arg = array_args->Get(i)->ToString();
      int nsize = arg->Utf8Length()+1;
      argv[i] = static_cast<char*>(malloc(nsize));
      if (argv[i]) {
        memset(argv[i], 0, nsize);
        arg->WriteUtf8(argv[i]);
      }
    }
    TizenAppFW::GetInstance().Init(argc, argv);
    for (int i=0; i<argc;i++) {
      free(argv[i]);
    }
  }
}

static void init(Handle<Object> target) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  TizenAppFW::GetInstance();
  target->Set(String::NewFromUtf8(isolate, "init"), v8::FunctionTemplate::New(isolate, appfwInit)->GetFunction());
  target->SetAccessor(String::NewFromUtf8(isolate, "onService"), onServiceGetter, onServiceSetter);
  target->SetAccessor(String::NewFromUtf8(isolate, "onTerminate"), onTerminateGetter, onTerminateSetter);
}

NODE_MODULE(appfw, init);

} // namespace service
} // namespace wrt
