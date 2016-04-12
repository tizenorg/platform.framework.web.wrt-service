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

#ifndef WRT_SERIVCE_NODE_TIZEN_APPFW_H_
#define WRT_SERIVCE_NODE_TIZEN_APPFW_H_

#include <string>
#include <aul.h>
#include <v8.h>

namespace wrt {
namespace service {

class TizenAppFW {
 public:
  static TizenAppFW& GetInstance();

  void set_service_handler(v8::Handle<v8::Function> handler);
  v8::Handle<v8::Function> service_handler();
  void set_terminate_handler(v8::Handle<v8::Function> handler);
  v8::Handle<v8::Function> terminate_handler();

  void Init(int argc, char* argv[]);
  void OnService(const char* bundle);
  void OnTerminate();
 private:
  TizenAppFW();
  virtual ~TizenAppFW();

  bool initialized_;
  v8::Persistent<v8::Function> service_handler_;
  v8::Persistent<v8::Function> terminate_handler_;
};

} // namespace service
} // namespace wrt

#endif // WRT_SERIVCE_NODE_TIZEN_APPFW_H_
