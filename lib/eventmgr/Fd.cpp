#include "Fd.h"

int CFd::Get() const
{
    return m_fd->Get();
}

bool CFd::operator<(const CFd &obj) const
{
    return Get() < obj.Get();
}

void CFd::SetNewfd(int fd)
{
    m_fd = PtrFd(new stFd(fd));
}