//
// Created by renwuxun on 3/16/17.
//

#ifndef WXSIGNAL_H
#define WXSIGNAL_H


#include <signal.h>
#include <sys/types.h>
#include <stddef.h>


//SIGTERM
//SIGINT
//SIGQUIT
//SIGHUP
//SIGCHLD
//SIGWINCH
//SIGUSR1
//SIGUSR2
//SIGALRM
//SIGIO
//SIGSYS
//SIGPIPE


struct wx_signal_s {
    int signo;
    void (*sighandler)(int signo, void*);
    void* data;
    struct wx_signal_s* next;
};


void wx_signal_add(struct wx_signal_s* sgn);

void wx_signal_del(struct wx_signal_s* sgn);

struct wx_signal_s* wx_signal_clear(int signo);

void wx_signal_dispatch();


#endif //WXSIGNAL_H
