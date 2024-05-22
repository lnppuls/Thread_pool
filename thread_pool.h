#include <iostream>
#include<thread>
#include<queue>
#include<mutex>
#include<functional>
#include<condition_variable>
#include<future>

template<typename T>
class Safe_queue
{

public:
    Safe_queue(){ }
    ~Safe_queue(){ }
    bool empty(){
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }
    int size(){
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size();
    }
    void enqueue(T &t){
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.emplace(t);
    }
    bool dequeue(T &t){
        std::unique_lock<std::mutex> lock(m_mutex);
        if(m_queue.empty())
            return false;
        t = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }
private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
};

class Thread_pool
{
public:
    Thread_pool(int thread_num = 4);
    Thread_pool(const Thread_pool &) = delete;
    Thread_pool(Thread_pool &&) = delete;
    Thread_pool &operator=(const Thread_pool &) = delete;
    Thread_pool &operator=(Thread_pool &&) = delete;

    void init();
    void shutdown();

    template <typename F, typename... Args>
    auto submit(F &&f, Args &&...args) -> std::future<decltype(f(args...))>;
private:
    class Worker{
    private:
        int _id;
        Thread_pool *this_pool;
    public:
        Worker(int id,Thread_pool *this_pool);
        ~Worker();
        void operator() ();
    };
    bool is_active;
    int thread_num;
    std::mutex data_lock;
    Safe_queue<std::function<void()>> m_queue;
    std::vector<std::thread> m_threads;
    std::mutex thread_lock;
    std::condition_variable m_conditional_lock;
};

template <typename F, typename... Args>
auto Thread_pool::submit(F &&f, Args &&...args) -> std::future<decltype(f(args...))> {
    // 连接函数和参数定义，特殊函数类型，避免左右值错误​
    std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...); 
    auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

    std::function<void()> warpper_func = [task_ptr]()
    {
        (*task_ptr)();
    };
    // 队列通用安全封包函数，并压入安全队列
    m_queue.enqueue(warpper_func);

    // 唤醒一个等待中的线程
    m_conditional_lock.notify_one();

    // 返回先前注册的任务指针
    return task_ptr->get_future();
}