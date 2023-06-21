#include "thread_pool.h"
Thread_pool::Thread_pool(int thread_num) : m_threads(std::vector<std::thread>(thread_num))
{   
}

Thread_pool::Worker::Worker(int id, Thread_pool *this_pool) : _id(id) , this_pool(this_pool)
{
}

Thread_pool::Worker::~Worker()
{
}

void Thread_pool::Worker::operator() ()
{
    std::function<void()> func; // 定义基础函数类func
    bool dequeued; // 是否正在取出队列中元素
    while (this_pool->is_active)
    {
        {
            std::unique_lock<std::mutex> lock(this_pool->thread_lock);

            if(this_pool->m_queue.empty()){
                this_pool->m_conditional_lock.wait(lock);
            }
            
            dequeued = this_pool->m_queue.dequeue(func);
        }
        if(dequeued)
            func();
    }
}

void Thread_pool::init()
{
    for(int i = 0;i < thread_num;i++){
        m_threads[i] = std::thread(Worker(i,this));
    }
}

void Thread_pool::shutdown()
{
    is_active = false;
    m_conditional_lock.notify_all();
    for (int i = 0; i < m_threads.size(); ++i)
        {
            if (m_threads.at(i).joinable()) // 判断线程是否在等待
            {
                m_threads.at(i).join(); // 将线程加入到等待队列
            }
        }
}
