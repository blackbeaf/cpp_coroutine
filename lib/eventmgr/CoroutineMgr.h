#pragma once

extern "C"
{
#include "../coroutine/coroutine.h"
}

#include "Fd.h"
#include "ExpireMgr.h"

struct stReqData
{
    CMemData recvdata;
    sockaddr_in remoteaddr;
    uint32_t id;
    int server_socket;
    Callback_Process m_cb;

    string DebugString()
    {
        stringstream ss;
        ss << "ReqData:" << endl
            << "recvdata size " << recvdata.Size() << " leftsize " << recvdata.LeftSize() << endl
            << "remoteaddr " << remoteaddr.sin_addr.s_addr << " " << remoteaddr.sin_port << endl
            << "id " << id << endl
            << "socket " << server_socket << endl
            << "callback " << (void*)m_cb << endl;
        return ss.str();
    }

    stReqData()
    {
        memset(&remoteaddr, 0, sizeof(remoteaddr));
        id = 0; server_socket = 0;
        m_cb = 0;
    }

    stReqData(const stReqData &obj):
        recvdata(obj.recvdata),
        id(obj.id),
        server_socket(obj.server_socket),
        m_cb(obj.m_cb)
    {
        remoteaddr = obj.remoteaddr;
    }
};

int64_t Entry(void *arg);

class CoroutineContext
{
    friend class CoroutineMgr;
    Coroutine *m_coroutine;

    stReqData m_req;
    ENodeState m_state;
    CTimerMS m_timer;

    CoroutineContext& operator=(const CoroutineContext&);

public:

    CoroutineContext()
    {
        m_state = E_READY;
        m_coroutine = CreateCoroutine(Entry, &m_req);
    }

    CoroutineContext(const CoroutineContext &cc):
        m_req(cc.m_req),
        m_state(cc.m_state),
        m_timer(cc.m_timer)
    {
        m_coroutine = CreateCoroutine(Entry, &m_req);
    }

    ~CoroutineContext()
    {
        DeleteCoroutine(m_coroutine);
        m_coroutine = NULL;
    }

    ENodeState GetState() const
    {
        return m_state;
    }

    void SetTimeout(uint32_t timeout)
    {
        m_timer.SetTimeout(timeout);
    }

    void BeginTimeout(uint32_t timeout)
    {
        m_timer.Reset();
        m_timer.SetTimeout(timeout);
    }

    uint64_t LeftTime()
    {
        return m_timer.LeftTime();
    }

    void SetState(ENodeState ns)
    {
        m_state = ns;
    }

    uint32_t GetID() const
    {
        return m_req.id;
    }

    Coroutine* GetContext()
    {
        return m_coroutine;
    }

    stReqData& GetReq()
    {
        return m_req;
    }
};

typedef map<uint32_t, CoroutineContext> MapIdToCC;

class CoroutineMgr : public nocopy, public single<CoroutineMgr>
{
    friend class single<CoroutineMgr>;
    MapIdToCC m_idtonode;
    CoroutineContext *m_active;
    static uint32_t staticid;
    CoroutineMgr(){}

public:

    static const uint32_t SBufferSize = 5 * 1024;

    void SetActive(CoroutineContext &cur)
    {
        m_active = &cur;
    }

    CoroutineContext* Get(uint32_t id)
    {
        MapIdToCC::iterator iter = m_idtonode.find(id);
        return m_idtonode.end() == iter ? NULL : &iter->second;
    }

    void Schedule(CoroutineContext &next)
    {
        LOG("switch to %d", next.m_req.id);
        Coroutine *c = m_active->GetContext();
        m_active = &next;
        __SwitchCoroutine__(c, next.GetContext());
    }

    void Schedule(uint32_t id = 0)
    {
        MapIdToCC::iterator iter = m_idtonode.find(id);
        if (m_idtonode.end() == iter)
        {
            LOG("id %u not exist", id);
            return;
        }

        Schedule(iter->second);
    }

    void WaitAndSchedule(uint32_t ms)
    {
        CExpireMgr::instance().Set(ms, m_active->GetID());
        m_active->SetState(E_WAIT);
        Schedule();
    }

    void SetReady(uint32_t id)
    {
        Get(id)->SetState(E_READY);
        CExpireMgr::instance().Del(id);
    }

    void ScheduleAll()
    {
        MapIdToCC::iterator iter;
        for (iter = m_idtonode.begin(); iter != m_idtonode.end();)
        {
            if (0 == iter->first)
            {
                ++iter;
                continue;
            }
            
            switch (iter->second.GetState())
            {
            case E_READY:
                Schedule(iter->second);
                continue;

            case E_END:
            {
                uint32_t id = iter->second.GetID();
                LOG("id %u is end.", id);
                ++iter;
                Destroy(id);
                continue;
            }
                break;

            case E_WAIT:
                LOG("is wait");
                break;

            default:
                break;
            }

            ++iter;
        }
    }

    EIORESULT Init() 
    { 
        if (!m_idtonode.empty())
        {
            return IO_ERR;
        }

        CoroutineContext &cc = Create();
        if (0 != cc.GetID())
        {
            return IO_ERR;
        }
        __SwitchCoroutine__(cc.GetContext(), cc.GetContext());
        m_active = &cc;
        return IO_OK;
    }

    CoroutineContext& Create()
    {
        CoroutineContext &item = m_idtonode[++staticid];
        item.GetReq().id = staticid;
        LOG("create new coroutine %d", staticid);
        return item;
    }

    EIORESULT Destroy(uint32_t id)
    {
        MapIdToCC::iterator iter = m_idtonode.find(id);
        if (m_idtonode.end() == iter)
        {
            return IO_ERR;
        }
        m_idtonode.erase(iter);
        return IO_OK;
    }
    CoroutineContext& GetActive()
    {
        return *m_active;
    }
};

