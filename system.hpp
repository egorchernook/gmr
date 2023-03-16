#ifndef SYSTEM_HPP_INCLUDED
#define SYSTEM_HPP_INCLUDED

#include <array>
#include <functional>
#include <type_traits>
#include <utility>

#include "config.hpp"

namespace task
{
struct sample_t
{
    using lattice_t = qss::multilayer_system<qss::multilayer<typename base_config::lattice_t>>;
    using n_lattice_t = qss::multilayer<typename base_config::electron_dencity_t>;

    lattice_t lattice;
    n_lattice_t n_up;
    n_lattice_t n_down;

    qss::spin_transport::nanostructure_type<qss::lattices::three_d::fcc, typename qss::spin_transport::proxy_spin>
        proxy_lattice;

    const base_config::config_t config;
    const typename decltype(std::function{base_config::createHamilton_f})::result_type hamilt;

    sample_t(lattice_t &&lattice_, n_lattice_t &&n_up_, n_lattice_t &&n_down_, const base_config::config_t &config_)
        : lattice{lattice_}, n_up{n_up_}, n_down{n_down_},
          proxy_lattice{qss::algorithms::spin_transport::prepare_proxy_structure<'x'>(lattice, n_up, n_down)},
          config{config_}, hamilt{base_config::createHamilton_f(config.field, base_config::getDelta(config.N))}
    {
    }

    std::array<typename base_config::spin_t::magn_t, 2> makeMonteCarloStep()
    {
        lattice.T = config.T_sample;
        lattice.evolve(hamilt);
        const auto magn1 = lattice.magns[0];
        const auto magn2 = lattice.magns[1];

        return {magn1, magn2};
    }
    std::array<double, 2> startObservation()
    {
        proxy_lattice.T = config.T_sample;
        const auto temp_magn1 = abs(lattice.magns[0]);
        const auto temp_magn2 = abs(lattice.magns[1]);

        const typename base_config::ed_t n_up_value{0.5 * (1.0 + temp_magn1)};
        const typename base_config::ed_t n_down_value{0.5 * (1.0 - temp_magn2)};

        n_up[0].fill(n_up_value);
        n_up[1].fill(n_up_value);
        n_down[0].fill(n_down_value);
        n_down[1].fill(n_down_value);

        const auto [j_up, j_down] = qss::algorithms::spin_transport::perform(proxy_lattice);
        constexpr auto area = base_config::L * base_config::L;
        return {j_up / area, j_down / area};
    }
    std::array<double, 2> makeJCalc()
    {
        const auto temp_magn1 = abs(lattice.magns[0]);
        const auto temp_magn2 = abs(lattice.magns[1]);

        const typename base_config::ed_t n_up_value{0.5 * (1.0 + temp_magn1)};
        const typename base_config::ed_t n_down_value{0.5 * (1.0 - temp_magn2)};

        n_up[0].fill_plane(0, n_up_value);
        n_down[0].fill_plane(0, n_down_value);
        const auto [j_up, j_down] = qss::algorithms::spin_transport::perform(proxy_lattice);
        constexpr auto area = base_config::L * base_config::L;
        return {j_up / area, j_down / area};
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

// готовит образец до температуры T_creation
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