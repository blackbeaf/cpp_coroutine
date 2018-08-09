#pragma once

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

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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

struct nocopy
{
    nocopy() {}
    virtual ~nocopy() {}

private:
    nocopy(const nocopy&);
    nocopy& operator=(const nocopy&);
};

template<typename T>
class single
{
public:
    static T& instance()
    {
        static T a;
        return a;
    }
};

enum EIORESULT
{
    IO_OK,
    IO_SOCKET_ERR,
    IO_ERR,
    IO_ARG_INVALID,

    IO_SEND_AGAIN,
    IO_TIMEOUT,
    IO_REMOTE_CLOSE,
    IO_CONNECT_WAIT,
};

enum EPORTTYPE
{
    PORT_TCP,
    PORT_UDP
};

enum EFDEVENT
{
    FD_READ = EPOLLIN,
    FD_WRITE = EPOLLOUT,
    FD_READ_WRITE = EPOLLIN | EPOLLOUT,
};

enum ENodeState
{
    E_READY,
    E_WAIT,
    E_END,
};


class CMemData
{
    typedef vector<uint8_t> Buffer;
    Buffer m_data;
    uint32_t m_offset;

public:

    CMemData()
    {
        m_offset = 0;
    }

    CMemData(const CMemData &obj)
    {
        m_data = obj.m_data;
        m_offset = obj.m_offset;
    }
    //CMemData(const CMemData &obj):m_data(obj.m_data), m_offset(obj.m_offset)
    //{
    //}

    //CMemData& operator=(const CMemData &obj)
    //{
    //    if (&obj == this)
    //        return *this;
    //    m_data = obj.m_data;
    //    m_offset = obj.m_offset;
    //    return *this;
    //}

    void Clear()
    {
        m_offset = 0;
    }

    void Copy(const void *addr, uint32_t size)
    {
        if (LeftSize() < size)
        {
            Expand(size);
        }

        memcpy(GetAddr(), addr, size);
        m_offset = size;
    }

    void Remove(uint32_t size)
    {
        if (size >= Size())
        {
            m_offset = 0;
        }

        m_data.erase(m_data.begin(), m_data.begin() + size);
    }

    //扩大指定字节, 增量数据
    void Expand(uint32_t size)
    {
        size += m_offset;
        if (size <= m_data.size())
        {
            return;
        }

        Buffer tmp(size, 0);
        memcpy(&tmp[0], &m_data[0], m_offset);
        m_data = tmp;
    }

    uint32_t Size() const
    {
        return m_offset;
    }

    uint8_t* GetStart()
    {
        if (m_data.empty())
        {
            return NULL;
        }
        return &m_data[0];
    }

    uint8_t* GetAddr()
    {
        if (m_data.empty())
        {
            return NULL;
        }
        return &m_data[m_offset];
    }

    void UpdateUse(uint32_t len)
    {
        m_offset += len;
    }

    uint32_t LeftSize() const
    {
        return m_data.size() - m_offset;
    }
};

//毫秒计时器
class CTimerMS
{
    timeval m_start;
    uint64_t m_expire;
public:

    CTimerMS(uint32_t total = 0)
    {
        m_expire = total;
        Reset();
    }

    CTimerMS(const CTimerMS &obj)
    {
        m_start = obj.m_start;
        m_expire = obj.m_expire;
    }

    uint64_t GetElapsedTime()
    {
        timeval end;
        gettimeofday(&end, NULL);
        uint64_t ret = (end.tv_sec - m_start.tv_sec) * 1000 + (end.tv_usec - m_start.tv_usec) / 1000;
        return ret;
    }

    uint64_t GetExpireMs()
    {
        return GetMs() + m_expire;
    }

    uint64_t GetMs()
    {
        return m_start.tv_sec * 1000 + m_start.tv_usec / 1000;
    }

    void Reset()
    {
        gettimeofday(&m_start, NULL);
    }

    bool IsTimeout()
    {
        return GetElapsedTime() >= m_expire;
    }

    void SetTimeout(uint32_t ms)
    {
        m_expire = ms;
    }

    uint64_t LeftTime()
    {
        uint64_t elapse = GetElapsedTime();
        if (m_expire <= elapse)
        {
            return 0;
        }
        return m_expire - elapse;
    }
};

typedef int (*Callback_Seq)(const void*, uint32_t);
typedef int (*Callback_TcpCheck)(const void*, uint32_t);
typedef int (*Callback_Process)(const void *arg);


inline string DumpHex(const void *start, int len) 
{
    stringstream ss;
    for (auto i = 0; i != len; ++i) 
    {
        int val = (int)(((const uint8_t*)start)[i]) & 0xff;
        ss << hex << "0x" << val << " ";
    }
    return ss.str();
}
