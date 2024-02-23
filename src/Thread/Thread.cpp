#include "Thread.h"

Thread::Thread() : is_start_(false), is_detach_(false) {}

Thread::~Thread() {
    if (is_start_ == true && is_detach_ == false) {
        detach();
    }
}

bool Thread::start(ThreadFunc func) {
    th_ = std::thread(func);

    is_start_ = true;
    return true;
}

bool Thread::detach() {
    if (is_start_ != true) {
        return false;
    }

    if (is_detach_ == true) {
        return true;
    }

    th_.detach();

    is_detach_ = true;

    return true;
}

bool Thread::join() {
    if (is_start_ != true || is_detach_ == true) {
        return false;
    }

    th_.join();

    return true;
}