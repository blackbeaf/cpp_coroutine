#include "EventMgr.h"

uint32_t Convert(EFDEVENT fe)
{
    switch (fe)
    {
    case FD_READ:
        return EPOLLIN;

    case FD_WRITE:
        return EPOLLOUT;

    case FD_READ_WRITE:
        return EPOLLIN | EPOLLOUT;

    default:
        return 0;
    }
}

EIORESULT CFdMonitor::RegisterPackage(Callback_TcpCheck check, Callback_Process process)
{
    if (NULL == check || NULL == process)
    {
        return IO_ARG_INVALID;
    }

    m_pkgcallback[check] = process;
    return IO_OK;
}

int stFd::Get() const
{
    return m_fd;
}

EIORESULT CMonitorEpoll::Init()
{
    uint64_t rsp = 0, rbp = 0;
    __GetStack__(&rsp, &rbp);
    LOG("rsp = %zx rbp = %zx", rsp, rbp);

    if (m_fd.Init())
    {
        return IO_ERR;
    }

    return IO_OK;
}

bool CMonitorEpoll::CanRead(uint32_t event)
{
    return (event & EPOLLIN) != 0 ? true : false;
}


bool CMonitorEpoll::CanWrite(uint32_t event)
{
    return (event & EPOLLOUT) != 0 ? true : false;
}

EIORESULT CFdEpoll::Init()
{
    /*
    epoll_create()  creates  an epoll(7) instance.
    Since Linux 2.6.8, the size argument is ignored,
    but must be greater than zero; see NOTES below.
    */
    int fd = epoll_create(100);

    if (fd < 0)
    {
        LOG("epoll_create=%d %m", fd);
        return IO_ERR;
    }

    SetNewfd(fd);
    return IO_OK;
}

EIORESULT tcpsend(int server, CTimerMS &timer, const void *src, size_t len)
{
    if (server < 0)
    {
        return IO_ARG_INVALID;
    }

    size_t total = 0;

    while (1)
    {
        //检查是否超时
        if (timer.IsTimeout())
        {
            return IO_TIMEOUT;
        }

        ssize_t ret = send(server, (const char*)src + total, len - total, 0);

        if (ret > 0)
        {
            total += ret;

            if (total == len)
            {
                LOG("send %zu ok", len);
                return IO_OK;
            }

            //无法一次发送完
            continue;
        }
        else if (0 == ret)
        {
            //连接关闭
            LOG("fd %d is closed %m", server);
            return IO_REMOTE_CLOSE;
        }
        else
        {
            if (EINTR != errno && EAGAIN != errno && EWOULDBLOCK != errno)
            {
                //发送失败,不能重试
                LOG("send = %zd %m", ret);
                return IO_ERR;
            }

            //可以重试
            //到这里说明无法一次性发送完,进行调度
            CMonitorEpoll::instance().Add(server, FD_WRITE);
            CoroutineMgr::instance().WaitAndSchedule(timer.LeftTime());
            CMonitorEpoll::instance().Del(server);
        }

        return IO_OK;
    }
}

EIORESULT udpsendrecv(
    const char *ip,
    uint16_t port,
    const void *src,
    uint32_t srclen,
    void *dst,
    uint32_t &dstlen,
    uint32_t timeoutms)
{
    if (NULL == ip || port < 1000 || NULL == src || NULL == dst)
    {
        return IO_ARG_INVALID;
    }

    return IO_OK;
}

