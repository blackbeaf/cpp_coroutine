#pragma once

#include "util.h"

struct stFd : public nocopy
{
protected:

    int m_fd;

public:

    stFd(int fd)
    {
        m_fd = fd;
    }

    ~stFd()
    {
        if (m_fd < 0)
        {
            return;
        }

        int ret = close(m_fd);
        if (ret)
        {
            LOG("close %d return %d %m", m_fd, ret);
        }
        else
        {
            LOG("close fd %d", m_fd);
            m_fd = -1;
        }
    }

    int Get() const;
};

typedef shared_ptr<stFd> PtrFd;

class CFd
{
public:

    CFd() :m_fd(new stFd(-1))
    {}

    CFd(const CFd &obj)
    {
        m_fd = obj.m_fd;
    }

    CFd(int fd): m_fd(new stFd(fd))
    {

    }

    CFd& operator=(const CFd &obj)
    {
        if (&obj == this)
        {
            return *this;
        }
        m_fd = obj.m_fd;
        return *this;
    }

    bool operator!=(const CFd &obj) const
    {
        return m_fd.get() != obj.m_fd.get();
    }

    virtual ~CFd() {}

    int Get() const;

    virtual EIORESULT Init() { return IO_OK; }

    bool operator<(const CFd &obj) const;

    bool empty() const
    {
        return m_fd->Get() < 0;
    }

protected:

    void SetNewfd(int fd);

    PtrFd m_fd;
};

class CServerSocket : public CFd
{
protected:

    uint16_t m_port;
    EPORTTYPE m_type;
    Callback_TcpCheck m_cb_check;

public:

    CServerSocket()
    {
        m_port = 0;
        m_type = PORT_TCP;
    }
    CServerSocket(uint16_t port, EPORTTYPE pt, Callback_TcpCheck cb)
    {
        m_port = port;
        m_type = pt;
        m_cb_check = cb;
    }

    CServerSocket& operator=(const CServerSocket &obj)
    {
        if (&obj == this)
        {
            return *this;
        }

        CFd::operator =(obj);
        m_port = obj.m_port;
        m_type = obj.m_type;
        m_cb_check = obj.m_cb_check;
        return *this;
    }

    CServerSocket(const CServerSocket &obj) :CFd(obj)
    {
        m_port = obj.m_port;
        m_type = obj.m_type;
        m_cb_check = obj.m_cb_check;
    }

    EPORTTYPE GetType() const
    {
        return m_type;
    }

    Callback_TcpCheck GetCbCheck() const
    {
        return m_cb_check;
    }

    virtual EIORESULT Init()
    {
        sockaddr_in servaddr;
        int type = PORT_TCP == m_type ? SOCK_STREAM : SOCK_DGRAM;
        type |= SOCK_NONBLOCK;
        int fd = socket(AF_INET, type, 0);
        if (fd < 0)
        {
            LOG("socket error %m");
            return IO_ERR;
        }

        SetNewfd(fd);

        int opt = 1;
        // sockfd为需要端口复用的套接字  
        int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
        if (ret)
        {
            LOG("setsockopt = %d %m", ret);
            return IO_ERR;
        }

        servaddr.sin_family = AF_INET;
        inet_pton(AF_INET, "0.0.0.0", &servaddr.sin_addr);
        servaddr.sin_port = htons(m_port);
        if (-1 == ::bind(fd, (struct sockaddr*)&servaddr, sizeof(servaddr)))
        {
            LOG("bind error: %m");
            return IO_ERR;
        }

        ret = listen(fd, 20);
        if (ret != 0)
        {
            LOG("listen error %d %m", ret);
            return IO_ERR;
        }
        return IO_OK;
    }

};

typedef map<int, CServerSocket> MapIdToSocket;

class CUdpClientSocket : public CFd
{
public:

    CUdpClientSocket()
    {}

    CUdpClientSocket(const CUdpClientSocket &obj) :CFd(obj)
    {
    }

    CUdpClientSocket& operator=(const CUdpClientSocket &obj)
    {
        if (&obj == this)
        {
            return *this;
        }
        return *this;
    }

    virtual EIORESULT Init()
    {
        int type = SOCK_DGRAM;
        type |= SOCK_NONBLOCK;
        int fd = socket(AF_INET, type, 0);
        if (fd < 0)
        {
            LOG("socket error %m");
            return IO_ERR;
        }

        SetNewfd(fd);
        return IO_OK;
    }
};


class CTcpClientSocket : public CFd
{
    sockaddr_in m_remote;

public:
    CTcpClientSocket(const char *remoteip, uint16_t port)
    {
        m_remote.sin_addr.s_addr = inet_addr(remoteip);
        m_remote.sin_family = AF_INET;
        m_remote.sin_port = htons(port);
    }

    CTcpClientSocket(const CTcpClientSocket &obj) :CFd(obj)
    {
        m_remote = obj.m_remote;
    }

    CTcpClientSocket& operator=(const CTcpClientSocket &obj)
    {
        if (&obj == this)
        {
            return *this;
        }
        CFd::operator =(obj);
        m_remote = obj.m_remote;
        return *this;
    }

    sockaddr_in& GetRemoteAddr()
    {
        return m_remote;
    }
    virtual EIORESULT Init()
    {
        int type = SOCK_STREAM;
        type |= SOCK_NONBLOCK;
        int fd = socket(AF_INET, type, 0);
        if (fd < 0)
        {
            LOG("socket error %m");
            return IO_ERR;
        }

        SetNewfd(fd);
        /*
        RETURN VALUE
        If  the  connection  or  binding  succeeds, zero is returned.
        On error, -1 is returned, and errno is set appropriately.
        */
        //int ret = connect(fd, (sockaddr*)&m_remote, sizeof(m_remote));
        //if (ret)
        //{
        //    if (errno != EINPROGRESS)
        //    {
        //        //出错
        //        LOG("connect=%d %m", ret);
        //        return IO_ERR;
        //    }
        //    //需要等待一会才能知道结果
        //    LOG("connect not ready");
        //    CFdMgr::instance().Add()
        //}

        //LOG("connect ok");
        return IO_OK;
    }
};

