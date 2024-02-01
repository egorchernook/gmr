#ifndef BASE_SYSTEM_CONFIGURATION_HPP_INCLUDED
#define BASE_SYSTEM_CONFIGURATION_HPP_INCLUDED

#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "algorithms/Metropolis.hpp"
#include "algorithms/spin_transport.hpp"
#include "lattices/3d/3d.hpp"
#include "lattices/3d/fcc.hpp"
#include "lattices/borders_conditions.hpp"
#include "models/electron_dencity.hpp"
#include "models/heisenberg.hpp"
#include "systems/multilayer.hpp"
#include "systems/multilayer_system.hpp"
#include "utility/functions.hpp"
#include "utility/quantities.hpp"

namespace task
{
struct base_config
{
    using spin_t = qss::heisenberg::spin;
    using magn_t = spin_t::magn_t;
    using lattice_t = qss::lattices::three_d::fcc<spin_t>;
    using sizes_t = qss::lattices::three_d::sizes_t;
    using ed_t = qss::electron_dencity;
    using electron_dencity_t = qss::lattices::three_d::fcc<ed_t>;

    struct config_t
    {
        const std::uint16_t stat_id;
        const std::uint8_t N;
        const double T_creation;
        const double T_sample;
        const magn_t field;

        config_t(std::uint16_t stat_id_, std::uint8_t N_, double T_creation_, double T_sample_, magn_t field_)
            : stat_id{stat_id_}, N{N_}, T_creation{T_creation_}, T_sample{T_sample_}, field{field_}
        {
        }
    };

    base_config() = delete;
    // линейный размер
    constexpr static std::uint16_t L = 32;
    // обменный интеграл взаимодействия плёнок
    constexpr static double J2 = -0.05;
    constexpr static std::array<double, 10> deltas{0.5, 0.6, 0.636, 0.7, 0.734, 0.77, 0.816, 0.85, 0.882, 0.9};
    constexpr static double getDelta(std::uint16_t N)
    {
        return deltas[--N];
    }
    constexpr static std::uint16_t m_stat_amount = 2; // количество статистических прогонок
    constexpr static std::uint16_t j_stat_amount = 3;
    constexpr static std::uint64_t mcs_init = 500;
    constexpr static std::uint64_t mcs_observation = 1'000;
    constexpr static std::array N_size_vec{3u};
    constexpr static std::array T_creation_vec{0.67};
    constexpr static std::array T_sample_vec{0.95};
    constexpr static std::array t_wait_vec{100u, 200u}; //, 400u, 1000u}; // должен быть отсортирован по увеличению
    constexpr static std::array magn_field_vec{
        magn_t{0.0, 0.0, 0.0}, magn_t{0.5, 0.0, 0.0},
        // magn_t{0.6, 0.0, 0.0}, //magn_t{0.65, 0.0, 0.0},
        // magn_t{0.7, 0.0, 0.0}, //magn_t{0.75, 0.0, 0.0},
        // magn_t{0.8, 0.0, 0.0}, //magn_t{0.85, 0.0, 0.0},
        // magn_t{0.9, 0.0, 0.0}, //magn_t{0.95, 0.0, 0.0},
        magn_t{1.0, 0.0, 0.0}, magn_t{2.0, 0.0, 0.0}};

    constexpr static double A_fb = 0.01;

    constexpr static auto createHamilton_f(const magn_t &h, double Delta)
    {
        return [&h, Delta](const magn_t &sum, const spin_t &spin_old,
                           const spin_t &spin_new) noexcept -> double {
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
        result.reserve(m_stat_amount * N_size_vec.size() * T_creation_vec.size() * T_sample_vec.size() *
                       magn_field_vec.size());
        for (const auto &N : N_size_vec)
        {
            for (auto id = 0u; id < m_stat_amount; ++id)
            {
                for (const auto &T_creation : T_creation_vec)
                {
                    for (const auto &T_sample : T_sample_vec)
                    {
                        for (const auto &field : magn_field_vec)
                        {
                            result.emplace_back(id, N, T_creation, T_sample, field);
                        }
                    }
                }
            }
        }
        return result;
    }
};

template <typename Iter> inline std::string values_as_string(Iter first, Iter last)
{
    using std::to_string;
    std::string res{};
    while (first != last)
    {
        res += to_string(*first);
        first++;
        res += first != last ? "," : "";
    }
    return res;
}

inline std::string create_config_info()
{
    std::ostringstream stream{};
    stream << "base_config : \n[\n\t m_stat_amount : " << base_config::m_stat_amount
           << "\n\t j_stat_amount : " << base_config::j_stat_amount << "\n\t mcs_init : " << base_config::mcs_init
           << "\n\t mcs_observation : " << base_config::mcs_observation << "\n\t N_size_vec : ["
           << values_as_string(base_config::N_size_vec.begin(), base_config::N_size_vec.end())
           << "]\n\t T_creation_vec : ["
           << values_as_string(base_config::T_creation_vec.begin(), base_config::T_creation_vec.end())
           << "]\n\t T_sample_vec : ["
           << values_as_string(base_config::T_sample_vec.begin(), base_config::T_sample_vec.end())
           << "]\n\t t_wait_vec : [" << values_as_string(base_config::t_wait_vec.begin(), base_config::t_wait_vec.end())
           << "]\n\t magn_field_vec : ["
           << values_as_string(base_config::magn_field_vec.begin(), base_config::magn_field_vec.end()) << "]\n]";
    return stream.str();
}

inline std::string createName(const base_config::config_t &config) noexcept
{
    using std::to_string;
    std::ostringstream stream{};
    stream << "N = " << to_string(config.N) << "/T_creation = " << config.T_creation
           << "/T_sample = " << config.T_sample << "/h = " << to_string(config.field);
    return stream.str();
}

inline std::ostream &operator<<(std::ostream &out, const base_config::config_t &data) noexcept
{
    using std::to_string;
    out << "[ "
        << "stat_id = " << data.stat_id << "; "
        << "N = " << to_string(data.N) << "; "
        << "T_creation = " << data.T_creation << "; "
        << "T_sample = " << data.T_sample << "; "
        << "field = " << to_string(data.field) << "; "
        << "]";
    return out;
}
} // namespace task

#endif
