//
// Created by mofan on 9/1/16.
//

#include "wxsignal.h"



static struct wx_signal_manager_s wx_signal_smgr = {0};


void wx_signal_reset() {
    memset(&wx_signal_smgr, 0, sizeof(struct wx_signal_manager_s));
}

static void wx_signal_log(int sig) {
    sig--;
    const uint64_t one = 1;
    wx_signal_smgr.signal_got |= (one<<sig);
}

void wx_signal_dispatch() {
    struct wx_signal_handler_s* h;
    const uint64_t one = 1;
    int i;
    for (i=0; i<64; i++) {
        if (wx_signal_smgr.signal_got & (one<<i)) {
            wx_signal_smgr.signal_got ^= one<<i;
            h = wx_signal_smgr.signal_handlers[i];
            while (h) {
                h->callback(i, h->data);
                h = h->next;
            }
        }
    }
}

void wx_signal_register(int sig, struct wx_signal_handler_s* h) {
    if (sig < 1 || sig > 64) {
        return;
    }
    sig--;
    struct wx_signal_handler_s* _h = wx_signal_smgr.signal_handlers[sig];
    if (_h == NULL) {
        signal(sig+1, wx_signal_log);
        wx_signal_smgr.signal_handlers[sig] = h;
    } else {
        while (_h->next) {
            _h = _h->next;
        }
        _h->next = h;
    }
}

void wx_signal_set(int sig, struct wx_signal_handler_s* h) {
    if (sig < 1 || sig > 64) {
        return;
    }
    signal(sig, wx_signal_log);
    sig--;
    wx_signal_smgr.signal_handlers[sig] = h;
}

void wx_signal_remove(int sig, struct wx_signal_handler_s* h) {
    if (sig < 1 || sig > 64) {
        return;
    }
    sig--;
    int l = 64;
    struct wx_signal_handler_s* _h;
    while (l--) {
        if (sig == l) {
            _h = wx_signal_smgr.signal_handlers[l];
            if (_h == h) {
                wx_signal_smgr.signal_handlers[l] = h->next;
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