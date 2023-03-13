#ifndef THREAD_FUNCTION_HPP_INCLUDED
#define THREAD_FUNCTION_HPP_INCLUDED

#include <queue>

#include "output.hpp"
#include "stat.hpp"
#include "system.hpp"

namespace task
{
inline typename task::base_config::config_t calculation(typename task::base_config::config_t config,
                                                        std::string_view current_dir)
{
    using task::base_config;
    outputer_t outputer{config, current_dir};
    outputer.EnterDirectory(task::createName(config));

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
        auto m_dynamic_out = outputer.createFile("m_dynamic");
        m_dynamic_out.printLn("m1", "m1x", "m1y", "m1z", "m2", "m2x", "m2y", "m2z");
        constexpr auto queue_size = 500u;
        std::queue<std::array<base_config::spin_t::magn_t, 2u>> queue{};
        for (auto mcs = 0u; mcs < queue_size; ++mcs)
        {
            const auto magns = sample.makeMonteCarloStep();
            m_dynamic_out.printLn(abs(magns[0]), magns[0], abs(magns[1]), magns[1]);
            magn_fst_average += magns[0];
            magn_snd_average += magns[1];
            queue.push(std::move(magns));
        }
        const auto size_as_double = static_cast<double>(queue.size());
        magn_fst_average /= size_as_double;
        magn_snd_average /= size_as_double;
        const auto eps = 1e-3; // 20.0 / (base_config::L * base_config::L * config.N);
        auto mcs_on_dynamic_stage = queue_size;
        for (auto _ = 0u; _ < 10'000; ++_)
        {
            const auto [magn1, magn2] = sample.makeMonteCarloStep();
            m_dynamic_out.printLn(abs(magn1), magn1, abs(magn2), magn2);

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

} // namespace task

#endif