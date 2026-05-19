#include "AsioIOServicePool.h"
#include <iostream>
#include <thread>
using namespace std;
AsioIOServicePool::AsioIOServicePool(std::size_t size) :_ioServices(size),
_workGuards(size), _nextIOService(0) {
    for (std::size_t i = 0; i < size; ++i) {
        _workGuards[i] = std::make_unique<WorkGuard>(
            boost::asio::make_work_guard(_ioServices[i])
        );
    }

    for (std::size_t i = 0; i < _ioServices.size(); ++i) {
        _threads.emplace_back([this, i]() {
            _ioServices[i].run();
            });
    }
}

AsioIOServicePool::~AsioIOServicePool() {
    Stop();
    std::cout << "AsioIOServicePool destruct" << endl;
}

// 无锁竞争：fetch_add 是硬件级别的原子操作，无需互斥锁
boost::asio::io_context& AsioIOServicePool::GetIOService() {
    std::size_t index = _nextIOService.fetch_add(1, std::memory_order_relaxed);
    index = index % _ioServices.size();
    return _ioServices[index];
}

void AsioIOServicePool::Stop() {
    for (auto& guard : _workGuards) {
        if (guard) {
            guard.reset();
        }
    }

    for (auto& thread : _threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    _threads.clear();
}