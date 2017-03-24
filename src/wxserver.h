//
// Created by renwuxun on 9/1/16.
//

#ifndef WXSERVER_H
#define WXSERVER_H


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>

#ifdef DEVINFO
#define wx_dev(fmt,...) fprintf(stderr, ""fmt" [pid:%d "__FILE__":%d]\n", ##__VA_ARGS__, getpid(), __LINE__)
#else
#define wx_dev(fmt,...)
#endif

struct wx_worker_s {
    void* data;
    int id;
    pid_t pid;
    void (*job)(struct wx_worker_s*);
    struct wx_worker_s* next;
};

struct wx_master_s {
    int stop;
    void (*worker_exit_error)(struct wx_worker_s*);
    void (*worker_exit_success)(struct wx_worker_s*);
    void (*worker_exit_bycmd)(struct wx_worker_s*);
    struct wx_worker_s* wkrs;
};


struct wx_master_s* wx_master_default();

void wx_master_init(
        void (*worker_exit_error)(struct wx_worker_s*),
        void (*worker_exit_success)(struct wx_worker_s*),
        void (*worker_exit_bycmd)(struct wx_worker_s*)
);

void wx_master_spwan_worker(struct wx_worker_s* w);

void wx_master_daemonize();

void wx_master_wait_workers();


#endif //WXSERVER_H
