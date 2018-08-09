#pragma once

#include "util.h"
#include "Fd.h"
#include "CoroutineMgr.h"
#include "FdMgr.h"
#include "ExpireMgr.h"

using namespace std;

typedef map<Callback_TcpCheck, Callback_Process> MapPkgCallback;

inline EIORESULT tcprecv(int server, CMemData &recvdata, Callback_TcpCheck cb_check, int &pkgsize)
{
    if (server < 0)
    {
        return IO_ARG_INVALID;
    }
    while (1)
    {
#define DEF_RECV_SIZE 1024
        if (recvdata.LeftSize() <= DEF_RECV_SIZE)
        {
            recvdata.Expand(DEF_RECV_SIZE);
        }

        int ret = recv(server, recvdata.GetAddr(), recvdata.LeftSize(), 0);
        //int ret = recv(server, x, 10, 0);
        if (ret < 0)
        {
            if (EINTR == errno || EAGAIN == errno)
            {
                //����
                LOG("errno=AGAIN or EINTR ret = %d %m", ret);
                return IO_OK;
            }
            //��������
            LOG("recv = %d %m", ret);
            return IO_ERR;
        }

        if (0 == ret)
        {
            //�Է��Ͽ�
            LOG("fd %d remote close", server);
            return IO_REMOTE_CLOSE;
        }

        recvdata.UpdateUse(ret);
        pkgsize = cb_check(recvdata.GetStart(), recvdata.Size());
        if (pkgsize > 0)
        {
            //����һ������
            LOG("check pkg ok, recv len = %d", pkgsize);
            return IO_OK;
        }
        //��������,�������Ѿ�����,����
        LOG("pkg check error, fun = %p;recv len = %d %m", cb_check, ret);
        return IO_ERR;
    }
}

class CFdMonitor : public nocopy
{
public:

    virtual ~CFdMonitor()
    {}

    virtual EIORESULT Init() = 0;

    //����һ���¼�
    virtual EIORESULT RunOnce() = 0;

    //���Ҫ�����¼���fd
    virtual EIORESULT Add(int fd, EFDEVENT fe) = 0;
    virtual EIORESULT Del(int fd) = 0;
    //virtual EIORESULT Del(int fd, EFDEVENT fe) = 0;

    //ע��Ҫ������������麯��, ������;
    EIORESULT RegisterPackage(Callback_TcpCheck check, Callback_Process process);

protected:

    MapPkgCallback m_pkgcallback;
    typedef map<int, Callback_TcpCheck> MapFdCheck;
};

inline string PrintEpollEvent(uint32_t e)
{
    const char *msg[] =
    {
        "EPOLLIN",
        "EPOLLOUT",
        "EPOLLRDHUP",
        "EPOLLPRI",
        "EPOLLERR",
        "EPOLLHUP",
        "EPOLLET",
        "EPOLLONESHOT",
        "EPOLLWAKEUP"
    };
    uint32_t total[] =
    {
        EPOLLIN,
        EPOLLOUT,
        EPOLLRDHUP,
        EPOLLPRI,
        EPOLLERR,
        EPOLLHUP,
        EPOLLET,
        EPOLLONESHOT,
        //EPOLLWAKEUP
    };

    uint32_t len = sizeof(total) / sizeof(total[0]);
    stringstream ss;
    for (uint32_t i = 0; i < len; ++i)
    {
        if (e & total[i])
        {
            ss << msg[i] << " ";
        }
    }
    return ss.str();
}

class CFdEpoll : public CFd
{
public:

    virtual EIORESULT Init();

};


class CMonitorEpoll : public CFdMonitor, public single<CMonitorEpoll>
{
    CFdEpoll m_fd;

    //typedef map<int, stRemoteInfo> MapClientFDToRemoteInfo;
    ////�ͻ��˵����Ӽ���
    //MapClientFDToRemoteInfo m_client_conn;
    //struct stOtherConn
    //{
    //    Callback_TcpCheck check;
    //    Callback_Seq   seq;
    //    MapSeqToCoroutine seq_to_coroutine;
    //};
    //typedef map<int, stOtherConn> MapOtherConn;
    //MapOtherConn m_other_conn;
    //typedef map<int, EPORTTYPE> MapFdType;
    //MapFdType m_fdtype;
    //typedef map<int, CMemData> MapFdRecvData;
    //MapFdRecvData m_fd_tcp_recv_data;


public:

    virtual EIORESULT Init();

    void TcpConnect(int fd, uint32_t event, stFdData &data)
    {
        if (0 == (EPOLLERR & event))
        {
            //connect �ɹ�
            data.last_op_ret = IO_OK;
        }
        else
        {
            //connect ʧ��
            int err;
            socklen_t len = sizeof(err);
            int ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
            data.last_op_ret = IO_ERR;
            if (ret)
            {
                //��ȡ������ʧ��
                LOG("getsockopt = %d %m", ret);
            }
            else
            {
                LOG("fd %u err = %d %s", fd, err, strerror(err));
            }
        }
        CoroutineMgr::instance().SetReady(data.context_id);
        return;
    }

