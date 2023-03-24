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

int main(int argc, char *argv[])
{
    constexpr auto results_folder = "results";
    constexpr auto raw_data_folder = "raw";

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y_%H-%M-%S");
    const auto time = "data_" + oss.str();

    unsigned long threads_amount{};
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

    const auto configs = task::base_config::getConfigs();
    std::for_each(configs.begin(), configs.end(), [](const auto &config) {
        std::filesystem::create_directories(std::filesystem::current_path() / task::createName(config));
    });

    std::vector<std::thread> vec_threads(threads_amount);
    std::vector<std::future<typename task::base_config::config_t>> vec_futures(threads_amount);
    {
        auto iter = configs.begin();
        while (iter != configs.end())
        {
            for (auto id = 0u; id < threads_amount; ++id)
            {
                if (iter != configs.end())
                {
                    const auto config = *iter;
                    std::packaged_task task{task::calculation};
                    vec_futures[id] = task.get_future();
                    vec_threads[id] = std::thread{std::move(task), config, currentDir};
                    iter++;
                }
            }

            for (auto id = 0u; id < vec_threads.size(); ++id)
            {
                if (vec_threads[id].joinable())
                {
                    std::cout << "config : " << vec_futures[id].get() << "\t --- done \n";
                    vec_threads[id].join();
                }
            }
            std::cout << std::endl;
        }
    }
    stat::stater::makeStat(init_dir, raw_data_folder);
    stat::stater::calcGMR(init_dir);

    return 0;
}