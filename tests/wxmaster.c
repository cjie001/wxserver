//
// Created by renwuxun on 9/1/16.
//



#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../src/wxserver.h"


//#define __USE_GNU


struct worker_env_s {
    int listen_fd;
    int worker_count;
    char* file;
    struct wx_master_s* master;
};


void job(struct wx_worker_s* wkr) {
    struct worker_env_s* wkr_env = (struct worker_env_s*)wkr->data;

    struct wx_worker_s* _wkr = wkr_env->master->wkr;
    struct worker_env_s* _wkr_env;
    for (;_wkr;) {
        if (_wkr != wkr) {
            _wkr_env = (struct worker_env_s*)_wkr->data;
            close(_wkr_env->listen_fd);
        }
        _wkr =  _wkr->next;
    }

    char* arg[] = {wkr_env->file, NULL};

    char envlistenfd[64];
    sprintf(envlistenfd, "LISTEN_FD=%d", wkr_env->listen_fd);
    char envworkerid[64];
    sprintf(envworkerid, "WKR_ID=%d", wkr->id);
    char envworkercount[64];
    sprintf(envworkercount, "WKR_COUNT=%d", wkr_env->worker_count);
    char* env[] = {envlistenfd, envworkerid, envworkercount, NULL};

    execve(wkr_env->file, arg, env);
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
    fprintf(stderr, "    -w worker file path\n");
    fprintf(stderr, "    -n worker process number, 1 by default\n");
    fprintf(stderr, "    -i listen ip, 0.0.0.0 by default\n");
    fprintf(stderr, "    -p listen port, 8080 by default\n");
    fprintf(stderr, "    -d daemonize, but you need to redirect stdout and stderr by yourselft\n");
    fprintf(stderr, "    -h this message\n");
}

int wxmaster_listen(char* ip, uint16_t port) {
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if (listenfd<0) {
        fprintf(stderr, "error on create socket(PF_INET, SOCK_STREAM, 0)\n");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in srvaddr;
    memset(&srvaddr, 0, sizeof(struct sockaddr_in));
    srvaddr.sin_family = PF_INET;
    srvaddr.sin_addr.s_addr = inet_addr(ip);
    srvaddr.sin_port = htons(port);

    int one = 1;
#ifdef SO_REUSEPORT
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
#endif
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    fcntl(listenfd, F_SETFL, fcntl(listenfd, F_GETFL) | O_NONBLOCK);

    if (bind(listenfd, (struct sockaddr*)&srvaddr, sizeof(struct sockaddr)) < 0) {
        fprintf(stderr, "bind error [ip:%s, port:%d]\n", ip, port);
        exit(EXIT_FAILURE);
    }
    if (listen(listenfd, 511) < 0) {
        fprintf(stderr, "listen error [ip:%s, port:%d]\n", ip, port);
        exit(EXIT_FAILURE);
    }

    return listenfd;
}


int main(int argc, char** argv) {

    char workerfile[128] = {0};
    int workercount = 1;
    char ip[16] = "127.0.0.1";
    int port = 8080;
    int daemon = 0;
    int option_char;
    while ((option_char = getopt(argc, argv, "w:n:i:p:dh")) != -1) {
        switch (option_char) {
            case 'w':
                memcpy(workerfile, optarg, strlen(optarg));
                break;
            case 'n':
                workercount = atoi(optarg);
                break;
            case 'i':
                memset(ip, 0, 16);
                memcpy(ip, optarg, strlen(optarg));
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'd':
                daemon = 1;
                break;
            case 'h':
                show_help(argv[0]);
                exit(EXIT_SUCCESS);
            case '?':
                show_help(argv[0]);
                exit(EXIT_FAILURE);
            default:
                show_help(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (strlen(workerfile) < 1 || access(workerfile, X_OK)!=0) {
        fprintf(stderr, "worker file [%s] not exist\n", workerfile);
        show_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    struct wx_master_s* master = wx_master_default();
    wx_master_init(master);
    if (daemon) {
        wx_master_demonize();
    }

#ifndef SO_REUSEPORT
    int listen_fd = wxmaster_listen(ip, (uint16_t)port);
#endif

    struct worker_env_s wkr_envs[workercount];
    struct wx_worker_s wkrs[workercount];
    int id;
    for (id=0; id<workercount; id++) {
        wkr_envs[id].master = master;
        wkr_envs[id].file = workerfile;
        wkr_envs[id].worker_count = workercount;
#ifdef SO_REUSEPORT
        wkr_envs[id].listen_fd = wxmaster_listen(ip, (uint16_t)port);
#else
        wkr_envs[id].listen_fd = listen_fd;
#endif
        wkrs[id].data = &wkr_envs[id];
        wx_master_init_worker(&wkrs[id], job, on_exit_0, on_exit_err, on_exit_term);
        wx_master_spawn(master, &wkrs[id]);
    }

    wx_master_wait_workers(master);

    printf("master exit\n");

    return 0;
}