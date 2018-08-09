#include "../lib/eventmgr/EventMgr.h"
#include "../pb/head.h"

int check(const void *src, uint32_t len)
{
    return len;
}

int process(const void *arg)
{
    if (NULL == arg)
    {
        return 0;
    }

    stReqData *rd = (stReqData*)arg;
    LOG("recv data len = %d ", rd->recvdata.Size());
    LOG("%s", rd->DebugString().c_str());
#define TIMEOUT 90*1000
    const char *ip = "10.244.181.24";
    uint16_t port = 31000;
    LOG("get data from server %s %u", ip, port);
    HeadPkg pkg;
    pbtest::DataReq req;
    req.set_cmd(100);
    req.set_query(time(NULL));
    string reqdata;
    req.SerializeToString(&reqdata);
    pkg.SetData(reqdata);
    pkg.Encode();
    LOG("head %s", pkg.Head().Utf8DebugString().c_str());
    LOG("body %s", req.Utf8DebugString().c_str());
    CMemData rspdata;
    EIORESULT ret = tcpsendrecv(ip, port, pkg.Data().c_str(), pkg.Data().size(), rspdata, TIMEOUT, HeadPkg::Seq, HeadPkg::Check);

    if (IO_OK != ret)
    {
        LOG("get data error. ret = %u", ret);
        return 0;
    }

    LOG("recv data len = %u", rspdata.Size());

    if (!pkg.Decode(rspdata.GetStart(), rspdata.Size()))
    {
        LOG("decode err");
        return 0;
    }

    pbtest::DataRsp rsp;

    if (!rsp.ParseFromString(pkg.Body()))
    {
        LOG("ParseFromString error");
        return 0;
    }

    LOG("head %s", pkg.Head().Utf8DebugString().c_str());
    LOG("rsp %s", rsp.Utf8DebugString().c_str());
    CTimerMS tm;
    tm.SetTimeout(100);
    tcpsend(rd->server_socket, tm, rsp.msg().c_str(), rsp.msg().size());
    return 0;
}

void test()
{
    CTimerMS tm;

    for (int i = 0; i != 1000 * 10000; ++i)
    {
        tm.GetElapsedTime();
    }

    LOG("result %zu", tm.GetElapsedTime());
}

int main()
{
    CMonitorEpoll::instance().Init();
    CServerSocket ss(30000, PORT_TCP, check);
    ss.Init();
    CMonitorEpoll::instance().AddServerFd(ss);
    CMonitorEpoll::instance().RegisterPackage(check, process);
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

//
//#include <map>
//
//#include <stdio.h>
//
//using namespace std;
//
//struct stkey
//{
//    int a;
//    int b;
//    int c;
//
//    stkey(int d, int e, int f)
//    {
//        a = d;
//        b = e;
//        c = f;
//    }
//
//    bool operator<(const stkey &obj) const
//    {
//        if (a && obj.a && (a != obj.a))
//        {
//            return a < obj.a;
//        }
//
//        if (b && obj.b && (b != obj.b))
//        {
//            return b < obj.b;
//        }
//
//        if (c&&obj.c &&( c != obj.c))
//        {
//            return c < obj.c;
//        }
//        return false;
//    }
//};
//
//#include "../lib/eventmgr/util.h"
//#include <iostream>
//
//using namespace std;
//int main()
//{
//    typedef map<stkey, const char*> maptest;
//    maptest mt;
//
//    stkey k1(1, 2, 3);
//
//    stkey k2(1, 0, 0);
//    stkey k3(0, 2, 0);
//    stkey k4(0, 0, 3);
//
//    stkey k5(1, 2, 0);
//    stkey k6(0, 2, 3);
//    stkey k7(1, 0, 3);
//
//    stkey k8(0, 0, 0);
//
//    mt[k1] = "aaaaaa";
//    LOG("%s", mt[k1]);
//    LOG("%s", mt[k2]);
//    LOG("%s", mt[k3]);
//    LOG("%s", mt[k4]);
//    LOG("%s", mt[k5]);
//    LOG("%s", mt[k6]);
//    LOG("%s", mt[k7]);
//    LOG("%s", mt[k8]);
//
//    LOG("size %zu", mt.size());
//}