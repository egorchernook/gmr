#ifndef THREAD_POOL_HPP_INCLUDED
#define THREAD_POOL_HPP_INCLUDED

#include <cassert>
#include <chrono>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <thread>
#include <vector>

class thread_pool_t {

    using task_t = std::packaged_task<void(void)>;

public:
    struct init_exception : public std::logic_error {
        using std::logic_error::logic_error;
    };

    thread_pool_t(uint threads_amount = 2) noexcept
    {
        assert(threads_amount > 0);
        m_threads.reserve(threads_amount);
    }

    template<typename F, typename... Args>
    std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>
    add_task(F&& f, Args&&... args)
    {
        using Return = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

        auto task_ptr = std::make_shared<std::packaged_task<Return()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        task_t wrapped_task{[task_ptr]() { (*task_ptr)(); }};

        mutex.lock();
        m_tasks.push_back(std::move(wrapped_task));
        mutex.unlock();
        return task_ptr->get_future();
    }

    std::size_t get_tasks_amount()
    {
        std::lock_guard lg{mutex};
        return m_tasks.size();
    };

    void init()
    {
        std::lock_guard lg{mutex};
        if (m_inited) {
            throw init_exception{"Method init called more than once"};
        };
        for (auto idx = 0u; idx < m_threads.capacity(); ++idx) {
            m_threads.emplace_back(&thread_pool_t::run, this);
        }
        m_inited = true;
    };

    std::future<void> resize(std::size_t size)
    {
        if (size == 0) {
            throw std::underflow_error{"Method resize called with size = 0"};
        }
        {
            std::lock_guard lg{mutex};
            if (size == m_threads.size()) {
                std::promise<void> promise;
                auto result = promise.get_future();
                promise.set_value();
                return result;
            }
        }
        mutex.lock();
        const auto current_size = m_threads.size();
        mutex.unlock();
        if (size > current_size) {
            std::promise<void> promise;
            auto result = promise.get_future();
            std::lock_guard lg{mutex};
            m_threads.reserve(size);
            for (std::size_t idx = current_size; idx < size; ++idx) {
                m_threads.emplace_back(&thread_pool_t::run, this);
            }
            promise.set_value();

            return result;
        }

        auto extra_threads = size - current_size;
        auto destoy_task_ptr
            = std::make_shared<std::packaged_task<void(void)>>([this, extra_threads]() {
                  static std::vector<std::future<std::thread::id>> futures;
                  futures.reserve(extra_threads);
                  for (uint idx = 0; idx < extra_threads; ++idx) {
                      auto task_ptr = std::make_shared<std::packaged_task<std::thread::id(void)>>(
                          []() { return std::this_thread::get_id(); });
                      task_t wrapped_task{[task_ptr]() { (*task_ptr)(); }};
                      futures.emplace_back(task_ptr->get_future());
                      std::lock_guard lg{mutex};
                      m_tasks.push_front(std::move(wrapped_task));
                  }

                  while (!futures.empty()) {
                      for (auto iter = futures.begin(); iter != futures.end(); ++iter) {
                          auto& future = *iter;
                          const auto status = future.wait_for(std::chrono::milliseconds(100));
                          if (status != std::future_status::timeout) {
                              const auto id = future.get();
                              std::lock_guard lg{mutex};
                              for (uint idx = 0; idx < m_threads.size(); ++idx) {
                                  if ((m_threads[idx].get_id() == id) && m_threads[idx].joinable()) {
                                      m_threads[idx].join();
                                  }
                              }
                              futures.erase(iter);
                              break;
                          }
                      }
                  }

                  std::lock_guard lg{mutex};
                  for (uint _ = 0; _ < extra_threads; ++_) {
                      for (auto iter = m_threads.begin(); iter != m_threads.end(); ++iter) {
                          if (!iter->joinable()) {
                              m_threads.erase(iter);
                              break;
                          }
                      }
                  }
              });

        task_t wrapped_task{[destoy_task_ptr]() { (*destoy_task_ptr)(); }};
        auto result = destoy_task_ptr->get_future();
        std::lock_guard lg{mutex};
        m_tasks.push_front(std::move(wrapped_task));
        return result;
    }

    ~thread_pool_t()
    {
        mutex.lock();
        m_termination = true;
        mutex.unlock();
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

private:
    void run()
    {
        mutex.lock();
        while (!m_termination) {
            if (!m_tasks.empty()) {
                auto task = std::move(m_tasks.front());
                m_tasks.pop_front();
                mutex.unlock();

                task();
            }
            mutex.unlock();
        }
        mutex.unlock();
    }

    bool m_termination = false;
    bool m_inited = false;
    std::mutex mutex;
    std::vector<std::thread> m_threads;
    std::deque<task_t> m_tasks;
};

#endif