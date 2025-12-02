#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h> // Epoll 的核心库
#include <unistd.h>    // close 函数
#include <vector>      // C++ 的动态数组

class Epoller {
private:
    // 1. epollFd_: 这就是那个“群号”
    // 我们把它藏在 private 里，不让外面的人随便改，只有我自己能用。
    int epollFd_;
    // events_: 这就是收信箱(用来装就绪事件的数组)
    // 现在用 std::vector，因为它更灵活
    std::vector<struct epoll_event> events_;

public:
    explicit Epoller(int maxEvent = 1024)
    {
        epollFd_ = epoll_create(1);
        events_.resize(maxEvent);
    }
    ~Epoller()
    {
        if(epollFd_!=-1)
        {
            close(epollFd_);
        }
    }
    bool AddFd(int fd,uint32_t events)
    {
        if(fd<0) return false;
        struct epoll_event ev = {0};
        ev.data.fd=fd;
        ev.events=events;
        return 0==epoll_ctl(epollFd_,EPOLL_CTL_ADD,fd,&ev);
    }
    bool DelFd(int fd)
    {
        if(fd<0) return false;
        return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
    }
    int Wait(int timeoutMs)
    {
        return epoll_wait(epollFd_,&events_[0],static_cast<int>(events_.size()),timeoutMs);
    }
    int GetEventFd(size_t i) const{
        return events_[i].data.fd;
    }
    uint32_t GetEvents(size_t i) const{
        return events_[i].events;
    }

};

#endif // EPOLLER_H