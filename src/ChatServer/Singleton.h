#pragma once
#include <memory>
#include <iostream>
using namespace std;
//用于CRTP（返回智能指针）
template<typename T>
class Singleton {
public:
    static std::shared_ptr<T> GetInstance() {
        static T m_instance;
        return std::shared_ptr<T>(&m_instance, [](T*) {});
    }

protected:
    Singleton() = default;
    ~Singleton() = default;

    // 禁用拷贝与赋值
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;
};
