#ifndef SYSTEM_HPP_INCLUDED
#define SYSTEM_HPP_INCLUDED

#include "config.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <queue>
#include <type_traits>
#include <utility>
#include <valarray>

namespace task {
struct sample_t {
    using lattice_t = qss::multilayer_system<qss::multilayer<typename base_config::lattice_t>>;
    using n_lattice_t = qss::multilayer<typename base_config::electron_dencity_t>;

    lattice_t lattice;
    std::vector<n_lattice_t> n_up_vec;
    std::vector<n_lattice_t> n_down_vec;

    std::array<std::valarray<double>, 2> N_up_values_arr{};
    std::array<std::valarray<double>, 2> N_down_values_arr{};

    std::vector<qss::spin_transport::nanostructure_type<
        qss::lattices::three_d::fcc,
        typename qss::spin_transport::proxy_spin>>
        proxy_lattice_arr;

    const base_config::config_t config;
    const typename decltype(std::function{base_config::createHamilton_f})::result_type hamilt;

    sample_t(
        lattice_t&& lattice_,
        n_lattice_t&& n_up_,
        n_lattice_t&& n_down_,
        const base_config::config_t& config_)
        : lattice{lattice_}
        , config{config_}
        , hamilt{base_config::createHamilton_f(config.field, base_config::getDelta(config.N))}
    {
        n_up_vec.reserve(task::base_config::j_stat_amount);
        n_up_vec.push_back(std::move(n_up_));
        n_down_vec.reserve(task::base_config::j_stat_amount);
        n_down_vec.push_back(std::move(n_down_));
        for (auto idx = 1u; idx < task::base_config::j_stat_amount; ++idx) {
            n_up_vec.push_back(n_up_vec[0]);
            n_down_vec.push_back(n_down_vec[0]);
        }
        {
            std::valarray<double> values(n_up_vec.size());
            std::valarray<double> sizes(n_up_vec.size());
            {
                auto idx = 0u;
                for (auto& film : n_up_vec[0]) {
                    sizes[idx] = static_cast<double>(film.get_amount_of_nodes());
                    double value = 0.0;
                    for (auto& elem : film) {
                        value += static_cast<double>(elem);
                    }
                    values[idx] = value;
                    idx++;
                }
                const auto Nup = values / sizes;
                N_up_values_arr.fill(Nup);
            }
        }
        {
            std::valarray<double> values(n_down_vec.size());
            std::valarray<double> sizes(n_down_vec.size());
            {
                auto idx = 0u;
                for (auto& film : n_down_vec[0]) {
                    sizes[idx] = static_cast<double>(film.get_amount_of_nodes());
                    double value = 0.0;
                    for (auto& elem : film) {
                        value += static_cast<double>(elem);
                    }
                    values[idx] = value;
                    idx++;
                }
                const auto Ndown = values / sizes;
                N_down_values_arr.fill(Ndown);
            }
        }

        proxy_lattice_arr.reserve(task::base_config::j_stat_amount);
        for (auto idx = 0u; idx < task::base_config::j_stat_amount; ++idx) {
            proxy_lattice_arr.push_back(qss::algorithms::spin_transport::prepare_proxy_structure(
                lattice, n_up_vec[idx], n_down_vec[idx], 'x'));
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
        std::for_each(
            proxy_lattice_arr.begin(), proxy_lattice_arr.end(), [this](auto& proxy_lattice) {
                proxy_lattice.T = config.T_sample;
            });
        // const auto temp_magn1 = std::abs(lattice.magns[0].x * lattice.magns[0].x +
        // lattice.magns[0].y * lattice.magns[0].x); const auto temp_magn2 =
        // -std::abs(lattice.magns[1].x * lattice.magns[1].x + lattice.magns[1].y *
        // lattice.magns[1].x);
        const auto temp_magn1 = lattice.magns[0].x;
        const auto temp_magn2 = lattice.magns[1].x;

        const typename base_config::ed_t n_up_value{0.5 * (1.0 + temp_magn1)};
        const typename base_config::ed_t n_down_value{0.5 * (1.0 - temp_magn2)};

        {
            auto idx = 0u;
            for (auto& n_up : n_up_vec) {
                auto idx_film = 0u;
                for (auto& film : n_up) {
                    film.fill(n_up_value);
                    const auto film_volume = static_cast<double>(film.get_amount_of_nodes());
                    N_up_values_arr[idx_film][idx] = n_up_value / film_volume;
                    idx_film++;
                }
                idx++;
            }
        }
        {
            auto idx = 0u;
            for (auto& n_down : n_down_vec) {
                auto idx_film = 0u;
                for (auto& film : n_down) {
                    film.fill(n_down_value);
                    const auto film_volume = static_cast<double>(film.get_amount_of_nodes());
                    N_down_values_arr[idx_film][idx] = n_down_value / film_volume;
                    idx_film++;
                }
                idx++;
            }
        }

        std::valarray<double> j_up_arr(task::base_config::j_stat_amount);
        std::valarray<double> j_down_arr(task::base_config::j_stat_amount);
        {
            auto idx = 0u;
            for (auto& proxy_lattice : proxy_lattice_arr) {
                const auto [j_up, j_down] = qss::algorithms::spin_transport::perform(proxy_lattice);
                j_up_arr[idx] = j_up;
                j_down_arr[idx] = j_down;
                idx++;
            }
        }
        constexpr auto area = base_config::L * base_config::L;
        return {j_up_arr / area, j_down_arr / area};
    }
    std::array<std::valarray<double>, 2> makeJCalc()
    {
        // const auto temp_magn1 = std::abs(lattice.magns[0].x * lattice.magns[0].x +
        // lattice.magns[0].y * lattice.magns[0].y); const auto temp_magn2 =
        // -std::abs(lattice.magns[1].x * lattice.magns[1].x + lattice.magns[1].y *
        // lattice.magns[1].y);
        const auto temp_magn1 = lattice.magns[0].x;
        const auto temp_magn2 = lattice.magns[1].x;

        const typename base_config::ed_t n_up_value{0.5 * (1.0 + temp_magn1)};
        const typename base_config::ed_t n_down_value{0.5 * (1.0 - temp_magn2)};

        {
            auto idx = 0u;
            for (auto& n_up : n_up_vec) {
                n_up[0].fill_plane(0, n_up_value);
                idx++;
            }
        }
        {
            auto idx = 0u;
            for (auto& n_down : n_down_vec) {
                n_down[0].fill_plane(0, n_down_value);
                idx++;
            }
        }
        std::valarray<double> j_up_arr(task::base_config::j_stat_amount);
        std::valarray<double> j_down_arr(task::base_config::j_stat_amount);
        {
            auto idx = 0u;
            for (auto& proxy_lattice : proxy_lattice_arr) {
                const auto [j_up, j_down] = qss::algorithms::spin_transport::perform(proxy_lattice);
                j_up_arr[idx] = j_up;
                j_down_arr[idx] = j_down;
                idx++;
            }
        }
        {
            auto idx = 0u;
            for (auto& proxy_lattice : proxy_lattice_arr) {
                for (auto& elem : N_up_values_arr) {
                    elem[idx] = 0.0;
                }
                for (auto& elem : N_down_values_arr) {
                    elem[idx] = 0.0;
                }
                auto film_id = 0u;
                for (auto& film : proxy_lattice.nanostructure) {
                    const auto volume = static_cast<double>(film.get_amount_of_nodes());
                    for (auto& atom : film) {
                        N_up_values_arr[film_id][idx] += atom.get_up() / volume;
                        N_down_values_arr[film_id][idx] += atom.get_down() / volume;
                    }
                    film_id++;
                }
                idx++;
            }
        }
        constexpr auto area = base_config::L * base_config::L;
        return {j_up_arr / area, j_down_arr / area};
    }
};

// создаёт решётку при температуре T = 0
inline sample_t createSample(const base_config::config_t& config) noexcept
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

