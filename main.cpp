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

class tread_pool_t
{
    using config_t = task::base_config::config_t;
    using task_t = std::packaged_task<config_t(config_t, std::string_view)>;

public:
    tread_pool_t(unsigned long threads_amount, std::vector<config_t> &&configs, const std::string &dir)
        : m_configs{configs}, m_dir{dir}
    {
        m_threads.reserve(threads_amount);
    }

    void init()
    {
        for (auto idx = 0u; idx < m_threads.capacity(); ++idx)
        {
            m_threads.emplace_back(&tread_pool_t::run, this);
        }
    };

    template <class Rep, class Period>
    std::optional<config_t> get_next_result(const std::chrono::duration<Rep, Period> &duration)
    {
        std::lock_guard lg{mutex};
        if (m_futures.empty())
        {
            if (m_configs.empty())
            {
                throw std::range_error("There are cannot be more results");
            }
            return {};
        }
        for (auto iter = m_futures.begin(); iter != m_futures.end(); ++iter)
        {
            auto &future = *iter;
            const auto status = future.wait_for(duration);
            if (status != std::future_status::timeout)
            {
                auto result = future.get();
                m_futures.erase(iter);
                return result;
            }
        }
        return {};
    }

    ~tread_pool_t()
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

protected:
    void run()
    {
        while (!m_configs.empty())
        {
            task_t task{task::calculation};

            mutex.lock();
            m_futures.push_back(task.get_future());

            auto config = std::move(m_configs.back());
            m_configs.pop_back();

            mutex.unlock();

            task(config, m_dir);
        }
    }

protected:
    std::mutex mutex;
    std::vector<std::thread> m_threads;
    std::vector<config_t> m_configs;
    std::string m_dir;
    std::vector<std::future<config_t>> m_futures;
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
    const auto calculations_amount = configs.size();
    tread_pool_t thread_pool{threads_amount, std::move(configs), currentDir};
    thread_pool.init();

    for (auto _ = 0u; _ < calculations_amount; ++_)
    {
        try
        {
            auto res = thread_pool.get_next_result(std::chrono::seconds(5));
            if (res.has_value())
            {
                std::cout << "config : " << res.value() << "\t --- done \n";
            }
            else
            {
                _--;
            }
        }
        catch (std::exception &ignored)
        {
            _--;
        }
    }

    stat::stater::makeStat(init_dir, raw_data_folder);
    stat::stater::calcGMR(init_dir);
    stat::stater::calcP(init_dir);

    return 0;
}