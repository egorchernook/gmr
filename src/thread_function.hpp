#ifndef THREAD_FUNCTION_HPP_INCLUDED
#define THREAD_FUNCTION_HPP_INCLUDED

#include "config.hpp"
#include "output.hpp"
#include "stat.hpp"
#include "system.hpp"

#include <chrono>
#include <cmath>
#include <queue>
#include <string>
#include <valarray>

namespace task {
inline typename task::base_config::config_t
calculation(typename task::base_config::config_t config, std::string_view current_dir)
{
    using task::base_config;
    outputer_t outputer{current_dir};
    outputer.EnterDirectory(task::createName(config));

    auto m_out = outputer.createFile("m_id=" + std::to_string(config.stat_id) + ".txt");
    m_out.printLn("m1", "m1x", "m1y", "m1z", "m2", "m2x", "m2y", "m2z");
    auto theta_out = outputer.createFile("cos_theta_id=" + std::to_string(config.stat_id) + ".txt");
    theta_out.printLn("cos_theta");
    auto thetaXZ_out
        = outputer.createFile("cos_thetaXZ_id=" + std::to_string(config.stat_id) + ".txt");
    thetaXZ_out.printLn("cos_thetaXZ");

    std::vector<typename outputer_t::output_file_t> j_out_vec{};
    std::vector<typename outputer_t::output_file_t> Nup_out_vec{};
    std::vector<typename outputer_t::output_file_t> Ndown_out_vec{};
    for (auto idx = 0u; idx < task::base_config::j_stat_amount; ++idx) {
        const std::string j_filename = "j_id="
            + std::to_string(config.stat_id * task::base_config::j_stat_amount + idx) + ".txt";
        auto j_out = outputer.createFile(j_filename);
        j_out_vec.push_back(std::move(j_out));
        j_out_vec[idx].printLn("j_up", "j_down");

        const std::string Nup_filename = "Nup_id="
            + std::to_string(config.stat_id * task::base_config::j_stat_amount + idx) + ".txt";
        auto Nup_out = outputer.createFile(Nup_filename);
        Nup_out_vec.push_back(std::move(Nup_out));
        Nup_out_vec[idx].printLn("N_up_1", "N_up_2");

        const std::string Ndown_filename = "Ndown_id="
            + std::to_string(config.stat_id * task::base_config::j_stat_amount + idx) + ".txt";
        auto Ndown_out = outputer.createFile(Ndown_filename);
        Ndown_out_vec.push_back(std::move(Ndown_out));
        Ndown_out_vec[idx].printLn("N_down_1", "N_down_2");
    }

    const auto start_timepoint = std::chrono::steady_clock::now();

    auto sample = task::createSample(config);
    const auto init_mcs_amount = task::prepare(sample);
    auto info_out
        = outputer.createFile("info_id=" + std::to_string(config.stat_id) + ".txt");
    info_out.printLn(
        "Initialization stage duration : ", std::to_string(init_mcs_amount), "MCS/s");

    std::valarray<double> j_up_arr(task::base_config::j_stat_amount);
    std::valarray<double> j_down_arr(task::base_config::j_stat_amount);
    base_config::spin_t::magn_t magn_fst_average{};
    base_config::spin_t::magn_t magn_snd_average{};
    constexpr auto mcs_amount = base_config::mcs_observation + base_config::t_wait_vec.back();

    {
        auto m_dynamic_out
            = outputer.createFile("m_dynamic_id=" + std::to_string(config.stat_id) + ".txt");
        m_dynamic_out.printLn("m1", "m1x", "m1y", "m1z", "m2", "m2x", "m2y", "m2z");
        auto theta_dynamic_out = outputer.createFile(
            "cos_theta_dynamic_id=" + std::to_string(config.stat_id) + ".txt");
        theta_dynamic_out.printLn("cos_theta");
        auto thetaXZ_dynamic_out = outputer.createFile(
            "cos_thetaXZ_dynamic_id=" + std::to_string(config.stat_id) + ".txt");
        thetaXZ_dynamic_out.printLn("cos_thetaXZ");

        constexpr auto queue_size = 500u;
        std::queue<std::array<base_config::spin_t::magn_t, 2u>> queue{};
        for (auto mcs = 0u; mcs < queue_size; ++mcs) {
            const auto magns = sample.makeMonteCarloStep();
            m_dynamic_out.printLn(abs(magns[0]), magns[0], abs(magns[1]), magns[1]);

            const auto cos_theta = cos_of_angle(magns[0], magns[1]);
            theta_dynamic_out.printLn(cos_theta);

            const auto cos_thetaXZ = cos_of_angle(
                task::base_config::magn_t{magns[0].x, 0.0, magns[0].z},
                task::base_config::magn_t{magns[1].x, 0.0, magns[1].z});
            thetaXZ_dynamic_out.printLn(cos_thetaXZ);

            magn_fst_average += magns[0];
            magn_snd_average += magns[1];
            queue.push(std::move(magns));
        }
        const auto size_as_double = static_cast<double>(queue.size());
        magn_fst_average /= size_as_double;
        magn_snd_average /= size_as_double;

        constexpr auto eps = 1e-2; // 20.0 / (base_config::L * base_config::L * config.N);

        auto mcs_on_dynamic_stage = queue_size;
        for (auto mcs = 0u; mcs < 10'000; ++mcs) {
            const auto [magn1, magn2] = sample.makeMonteCarloStep();
            m_dynamic_out.printLn(abs(magn1), magn1, abs(magn2), magn2);

            const auto elem_to_pop = queue.front();
            magn_fst_average -= elem_to_pop[0] / size_as_double;
            magn_snd_average -= elem_to_pop[1] / size_as_double;

            magn_fst_average += magn1 / size_as_double;
            magn_snd_average += magn2 / size_as_double;

            queue.pop();
            queue.push({magn1, magn2});

            mcs_on_dynamic_stage ++;
            if (is_almost_equals(magn_fst_average, magn1, eps)
                && is_almost_equals(magn_snd_average, magn2, eps)) {
                break;
            }
        };

        info_out.printLn(
            "Dynamic stage duration : ", std::to_string(mcs_on_dynamic_stage), "MCS/s");
    }

    const auto first_timepoint = std::chrono::steady_clock::now();
    const auto initialization_time = std::chrono::duration_cast<std::chrono::hours>(first_timepoint - start_timepoint);

    for (auto mcs = 0u; mcs < mcs_amount; ++mcs) {
        if (mcs == base_config::t_wait_vec.front()) {
            const auto [up, down] = sample.startObservation();
            j_up_arr += up;
            j_down_arr += down;
            for (auto idx = 0u; idx < j_out_vec.size(); ++idx) {
                j_out_vec[idx].printLn(j_up_arr[idx], j_down_arr[idx]);
            }
            const auto& Nup_arr = sample.N_up_values_arr;
            for (auto film_idx = 0u; film_idx < 2u; ++film_idx) {
                for (auto idx = 0u; idx < Nup_out_vec.size(); ++idx) {
                    const auto elem = Nup_arr[film_idx][idx];
                    Nup_out_vec[idx].print(elem);
                }
            }
            std::for_each(
                Nup_out_vec.begin(), Nup_out_vec.end(), [](outputer_t::output_file_t& out) {
                    out.printLn();
                });
            const auto& Ndown_arr = sample.N_down_values_arr;
            for (auto film_idx = 0u; film_idx < 2u; ++film_idx) {
                for (auto idx = 0u; idx < Ndown_out_vec.size(); ++idx) {
                    const auto elem = Ndown_arr[film_idx][idx];
                    Ndown_out_vec[idx].print(elem);
                }
            }
            std::for_each(
                Ndown_out_vec.begin(), Ndown_out_vec.end(), [](outputer_t::output_file_t& out) {
                    out.printLn();
                });
        }
        if (mcs > base_config::t_wait_vec.front()) {
            const auto [up, down] = sample.makeJCalc();
            j_up_arr += up;
            j_down_arr += down;
            for (auto idx = 0u; idx < j_out_vec.size(); ++idx) {
                j_out_vec[idx].printLn(j_up_arr[idx], j_down_arr[idx]);
            }

            const auto& Nup_arr = sample.N_up_values_arr;
            for (auto film_idx = 0u; film_idx < sample.lattice.nanostructure.size(); ++film_idx) {
                for (auto idx = 0u; idx < Nup_out_vec.size(); ++idx) {
                    const auto elem = Nup_arr[film_idx][idx];
                    Nup_out_vec[idx].print(elem);
                }
            }
            std::for_each(
                Nup_out_vec.begin(), Nup_out_vec.end(), [](outputer_t::output_file_t& out) {
                    out.printLn();
                });

            const auto& Ndown_arr = sample.N_down_values_arr;
            for (auto film_idx = 0u; film_idx < sample.lattice.nanostructure.size(); ++film_idx) {
                for (auto idx = 0u; idx < Ndown_out_vec.size(); ++idx) {
                    const auto elem = Ndown_arr[film_idx][idx];
                    Ndown_out_vec[idx].print(elem);
                }
            }
            std::for_each(
                Ndown_out_vec.begin(), Ndown_out_vec.end(), [](outputer_t::output_file_t& out) {
                    out.printLn();
                });
        }

        const auto [magn1, magn2] = sample.makeMonteCarloStep();
        m_out.printLn(abs(magn1), magn1, abs(magn2), magn2);

        const auto cos_theta = cos_of_angle(magn1, magn2);
        theta_out.printLn(cos_theta);

        const auto cos_thetaXZ = cos_of_angle(
            task::base_config::magn_t{magn1.x, 0.0, magn1.z},
            task::base_config::magn_t{magn2.x, 0.0, magn2.z});
        thetaXZ_out.printLn(cos_thetaXZ);
    }

    const auto end_timepoint = std::chrono::steady_clock::now();
    const auto observation_time = std::chrono::duration_cast<std::chrono::hours>(first_timepoint - start_timepoint);
    const auto full_calculation_time = std::chrono::duration_cast<std::chrono::hours>(end_timepoint - start_timepoint);

    auto timepoints_out = outputer.createFile("time_id=" + std::to_string(config.stat_id) + ".txt");
    timepoints_out.printLn("initialization, h", "observation, h", "full, h");
    timepoints_out.printLn(initialization_time.count(), observation_time.count(), full_calculation_time.count());

    return config;
}

} // namespace task

#endif