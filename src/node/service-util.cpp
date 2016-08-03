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

#include <string>

#include <glib.h>
#include <sqlite3.h>
#include <dlog.h>
#include <node.h>
#include <v8.h>
#include <memory>
#include <security-manager.h>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <wgt_manifest_handlers/widget_config_parser.h>
#include <wgt_manifest_handlers/service_handler.h>
#include <wgt_manifest_handlers/application_manifest_constants.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "WRT_SERVICE"

using namespace node;
using namespace v8;

namespace {
    const char *kConfigXml = "/res/wgt/config.xml";
}

namespace wrt {
namespace service {

static std::string GetStartScript(const std::string& appid, const std::string& rootpath){
    std::string value;
    std::string config_xml_path(rootpath + kConfigXml);

    if (!boost::filesystem::exists(boost::filesystem::path(config_xml_path))) {
        LOGE("Failed to load config.xml data : No such file [%s]", config_xml_path.c_str());
        return value;
    }

    std::unique_ptr<wgt::parse::WidgetConfigParser> widget_config_parser;
    widget_config_parser.reset(new wgt::parse::WidgetConfigParser());
    if (!widget_config_parser->ParseManifest(config_xml_path)) {
        LOGE("Failed to load widget config parser data");
        return value;
    }

    std::shared_ptr<const wgt::parse::ServiceList> service_list =
        std::static_pointer_cast<const wgt::parse::ServiceList>(
            widget_config_parser->GetManifestData(
                wgt::application_widget_keys::kTizenServiceKey));
    if (!service_list)
        return value;

    for (auto& service : service_list->services) {
        std::string svc_appid = service.id();
        boost::filesystem::path svc_startfile = service.content();
        if (!boost::filesystem::exists(svc_startfile)) {
            return value;
        } else {
            if (!strcmp(svc_appid.c_str(), appid.c_str())) {
                value = svc_startfile.string();
                break;
            }
        }
    }

    return value;
}

static void InitAce(const std::string & /*appid*/){
    //wrt::common::AccessControl::GetInstance()->InitAppPrivileges(appid);
}

static void SetPrivilege(const std::string& appid){
    SECURE_LOGD("Set privilege : %s", pkgid.c_str());
    int ret = security_manager_set_process_label_from_appid(appid.c_str());
    if (ret != SECURITY_MANAGER_SUCCESS) {
        LOGE("error security_manager_set_process_label_from_appid : (%d)", ret);
    }
}

static void initAce(const FunctionCallbackInfo<Value>& args){
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    if( args.Length() < 1 )
        return;

    std::string appid(*String::Utf8Value(args[0]->ToString()));
    InitAce(appid);
}

static void getStartScript(const FunctionCallbackInfo<Value>& args){
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    if( args.Length() < 1 )
        return;

    std::string appid(*String::Utf8Value(args[0]->ToString()));
    std::string rootpath(*String::Utf8Value(args[1]->ToString()));
    std::string start_script = GetStartScript(appid, rootpath);

    args.GetReturnValue().Set(String::NewFromUtf8(isolate,start_script.c_str()));
}

static void setPrivilege(const FunctionCallbackInfo<Value>& args){
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    if( args.Length() < 1 ){
        LOGE("No enough arguments");
        return;
    }
    std::string appid(*String::Utf8Value(args[0]->ToString()));
    SetPrivilege(appid);
}

static void init(Handle<Object> target) {
    NODE_SET_METHOD(target, "getStartScript", getStartScript);
    NODE_SET_METHOD(target, "initAce", initAce);
    NODE_SET_METHOD(target, "setPrivilege", setPrivilege);
}

NODE_MODULE(serviceutil, init);

} // namespace service
} // namespace wrt

