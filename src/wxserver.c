//
// Created by renwuxun on 9/1/16.
//


#include "wxserver.h"


static struct wx_master_s wx__master_default = {0};

struct wx_master_s* wx_master_default() {
    return &wx__master_default;
}

static void wx_flag_stop(int sig) {
    wx__master_default.stop = 1;
    struct wx_worker_s* wkr = wx__master_default.wkrs;
    for (;wkr;) {
        kill(wkr->pid, sig);
        wkr =  wkr->next;
    }
}


static void wx_master_spwan_worker_internal(struct wx_worker_s* w) {
    pid_t pid = fork();
    w->pid = pid;
    switch (pid) {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
        case 0://child
        {
            signal(SIGTERM, SIG_DFL);
            signal(SIGINT, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);
            sigprocmask(SIG_SETMASK, &wx__master_default.orignal_set, NULL);
            if (w->job) {
                w->job(w);
            }
            exit(EXIT_SUCCESS);
        }
        default:;//parent
    }
}


static void wx_on_child_exit(int _) {
    struct wx_worker_s* wkr, *_wkr;
    int status;
    pid_t pid;
    for (;(pid = waitpid(-1, &status, WNOHANG)) > 0;) {
        wkr = wx__master_default.wkrs;
        for (;wkr;) {
            if (pid == wkr->pid) {
                break;
            }
            wkr = wkr->next;
        }

        if (wx__master_default.stop || (WIFEXITED(status) && 0 == WEXITSTATUS(status))) {
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

        if (wx__master_default.worker_exit_error) {
            wx__master_default.worker_exit_error(wkr);
        }

        wx_master_spwan_worker_internal(wkr);
    }
}


void wx_master_init(void (*worker_exit_error)(struct wx_worker_s*), void (*worker_exit_success)(struct wx_worker_s*)) {
    wx__master_default.worker_exit_error = worker_exit_error;
    wx__master_default.worker_exit_success = worker_exit_success;

    signal(SIGTERM, wx_flag_stop);
    signal(SIGINT, wx_flag_stop);
    signal(SIGCHLD, wx_on_child_exit);

    sigemptyset(&wx__master_default.save_sigs);
    // 这些信号需要拦截下来
    sigaddset(&wx__master_default.save_sigs, SIGCHLD);
    sigaddset(&wx__master_default.save_sigs, SIGINT);
    sigaddset(&wx__master_default.save_sigs, SIGTERM);
    sigaddset(&wx__master_default.save_sigs, SIGQUIT);
    sigaddset(&wx__master_default.save_sigs, SIGHUP);
    sigaddset(&wx__master_default.save_sigs, SIGALRM);
    sigaddset(&wx__master_default.save_sigs, SIGIO);
    sigaddset(&wx__master_default.save_sigs, SIGTTOU);
    sigaddset(&wx__master_default.save_sigs, SIGTTIN);
    sigaddset(&wx__master_default.save_sigs, SIGTSTP);

    sigprocmask(SIG_BLOCK, &wx__master_default.save_sigs, &wx__master_default.orignal_set);
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
    for (;;) {
        sigsuspend(&wx__master_default.orignal_set);
        if (!wx__master_default.wkrs) {
            break;
        }
    }
}