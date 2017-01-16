//
// Created by mofan on 9/1/16.
//


#include "wxsrv.h"


void wx_worker_on_got_term(int sig, void* data) {
    struct wx_worker_s* wkr = (struct wx_worker_s*)data;
    wkr->gotterm = 1;
}

int wx_worker_should_exit(struct wx_worker_s* wkr) {
    if (wkr->pid != 0) {
        printf("u can not call wx_worker_should_exit() in master context");
        exit(1);
    }
    wx_signal_dispatch();
    return wkr->gotterm;
}

void wx_master_internal_spawn(struct wx_worker_s* worker) {
    pid_t pid = fork();
    worker->pid = pid;
    switch (pid) {
        case -1:
            fprintf(stderr, "fork()==-1\n");
            exit(EXIT_FAILURE);
            break;
        case 0://child
            {
                wx_signal_reset();
                signal(SIGCHLD, SIG_IGN);
                signal(SIGTERM, SIG_IGN);
                signal(SIGINT, SIG_IGN);
                struct wx_signal_handler_s h = {0};
                h.next = NULL;
                h.data = worker;
                h.callback = wx_worker_on_got_term;
                wx_signal_set(SIGTERM, &h);
                worker->job(worker);
                exit(0);
            }
            break;
        default:;//parent
    }
}

void wx_master_spawn(struct wx_master_s* mst, struct wx_worker_s* worker) {
    worker->id = 0;
    struct wx_worker_s* wkr = mst->wkr;
    for (;wkr;) {
        if (wkr == worker) {
            fprintf(stderr, "u can not spawn a worker instance twice\n");
            exit(EXIT_FAILURE);
        }
        worker->id++;
        wkr = wkr->next;
    }

    if (mst->wkr == NULL) {
        mst->wkr = worker;
    } else {
        wkr = mst->wkr;
        for (;wkr->next;) {
            wkr = wkr->next;
        }
        wkr->next = worker;
    }

    wx_master_internal_spawn(worker);
}

void wx_master_on_child_exit(int sig, void* data) {
    struct wx_master_s* mst = (struct wx_master_s*)data;
    int status;
    pid_t pid;
    struct wx_worker_s* wkr, *_wkr=NULL;
    for (;(pid = waitpid(-1, &status, WNOHANG)) > 0;) {
        // on child exit
        wkr = mst->wkr;
        for (;wkr;) {
            if (wkr->pid == pid) {
                _wkr = wkr;
                break;
            }
            wkr = wkr->next;
        }
        if (!_wkr) {
            fprintf(stderr, "worker not found[pid=%d]\n", pid);
            exit(EXIT_FAILURE);
        }

        if (mst->gotterm) {
            if (_wkr->call_from_master_on_term) {
                _wkr->call_from_master_on_term(_wkr);
            }
            // remove
            if (mst->wkr == _wkr){
                mst->wkr = _wkr->next;
            } else {
                wkr = mst->wkr;
                for (;wkr->next;) {
                    if (wkr->next == _wkr) {
                        wkr->next = _wkr->next;
                        break;
                    }
                    wkr = wkr->next;
                }
            }
            continue;
        }

        if (WIFEXITED(status) && 0 == WEXITSTATUS(status)) {
            if (_wkr->call_from_master_on_exit_0) {
                _wkr->call_from_master_on_exit_0(_wkr);
            }
            // remove
            if (mst->wkr == _wkr){
                mst->wkr = _wkr->next;
            } else {
                wkr = mst->wkr;
                for (;wkr->next;) {
                    if (wkr->next == _wkr) {
                        wkr->next = _wkr->next;
                        break;
                    }
                    wkr = wkr->next;
                }
            }
            continue;
        }

        if (_wkr->call_from_master_on_exit_err) {
            _wkr->call_from_master_on_exit_err(_wkr);
        }

        wx_master_internal_spawn(_wkr);
    }
}

void wx_master_on_got_term(int sig, void* data) {
    fprintf(stderr, "wx_master_on_got_term\n");
    struct wx_master_s* mst = (struct wx_master_s*)data;
    mst->gotterm = 1;
    struct wx_worker_s* wkr = mst->wkr;
    for (;wkr;) {
        kill(wkr->pid, SIGTERM);
        wkr = wkr->next;
    }
}

void wxsrv_empty_signal_handle(int sig, void* data) {}

void wx_master_wait_workers(struct wx_master_s* mst) {
    while (mst->wkr) {
        wx_signal_dispatch();
        usleep(100000);
    }
}

void wx_master_init_worker(
        struct wx_worker_s* wkr,
        void(*job)(struct wx_worker_s*),
        void(*on_exit_0)(struct wx_worker_s*),
        void(*on_exit_err)(struct wx_worker_s*),
        void(*on_exit_term)(struct wx_worker_s*),
        struct wx_master_s* mst
) {
    wkr->next = NULL;
    wkr->gotterm = 0;
    wkr->job = job;
    wkr->call_from_master_on_exit_0 = on_exit_0;
    wkr->call_from_master_on_exit_err = on_exit_err;
    wkr->call_from_master_on_term = on_exit_term;
    wkr->master = mst;
}
