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

    sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &servaddr.sin_addr);
    servaddr.sin_port = htons(30000);
    socklen_t socklen = sizeof(servaddr);
    int ret = connect(fd, (sockaddr*)&servaddr, socklen);
    LOG("connect = %d errno %d %m", ret, errno);
    const char *data = "12312321313";
    send(fd, data, strlen(data) + 1, 0);
    char buf[LEN_1K];
    ssize_t sret = recv(fd, buf, LEN_1K, 0);
    LOG("recv = %zd", sret);
    ret = connect(fd, (sockaddr*)&servaddr, socklen);
    LOG("connect = %d errno %d %m", ret, errno);
    close(fd);
    sleep(10);
    return 0;
}