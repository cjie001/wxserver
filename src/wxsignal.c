//
// Created by renwuxun on 3/16/17.
//


#include "wxsignal.h"


static int64_t wx_signal_got = 0;

static void wx_signal_comm_handler(int signo) {
    wx_signal_got |= 1<<(signo-1);
}

static struct wx_signal_s* wx_signal_conf[64] = {0};

void wx_signal_add(struct wx_signal_s* sgn) {
    if (sgn->signo < 1) {
        return;
    }

    struct wx_signal_s* cur = wx_signal_conf[sgn->signo-1];

    if (!cur) { // first
        wx_signal_conf[sgn->signo-1] = sgn;
        signal(sgn->signo, wx_signal_comm_handler);
        return;
    }

    for ( ; cur->next ; ) {
        cur = cur->next;
    }

    cur->next = sgn;
}

void wx_signal_del(struct wx_signal_s* sgn) {
    struct wx_signal_s* cur = wx_signal_conf[sgn->signo-1];
    struct wx_signal_s* last = NULL;

    for ( ; cur ; cur = cur->next) {
        if (cur == sgn) {
            if (last == NULL) { // last
                wx_signal_conf[sgn->signo-1] = NULL;
                signal(sgn->signo, SIG_DFL);
            } else {
                last->next = cur->next;
            }
            return;
        }
        last = cur;
    }
}

struct wx_signal_s* wx_signal_clear(int signo) {
    struct wx_signal_s* cur = wx_signal_conf[signo-1];

    if (!cur) {
        return NULL;
    }

    signal(signo, SIG_DFL);
    wx_signal_conf[signo-1] = NULL;

    return cur;
}

void wx_signal_dispatch() {
    struct wx_signal_s* sgn;
    int i;
    for (i=0; i<64; i++) {
        if (0 != wx_signal_got&(1<<i)) { // signo = i+1
            sgn = wx_signal_conf[i];//->sighandler(i+1);
            for ( ; sgn ; sgn=sgn->next) {
                sgn->sighandler(i+1, sgn->data);
            }
        }
    }
}