    //����һ���¼�
    virtual EIORESULT RunOnce()
    {
        /*
        When successful, epoll_wait() returns the number of file descriptors ready for the requested I/O,
        or zero if  no  file  descriptor  became  ready during the requested timeout milliseconds.
        When an error occurs, epoll_wait() returns -1 and errno is set appropriately.
        */

#define NUM_100 100
        epoll_event clients[NUM_100];

        uint64_t ms = 0;
        uint32_t id = 0;
        CExpireMgr::instance().NextWaitTime(ms, id);
        if (id)
        {
            //�ж����Ѿ���ʱ
            CoroutineMgr::instance().SetReady(id);
            CoroutineMgr::instance().Schedule(id);
            return IO_OK;
        }

        if (0 == ms)
        {
            LOG("ms = 0 ???");
        }

        int ret = epoll_wait(m_fd.Get(), clients, NUM_100 - 1, ms);
        if (0 == ret)
        {
            //��ʱ, ����ʱ�¼�
            LOG("epoll wait timeout");
            return IO_OK;
        }

        if (ret < 0)
        {
            //����
            LOG("epoll wait = %d, %m", ret);
            return IO_ERR;
        }

        LOG("epollwait = %d", ret);
        for (int i = 0; i < ret; ++i)
        {
            stFdData *fddata = CFdMgr::instance().Get(clients[i].data.fd);
            if (NULL == fddata)
            {
                LOG("unknow fd %d", clients[i].data.fd);
                Del(clients[i].data.fd);
                continue;
            }

            LOG("event %s", PrintEpollEvent(clients[i].events).c_str());
            //�����ж�connect״̬
            if (E_CONNECT == fddata->fd_state)
            {
                TcpConnect(clients[i].data.fd, clients[i].events, *fddata);
                continue;
            }

            //�����׽����¼�
            LOG("process %d fd = %d", i, clients[i].data.fd);
            if (IsListenFd(*fddata))
            {
                if (CanRead(clients[i].events))
                {
                    //������
                    NewReq(clients[i].data.fd, *fddata);
                }
                else
                {
                    //���ߵ�����?
                    LOG("listen socket unknow event %u", clients[i].events);
                }
            }
            else
            {
                //�����׽����¼�
                if (CanRead(clients[i].events))
                {
                    if (PORT_TCP == fddata->port_type)
                    {
                        DoReadTcp(clients[i].data.fd, *fddata);
                    }
                    else
                    {
                        DoReadUdp(clients[i].data.fd, *fddata);
                    }
                }

                if (CanWrite(clients[i].events))
                {
                    LOG("no implement");
                }
            }
        }
        return IO_OK;
    }

    virtual EIORESULT Add(int fd, EFDEVENT fe)
    {
        /*
        typedef union epoll_data {
            void        *ptr;
            int          fd;
            uint32_t     u32;
            uint64_t     u64;
        } epoll_data_t;

        struct epoll_event {
            uint32_t     events;       Epoll events
            epoll_data_t data;         User data variable
        };
        */

        epoll_event events;
        events.events = fe;
        events.data.fd = fd;
        /*
        When successful, epoll_ctl() returns zero.
        When an error occurs, epoll_ctl() returns -1 and errno is set appropriately.
        */
        int ret = epoll_ctl(m_fd.Get(), EPOLL_CTL_ADD, fd, &events);
        if (ret)
        {
            LOG("fd=%d epoll_ctl=%d %m", fd, ret);
            return IO_ERR;
        }
        LOG("add fd %d event %d", fd, fe);
        return IO_OK;
    }

    EIORESULT Del(int fd)
    {
        epoll_event events;
        /*
        BUGS
        In kernel versions before 2.6.9, the EPOLL_CTL_DEL operation required a non-NULL pointer in  event,  even
        though  this  argument  is  ignored.   Since  Linux  2.6.9,  event  can  be  specified as NULL when using
        EPOLL_CTL_DEL.  Applications that need to be portable to kernels before 2.6.9 should specify  a  non-NULL
        pointer in event.
        */
        int ret = epoll_ctl(m_fd.Get(), EPOLL_CTL_DEL, fd, &events);
        if (ret)
        {
            LOG("fd=%d epoll_ctl=%d %m", fd, ret);
            return IO_ERR;
        }
        return IO_OK;
    }

    virtual EIORESULT NewReq(int ss, stFdData &data)
    {
        LOG("new req");
        if (PORT_TCP == data.port_type)
        {
            /*
            RETURN VALUE
            Upon successful completion, accept() shall return  the  non-negative  file  descriptor  of  the  accepted
            socket. Otherwise, -1 shall be returned and errno set to indicate the error.
            */
            sockaddr_in remote;
            socklen_t len = sizeof(remote);
            int ret = accept(ss, (sockaddr*)&remote, &len);
            if (ret<0)
            {
                LOG("accpet fd %d = %d %m", ss, ret);
                return IO_ERR;
            }

            LOG("accept = %d ip=%s port=%u", ret, inet_ntoa(remote.sin_addr), ntohs(remote.sin_port));
            stFdData &data = *CFdMgr::instance().Add(ret);
            data.remote = remote;
            data.port_type = PORT_TCP;
            data.check = CFdMgr::instance().Get(ss)->check;
            data.fd_state = E_SERVER_CONN;
            //�����ɶ��¼�
            Add(ret, FD_READ);
        }
        else
        {
            CoroutineContext &node = CoroutineMgr::instance().Create();
            node.GetReq().server_socket = ss;

            CoroutineMgr::instance().SetReady(node.GetID());
            CFdMgr::instance().Get(ss)->port_type = PORT_UDP;
        }
        return IO_OK;
    }

