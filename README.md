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