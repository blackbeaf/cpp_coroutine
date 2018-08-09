#include "../lib/eventmgr/EventMgr.h"
#include "../pb/head.h"

#define IPADDRESS   "127.0.0.1"
#define PORT        23333
#define MAXSIZE     1024
#define LISTENQ     5
#define FDSIZE      1000
#define EPOLLEVENTS 100

#define TIME_10_SECOND 10*1000

int process(const void *arg)
{
    if (NULL == arg)
    {
        return 0;
    }
    stReqData *rd = (stReqData*)arg;
    LOG("recv data len = %d ", rd->recvdata.Size());
    LOG("%s", rd->DebugString().c_str());

    HeadPkg pkg;

    if (!pkg.Decode(rd->recvdata.GetStart(), rd->recvdata.Size()))
    {
        LOG("decode err");
        return 0;
    }

    pbtest::DataReq bodyreq;
    pbtest::DataRsp bodyrsp;

    if (!bodyreq.ParseFromString(pkg.Body()))
    {
        LOG("ParseFromString error");
        return 0;
    }


    LOG("recv     head %s", pkg.Head().Utf8DebugString().c_str());
    LOG("recv     req %s", bodyreq.Utf8DebugString().c_str());

    wait_ms(TIME_10_SECOND);

    bodyrsp.set_cmd(bodyreq.cmd());
    bodyrsp.set_data(bodyreq.query());
    bodyrsp.set_msg(GetYMDHMS());
    bodyrsp.mutable_msg()->append("\r\n");

    string data;
    bodyrsp.SerializeToString(&data);
    pkg.SetData(data);
    pkg.Encode();

    LOG("send     head %s", pkg.Head().Utf8DebugString().c_str());
    LOG("send     rsp %s", bodyrsp.Utf8DebugString().c_str());

    CTimerMS timer(100);
    LOG("reply to client ");
    tcpsend(rd->server_socket, timer, pkg.Data().c_str(), pkg.Data().size());
    return 0;
}


void testmap()
{
    typedef map<int, string> MapIS;
    MapIS is;
    is[3] = "234";
    is[-43] = "34";
    is[92] = "32423";
    is[39] = "32423";
    is[-389] = "3434";

    for (MapIS::iterator it = is.begin(); it != is.end(); ++it)
    {
        LOG("%d %s", it->first, it->second.c_str());
    }
}

int main()
{
    //aaa *p = new aaa();
    //shared_ptr<aaa> bb = make_shared<aaa>(*p);
    //delete p;
    //return 0;

    //c1 = CreateCoroutine(func1, NULL);
    //c2 = CreateCoroutine(func2, NULL);
    //c3 = CreateCoroutine(NULL, NULL);
    //
    //__SwitchCoroutine__(c3, c1);

    CMonitorEpoll::instance().Init();
    CServerSocket ss(31000, PORT_TCP, HeadPkg::Check);
    ss.Init();
    CMonitorEpoll::instance().AddServerFd(ss);
    CMonitorEpoll::instance().RegisterPackage(HeadPkg::Check, process);
    EIORESULT ret = CoroutineMgr::instance().Init();
    if (IO_OK != ret)
    {
        LOG("init err");
        return 0;
    }

    while (1)
    {
        CMonitorEpoll::instance().RunOnce();
        CoroutineMgr::instance().ScheduleAll();
    }
    return 0;
}
