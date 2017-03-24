# wxserver
简易master/worker架构工具包

# tests编译
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Debug ..
    make

# tests运行
    ./testmaster -w ./testworker -n 2
## testworker收到的环境变量
    LISTEN_FD master监听的fd
    WKR_COUNT worker数量
    WKR_ID worker编号

# 信号
    worker: -QUIT=立即退出 -WINCH=平滑退出
    master: -INT/-TERM=立即退出 -QUIT=平滑退出