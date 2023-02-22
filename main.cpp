#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <cstdlib>
#include <string_view>
#include <filesystem>
#include <coroutine>
#include <future>
#include <iomanip>
#include <ctime>

#include "config.hpp"
#include "system.hpp"
#include "output.hpp"
#include "stat.hpp"

typename task::config_t
calculation(typename task::config_t config,
            std::string_view current_dir)
{
    using task::base_config;
    outputer_t outputer{config, current_dir};
    outputer.createDirectoriesAndEnter(task::createName(config));

    auto m_out = outputer.createFile("m");
    m_out.printLn(
             "m1", "m1x", "m1y", "m1z",
             "m2", "m2x", "m2y", "m2z")
        .printLn("mcs/s");
    auto j_out = outputer.createFile("j");
    j_out.printLn("j_up", "j_down")
        .printLn("mcs/s");

    auto sample = task::createSample(config);
    task::prepare(sample);

    double j_up{};
    double j_down{};
    constexpr auto mcs_amount = base_config::mcs_observation + base_config::t_wait_vec::back();
    for (auto mcs = 0u; mcs < mcs_amount; ++mcs)
    {
        if (mcs == base_config::t_wait_vec.front())
        {
            const auto [up, down] =
                sample.startObservation();
            j_up += up;
            j_down += down;
        }
        if (mcs > base_config::t_wait_vec.front())
        {
            const auto [up, down] =
                sample.makeJCalc();
            j_up += up;
            j_down += down;
        }
        j_out.printLn(mcs, j_up, j_down);

        const auto [magn1, magn2] = sample.makeMonteCarloStep();
        m_out.printLn(mcs,
                      abs(magn1), magn1,
                      abs(magn2), magn2);
    }
    return config;
}

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

    std::vector<std::future<typename task::config_t>> vec(threads_amount - 1);

    const auto init_dir = std::filesystem::current_path() / results_folder / time;
    {
        std::ofstream info{"info.txt"};
        info << init_dir << "\t" << raw_data_folder;
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
                vec[id] = std::async(std::launch::async, calculation, config, currentDir);
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

    stat::stater::makeStat(init_dir, raw_data_folder);
    return 0;
}