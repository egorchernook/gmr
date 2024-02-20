#include "cxxopts.hpp"
#include "thread_pool.hpp"

#include <unistd.h>

void foo(unsigned int time)
{
    sleep(time);
    std::cout << "Hello from thread " << std::this_thread::get_id() << std::endl;
}

int main(int argc, char* argv[])
{
    cxxopts::Options options(
        "MRCalc", "Magnetiresistance Calculation programm, can be used to calculate GMR or TMR.");

    // clang-format off
    options.add_options()
        ("h,help", "Print help")
        ("t,threads", "Initial amount of parallel threads", cxxopts::value<uint>()->default_value("2"));
    // clang-format on

    auto initOpts = options.parse(argc, argv);

    const auto threads_amount = initOpts["threads"].as<uint>();
    thread_pool_t thread_pool{threads_amount};

    for (unsigned int i = 1; i < 5; ++i) {
        thread_pool.add_task(foo, i);
    }

    thread_pool.init();
}