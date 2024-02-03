#ifndef THREAD_POOL_HPP_INCLUDED
#define THREAD_POOL_HPP_INCLUDED

class thread_pool_t {
public:
    thread_pool_t(unsigned long threads_amount)
    {
        m_threads.reserve(threads_amount);
    }

    template<typename F, typename... Args>
    std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>
    add_task(F&& f, Args&&... args)
    {
        using Return = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

        auto task = std::make_shared<std::packaged_task<Return()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::packaged_task<void(void)> wrapped_task{[task]() -> void { (*task)(); }};

        mutex.lock();
        m_tasks.push(std::move(wrapped_task));
        mutex.unlock();
        return task->get_future();
    }

    std::size_t get_tasks_amount()
    {
        std::lock_guard lg{mutex};
        return m_tasks.size();
    };

    void init()
    {
        for (auto idx = 0u; idx < m_threads.capacity(); ++idx) {
            m_threads.emplace_back(&thread_pool_t::run, this);
        }
    };

    ~thread_pool_t()
    {
        std::for_each(m_threads.begin(), m_threads.end(), [](std::thread& thread) {
            if (thread.joinable()) {
                thread.join();
            }
        });
    }
    thread_pool_t(const thread_pool_t&) = delete;
    thread_pool_t(thread_pool_t&&) = delete;
    thread_pool_t& operator=(thread_pool_t&) = delete;
    thread_pool_t& operator=(thread_pool_t&&) = delete;

protected:
    void run()
    {
        while (!m_tasks.empty()) {
            mutex.lock();
            auto task = std::move(m_tasks.front());
            m_tasks.pop();
            mutex.unlock();

            task();
        }
    }

protected:
    std::mutex mutex;
    std::vector<std::thread> m_threads;
    std::queue<std::packaged_task<void(void)>> m_tasks;
};

#endif