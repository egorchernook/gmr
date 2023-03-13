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
            if (elem.stat_id == 0 && !is_almost_equals(elem.field, {0.0, 0.0, 0.0}))
            {
                configs.emplace_back(std::move(elem));
            }
        }
        for (const auto &config : configs)
        {
            task::base_config::config_t config_0{
                config.stat_id, config.N, config.T_creation, config.T_sample, {0.0, 0.0, 0.0}};

            std::filesystem::current_path(path_to_result_folder / stat_folder);
            auto j_file_0 = std::ifstream{std::filesystem::current_path() / task::createName(config_0) / "j.txt"};

            const auto folder_name = task::createName(config);
            std::filesystem::current_path(std::filesystem::current_path() / folder_name);
            auto j_file = std::ifstream{std::filesystem::current_path() / "j.txt"};

            {
                std::string j_file_head{};
                std::string j_file_head_0{};
                std::getline(j_file, j_file_head);
                std::getline(j_file_0, j_file_head_0);
            }

            std::array<std::ofstream, task::base_config::t_wait_vec.size()> outers{};
            for (auto tw_counter = 0u; tw_counter < task::base_config::t_wait_vec.size(); ++tw_counter)
            {
                outers[tw_counter].open("GMR_tw=" + std::to_string(task::base_config::t_wait_vec[tw_counter]) + ".txt");
                outers[tw_counter] << "GMR_h_lower_hc\t\tGMR_h_upper_hc\t" << std::endl;
            }
            auto t_minus_tw_counter = 0u;
            while (!j_file.eof() && !j_file_0.eof())
            {
                std::string line{};
                std::getline(j_file, line);
                if (line == "")
                {
                    break;
                }
                std::istringstream stream1{line};
                const auto j_line = get_double_line(stream1);
                auto j_up = j_line[0];
                auto j_up_err = j_line[1];
                auto j_down = j_line[2];
                auto j_down_err = j_line[3];

                std::getline(j_file_0, line);
                if (line == "")
                {
                    break;
                }
                std::istringstream stream2{line};
                const auto j_line_0 = get_double_line(stream2);
                auto j_up_0 = j_line_0[0];
                auto j_up_err_0 = j_line_0[1];
                auto j_down_0 = j_line_0[2];
                auto j_down_err_0 = j_line_0[3];

                double GMR_h_lower_hc{};
                double GMR_h_lower_hc_err{};
                double GMR_h_upper_hc{};
                double GMR_h_upper_hc_err{};

                {
                    const auto R_up_0 = 1.0 / j_up_0;
                    const auto R_up_0_err = j_up_err_0 / (j_up_0 * j_up_0);
                    const auto R_down_0 = 1.0 / j_down_0;
                    const auto R_down_0_err = j_down_err_0 / (j_down_0 * j_down_0);

                    const auto R_up_H = 1.0 / j_up;
                    const auto R_up_H_err = j_up_err / (j_up * j_up);
                    const auto R_down_H = 1.0 / j_down;
                    const auto R_down_H_err = j_down_err / (j_down * j_down);

                    const auto R_AP_H = (R_up_H + R_down_H) / 2.0;
                    const auto R_AP_H_err = (R_up_H_err + R_down_H_err) / 2.0;

                    const auto R_P_H = 2.0 * R_up_H * R_down_H / (R_up_H + R_down_H);
                    const auto R_up_H_mult_R_down_H__err = R_up_H * R_down_H_err + R_up_H_err * R_down_H;
                    const auto R_up_H_plus_R_down_H__err = R_up_H_err + R_down_H_err;
                    const auto R_P_H_err = R_P_H * (R_up_H_mult_R_down_H__err / (R_up_H * R_down_H) +
                                                    R_up_H_plus_R_down_H__err / (R_up_H_err + R_down_H_err));

                    const auto R_0 = (R_up_0 + R_down_0) / 2.0; // R_0 = R_AP_0
                    const auto R_0_err = (R_up_0_err + R_down_0_err) / 2.0;

                    GMR_h_lower_hc = R_AP_H / R_0 - 1.0;
                    GMR_h_lower_hc_err = GMR_h_lower_hc * (R_AP_H_err / R_AP_H + R_0_err / R_0_err);
                    GMR_h_upper_hc = R_P_H / R_0 - 1.0;
                    GMR_h_upper_hc_err = GMR_h_upper_hc * (R_P_H_err / R_P_H + R_0_err / R_0_err);
                }

                for (auto tw_counter = 0u; tw_counter < task::base_config::t_wait_vec.size(); ++tw_counter)
                {
                    const auto mcs = task::base_config::t_wait_vec[tw_counter] - task::base_config::t_wait_vec[0];
                    if (t_minus_tw_counter >= mcs)
                    {
                        outers[tw_counter] << GMR_h_lower_hc << '\t' << GMR_h_lower_hc_err << '\t' << GMR_h_upper_hc
                                           << '\t' << GMR_h_upper_hc_err << '\n';
                    }
                }
                t_minus_tw_counter++;
            }
            for (auto &outer : outers)
            {
                outer.flush();
                outer.close();
            }
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
                    out << average[idx] << '\t' << err[idx] << '\t';
                }
                out << '\n';
            }
            out.flush();
            out.close();

            std::cout << '\t' << fs::current_path().string() << '/' << name << " file stat ends\n";
        }
    }

    static line_t get_double_line(std::istream &stream) noexcept
    {
        static line_t buf_line{};
        buf_line.clear();
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
                        "std::stod on value{%s}\n",
                        exc.what(), data.c_str());
            }
            catch (std::invalid_argument &exc)
            {
                value = 0.0;
                fprintf(stdout,
                        "The std::invalid_argument[%s] appears when trying to make "
                        "std::stod on value{%s}\n",
                        exc.what(), data.c_str());
            }
            buf_line.push_back(value);
        }
        return buf_line;
    }

    static void get_values_from_streams(std::array<std::ifstream, task::base_config::stat_amount>::iterator first,
                                        std::array<std::ifstream, task::base_config::stat_amount>::iterator last,
                                        std::array<std::optional<line_t>, task::base_config::stat_amount> &buf) noexcept
    {
        for (auto idx = 0u; first != last; ++first, ++idx)
        {
            if (!first->eof())
            {
                std::string line{};
                std::getline(*first, line);

                std::istringstream stream{line};
                buf[idx] = std::make_optional(get_double_line(stream));
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
