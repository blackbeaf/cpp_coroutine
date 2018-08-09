#pragma once

#include "util.h"

class CExpireMgr : public nocopy, public single<CExpireMgr>
{
protected:

    typedef set<uint32_t> SetU32;
    typedef map<uint64_t, SetU32> MapExpireToId;

    MapExpireToId m_expire_to_id;

    typedef map<uint32_t, uint64_t> MapIdToExpire;

    MapIdToExpire m_id_to_expire;

public:

    void Del(uint32_t id)
    {
        MapIdToExpire::iterator iter = m_id_to_expire.find(id);
        if (m_id_to_expire.end() == iter)
        {
            LOG("id %u not exist", id);
            return;
        }

        MapExpireToId::iterator it2 = m_expire_to_id.find(iter->second);
        if (m_expire_to_id.end() == it2)
        {
            LOG("id %u expire %zu not exist", id, iter->second);
            return;
        }

        SetU32::iterator it3 = it2->second.find(id);
        if (it2->second.end() == it3)
        {
            LOG("id %u expire %zu has no id %u", id, iter->second, id);
            return;
        }

        LOG("key expire %zu value set size %zu", it2->first, it2->second.size());
        it2->second.erase(it3);
        LOG("set size %zu", it2->second.size());

        if (it2->second.empty())
        {
            LOG("expire %zu has no child, erase", iter->second);
            m_expire_to_id.erase(it2);
        }

        m_id_to_expire.erase(iter);
    }

    void Add(uint32_t id, uint64_t expire)
    {
        m_expire_to_id[expire].insert(id);
        m_id_to_expire[id] = expire;
    }

    void Set(uint64_t expire, uint32_t id)
    {
        Del(id);
        Add(id, CTimerMS().GetMs() + expire);
    }

    //返回需要等待的超时时间或者已经超时的协程id
    void NextWaitTime(uint64_t &ms, uint32_t &id)
    {
        ms = 0;
        id = 0;
        uint64_t now = CTimerMS().GetMs();
        MapExpireToId::iterator iter = m_expire_to_id.begin();
        if (m_expire_to_id.end() == iter)
        {
            //没有对象在等待超时, 将ms设为比较大的一个时间
            ms = TIME_EXPIRE;
            LOG("no obj exist, ms = %zu", ms);
            return;
        }

        if (iter->first > now)
        {
            ms = iter->first - now;
            LOG("need wait %zu", ms);
            return;
        }

        //该定时器已经失效
        if (iter->second.empty())
        {
            //child为空??
            LOG("ERROR!!! %zu is expired but no child", iter->first);
        }
        else
        {
            LOG("%zu is expired, diff=%zu child = %zu", 
                iter->first, now - iter->first, iter->second.size());
            id = *iter->second.begin();
            return;
        }

    }
};
