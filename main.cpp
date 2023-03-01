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

    std::vector<std::future<typename task::base_config::config_t>> vec(threads_amount - 1);

    const auto init_dir = std::filesystem::current_path() / results_folder / time;
    {
        std::ofstream info{"info.txt"};
        info << init_dir.string() << "\t" << raw_data_folder << "\n";
        info << task::create_config_info();
        info.flush();
        info.close();
    }
    const auto currentDir = (init_dir / raw_data_folder).string();
    outputer_t::create_directories(currentDir);
    std::filesystem::current_path(currentDir);
    const auto configs = task::base_config::getConfigs();

    auto iter = configs.begin();
    while (iter != configs.end())
    {
        for (auto id = 0u; id < threads_amount - 1; ++id)
        {
            if (iter != configs.end())
            {
                const auto config = *iter;
                vec[id] = std::async(std::launch::async, task::calculation, config,
                                     currentDir); // TODO: не использовать std::async
                iter++;
            }
        }

        if (iter != configs.end())
        {
            const auto this_config = *iter;
            iter++;
            auto this_calc = calculation(this_config, currentDir);
            std::cout << "config : " << this_calc << "\t --- done \n";
        }

        for (auto &elem : vec)
        {
            std::cout << "config : " << elem.get() << "\t --- done \n";
        }
        std::cout << std::endl;
    }

    stat::stater::makeStat(init_dir.string(), raw_data_folder);
    return 0;
}