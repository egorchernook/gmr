#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <array>
#include <algorithm>
#include <execution>
#include <optional>

#include "config.hpp"
#include "line_t.hpp"

namespace stat
{
    struct stater
    {
        static void makeStat(std::filesystem::path path_to_result_folder, std::string_view raw_data_folder)
        {
            namespace fs = std::filesystem;
            const static std::string stat_folder = "processed";

            const auto data_folder = prepare_folder(path_to_result_folder, raw_data_folder, stat_folder);

            auto init_configs = task::base_config::getConfigs();
            std::vector<task::config_t> configs{};
            for(auto&& elem: init_configs) {
                if(elem.stat_id == 0) {
                    configs.push_back(std::move(elem));
                }
            }
            std::array<std::ifstream, task::base_config::stat_amount> input_streams;
            for (const auto &config : configs)
            {
                const auto folder_name = task::createName(config);
                const auto path_to_file = data_folder / stat_folder / folder_name;
                fs::current_path(path_to_file);

                for (auto idx = 0u; idx < input_streams.size(); ++idx)
                {
                    input_streams[idx] = std::ifstream(path_to_file / ("m_id=" + std::to_string(idx) + ".txt"));
                }

                const auto init_head = remove_heads(input_streams.begin(), input_streams.end());

                auto is_end = [&input_streams]() noexcept -> bool
                {
                    bool res = true;
                    std::ranges::for_each(input_streams.begin(), input_streams.end(),
                                          [&res](auto &elem) noexcept -> void
                                          {
                                              res &= elem.eof();
                                          });
                    return res;
                };

                std::ofstream out{"m.txt"};
                {
                    std::istringstream init_head_stream{init_head};
                    for (std::string head; std::getline(init_head_stream, head, '\t');)
                    {
                        out << head << "\t\t";
                    }
                }

                while (!is_end())
                {
                    const auto values = get_values_from_streams(input_streams.begin(), input_streams.end());

                    line_t average{};
                    auto amount = 0u;
                    for (const auto &elem : values)
                    {
                        if (elem)
                        {
                            average += elem.value();
                            amount++;
                        }
                    }
                    average /= amount;

                    line_t err{};
                    for (const auto &elem : values)
                    {
                        if (elem)
                        {
                            err += stat::sqr(elem.value() - average);
                        }
                    }
                    err /= amount;

                    for (auto idx = 0u; idx < amount; ++idx)
                    {
                        out << average[idx] << "\t" << err[idx] << "\t";
                    }
                }
                out.flush();
                out.close();
            }
        }

    private:
        static std::array<std::optional<line_t>, task::base_config::stat_amount>
        get_values_from_streams(std::array<std::ifstream, task::base_config::stat_amount>::iterator first,
                                std::array<std::ifstream, task::base_config::stat_amount>::iterator last) noexcept
        {
            std::array<std::optional<line_t>, task::base_config::stat_amount> result{};
            for (auto idx = 0u; first != last; ++first, ++idx)
            {
                if (!first->eof())
                {
                    line_t result_line{};
                    std::string line{};
                    std::getline(*first, line);

                    std::istringstream stream{line};
                    for (std::string data; std::getline(stream, data, '\t');)
                    {
                        double value;
                        try
                        {
                            value = std::stod(data);
                        }
                        catch (std::out_of_range &exc)
                        {
                            value = std::numeric_limits<double>::max();
                            fprintf(stderr,
                                    "The std::out_of_range[%s] appears when trying to make std::stod on data from file with id = %d\n",
                                    exc.what(),
                                    idx);
                        }
                        catch (std::invalid_argument &exc)
                        {
                            value = 0.0;
                            fprintf(stderr,
                                    "The std::invalid_argument[%s] appears when trying to make std::stod on data from file with id = %d\n",
                                    exc.what(),
                                    idx);
                        }
                        result_line.push_back(value);
                        result[idx] = result_line;
                    }
                }
                else
                {
                    result[idx] = {};
                }
            }

            return result;
        }

        static std::string remove_heads(std::array<std::ifstream, task::base_config::stat_amount>::iterator begin,
                                        std::array<std::ifstream, task::base_config::stat_amount>::iterator end) noexcept
        {
            std::string ret_val{};
            for (int _ = 0; _ < 2; ++_)
            {
                std::string line{};
                std::for_each(std::execution::par_unseq,
                              begin, end,
                              [&ret_val, &line](auto &elem) mutable noexcept -> void
                              {
                                  std::getline(elem, line);
                              });
                ret_val += line;
            }
            return ret_val;
        }

        static std::filesystem::path prepare_folder(std::filesystem::path path_to_result_folder,
                                                    std::string_view raw_data_folder,
                                                    std::string_view stat_folder) noexcept
        {
            namespace fs = std::filesystem;
            fs::current_path(path_to_result_folder);
            const auto data_folder = fs::current_path();

            if (!fs::exists(stat_folder))
            {
                fs::create_directory(stat_folder);
            }

            fs::copy(path_to_result_folder / raw_data_folder,
                     fs::current_path() / stat_folder,
                     fs::copy_options::recursive & fs::copy_options::directories_only);
            fs::current_path(fs::current_path() / stat_folder);

            return data_folder;
        }
    };
}
