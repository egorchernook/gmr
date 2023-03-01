#include <algorithm>
#include <array>
#include <cstdio>
#include <exception>
#include <execution>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#include "config.hpp"
#include "line_t.hpp"

namespace stat
{
struct stater
{
    constexpr static std::string_view stat_folder{"processed"};

    static void makeStat(std::filesystem::path path_to_result_folder, std::string_view raw_data_folder)
    {
        std::cout << "Stat calculations begins"
                  << "\n";
        prepare_folder(path_to_result_folder, raw_data_folder);

        auto init_configs = task::base_config::getConfigs();
        std::vector<task::base_config::config_t> configs{};
        for (auto &&elem : init_configs)
        {
            if (elem.stat_id == 0)
            {
                configs.push_back(std::move(elem));
            }
        }
        create_stat("m", configs, path_to_result_folder, raw_data_folder);
        create_stat("j", configs, path_to_result_folder, raw_data_folder);

        std::cout << "Stat calculations ends"
                  << "\n";
    }

    static void calcGMR(std::filesystem::path path_to_result_folder)
    {
        std::cout << "GMR calculations begins"
                  << "\n";

        auto init_configs = task::base_config::getConfigs();
        std::vector<task::base_config::config_t> configs{};
        for (auto &&elem : init_configs)
        {
            if (elem.stat_id == 0)
            {
                configs.push_back(std::move(elem));
            }
        }
        for (const auto &config : configs)
        {
            const auto folder_name = task::createName(config);
            std::filesystem::current_path(path_to_result_folder / stat_folder / folder_name);
            std::array j_file = {std::ifstream{std::filesystem::current_path()}};
            remove_heads(j_file.begin(), j_file.end());
            auto is_end = [&j_file]() noexcept -> bool {
                bool res = false;
                std::ranges::for_each(j_file.begin(), j_file.end(),
                                      [&res](auto &elem) noexcept -> void { res += elem.eof(); });
                return res;
            };
            std::array<double, task::base_config::t_wait_vec.size()> GMR_tw{};

            std::ofstream out{"GMR.txt"};

            out.flush();
            out.close();
        }
        std::cout << "GMR calculations ends"
                  << "\n";
    }

  private:
    static std::string create_file_name(const std::string &base_name, const std::size_t id)
    {
        return {base_name + "_id=" + std::to_string(id) + ".txt"};
    }
    static void create_stat(std::string &&name, const std::vector<task::base_config::config_t> &configs,
                            std::filesystem::path path_to_result_folder, std::string_view raw_data_folder)
    {
        std::array<std::ifstream, task::base_config::stat_amount> input_streams;
        for (const auto &config : configs)
        {
            namespace fs = std::filesystem;
            const auto folder_name = task::createName(config);
            const auto path_to_file = path_to_result_folder / raw_data_folder / folder_name;

            for (auto idx = 0u; idx < input_streams.size(); ++idx)
            {
                input_streams[idx] = std::ifstream(path_to_file / create_file_name(name, idx));
            }

            const auto init_head = remove_heads(input_streams.begin(), input_streams.end());

            auto is_end = [&input_streams]() noexcept -> bool {
                bool res = false;
                std::ranges::for_each(input_streams.begin(), input_streams.end(),
                                      [&res](auto &elem) noexcept -> void { res += elem.eof(); });
                return res;
            };

            fs::current_path(path_to_result_folder / stat_folder / folder_name);
            std::cout << "\t" << fs::current_path().string() << "/" << name << " file stat begins\n";
            std::ofstream out{name + ".txt"};
            {
                std::istringstream init_head_stream{init_head};
                for (std::string elem; std::getline(init_head_stream, elem, '\t');)
                {
                    out << elem << "\t\t";
                }
                out << "\n";
            }

            std::array<std::optional<line_t>, input_streams.size()> buf{};
            while (!is_end())
            {
                get_values_from_streams(input_streams.begin(), input_streams.end(), buf);

                line_t average{};
                auto amount = 0u;
                for (const auto &elem : buf)
                {
                    if (elem)
                    {
                        average += elem.value();
                        amount++;
                    }
                }
                average /= amount;
                line_t err{};
                for (const auto &elem : buf)
                {
                    if (elem)
                    {
                        err += stat::sqr(elem.value() - average);
                    }
                }
                err /= amount;

                for (auto idx = 0u; idx < average.size(); ++idx)
                {
                    out << average[idx] << "\t" << err[idx] << "\t";
                }
                out << "\n";
            }
            out.flush();
            out.close();

            std::cout << "\t" << fs::current_path().string() << "/" << name << " file stat ends\n";
        }
    }

    static void get_values_from_streams(std::array<std::ifstream, task::base_config::stat_amount>::iterator first,
                                        std::array<std::ifstream, task::base_config::stat_amount>::iterator last,
                                        std::array<std::optional<line_t>, task::base_config::stat_amount> &buf) noexcept
    {
        static line_t buf_line{};
        for (auto idx = 0u; first != last; ++first, ++idx)
        {
            if (!first->eof())
            {
                buf_line.clear();
                std::string line{};
                std::getline(*first, line);

                std::istringstream stream{line};
                for (std::string data; std::getline(stream, data, '\t');)
                {
                    double value{};
                    try
                    {
                        value = std::stod(data);
                    }
                    catch (std::out_of_range &exc)
                    {
                        value = std::numeric_limits<double>::max();
                        fprintf(stdout,
                                "The std::out_of_range[%s] appears when trying to make "
                                "std::stod on data from file with id = %d\n",
                                exc.what(), idx);
                    }
                    catch (std::invalid_argument &exc)
                    {
                        value = 0.0;
                        fprintf(stdout,
                                "The std::invalid_argument[%s] appears when trying to make "
                                "std::stod on data from file with id = %d\n",
                                exc.what(), idx);
                    }
                    buf_line.push_back(value);
                }
                buf[idx] = std::make_optional(buf_line);
            }
            else
            {
                buf[idx] = {};
            }
        }
    }

    static std::string remove_heads(std::array<std::ifstream, task::base_config::stat_amount>::iterator begin,
                                    std::array<std::ifstream, task::base_config::stat_amount>::iterator end) noexcept
    {
        std::string ret_val{};

        std::string line{};
        std::for_each(std::execution::par_unseq, begin, end,
                      [&line](auto &elem) mutable noexcept -> void { std::getline(elem, line); });
        ret_val += line;

        return ret_val;
    }

    static std::filesystem::path prepare_folder(std::filesystem::path path_to_result_folder,
                                                std::string_view raw_data_folder) noexcept
    {
        namespace fs = std::filesystem;
        fs::current_path(path_to_result_folder);

        if (!fs::exists(stat_folder))
        {
            fs::create_directory(stat_folder);
        }

        fs::copy(path_to_result_folder / raw_data_folder, path_to_result_folder / stat_folder,
                 fs::copy_options::recursive | fs::copy_options::directories_only);

        return path_to_result_folder;
    }
};
} // namespace stat