    return sample_t{
        typename sample_t::lattice_t{qss::multilayer<typename base_config::lattice_t>{
            {fst_film, snd_film}, {base_config::J2}}},
        typename sample_t::n_lattice_t{{n_film, n_film}, {base_config::J2}},
        typename sample_t::n_lattice_t{{n_film, n_film}, {base_config::J2}},
        config};
}

// готовит образец до температуры T_creation ровно base_config::mcs_init шагов
inline std::uint64_t prepare(sample_t& sam)
{
    const auto config = sam.config;
    sam.lattice.T = config.T_creation;
    const auto N = config.N;
    const auto Delta = base_config::getDelta(N);
    auto hamilt = base_config::createHamilton_f(config.field, Delta);

    base_config::spin_t::magn_t magn_fst_average{};
    base_config::spin_t::magn_t magn_snd_average{};

    constexpr auto queue_size = base_config::mcs_init / 2;
    std::queue<std::array<base_config::spin_t::magn_t, 2u>> queue{};
    for (auto _ = 0u; _ < base_config::mcs_init / 2; ++_) {
        sam.lattice.evolve(hamilt);
    }
    for (auto _ = 0u; _ < base_config::mcs_init / 2; ++_) {
        sam.lattice.evolve(hamilt);
        const auto magns = sam.lattice.magns;
        magn_fst_average += magns[0];
        magn_snd_average += magns[1];
        queue.push({magns[0], magns[1]});
    }
    const auto size_as_double = static_cast<double>(queue_size);
    magn_fst_average /= size_as_double;
    magn_snd_average /= size_as_double;

    constexpr auto eps = 1e-2; // 20.0 / (base_config::L * base_config::L * config.N);
    auto mcs_to_init = base_config::mcs_init;
    for (auto mcs = 0u; mcs < 10'000; ++mcs) {
        sam.lattice.evolve(hamilt);
        const auto magn1 = sam.lattice.magns[0];
        const auto magn2 = sam.lattice.magns[1];

        const auto elem_to_pop = queue.front();
        magn_fst_average -= elem_to_pop[0] / size_as_double;
        magn_snd_average -= elem_to_pop[1] / size_as_double;

        magn_fst_average += magn1 / size_as_double;
        magn_snd_average += magn2 / size_as_double;

        queue.pop();
        queue.push({magn1, magn2});

        mcs_to_init += mcs;
        if (is_almost_equals(magn_fst_average, magn1, eps)
            && is_almost_equals(magn_snd_average, magn2, eps)) {
            break;
        }
    };

    return mcs_to_init;
}
} // namespace task

#endif