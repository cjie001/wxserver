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


void show_help(char* argv_0) {
    fprintf(stderr, "Usage: %s [OPTION]\n", argv_0);
    fprintf(stderr, "Option:\n");
    fprintf(stderr, "    -d daemonize, but you need to redirect stdout and stderr by yourselft.\n");
    fprintf(stderr, "    -h this message.\n");
}


int main(int argc, char** argv) {

    int daemon = 0;
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != -1) {
        switch (option_char) {
            case 'd':
                daemon = 1;
                break;
            case 'h':
                show_help(argv[0]);
                exit(EXIT_SUCCESS);
            case '?':
                show_help(argv[0]);
                exit(EXIT_FAILURE);
            default:;
        }
    }


    struct wx_master_s* master = wx_master_default();
    wx_master_init(master);
    if (daemon) {
        wx_master_demonize();
    }

    struct wx_worker_s wkr1;
    wx_master_init_worker(&wkr1, job, on_exit_0, on_exit_err, on_exit_term);
    struct wx_worker_s wkr2;
    wx_master_init_worker(&wkr2, job, on_exit_0, on_exit_err, on_exit_term);

    wx_master_spawn(master, &wkr1);
    wx_master_spawn(master, &wkr2);

    wx_master_wait_workers(master);

    printf("master exit\n");

    return 0;
}