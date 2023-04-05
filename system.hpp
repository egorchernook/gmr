#ifndef SYSTEM_HPP_INCLUDED
#define SYSTEM_HPP_INCLUDED

#include <algorithm>
#include <array>
#include <functional>
#include <type_traits>
#include <utility>
#include <valarray>

#include "config.hpp"

namespace task
{
struct sample_t
{
    using lattice_t = qss::multilayer_system<qss::multilayer<typename base_config::lattice_t>>;
    using n_lattice_t = qss::multilayer<typename base_config::electron_dencity_t>;

    lattice_t lattice;
    std::vector<n_lattice_t> n_up_vec;
    std::vector<n_lattice_t> n_down_vec;

    std::array<std::valarray<double>, task::base_config::j_stat_amount> N_up_values_arr{};
    std::array<std::valarray<double>, task::base_config::j_stat_amount> N_down_values_arr{};

    std::vector<std::size_t> volumes{};

    std::vector<
        qss::spin_transport::nanostructure_type<qss::lattices::three_d::fcc, typename qss::spin_transport::proxy_spin>>
        proxy_lattice_arr;

    const base_config::config_t config;
    const typename decltype(std::function{base_config::createHamilton_f})::result_type hamilt;

    sample_t(lattice_t &&lattice_, n_lattice_t &&n_up_, n_lattice_t &&n_down_, const base_config::config_t &config_)
        : lattice{lattice_}, config{config_}, hamilt{base_config::createHamilton_f(config.field,
                                                                                   base_config::getDelta(config.N))}
    {
        n_up_vec.reserve(task::base_config::j_stat_amount);
        n_up_vec.push_back(n_up_);
        n_down_vec.reserve(task::base_config::j_stat_amount);
        n_down_vec.push_back(n_down_);
        for (auto idx = 1u; idx < task::base_config::j_stat_amount; ++idx)
        {
            n_up_vec.push_back(n_up_vec[0]);
            n_down_vec.push_back(n_down_vec[0]);
        }

        volumes.reserve(lattice.nanostructure.size());
        for (auto &film : lattice.nanostructure)
        {
            volumes.push_back(film.get_amount_of_nodes());
        }

        {
            std::valarray<double> values(n_up_vec.size());
            std::valarray<double> sizes(n_up_vec.size());
            for (auto idx = 0u; auto &film : n_up_vec[0])
            {
                sizes[idx] = static_cast<double>(film.get_amount_of_nodes());
                double value = 0.0;
                for (auto &elem : film)
                {
                    value += static_cast<double>(elem);
                }
                values[idx] = value;
                idx++;
            }
            const auto Nup = values / sizes;
            N_up_values_arr.fill(Nup);
        }
        {
            std::valarray<double> values(n_down_vec.size());
            std::valarray<double> sizes(n_down_vec.size());
            for (auto idx = 0u; auto &film : n_down_vec[0])
            {
                sizes[idx] = static_cast<double>(film.get_amount_of_nodes());
                double value = 0.0;
                for (auto &elem : film)
                {
                    value += static_cast<double>(elem);
                }
                values[idx] = value;
                idx++;
            }
            const auto Ndown = values / sizes;
            N_down_values_arr.fill(Ndown);
        }

        proxy_lattice_arr.reserve(task::base_config::j_stat_amount);
        for (auto idx = 0u; idx < task::base_config::j_stat_amount; ++idx)
        {
            proxy_lattice_arr.push_back(
                qss::algorithms::spin_transport::prepare_proxy_structure(lattice, n_up_vec[idx], n_down_vec[idx], 'x'));
        }
    }

