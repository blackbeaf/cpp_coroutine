#pragma once
#include "Fd.h"

struct stSendRecv 
{
    uint32_t id;
    CMemData data;
};
typedef map<int, stSendRecv> MapSeqToSendrecv;

enum E_FD_TYPE
{
    E_LISTEN,
    E_SERVER_CONN,
    E_OTHER,
    E_CONNECT,
};


struct stFdData
{
    Callback_TcpCheck check;
    Callback_Seq   seq;
    MapSeqToSendrecv seq_to_coroutine;
    CMemData recvdata;
    sockaddr_in remote;
    EPORTTYPE port_type;
    E_FD_TYPE fd_state;
    uint32_t context_id;
    EIORESULT last_op_ret;
    CFd fd;

    stFdData()
    {
        check = NULL;
        seq = NULL;
        port_type = PORT_UDP;
        fd_state = E_OTHER;
        context_id = 0;
    }

    bool operator<(const stFdData &obj) const
    {
        return fd < obj.fd;
    }

    string DebugString()
    {
        stringstream ss;
        ss << " port type " << port_type << " fdtype " << fd_state << " fd " << fd.Get();
        return ss.str();
    }

    void Clear()
    {
        last_op_ret = IO_OK;
    }
};

struct stFdKey
{
    struct stIpPort
    {
        string ip;
        uint16_t port;
        EPORTTYPE type;

        bool empty() const
        {
            if (ip.empty() && 0 == port)
            {
                return true;
            }
            return false;
        }

        bool operator==(const stIpPort &obj) const
        {
            return ip == obj.ip && port == obj.port && type == obj.type;
            //if (!ip.empty() && !obj.ip.empty() && ip != obj.ip)
            //{
            //    return ip < obj.ip;
            //}

            //if (port && obj.port && port != obj.port)
            //{
            //    return port < obj.port;
            //}
            //if (type && obj.type && type != obj.type)
            //{
            //    return type < obj.type;
            //}
            //return false;
        }

        bool operator!=(const stIpPort &obj) const
        {
            return ! (*this == obj);
        }

        bool operator<(const stIpPort &obj) const
        {
            if (ip != obj.ip)
            {
                return ip < obj.ip;
            }

            if (port != obj.port)
            {
                return port < obj.port;
            }

            if (type != obj.type)
            {
                return type < obj.type;
            }
            return false;
        }

        stIpPort()
        {
            port = 0;
            type = PORT_UDP;
        }
    } ip_port;

    struct stFd
    {
        int fd;

        stFd()
        {
            fd = -1;
        }

        bool operator<(const stFd &obj) const
        {
            return fd < obj.fd;
        }

        bool empty() const
        {
            return fd < 0;
        }

        bool operator==(const stFd &obj) const
        {
            return fd == obj.fd;
        }
        bool operator!=(const stFd &obj) const
        {
            return fd != obj.fd;
        }
    } socket_fd;
    //CFd socket_fd;

    bool operator<(const stFdKey &obj) const
    {
        //任意字段相等认为元素相等; 但所有元素都不等则走比较逻辑
        if (!ip_port.empty() && !obj.ip_port.empty())
        {
            if (ip_port == obj.ip_port)
            {
                //LOG("1");
                return false;
            }
        }

        if (!socket_fd.empty() && !obj.socket_fd.empty())
        {
            if (socket_fd == obj.socket_fd)
            {
                //LOG("2");
                return false;
            }
        }

        if (ip_port != obj.ip_port)
        {
            //LOG("3");
            return ip_port < obj.ip_port;
        }

        if (socket_fd != obj.socket_fd)
        {
            //LOG("4");
            return socket_fd < obj.socket_fd;
        }

        LOG("?????");
        return false;
    }

    string DebugString() const
    {
        stringstream ss;
        ss << "ip " << ip_port.ip << " port " << ip_port.port << " fd " << socket_fd.fd ;
        return ss.str();
    }
};

class CFdMgr : public nocopy, public single<CFdMgr>
{
    typedef map<stFdKey, stFdData> MapFdToData;
    MapFdToData m_fd_to_data;

    bool Exist(int fd, MapFdToData::iterator &iter)
    {
        stFdKey key;
        key.socket_fd.fd = fd;
        iter = m_fd_to_data.find(key);
        return m_fd_to_data.end() != iter;
    }

    bool Exist(const char *ip, uint16_t port, EPORTTYPE type, MapFdToData::iterator &iter)
    {
        stFdKey key;
        key.ip_port.ip = ip;
        key.ip_port.port = port;
        key.ip_port.type = type;
        iter = m_fd_to_data.find(key);
        return m_fd_to_data.end() != iter;
    }

public:

