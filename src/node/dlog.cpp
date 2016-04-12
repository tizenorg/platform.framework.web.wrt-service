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
#include <dlog.h>
#include <node.h>
#include <v8.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "WRT_SERVICE"

using namespace node;
using namespace v8;

namespace wrt {
namespace service {

static void logd(const char* tag, const char* format, ...){
    va_list args;
    va_start(args, format);
    LOG_VA(LOG_DEBUG, tag, format, args);
    va_end(args);
}

static void logv(const char* tag, const char* format, ...){
    va_list args;
    va_start(args, format);
    LOG_VA(LOG_VERBOSE, tag, format, args);
    va_end(args);
}

static void loge(const char* tag, const char* format, ...){
    va_list args;
    va_start(args, format);
    LOG_VA(LOG_ERROR, tag, format, args);
    va_end(args);
}

static void logD(const FunctionCallbackInfo<Value>& args){
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    if( args.Length() == 1 ){
        logd(LOG_TAG, *String::Utf8Value(args[0]->ToString()));
    }else if( args.Length() > 1 ){
        logd(*String::Utf8Value(args[0]->ToString()), *String::Utf8Value(args[1]->ToString()));
    }
    args.GetReturnValue().Set(Undefined(isolate));
}

static void logV(const FunctionCallbackInfo<Value>& args){
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    if( args.Length() == 1 ){
        logv(LOG_TAG, *String::Utf8Value(args[0]->ToString()));
    }else if( args.Length() > 1 ){
        logv(*String::Utf8Value(args[0]->ToString()), *String::Utf8Value(args[1]->ToString()));
    }
    args.GetReturnValue().Set(Undefined(isolate));
}

static void logE(const FunctionCallbackInfo<Value>& args){
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    if( args.Length() == 1 ){
        loge(LOG_TAG, *String::Utf8Value(args[0]->ToString()));
    }else if( args.Length() > 1 ){
        loge(*String::Utf8Value(args[0]->ToString()), *String::Utf8Value(args[1]->ToString()));
    }
    args.GetReturnValue().Set(Undefined(isolate));
}
static void init(Handle<Object> target) {
    NODE_SET_METHOD(target, "log", logD);
    NODE_SET_METHOD(target, "logd", logD);
    NODE_SET_METHOD(target, "logv", logV);
    NODE_SET_METHOD(target, "loge", logE);
}

NODE_MODULE(nodedlog, init);

} // namespace service
} // namespace wrt