    bool IsClient(const stFdData &fd)
    {
        return E_SERVER_CONN == fd.fd_state ? true : false;
    }

    bool IsOtherConn(const stFdData &fd)
    {
        return E_OTHER == fd.fd_state ? true : false;
    }

 void DelFd(int fd)
{
     CMonitorEpoll::instance().Del(fd);
    CExpireMgr::instance().Del(fd);
    CFdMgr::instance().Destroy(fd);
}

    EIORESULT DoReadTcp(int fd, stFdData &data)
    {
        LOG("read tcp fd = %d", fd);
        Callback_TcpCheck check = data.check;
        if (NULL == check)
        {
            LOG("tcp check is null");

            return IO_ERR;
        }

        int pkgsize = 0;
        CMemData md /*= CFdMgr::instance().Get(fd)->recvdata*/;
        EIORESULT ret = tcprecv(fd, md, check, pkgsize);
        if (pkgsize <= 0 || IO_OK != ret)
        {
            LOG("recv error");
            md.Clear();
            if (IO_REMOTE_CLOSE == ret)
            {
                DelFd(fd);
            }
            return IO_ERR;
        }

        if (IsClient(data))
        {
            LOG("is client conn");
            //���Կͻ�������,������Э�̴���
            CoroutineContext &cc = CoroutineMgr::instance().Create();
            cc.GetReq().m_cb = m_pkgcallback[check];
            //����һ�������ݵ�Э��������
            cc.GetReq().recvdata.Copy(md.GetStart(), pkgsize);
            md.Remove(pkgsize);
            cc.GetReq().remoteaddr = data.remote;
            cc.GetReq().server_socket = (fd);
            return IO_OK;
        }

        if (IsOtherConn(data))
        {
            LOG("other conn");
            //Ϊ�˴���ͻ�������ȥ���ú�˵�����
            int seq = data.seq(md.GetStart(), pkgsize);
            LOG("seq %d", seq);
            MapSeqToSendrecv::iterator iter2 = data.seq_to_coroutine.find(seq);
            if (data.seq_to_coroutine.end() == iter2)
            {
                //û�ж�Ӧ��seq
                LOG("no coroutine match seq %u", seq);
                md.Clear();
                Del(data.fd.Get());
                return IO_ERR;
            }

            LOG("seq found");
            CoroutineContext *cc = CoroutineMgr::instance().Get(iter2->second.id);
            if (NULL == cc)
            {
                LOG("no CoroutineContext exist which id = %u", iter2->second.id);
                md.Clear();
                Del(data.fd.Get());
                return IO_ERR;
            }

            LOG("matched context found");
            iter2->second.data.Clear();
            iter2->second.data.Copy(md.GetStart(), md.Size());
            //ע�������Э���Ѿ���ʼִ����,ֻ��Ҫ�޸�״̬����
            CoroutineMgr::instance().SetReady(iter2->second.id);
        }
        return IO_OK;
    }

    EIORESULT DoReadUdp(int fd, stFdData &data)
    {
        LOG("read udp ");
        return IO_OK;
    }

    EIORESULT DoWrite(int fd, stFdData &data)
    {
        return IO_OK;
    }
    void AddServerFd(const CServerSocket &fd)
    {
        LOG("server fd = %d", fd.Get());
        Add(fd.Get(), FD_READ);
        stFdData &data = *CFdMgr::instance().Add(fd);
        data.port_type = fd.GetType();
        data.check = fd.GetCbCheck();
        data.fd_state = E_LISTEN;
    }

    bool IsListenFd(const stFdData &fd) const
    {
        return E_LISTEN == fd.fd_state ? true : false;
    }

    static bool CanRead(uint32_t event) ;
    static bool CanWrite(uint32_t event) ;

};


EIORESULT tcpsend(int server, CTimerMS &timer, const void *src, size_t len);
EIORESULT udpsendrecv(const char *ip,
                      uint16_t port,
                      const void *src,
                      uint32_t srclen,
                      void *dst,
                      uint32_t &dstlen,
                      uint32_t timeoutms);


typedef vector<EntryCallback> VectorTask;

EIORESULT tcpsendrecv(const char *ip, uint16_t port, const void *src, uint32_t srclen, CMemData &dst, uint32_t timeoutms, Callback_Seq cb_seq, Callback_TcpCheck cb_check);
EIORESULT DoTask(VectorTask &task);

inline void wait_ms(uint32_t ms)
{
    CoroutineMgr::instance().WaitAndSchedule(ms);
}
