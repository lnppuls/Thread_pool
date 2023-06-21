#include<mutex>
#include<functional>
#include<condition_variable>
#include<iostream>
#include<thread>
#include<queue>
#include<future>
#include<chrono>



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
private:
    class Work{
    private:
        int _id;
        Thread_pool * mpool;
    public:
        Work(int id,Thread_pool *the_pool) : _id(id) , mpool(the_pool) { }
        ~Work(){ } 
        void operator() (){
            while (mpool->isrun)
            {
                std::function<void()> func;
                bool dequeued;
                {
                    std::unique_lock<std::mutex> lock(mpool->threadlock);
                    if(mpool->Tasks.empty()){
                        mpool->condition.wait(lock);
                    }

                    dequeued = mpool->Tasks.dequeue(func);
                }
                if(dequeued)
                    func();
            }
            
        }
    };
    int thread_num;
    bool isrun = false;
    std::mutex threadlock;
    std::condition_variable condition;
    Safe_queue<std::function<void()>> Tasks;
    std::vector<std::thread> m_threads;
    
public:
    Thread_pool(int num) : thread_num(num), isrun(true) , m_threads(std::vector<std::thread>(num))   { }
    void init(){
        for(int i = 0;i < thread_num;i++){
            m_threads[i] = std::thread(Work(i,this));
        }
    }
    void shutdown(){
        isrun = false;
        condition.notify_all(); // 通知，唤醒所有工作线程

        for (int i = 0; i < m_threads.size(); ++i)
        {
            if (m_threads.at(i).joinable()) // 判断线程是否在等待
            {
                m_threads.at(i).join(); // 将线程加入到等待队列
            }
        }
    }

    template <typename F,typename... Args>
    auto submit(F &&f,Args &&...args) -> std::future<decltype(f(args...))>      
    {
        std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
        std::function<void()> warpper_func = [task_ptr]()
        {
            (*task_ptr)();
        };
        Tasks.enqueue(warpper_func);
        condition.notify_one();
        return task_ptr->get_future();
    }   

};

int tes_add(int a,int b){
    int res = 0;
    for(int i = a;i <= b;i++){
        res += i;
    }
    return res;
}

void pooltes(int num,Thread_pool & pool){
    std::queue<std::future<int>> res;
    for(int i = 1;i <= num;i++){
        res.emplace(pool.submit(tes_add,i+1,i*1000));
    }

    for(int i = 1;i <= num;i++){
        std::cout << res.front().get() << std::endl;
        res.pop();
    }
}


int main(){
    auto m_pool = Thread_pool(1);
    m_pool.init();
    auto start_time = std::chrono::steady_clock::now();
    pooltes(10000,m_pool);
    auto end_time = std::chrono::steady_clock::now();
    double usetime = std::chrono::duration<double>(end_time-start_time).count();
    std::cout << "time: " << usetime << std::endl;
    m_pool.shutdown();
}

