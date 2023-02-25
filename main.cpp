#include <coroutine>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <future>
#include <iomanip>
#include <iostream>
#include <queue>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "config.hpp"
#include "output.hpp"
#include "stat.hpp"
#include "system.hpp"

typename task::base_config::config_t calculation(typename task::base_config::config_t config,
                                                 std::string_view current_dir)
{
    using task::base_config;
    outputer_t outputer{config, current_dir};
    outputer.createDirectoriesAndEnter(task::createName(config));

    auto m_out = outputer.createFile("m");
    m_out.printLn("m1", "m1x", "m1y", "m1z", "m2", "m2x", "m2y", "m2z");
    auto j_out = outputer.createFile("j");
    j_out.printLn("j_up", "j_down");

    auto sample = task::createSample(config);
    task::prepare(sample);

    double j_up{};
    double j_down{};
    base_config::spin_t::magn_t magn_fst_average{};
    base_config::spin_t::magn_t magn_snd_average{};
    constexpr auto mcs_amount = base_config::mcs_observation + base_config::t_wait_vec.back();

    {
        constexpr auto queue_size = 500u;
        std::queue<std::array<base_config::spin_t::magn_t, 2u>> queue{};
        for (auto mcs = 0u; mcs < queue_size; ++mcs)
        {
            const auto magns = sample.makeMonteCarloStep();
            queue.push(magns);
            magn_fst_average += magns[0];
            magn_snd_average += magns[1];
        }
        const auto size_as_double = static_cast<double>(queue.size());
        magn_fst_average /= size_as_double;
        magn_snd_average /= size_as_double;
        const auto eps = 10.0 / (base_config::L * base_config::L * config.N);
        auto mcs_on_dynamic_stage = queue_size;
        for (auto _ = 0u; _ < 10'000; ++_)
        {
            const auto [magn1, magn2] = sample.makeMonteCarloStep();
            m_out.printLn(abs(magn1), magn1, abs(magn2), magn2);

            const auto elem_to_pop = queue.front();
            magn_fst_average -= elem_to_pop[0] / size_as_double;
            magn_snd_average -= elem_to_pop[1] / size_as_double;

            magn_fst_average += magn1 / size_as_double;
            magn_snd_average += magn2 / size_as_double;

            queue.pop();
            queue.push({magn1, magn2});

            if (is_almost_equals(magn_fst_average, magn1, eps) && is_almost_equals(magn_snd_average, magn2, eps))
            {
                mcs_on_dynamic_stage += _;
                break;
            }
        };

        {
            auto info_out = outputer.createFile("info");
            info_out.printLn("Dynamic stage duration : ", std::to_string(mcs_on_dynamic_stage), "MCS/s");
        }
    }
    for (auto mcs = 0u; mcs < mcs_amount; ++mcs)
    {
        if (mcs == base_config::t_wait_vec.front())
        {
            const auto [up, down] = sample.startObservation();
            j_up += up;
            j_down += down;
        }
        if (mcs > base_config::t_wait_vec.front())
        {
            const auto [up, down] = sample.makeJCalc();
            j_up += up;
            j_down += down;
            j_out.printLn(j_up, j_down);
        }

        const auto [magn1, magn2] = sample.makeMonteCarloStep();
        m_out.printLn(abs(magn1), magn1, abs(magn2), magn2);
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