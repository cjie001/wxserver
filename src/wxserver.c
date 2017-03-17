//
// Created by renwuxun on 9/1/16.
//


#include <errno.h>
#include "wxserver.h"
#include "wxsignal.h"


static struct wx_master_s wx__master_default = {0};

struct wx_master_s* wx_master_default() {
    return &wx__master_default;
}

static void wx_signal_empty_handler(int _, void* __) { }

static void wx_stop_workers(int signo, void* __) {
    wx__master_default.stop = 1;
    struct wx_worker_s* wkr = wx__master_default.wkrs;

    switch (signo) {
        case SIGTERM:
        case SIGINT:
            for ( ; wkr ; wkr = wkr->next) {
                kill(wkr->pid, SIGQUIT);
            }
            break;
        case SIGQUIT:
            for ( ; wkr ; wkr = wkr->next) {
                kill(wkr->pid, SIGWINCH);
            }
            break;
        default:;
    }
}

static void wx_send_winch(int _, void* __) {
    struct wx_worker_s* wkr = wx__master_default.wkrs;
    for ( ; wkr ; wkr = wkr->next) {
        kill(wkr->pid, SIGWINCH);
    }
}

static void wx_on_child_exit(int _, void* __);


struct wx_signal_s wx_signals[] = {
        {SIGTERM, wx_stop_workers, NULL, NULL},
        {SIGINT, wx_stop_workers, NULL, NULL},
        {SIGQUIT, wx_stop_workers, NULL, NULL},
        {SIGHUP, wx_send_winch, NULL, NULL},
        {SIGCHLD, wx_on_child_exit, NULL, NULL},
        {SIGWINCH, wx_signal_empty_handler, NULL, NULL},
        {SIGUSR1, wx_signal_empty_handler, NULL, NULL},
        {SIGUSR2, wx_signal_empty_handler, NULL, NULL},
        {SIGALRM, wx_signal_empty_handler, NULL, NULL},
        {SIGIO, wx_signal_empty_handler, NULL, NULL}
};


static void wx_master_spwan_worker_internal(struct wx_worker_s* w) {
    pid_t pid = fork();
    w->pid = pid;
    switch (pid) {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
        case 0://child
        {
            int i, l = sizeof(wx_signals) / sizeof(wx_signals[0]);
            for (i = 0; i < l; i++) {
                wx_signal_del(&wx_signals[i]);
            }
            if (w->job) {
                w->job(w);
            }
            exit(EXIT_SUCCESS);
        }
        default:;//parent
    }
}


static void wx_on_child_exit(int _, void* __) {
    struct wx_worker_s* wkr, *_wkr;
    int status;
    pid_t pid;

    for (;;) {
        pid = waitpid(-1, &status, WNOHANG);
        if (pid == 0) {
            break;
        }
        if (pid == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                break;
            }
        }

        wkr = wx__master_default.wkrs;
        for (;wkr;) {
            if (pid == wkr->pid) {
                break;
            }
            wkr = wkr->next;
        }

        if ((WIFEXITED(status) && 0 == WEXITSTATUS(status))) {
            if (wx__master_default.worker_exit_success) {
                wx__master_default.worker_exit_success(wkr);
            }
            // remove
            _wkr = wx__master_default.wkrs;
            if (_wkr == wkr) {
                wx__master_default.wkrs = _wkr->next;
            } else {
                for (;_wkr->next;) {
                    if (_wkr->next == wkr) {
                        _wkr->next = wkr->next;
                        break;
                    }
                    _wkr = _wkr->next;
                }
            }
            continue;
        }

        if (wx__master_default.stop) {
            if (wx__master_default.worker_exit_bycmd) {
                wx__master_default.worker_exit_bycmd(wkr);
            }
            // remove
            _wkr = wx__master_default.wkrs;
            if (_wkr == wkr) {
                wx__master_default.wkrs = _wkr->next;
            } else {
                for (;_wkr->next;) {
                    if (_wkr->next == wkr) {
                        _wkr->next = wkr->next;
                        break;
                    }
                    _wkr = _wkr->next;
                }
            }
            continue;
        }

        if (wx__master_default.worker_exit_error) {
            wx__master_default.worker_exit_error(wkr);
        }

        wx_master_spwan_worker_internal(wkr);
    }
}


void wx_master_init(
        void (*worker_exit_error)(struct wx_worker_s*),
        void (*worker_exit_success)(struct wx_worker_s*),
        void (*worker_exit_bycmd)(struct wx_worker_s*)
) {
    wx__master_default.worker_exit_error = worker_exit_error;
    wx__master_default.worker_exit_success = worker_exit_success;
    wx__master_default.worker_exit_bycmd = worker_exit_bycmd;

    int i, l = sizeof(wx_signals)/sizeof(wx_signals[0]);
    for (i=0; i<l; i++) {
        wx_signal_add(&wx_signals[i]);
    }

    signal(SIGSYS, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(0, NULL);
}


void wx_master_spwan_worker(struct wx_worker_s* w) {
    w->id = 0;
    struct wx_worker_s* wkr = wx__master_default.wkrs;
    for (;wkr;) {
        if (wkr == w) {
            fprintf(stderr, "u can not spawn a worker instance twice\n");
            exit(EXIT_FAILURE);
        }
        w->id++;
        wkr = wkr->next;
    }

    if (wx__master_default.wkrs == NULL) {
        wx__master_default.wkrs = w;
    } else {
        wkr = wx__master_default.wkrs;
        for (;wkr->next;) {
            wkr = wkr->next;
        }
        wkr->next = w;
    }

    wx_master_spwan_worker_internal(w);
}


void wx_master_daemonize() {
    switch (fork()) {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
        case 0:
            break;
        default:
            exit(EXIT_SUCCESS);
    }
    setsid();
    umask(0);
}


void wx_master_wait_workers() {
    usleep(100000);
    for (;;) {
        wx_signal_dispatch();
        if (!wx__master_default.wkrs) {
            break;
        }
        pause();
    }
}