//
// Created by renwuxun on 9/1/16.
//



#include "../src/wxserver.h"


void job(struct wx_worker_s* wkr) {
    printf("worker start [from worker:%d]\n", getpid());
    while (!wx_worker_should_exit(wkr)) {
        printf("hello renwuxun [from worker:%d]\n", getpid());
        usleep(1000 * 1000);
    }
    printf("worker end [from worker:%d]\n", getpid());
    exit(1); //非零退出码将导致worker被重新拉起
}

void on_exit_0(struct wx_worker_s* wkr) {
    printf("worker %d exit with ok [from master]\n", wkr->pid);
}

void on_exit_err(struct wx_worker_s* wkr) {
    printf("worker %d exit with error [from master]\n", wkr->pid);
}

void on_exit_term(struct wx_worker_s* wkr) {
    printf("worker %d exit with signal term [from master]\n", wkr->pid);
}



int main(int argc, char** argv) {
    struct wx_master_s* master = wx_master_default();
    wx_master_init_in_main(master);
    if (argc > 1 && 0==strncmp(argv[1], "-d", 2)) {
        wx_master_demonize_in_main();
    }

    struct wx_worker_s wkr1;
    wx_master_init_worker(&wkr1, job, on_exit_0, on_exit_err, on_exit_term, master);
    struct wx_worker_s wkr2;
    wx_master_init_worker(&wkr2, job, on_exit_0, on_exit_err, on_exit_term, master);

    wx_master_spawn(master, &wkr1);
    wx_master_spawn(master, &wkr2);

    wx_master_wait_workers(master);

    printf("master exit\n");

    return 0;
}