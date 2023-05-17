#include <algorithm>
#include <coroutine>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <future>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <memory>

#include "config.hpp"
#include "thread_function.hpp"

unsigned long parseArgs(int argc, char *argv[])
{
    unsigned long threads_amount{0};
    if (argc == 1)
    {
        threads_amount = 2;
    }
    else
    {
        threads_amount = std::stoul(argv[1]);
    }
    if (threads_amount > std::thread::hardware_concurrency())
    {
        threads_amount = std::thread::hardware_concurrency();
    }
    if (threads_amount <= 1)
    {
        threads_amount = 2;
    }
    return threads_amount;
}

class thread_pool_t
{
public:
    thread_pool_t(unsigned long threads_amount)
    {
        m_threads.reserve(threads_amount);
    }

    template <typename F, typename... Args>
    std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> add_task(F &&f, Args &&...args)
    {
        using Return = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

        auto task = std::make_shared<std::packaged_task<Return()>>(std::bind_front(std::forward<F>(f), std::forward<Args>(args)...));
        std::packaged_task<void(void)> wrapped_task{
            [task]() -> void
            {
                (*task)();
            }};

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
        for (auto idx = 0u; idx < m_threads.capacity(); ++idx)
        {
            m_threads.emplace_back(&thread_pool_t::run, this);
        }
    };

    ~thread_pool_t()
    {
        std::for_each(m_threads.begin(), m_threads.end(),
                      [](std::thread &thread)
                      {
                          if (thread.joinable())
                          {
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
        while (!m_tasks.empty())
        {
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

int main(int argc, char *argv[])
{
    constexpr auto results_folder = "results";
    constexpr auto raw_data_folder = "raw";

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y_%H-%M-%S");
    const auto time = "data_" + oss.str();

    const auto threads_amount = parseArgs(argc, argv);
    std::cout << "threads_amount = " << threads_amount << "\n";

    const auto init_dir = std::filesystem::current_path() / results_folder / time;
    {
        std::ofstream info{"info.txt"};
        info << init_dir.string() << "\t" << raw_data_folder << "\n";
        info << task::create_config_info();
        info.flush();
        info.close();
        std::cout << "info.txt filled\n";
    }
    const auto currentDir = (init_dir / raw_data_folder).string();
    std::filesystem::create_directories(currentDir);
    std::filesystem::current_path(currentDir);

    auto configs = task::base_config::getConfigs();
    std::for_each(configs.begin(), configs.end(),
                  [](const auto &config)
                  {
                      std::filesystem::create_directories(std::filesystem::current_path() / task::createName(config));
                  });

    thread_pool_t thread_pool{threads_amount};

    std::vector<std::future<task::base_config::config_t>> futures{};
    futures.reserve(configs.size());
    std::for_each(configs.begin(), configs.end(),
                  [&futures, &thread_pool, &currentDir](auto config) -> void
                  {
                      std::string_view dir = currentDir;
                      futures.push_back(thread_pool.add_task(task::calculation, std::move(config), std::move(dir)));
                  });

    thread_pool.init();

    while (!futures.empty())
    {
        for (auto iter = futures.begin(); iter != futures.end(); ++iter)
        {
            auto &future = *iter;
            const auto status = future.wait_for(std::chrono::seconds(3));
            if (status != std::future_status::timeout)
            {
                const auto result = future.get();
                std::cout << "config : " << result << "\t --- done \n";
                futures.erase(iter);
                break;
            }
        }
    }

    stat::stater::makeStat(init_dir, raw_data_folder);
    stat::stater::calcGMR(init_dir);
    stat::stater::calcP(init_dir);

    return 0;
}