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

#include <stdlib.h>
#include <string.h>
#include <glib-object.h>

#include <v8.h>
#include <node.h>

#include "gcontext.h"

using namespace node;
using namespace v8;

namespace wrt {
namespace service {

struct poll_handler {
    int fd;
    uv_poll_t *uv_poll;
    int eventmask;
    int ref;
    std::list<GPollFD*> fd_list;
};

GContext::GContext():initialized_(false){
}

GContext::~GContext(){
};

void GContext::Init(){
    if(initialized_)
        return;
    initialized_ = true;

    GMainContext *gc = g_main_context_default();

#if !GLIB_CHECK_VERSION(2, 35, 0)
    g_type_init();
#endif

    g_main_context_acquire(gc);
    context_ = g_main_context_ref(gc);
    fd_list_ = NULL;
    fd_list_size_ = 0;
    fd_count_ = 0;


    // allocate memory to memory
    prepare_handle_ = (uv_prepare_t*)malloc(sizeof(uv_prepare_t));
    check_handle_ = (uv_check_t*)malloc(sizeof(uv_check_t));
    timeout_handle_ = (uv_timer_t*)malloc(sizeof(uv_timer_t));

    assert(prepare_handle_ != NULL);
    assert(check_handle_ != NULL);
    assert(timeout_handle_ != NULL);

    memset(prepare_handle_, 0, sizeof(uv_prepare_t));
    memset(check_handle_, 0, sizeof(uv_check_t));
    memset(timeout_handle_, 0, sizeof(uv_timer_t));

    uv_prepare_init(uv_default_loop(), prepare_handle_);
    uv_check_init(uv_default_loop(), check_handle_);
    uv_timer_init(uv_default_loop(), timeout_handle_);

    prepare_handle_->data = this;
    check_handle_->data = this;
    timeout_handle_->data = this;

    uv_prepare_start(prepare_handle_, GContext::OnPrepare);
    uv_check_start(check_handle_, GContext::OnCheck);

}

void GContext::Uninit(){
    if(!initialized_)
        return;

    initialized_ = false;

    /* Remove all handlers */
    std::list<poll_handler*>::iterator itr = poll_handle_list_.begin();
    while(itr != poll_handle_list_.end()) {
        /* Stop polling handler */
        uv_unref((uv_handle_t *)(*itr)->uv_poll);
        uv_poll_stop((*itr)->uv_poll);
        uv_close((uv_handle_t *)(*itr)->uv_poll, (uv_close_cb)free);
        delete *itr;
        itr = poll_handle_list_.erase(itr);
    }

    uv_unref((uv_handle_t *)check_handle_);
    uv_unref((uv_handle_t *)prepare_handle_);
    uv_unref((uv_handle_t *)timeout_handle_);

    uv_check_stop(check_handle_);
    uv_prepare_stop(prepare_handle_);
    uv_timer_stop(timeout_handle_);

    uv_close((uv_handle_t *)check_handle_, (uv_close_cb)free);
    uv_close((uv_handle_t *)prepare_handle_, (uv_close_cb)free);
    uv_close((uv_handle_t *)timeout_handle_, (uv_close_cb)free);

    check_handle_ = NULL;
    prepare_handle_ = NULL;
    timeout_handle_ = NULL;

    g_free(fd_list_);
    fd_list_ = NULL;

    /* Release GMainContext loop */
    g_main_context_unref(context_);
}

static void poll_cb(uv_poll_t *uv_handle, int status, int events){
    poll_handler *handle = static_cast<poll_handler*>(uv_handle->data);
    if( status == 0 ){
        //printf("poll cb!!!! fd = %d, read = %d , write  = %d \n",handle->fd, events & UV_READABLE , events & UV_WRITABLE );
        std::list<GPollFD*>::iterator itr = handle->fd_list.begin();
        while( itr != handle->fd_list.end()){
            GPollFD* pfd = *itr;
            pfd->revents |= pfd->events & ((events & UV_READABLE ? G_IO_IN : 0) | (events & UV_WRITABLE ? G_IO_OUT : 0));
            ++itr;
        }
    }else{
        uv_poll_stop(uv_handle);
    }
}

void GContext::onPrepare(){
    int i;
    int timeout;

    g_main_context_prepare(context_, &max_priority_);

    /* Getting all sources from GLib main context */
    while(fd_list_size_ < (fd_count_ = g_main_context_query(context_,
            max_priority_,
            &timeout,
            fd_list_,
            fd_list_size_))){
        g_free(fd_list_);
        fd_list_size_ = fd_count_;
        fd_list_ = g_new(GPollFD, fd_list_size_);
    }

    //printf("context query : fdCount=%d , timeout = %d\n", fd_count_, timeout);

    /* Poll */
    if (fd_count_ > 0) {
        char flagsTable[fd_count_];
        memset(flagsTable, 0, fd_count_);
        std::list<poll_handler*>::iterator itr = poll_handle_list_.begin();

        //for each poll handler
        while ( itr != poll_handle_list_.end() ) {
            poll_handler *handle = *itr;
            int origin_mask = handle->eventmask;
            handle->ref = 0;
            handle->eventmask = 0;
            handle->fd_list.clear();

            //check already initialized poll handles
            for (i = 0; i < fd_count_; ++i) {
                GPollFD *pfd = fd_list_ + i;
                if (handle->fd == pfd->fd){
                    flagsTable[i] = 1;
                    handle->ref++;
                    pfd->revents = 0;
                    handle->eventmask |= (pfd->events & G_IO_IN ? UV_READABLE: 0) | (pfd->events & G_IO_OUT ? UV_WRITABLE: 0);
                    handle->fd_list.push_back(pfd);
                }
            }

            // remasking events
            if( handle->ref > 0){
               if( handle->eventmask != origin_mask ){
                    //printf("polling restart fd = %d\n", handle->fd);
                    uv_poll_stop(handle->uv_poll);
                    uv_poll_start(handle->uv_poll, handle->eventmask, poll_cb);
                }
            }

            // remove unused poll handles
            if( handle->ref == 0 ){
                uv_unref((uv_handle_t *)handle->uv_poll);
                uv_poll_stop(handle->uv_poll);
                uv_close((uv_handle_t *)handle->uv_poll, (uv_close_cb)free);
                itr = poll_handle_list_.erase(itr);
                delete handle;
            }else{
                ++itr;
            }
        }

        std::list<poll_handler*> new_poll_fds;
        /* Process current file descriptors from GContext */
        for (i = 0; i < fd_count_; ++i) {
            GPollFD *pfd = &fd_list_[i];
            int exists = flagsTable[i];
            if (exists)
                continue;

            pfd->revents = 0;
            for( itr = new_poll_fds.begin(); itr != new_poll_fds.end(); ++itr){
                poll_handler *handle = *itr;
                if(handle->fd == pfd->fd){
                    int oldmask = handle->eventmask;
                    handle->eventmask |= (pfd->events & G_IO_IN ? UV_READABLE: 0) | (pfd->events & G_IO_OUT ? UV_WRITABLE: 0);
                    if( oldmask != handle->eventmask ){
                        uv_poll_stop(handle->uv_poll);
                        uv_poll_start(handle->uv_poll, handle->eventmask, poll_cb);
                    }
                    exists = 1;
                    break;
                };
            }

            if(exists)
                continue;

            /* Preparing poll handler */
            struct poll_handler* handle = new poll_handler();
            handle->fd = pfd->fd;
            handle->ref = 1;

            /* Create uv poll handler, then append own poll handler on it */
            uv_poll_t *pt = (uv_poll_t *)malloc(sizeof(uv_poll_t));
            memset(pt,0, sizeof(uv_poll_t));
            pt->data = handle;
            handle->uv_poll= pt;

            uv_poll_init(uv_default_loop(), pt, pfd->fd);
            //printf("uv poll start fd = %d\n", pfd->fd);
            int uv_events = (pfd->events & G_IO_IN ? UV_READABLE: 0) | (pfd->events & G_IO_OUT ? UV_WRITABLE: 0);
            handle->eventmask = uv_events;
            uv_poll_start(pt, uv_events, poll_cb);
            new_poll_fds.push_back(handle);
        }
        if( !new_poll_fds.empty() )
            poll_handle_list_.merge(new_poll_fds);
    }

    if( timeout >= 0 ){
        uv_timer_start(timeout_handle_, OnTimeout, timeout, 0);
    }


}


void GContext::OnPrepare(uv_prepare_t* handle){
    GContext *This = static_cast<GContext*>(handle->data);
    This->onPrepare();
}

void GContext::OnCheck(uv_check_s* handle){
    GContext* This = static_cast<GContext*>(handle->data);
    This->onCheck();
}

void GContext::onCheck(){
    int ready = g_main_context_check(context_, max_priority_, fd_list_, fd_count_);
    if (ready){
        g_main_context_dispatch(context_);
    }
}

void GContext::OnTimeout(uv_timer_s* handle){
    GContext* This = static_cast<GContext*>(handle->data);
    This->onTimeout();
}

void GContext::onTimeout(){
}


static GContext *g_context = NULL;

static void GContextInit(const FunctionCallbackInfo<Value>& /*args*/){
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    if (g_context == NULL) {
        g_context = new GContext();
    }
    g_context->Init();
}

static void GContextUninit(const FunctionCallbackInfo<Value>& /*args*/){
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    if (g_context != NULL) {
        g_context->Uninit();
        delete g_context;
        g_context = NULL;
    }
}

static void init(Handle<Object> target) {
    NODE_SET_METHOD(target, "init", GContextInit);
    NODE_SET_METHOD(target, "uninit", GContextUninit);
}

NODE_MODULE(gcontext, init);

} // namespace service
} // namespace wrt
