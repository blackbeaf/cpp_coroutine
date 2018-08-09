#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <stdio.h>
#include <vector>
#include <stdint.h>
#include <string>
#include <map>
#include <set>
#include <sstream>
#include <memory>
#include <list>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#define DEFAULT_MEM_SIZE 2*1024
#define TIME_EXPIRE 2000*1000

using namespace std;


template<int SIZE>
inline std::string __attribute__((format(printf, 1, 2))) format(const char *fmt, ...)
{
    char str[SIZE] = { 0 };
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(str, SIZE, fmt, arg);
    va_end(arg);
    return string(str);
}

#define LEN_1K 1024

inline string GetYMDHMS(time_t ts = time(NULL))
{
    tm timeinfo;
    localtime_r(&ts, &timeinfo);
    timeinfo.tm_year += 1900;
    ++timeinfo.tm_mon;
    return format<LEN_1K>("%u-%02u-%02u %02u:%02u:%02u",
                          timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday,
                          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}


#define LOG(fmt, args...) printf("[%s][%s:%d] " fmt "\n", GetYMDHMS().c_str(), __func__, __LINE__, ##args)

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0)
    {
        LOG("socket = %d %m", fd);
        return 0;
    }

    int opt = 1;
    // sockfd为需要端口复用的套接字
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
    sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &servaddr.sin_addr);
    servaddr.sin_port = htons(30000);

    if (-1 == ::bind(fd, (struct sockaddr*)&servaddr, sizeof(servaddr)))
    {
        LOG("BIND err %m");
        return 0;
    }

    int ret = listen(fd, 10);

    if (ret)
    {
        LOG("listen %m");
        return 0;
    }

    sockaddr clientaddr;
    socklen_t socklen = sizeof(clientaddr);
    int fd_client = accept(fd, &clientaddr, &socklen);
    LOG("accept client fd %d", clientaddr);
    char buf[LEN_1K] = { 0 };
    ssize_t sret = recv(fd_client, buf, LEN_1K, 0);
    LOG("recv = %zd %s", sret, buf);
    sleep(10);
    LOG("do close fd %d", fd_client);
    close(fd_client);
    LOG("do send");
    sret = send(fd_client, "12321321", 6, 0);
    LOG("send = %zd %m", sret);
    fd_client = accept(fd, &clientaddr, &socklen);
    LOG("accept client fd %d", clientaddr);
    sleep(10);
    return 0;
}