//
// Created by 李文龙 on 2019/11/30.
//

#include "task_queue.h"

NS_BEGIN

TaskQueue::TaskQueue(const std::string& tag):
started_{false},
closed_{false},
tag_{tag}{

}

TaskQueue::~TaskQueue() {
    stop();
}

void TaskQueue::start() {
    if (started_.exchange(true)) {
        return;
    }
    closed_.store(false);
    auto promise = std::make_shared<std::promise<void>>();
    auto work = [this, &promise](){
        promise->set_value();
        std::unique_lock<std::mutex> lock(task_mutex_);
        while (!closed_){
            if(!closed_ && task_queue_.empty()){
                new_task_scheduled_.wait(lock);
            }
            std::chrono::system_clock::time_point time = getFirstTime();
            while (!closed_ && !task_queue_.empty()
                && new_task_scheduled_.wait_until(lock, time) != std::cv_status::timeout) {
                time = getFirstTime();
            }
            if(closed_){
                break;
            }

            if (task_queue_.empty()) {
                continue;
            }

            Task f = task_queue_.begin()->second;
            task_queue_.erase(task_queue_.begin());

            lock.unlock();
            f();
            lock.lock();
        }
    };
    thread_.reset(new std::thread(work));
    promise->get_future().wait();
}

void TaskQueue::task(TaskQueue::Task f) {
    task(f, 0);
}

void TaskQueue::task(TaskQueue::Task f, int delay_ms) {
    std::lock_guard<std::mutex> lock(task_mutex_);
    task_queue_.insert(std::make_pair(std::chrono::system_clock::now() + std::chrono::milliseconds(delay_ms), f));
    new_task_scheduled_.notify_one();
}

int TaskQueue::taskCount() const {
    std::lock_guard<std::mutex> lock(task_mutex_);
    return task_queue_.size();
}

void TaskQueue::stop() {
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        if (!closed_.exchange(true)) {
            new_task_scheduled_.notify_all();
        }
    }

    if (thread_) {
        thread_->join();
    }
    started_.store(false);
    task_queue_.clear();
}

bool TaskQueue::isCurrent() const {
    if(thread_){
        std::thread::id current_thread_id = std::this_thread::get_id();
        if(current_thread_id == thread_->get_id()){
            return true;
        }
    }
    return false;
}

std::chrono::system_clock::time_point TaskQueue::getFirstTime() {
    return task_queue_.empty() ? std::chrono::system_clock::now() : task_queue_.begin()->first;
}

NS_END