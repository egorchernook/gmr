#ifndef BASE_SYSTEM_CONFIGURATION_HPP_INCLUDED
#define BASE_SYSTEM_CONFIGURATION_HPP_INCLUDED

#include <cstdint>
#include <array>
#include <vector>
#include <string_view>
#include <string>

#include "qss/src/algorithms/Metropolis.hpp"
#include "qss/src/models/heisenberg.hpp"
#include "qss/src/lattices/3d/fcc.hpp"
#include "qss/src/lattices/3d/3d.hpp"
#include "qss/src/lattices/borders_conditions.hpp"
#include "qss/src/utility/quantities.hpp"
#include "qss/src/utility/functions.hpp"
#include "qss/src/systems/multilayer.hpp"
#include "qss/src/systems/multilayer_system.hpp"
#include "qss/src/algorithms/spin_transport.hpp"
#include "qss/src/models/electron_dencity.hpp"

namespace task
{
    struct config_t;
    struct base_config
    {
        using spin_t = qss::heisenberg::spin;
        using lattice_t = qss::lattices::three_d::fcc<spin_t>;
        using sizes_t = qss::lattices::three_d::sizes_t;
        using ed_t = qss::electron_dencity;
        using electron_dencity_t = qss::lattices::three_d::fcc<ed_t>;

        base_config() = delete;
        // линейный размер
        constexpr static std::uint16_t L = 64;
        // обменный интеграл взаимодействия плёнок
        constexpr static double J2 = -0.3;
        constexpr static std::array<double, 10> deltas{
            0.5,
            0.6,
            0.636,
            0.7,
            0.734,
            0.77,
            0.816,
            0.85,
            0.882,
            0.9};
        constexpr static double getDelta(std::uint16_t N)
        {
            return deltas[--N];
        }
        // количество статистических прогонок
        constexpr static std::uint16_t stat_amount = 5;
        constexpr static std::uint64_t mcs_init = 1'000;
        constexpr static std::uint64_t mcs_observation = 5'000;
        constexpr static std::array<std::uint8_t, 2> N_size_vec{3, 5};
        constexpr static std::array<double, 1> T_creation_vec{0.67};
        constexpr static std::array<double, 1> T_sample_vec{0.95};
        constexpr static std::array<std::uint32_t, 4>
            t_wait_vec{100, 200, 400, 1000}; // должен быть отсортирован по увеличению
        constexpr static std::array<typename spin_t::magn_t, 5>
            magn_field_vec{typename spin_t::magn_t{0.0, 0.0, 0.0},
                           typename spin_t::magn_t{0.0, 0.0, 0.1},
                           typename spin_t::magn_t{0.0, 0.0, 0.5},
                           typename spin_t::magn_t{0.0, 0.0, 1.0},
                           typename spin_t::magn_t{0.0, 0.0, 5.0}};

        constexpr static auto
        createHamilton_f(
            const typename spin_t::magn_t &h,
            double Delta)
        {
            return [&h, Delta](const typename spin_t::magn_t &sum,
                               const spin_t &spin_old,
                               const spin_t &spin_new) noexcept -> double
            {
                const auto sum_with_field = sum + h;
                auto diff = spin_old - spin_new;
                diff.y *= 0.8;
                diff.z *= (1.0 - Delta);
                return scalar_multiply(sum_with_field, diff);
            };
        };

        static std::vector<config_t> getConfigs()
        {
            std::vector<config_t> result{};
            result.reserve(stat_amount *
                           N_size_vec.size() *
                           T_creation_vec.size() *
                           T_sample_vec.size() *
                           magn_field_vec.size());
            for (const auto &N : N_size_vec)
            {
                for (auto id = 0u; id < stat_amount; ++id)
                {
                    for (const auto &T_creation : T_creation_vec)
                    {
                        for (const auto &T_sample : T_sample_vec)
                        {
                            for (const auto &field : magn_field_vec)
                            {
                                result.emplace_back(id, N,
                                                    T_creation,
                                                    T_sample,
                                                    field);
                            }
                        }
                    }
                }
            }
            return result;
        }
    };

    struct config_t
    {
        const std::uint16_t stat_id;
        const std::uint8_t N;
        const double T_creation;
        const double T_sample;
        const typename base_config::spin_t::magn_t field;
    };

    inline std::string createName(const config_t &config) noexcept
    {
        using std::to_string;
        std::ostringstream stream{};
        stream << "N = " << to_string(config.N)
               << "/T_creation = " << config.T_creation
               << "/T_sample = " << config.T_sample
               << "/h = " << to_string(config.field);
        return stream.str();
    }

    inline std::ostream &operator<<(std::ostream &out, const config_t &data) noexcept
    {
        out << "[ "
            << "stat_id = " << data.stat_id << "; "
            << "N = " << data.N << "; "
            << "T_creation = " << data.T_creation << "; "
            << "T_sample = " << data.T_sample << "; "
            << "field = " << data.field << "; "
            << "]";
        return out;
    }
}

#endif