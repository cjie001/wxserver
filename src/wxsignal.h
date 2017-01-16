//
// Created by mofan on 9/1/16.
//

#ifndef WXSERVER_WXSIGNAL_H
#define WXSERVER_WXSIGNAL_H


#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>


struct wx_signal_handler_s {
    void* data;
    void (*callback)(int sig, void* data);
    struct wx_signal_handler_s* next;
};

struct wx_signal_manager_s {
    uint64_t signal_got;
    struct wx_signal_handler_s* signal_handlers[64];
};

void wx_signal_reset();

void wx_signal_dispatch();

void wx_signal_register(int sig, struct wx_signal_handler_s* h);

void wx_signal_set(int sig, struct wx_signal_handler_s* h);

void wx_signal_remove(int sig, struct wx_signal_handler_s* h);



#endif //WXSERVER_WXSIGNAL_H