    std::array<typename base_config::spin_t::magn_t, 2> makeMonteCarloStep()
    {
        lattice.T = config.T_sample;
        lattice.evolve(hamilt);
        const auto magn1 = lattice.magns[0];
        const auto magn2 = lattice.magns[1];

        return {magn1, magn2};
    }
    std::array<std::valarray<double>, 2> startObservation()
    {
        std::for_each(proxy_lattice_arr.begin(), proxy_lattice_arr.end(),
                      [this](auto &proxy_lattice) { proxy_lattice.T = config.T_sample; });
        const auto temp_magn1 = abs(lattice.magns[0]);
        const auto temp_magn2 = -abs(lattice.magns[1]);

        const typename base_config::ed_t n_up_value{0.5 * (1.0 + temp_magn1)};
        const typename base_config::ed_t n_down_value{0.5 * (1.0 - temp_magn2)};

        for (auto idx = 0u; auto &n_up : n_up_vec)
        {
            for (auto idx_film = 0u; auto &film : n_up)
            {
                film.fill(n_up_value);
                const auto film_volume = static_cast<double>(film.get_amount_of_nodes());
                N_up_values_arr[idx][idx_film] = n_up_value / film_volume;
                idx_film++;
            }
            idx++;
        };
        for (auto idx = 0u; auto &n_down : n_down_vec)
        {
            for (auto idx_film = 0u; auto &film : n_down)
            {
                film.fill(n_down_value);
                const auto film_volume = static_cast<double>(film.get_amount_of_nodes());
                N_down_values_arr[idx][idx_film] = n_down_value / film_volume;
                idx_film++;
            }
            idx++;
        };

        std::valarray<double> j_up_arr(task::base_config::j_stat_amount);
        std::valarray<double> j_down_arr(task::base_config::j_stat_amount);
        for (auto idx = 0u; auto &proxy_lattice : proxy_lattice_arr)
        {
            const auto [j_up, j_down] = qss::algorithms::spin_transport::perform(proxy_lattice);
            j_up_arr[idx] = j_up;
            j_down_arr[idx] = j_down;
        }
        constexpr auto area = base_config::L * base_config::L;
        return {j_up_arr / area, j_down_arr / area};
    }
    std::array<std::valarray<double>, 2> makeJCalc()
    {
        const auto temp_magn1 = abs(lattice.magns[0]);
        const auto temp_magn2 = -abs(lattice.magns[1]);

        const typename base_config::ed_t n_up_value{0.5 * (1.0 + temp_magn1)};
        const typename base_config::ed_t n_down_value{0.5 * (1.0 - temp_magn2)};

        for (auto idx = 0u; auto &n_up : n_up_vec)
        {
            const auto [old, amount] = n_up[0].fill_plane(0, n_up_value);
            N_up_values_arr[idx][0] -= old / amount;
            N_up_values_arr[idx][0] += n_up_value;
            idx++;
        }
        for (auto idx = 0u; auto &n_down : n_down_vec)
        {
            const auto [old, amount] = n_down[0].fill_plane(0, n_down_value);
            N_down_values_arr[idx][0] -= old / amount;
            N_down_values_arr[idx][0] += n_down_value;
            idx++;
        }

        std::valarray<double> j_up_arr(task::base_config::j_stat_amount);
        std::valarray<double> j_down_arr(task::base_config::j_stat_amount);
        for (auto idx = 0u; auto &proxy_lattice : proxy_lattice_arr)
        {
            const auto [j_up, j_down] = qss::algorithms::spin_transport::perform(proxy_lattice);
            j_up_arr[idx] = j_up;
            j_down_arr[idx] = j_down;
        }
        constexpr auto area = base_config::L * base_config::L;
        return {j_up_arr / area, j_down_arr / area};
    }
};

// создаёт решётку при температуре T = 0
inline sample_t createSample(const base_config::config_t &config) noexcept
{
    const typename base_config::sizes_t sizes{base_config::L, base_config::L, config.N};
    const typename base_config::spin_t plus{1.0, 0.0, 0.0};
    const typename base_config::spin_t minus{-1.0, 0.0, 0.0};

    const typename base_config::lattice_t fst{plus, sizes};
    const typename base_config::lattice_t snd{minus, sizes};

    const qss::film<typename base_config::lattice_t> fst_film{fst, 1.0};
    const qss::film<typename base_config::lattice_t> snd_film{snd, 1.0};

    const typename base_config::ed_t n_0{0.0};
    const typename base_config::electron_dencity_t n_film{n_0, sizes};

    return sample_t{typename sample_t::lattice_t{
                        qss::multilayer<typename base_config::lattice_t>{{fst_film, snd_film}, {base_config::J2}}},
                    typename sample_t::n_lattice_t{{n_film, n_film}, {base_config::J2}},
                    typename sample_t::n_lattice_t{{n_film, n_film}, {base_config::J2}}, config};
}

// готовит образец до температуры T_creation ровно base_config::mcs_init шагов
inline void prepare(sample_t &sam)
{
    const auto config = sam.config;
    sam.lattice.T = config.T_creation;
    const auto N = config.N;
    const auto Delta = base_config::getDelta(N);
    auto hamilt = base_config::createHamilton_f(config.field, Delta);
    for (auto _ = 0u; _ < base_config::mcs_init; ++_)
    {
        sam.lattice.evolve(hamilt);
    }
}
} // namespace task

#endif