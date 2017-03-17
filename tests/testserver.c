//
// Created by renwuxun on 9/1/16.
//



#include "../src/wxserver.h"


struct wx_worker_s w1 = {0};
struct wx_worker_s w2 = {0};

void worker_say(struct wx_worker_s* w) {
    fprintf(stdout, "%d say id:%d\n", getpid(), w->id);
    pause();
}

void exit_err(struct wx_worker_s* w) {
    fprintf(stderr, "id:%d exit error\n", w->id);
}

void exit_ok(struct wx_worker_s* w) {
    fprintf(stderr, "id:%d exit success\n", w->id);
}

void exit_withcmd(struct wx_worker_s* w) {
    fprintf(stderr, "id:%d exit withcmd\n", w->id);
}


int main(int argc, char** argv) {
    wx_master_init(exit_err, exit_ok, exit_withcmd);

    w1.job = worker_say;
    w2.job = worker_say;

    wx_master_spwan_worker(&w1);
    wx_master_spwan_worker(&w2);

    wx_master_wait_workers();

    printf("master exit\n");

    return 0;
}