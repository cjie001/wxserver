//
// Created by mofan on 9/1/16.
//

#ifndef WXSERVER_WXSRV_H
#define WXSERVER_WXSRV_H


#include "wxsignal.h"
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>


struct wx_worker_s {
    void* data;
    int id;
    int gotterm;
    pid_t pid;
    void (*call_from_master_on_term)(struct wx_worker_s* wkr);
    void (*call_from_master_on_exit_0)(struct wx_worker_s* wkr);
    void (*call_from_master_on_exit_err)(struct wx_worker_s* wkr);
    void (*job)(struct wx_worker_s* wkr);
    struct wx_worker_s* next;
    struct wx_master_s* master;
};

struct wx_master_s {
    int pid;
    int argc;
    char** argv;
    struct wx_worker_s* wkr;
    int gotterm;
};


int wx_worker_should_exit(struct wx_worker_s* wkr);

void wx_master_spawn(struct wx_master_s* mst, struct wx_worker_s* worker);

void wx_master_on_child_exit(int sig, void* data);

void wx_master_on_got_term(int sig, void* data);

#define wx_master_init_in_main(master)                                                 \
do {                                                                                   \
    memset((master), 0, sizeof(struct wx_master_s));                                   \
    (master)->pid = getpid();                                                          \
    (master)->argc = argc;                                                             \
    (master)->argv = argv;                                                             \
    struct wx_signal_handler_s child_exit_handler={0};                                 \
    child_exit_handler.next = NULL;                                                    \
    child_exit_handler.data = (master);                                                \
    child_exit_handler.callback = wx_master_on_child_exit;                             \
    wx_signal_register(SIGCHLD, &child_exit_handler);                                  \
    struct wx_signal_handler_s master_got_term_handler={0};                            \
    master_got_term_handler.next = NULL;                                               \
    master_got_term_handler.data = (master);                                           \
    master_got_term_handler.callback = wx_master_on_got_term;                          \
    wx_signal_register(SIGTERM, &master_got_term_handler);                             \
    struct wx_signal_handler_s master_got_int_handler={0};                             \
    master_got_int_handler.next = NULL;                                                \
    master_got_int_handler.data = (master);                                            \
    master_got_int_handler.callback = wx_master_on_got_term;                           \
    wx_signal_register(SIGINT, &master_got_int_handler);                               \
} while (0)

void wxsrv_empty_signal_handle(int sig, void* data);

#define wx_master_demonize_in_main()                                                   \
do {                                                                                   \
    struct wx_signal_handler_s empty_sig_handler;                                      \
    empty_sig_handler.data=NULL;                                                       \
    empty_sig_handler.next=NULL;                                                       \
    empty_sig_handler.callback = wxsrv_empty_signal_handle;                            \
    wx_signal_register(SIGTTOU, &empty_sig_handler);                                   \
    wx_signal_register(SIGTTIN, &empty_sig_handler);                                   \
    wx_signal_register(SIGTSTP, &empty_sig_handler);                                   \
    wx_signal_register(SIGHUP, &empty_sig_handler);                                    \
    switch (fork()){                                                                   \
        case -1:                                                                       \
            fprintf(stderr, "fork()==-1\n");                                           \
            exit(EXIT_FAILURE);                                                        \
            break;                                                                     \
        case 0:                                                                        \
            break;                                                                     \
        default:                                                                       \
            exit(0);                                                                   \
    }                                                                                  \
    setsid();                                                                          \
    switch (fork()) {                                                                  \
        case -1:                                                                       \
            fprintf(stderr, "fork()==-1\n");                                           \
            exit(EXIT_FAILURE);                                                        \
            break;                                                                     \
        case 0:                                                                        \
            break;                                                                     \
        default:                                                                       \
            exit(0);                                                                   \
    }                                                                                  \
    umask(0);                                                                          \
} while (0)

void wx_master_wait_workers(struct wx_master_s* mst);

void wx_master_init_worker(
        struct wx_worker_s* wkr,
        void(*job)(struct wx_worker_s*),
        void(*on_exit_0)(struct wx_worker_s*),
        void(*on_exit_err)(struct wx_worker_s*),
        void(*on_exit_term)(struct wx_worker_s*),
        struct wx_master_s* mst
);


#endif //WXSERVER_WXSRV_H
