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

#ifndef WRT_SERVICE_NODE_GCONTEXT_H_
#define WRT_SERVICE_NODE_GCONTEXT_H_

#include <glib.h>
#include <list>
#include <uv.h>

namespace wrt {
namespace service {

class poll_handler;

class GContext {
public:
    GContext();
    virtual ~GContext();
    void Init();
    void Uninit();

private:
    void onPrepare();
    void onCheck();
    void onTimeout();

    static void OnPrepare(uv_prepare_t* handle);
    static void OnCheck(uv_check_s* handle);
    static void OnTimeout(uv_timer_s* handle);

    bool initialized_;
    GMainContext *context_;
    int max_priority_;

    GPollFD *fd_list_;
    int fd_list_size_;
    int fd_count_;
    std::list<poll_handler*> poll_handle_list_;

    uv_prepare_t *prepare_handle_;
    uv_check_t *check_handle_;
    uv_timer_t *timeout_handle_;

};

} // namespace service
} // namespace wrt

#endif // WRT_SERVICE_NODE_GCONTEXT_H_