int tcpconnect(const char *ip, uint16_t port, uint32_t timeout, Callback_Seq cb_seq, Callback_TcpCheck cb_check, uint32_t seq, stFdData *&pdata)
{
    stFdData *data = CFdMgr::instance().Get(ip, port, PORT_TCP);

    if (data)
    {
        LOG("found previous fd");
        LOG("%s", data->DebugString().c_str());
        CFdMgr::instance().Dump();
        pdata = data;
        //return data->fd.Get();
    }
    else
    {
        CTcpClientSocket tcs(ip, port);

        if (IO_OK != tcs.Init())
        {
            LOG("init fail");
            return -1;
        }

        /*
        RETURN VALUE
        If  the  connection  or  binding  succeeds, zero is returned.
        On error, -1 is returned, and errno is set appropriately.
        */
        int ret = connect(tcs.Get(), (sockaddr*)&tcs.GetRemoteAddr(), sizeof(sockaddr));
        data = CFdMgr::instance().Add(tcs, ip, port, PORT_TCP);

        if (ret)
        {
            //非阻塞fd connect几乎第一次都会失败
            if (errno != EINPROGRESS)
            {
                //出错
                LOG("connect=%d %m", ret);
                return IO_ERR;
            }

            //需要等待一会才能知道结果
            LOG("connect not ready");
            data->fd_state = E_CONNECT;
            data->check = cb_check;
            data->seq = cb_seq;
            data->port_type = PORT_TCP;
            data->context_id = CoroutineMgr::instance().GetActive().GetID();
            CMonitorEpoll::instance().Add(tcs.Get(), FD_READ_WRITE);
            CoroutineMgr::instance().WaitAndSchedule(timeout);
            CMonitorEpoll::instance().Del(tcs.Get());
            data->context_id = 0;

            if (data->last_op_ret != IO_OK)
            {
                //connect 失败
                LOG("last_op_ret err; destroy fd");
                CFdMgr::instance().Destroy(tcs.Get());
                return -2;
            }
        }
    }

    data->fd_state = E_OTHER;
    pdata = data;
    data->Clear();
    stSendRecv &sr = data->seq_to_coroutine[seq];
    LOG("add seq_to_coroutine %d", seq);
    sr.id = CoroutineMgr::instance().GetActive().GetID();
    sr.data.Clear();
    LOG("connect ok");
    return data->fd.Get();
}

EIORESULT tcpsendrecv(const char *ip,
                      uint16_t port,
                      const void *src,
                      uint32_t srclen,
                      CMemData &dst,
                      uint32_t timeoutms,
                      Callback_Seq cb_seq,
                      Callback_TcpCheck cb_check)
{
    if (NULL == ip || 0 == port || NULL == src)
    {
        return IO_ARG_INVALID;
    }

    //Callback_Seq cb_seq = CConnectionInfo::instance().CbSeq(ip, port, PORT_TCP);
    if (NULL == cb_check)
    {
        return IO_ARG_INVALID;
    }

    CTimerMS timer(timeoutms);
    stFdData *pdata = NULL;
    int seq = cb_seq(src, srclen);
    //获取连接
    int fd = tcpconnect(ip, port, timeoutms, cb_seq, cb_check, seq, pdata);

    if (fd < 0)
    {
        return IO_ERR;
    }

    EIORESULT ret = tcpsend(fd, timer, src, srclen);

    if (ret != IO_OK)
    {
        return ret;
    }

    if (1 == pdata->seq_to_coroutine.size())
    {
        //第一个协程关联到该fd
        CMonitorEpoll::instance().Add(fd, FD_READ);
    }

    CoroutineMgr::instance().WaitAndSchedule(timer.LeftTime());
    MapSeqToSendrecv::iterator it = pdata->seq_to_coroutine.find(seq);

    if (pdata->seq_to_coroutine.end() == it)
    {
        LOG("seq not exist");
    }

    if (0 == it->second.data.Size())
    {
        LOG("no sendrecv data");
        return IO_ERR;
    }

    dst = pdata->seq_to_coroutine[seq].data;
    LOG("del seq_to_coroutine %d", it->first);
    pdata->seq_to_coroutine.erase(it);

    if (pdata->seq_to_coroutine.empty())
    {
        //没有协程关联该fd了, 不再监听
        CMonitorEpoll::instance().Del(fd);
    }

    return IO_OK;
}


EIORESULT DoTask(VectorTask &task)
{
    return IO_OK;
}
