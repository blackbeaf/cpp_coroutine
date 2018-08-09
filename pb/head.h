#pragma once

#include "testpb.pb.h"
#include <string>
#include "../lib/eventmgr/util.h"

using namespace std;

class HeadPkg
{
protected:

    static uint32_t id;
    pbtest::Head m_head;
    string m_data;

public:

    static int Check(const void *src, uint32_t len)
    {
        if (0 == len)
        {
            return 0;
        }

        uint32_t reallen = *(uint32_t*)src;
        reallen = ntohl(reallen);
        if (reallen > len)
        {
            return 0;
        }
        return reallen + sizeof(reallen);
    }

    bool Decode(const void *src, uint32_t len)
    {
        if (0 == len)
        {
            return false;
        }

        return m_head.ParseFromArray((uint8_t*)src + sizeof(len), len - sizeof(len));
    }

    static int Seq(const void *src, uint32_t len)
    {
        if (Check(src, len) < 1)
        {
            return -1;
        }

        HeadPkg pkg;
        if (!pkg.Decode(src, len))
        {
            return -2;
        }
        LOG("seq %u", pkg.Head().seq());
        return pkg.Head().seq();
    }

    pbtest::Head& Head()
    {
        return m_head;
    }

    void SetData(const string &data)
    {
        m_head.set_data(data);
    }

    void Encode()
    {
        uint32_t len = 0;
        string pb;
        m_head.set_seq(++id);
        m_head.SerializeToString(&pb);
        len = pb.size();
        len = htonl(len);
        m_data.clear();
        m_data.append((char*)&len, sizeof(len));
        m_data.append(pb);
    }

    string& Body()
    {
        return *m_head.mutable_data();
    }

    string& Data()
    {
        return m_data;
    }
};