#include "CoroutineMgr.h"

uint32_t CoroutineMgr::staticid = -1;

int64_t Entry(void *arg)
{
    if (NULL == arg)
    {
        LOG("arg is null");
    }
    else
    {

        stReqData *rd = reinterpret_cast<stReqData*>(arg);
        CoroutineMgr::instance().SetActive(*CoroutineMgr::instance().Get(rd->id));
        rd->m_cb(arg);
        CoroutineMgr::instance().GetActive().SetState(E_END);
    }
    CoroutineMgr::instance().Schedule();
    return 0;
}
