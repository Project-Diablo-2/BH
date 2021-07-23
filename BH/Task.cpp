#include "Task.h"

#include <concurrent_queue.h>
#include <Windows.h>

#include <vector>


class TaskHolder
{
    public:
        std::function<void()> task_;

        explicit TaskHolder( const std::function<void()> task )
        {
            task_ = task;
        }
};

class WorkerThread
{
    public:
        bool   canceled_;
        HANDLE t_handle_;

        WorkerThread() = default;

        ~WorkerThread()
        {
            canceled_ = true;
            CloseHandle(t_handle_);
        }
};


Concurrency::concurrent_queue<TaskHolder*> task_queue;
std::vector<std::unique_ptr<WorkerThread>> thread_pool;
HANDLE                                     wait_handle = nullptr;

namespace Task
{
    DWORD WINAPI QueueThread( void* arg )
    {
        TaskHolder* t;

        auto* const token = static_cast<WorkerThread*>(arg);
        while ( !token->canceled_ )
        {
            WaitForSingleObject(wait_handle, 2000);
            if ( token->canceled_ ) break;

            if ( task_queue.try_pop(t) )
            {
                if ( !task_queue.empty() )
                {
                    SetEvent(wait_handle); // Let the next task start while this one is processing
                }

                t->task_();
                delete t;
            }
        }

        return 0;
    }

    void InitializeThreadPool( const int size )
    {
        wait_handle = CreateEvent(nullptr, false, false, nullptr);

        for ( auto i = 0; i < size; i++ )
        {
            auto* const t        = new WorkerThread();
            auto* const t_handle = CreateThread(nullptr, 0, QueueThread, static_cast<void*>(t), 0, nullptr);
            t->t_handle_         = t_handle;
            thread_pool.push_back(std::unique_ptr<WorkerThread>(t));
        }
    }

    void StopThreadPool()
    {
        CloseHandle(wait_handle);
        thread_pool.clear();
    }

    void Enqueue( const std::function<void()> task )
    {
        task_queue.push(new TaskHolder(task));
        SetEvent(wait_handle);
    }
}