    void Dump()
    {
        MapFdToData::iterator iter;
        for (iter = m_fd_to_data.begin(); iter != m_fd_to_data.end(); ++iter)
        {
            LOG("%s %s", iter->first.DebugString().c_str(), iter->second.DebugString().c_str());
        }
    }

    stFdData* Add(const CFd &fd)
    {
        stFdKey key;
        key.socket_fd.fd = fd.Get();
        stFdData sfd;
        sfd.fd = fd;
        auto ret = m_fd_to_data.insert(make_pair(key, sfd));
        if (!ret.second)
        {
            LOG("insert fail");
            return NULL;
        }
        return &ret.first->second;
    }

    stFdData* Add(const CFd &fd, const char *ip, uint16_t port, EPORTTYPE type)
    {
        stFdKey key;
        key.ip_port.ip = ip;
        key.ip_port.port = port;
        key.ip_port.type = type;
        key.socket_fd.fd = fd.Get();
        stFdData sfd;
        sfd.fd = fd;
        auto ret = m_fd_to_data.insert(make_pair(key, sfd));
        if (!ret.second)
        {
            LOG("insert fail");
            return NULL;
        }
        return &ret.first->second;
    }

    stFdData* Get(int fd)
    {
        MapFdToData::iterator iter;
        if (!Exist(fd, iter))
        {
            LOG("fd %u not exist", fd);
            return NULL;
        }

        return &iter->second;
    }

    stFdData* Get(const char *ip, uint16_t port, EPORTTYPE type)
    {
        MapFdToData::iterator iter;
        if (!Exist(ip, port, type, iter))
        {
            LOG("data not exist");
            return NULL;
        }

        return &iter->second;
    }

    //EIORESULT Add(const stIpPort &conn)
    //{
    //    //连接不存在,创建
    //    stFdData cd;
    //    if (PORT_TCP == conn.type)
    //    {
    //        cd.fd = shared_ptr<CFd>(new CTcpClientSocket(conn.ip.c_str(), conn.port));
    //    }
    //    else
    //    {
    //        cd.fd = shared_ptr<CFd>(new CUdpClientSocket());
    //    }

    //    //尝试初始化
    //    EIORESULT ret = cd.fd->Init();
    //    if (IO_OK != ret && IO_CONNECT_WAIT != ret)
    //    {
    //        //发生错误
    //        LOG("init fail");
    //        return IO_ERR;
    //    }

    //    if (IO_CONNECT_WAIT == ret)
    //    {
    //        //异步connect需要等待
    //        LOG("wait fd %u to connect ", cd.fd->Get());
    //        CMonitorEpoll::instance().RegisterTcpConnectFd(cd.fd->Get());
    //        CoroutineMgr::instance().WaitAndSchedule(timeout);
    //        //获取connect结果
    //        const stFdData *sfd = CMonitorEpoll::instance().Get(cd.fd->Get());
    //        if (NULL == sfd)
    //        {
    //            LOG("get fd data fail! connect fail ");
    //        }
    //        else
    //        {
    //            LOG("last_op_ret = %u.", sfd->last_op_ret);
    //        }

    //        if (IO_OK != sfd->last_op_ret)
    //        {
    //            //connect失败
    //            CMonitorEpoll::instance().Destroy(cd.fd->Get());
    //            cd
    //            return -2;
    //        }
    //    }
    //    m_conninfo[conn] = cd;
    //    return cd.fd->Get();
    //}

    //EIORESULT RegisterTcpConnectFd(int fd)
    //{
    //    MapFdToData::iterator iter;
    //    if (m_fd_to_data.end() != iter)
    //    {
    //        //已存在 禁止添加
    //        LOG("fd %u exist!!", fd);
    //        return IO_ERR;
    //    }

    //    //业务发起的fd
    //    //stFdData *data = m_fd_to_data[fd];
    //    //data.check = NULL;
    //    //data.fd_type = E_CONNECT;
    //    //data.port_type = PORT_TCP;
    //    //data.context_id = CoroutineMgr::instance().GetActive().GetID();
    //    //Add(fd, FD_READ_WRITE);
    //    return IO_OK;
    //}

    EIORESULT Destroy(int fd)
    {
        MapFdToData::iterator iter;
        if (!Exist(fd, iter))
        {
            LOG("fd %u not exist", fd);
            return IO_ERR;
        }

        m_fd_to_data.erase(iter);
        return IO_OK;
    }
};
