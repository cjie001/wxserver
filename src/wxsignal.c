//
// Created by renwuxun on 9/1/16.
//

#include "wxsignal.h"


// 一个进程只允许存在一个wx_signal
static struct wx_signal_s wx_signal = {0};


void wx_signal_init() {
    memset(&wx_signal, 0, sizeof(struct wx_signal_s));
}

static void wx_signal_log(int sig) {
    sig--;
    const uint64_t one = 1;
    wx_signal.signal_got |= (one<<sig);
}

void wx_signal_dispatch() {
    struct wx_signal_handler_s* h;
    const uint64_t one = 1;
    int i;
    for (i=0; i<64; i++) {
        if (wx_signal.signal_got & (one<<i)) {
            wx_signal.signal_got ^= one<<i;
            h = wx_signal.signal_handlers[i];
            while (h) {
                h->callback(i, h->data);
                h = h->next;
            }
        }
    }
}

void wx_signal_register(int sig, struct wx_signal_handler_s* h) {
    if (sig < 1 || sig > 64) {
        fprintf(stderr, "sig must in [1,64]\n");
        return;
    }

    sig--;
    struct wx_signal_handler_s* _h = wx_signal.signal_handlers[sig];
    if (_h == NULL) {
        signal(sig+1, wx_signal_log);
        wx_signal.signal_handlers[sig] = h;
    } else {
        while (_h->next) {
            _h = _h->next;
        }
        _h->next = h;
    }
}

void wx_signal_set(int sig, struct wx_signal_handler_s* h) {
    if (sig < 1 || sig > 64) {
        fprintf(stderr, "sig must in [1,64]\n");
        return;
    }

    signal(sig, wx_signal_log);
    sig--;
    wx_signal.signal_handlers[sig] = h;
}

void wx_signal_remove(int sig, struct wx_signal_handler_s* h) {
    if (sig < 1 || sig > 64) {
        fprintf(stderr, "sig must in [1,64]\n");
        return;
    }

    sig--;
    int l = 64;
    struct wx_signal_handler_s* _h;
    while (l--) {
        if (sig == l) {
            _h = wx_signal.signal_handlers[l];
            if (_h == h) {
                wx_signal.signal_handlers[l] = h->next;
            } else {
                while (_h->next) {
                    if (_h->next == h) {
                        _h->next = h->next;
                        break;
                    }
                    _h = _h->next;
                }
            }
            break;
        }
    }
}

void wx_signal_empty_handle(int sig, void* data) {}