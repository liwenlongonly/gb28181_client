//
// Created by 李文龙 on 2019/11/30.
//

#ifndef ASYNC_TASK_POOL_TEST_TASK_QUEUE_H
#define ASYNC_TASK_POOL_TEST_TASK_QUEUE_H

#include <atomic>
#include <memory>
#include <string>
#include <mutex>  // NOLINT
#include <future>  // NOLINT
#include <thread>  // NOLINT
#include <map>
#include "macro_definition.h"

NS_BEGIN

class TaskQueue: public std::enable_shared_from_this<TaskQueue>{
public:
    using Task = std::function<void()>;

    TaskQueue(const std::string& tag = "");
    virtual ~TaskQueue();

    virtual void start();

    virtual void task(Task f);

    virtual void task(Task f ,int delay_ms);

    virtual void stop();

    virtual int taskCount() const;

    virtual bool isCurrent() const;

    TaskQueue(const TaskQueue &) = delete;
    const TaskQueue& operator=(const TaskQueue&) = delete;
private:
    std::chrono::system_clock::time_point getFirstTime();
private:
    std::multimap<std::chrono::system_clock::time_point, Task> task_queue_;
    std::atomic<bool> started_;
    std::atomic<bool> closed_;
    std::unique_ptr<std::thread> thread_;
    mutable std::mutex task_mutex_;
    std::condition_variable new_task_scheduled_;

    std::string tag_;
};

NS_END

#endif //ASYNC_TASK_POOL_TEST_TASK_QUEUE_H
