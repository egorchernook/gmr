#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <array>
#include <algorithm>

#include "config.hpp"

namespace stat
{
    void makeStat(std::filesystem::path path_to_result_folder, std::string_view raw_data_folder)
    {
        namespace fs = std::filesystem;
        const static std::string stat_folder = "processed";

        const auto data_folder = prepare_folder(path_to_result_folder, raw_data_folder, stat_folder);

        const static auto configs = std::ranges::remove_if(task::base_config::getConfigs(), 
            [](const task::config_t &conf) noexcept -> bool {
                return conf.stat_id == 0;
            });

        std::array<std::ifstream, task::base_config::stat_amount> input_streams;
        std::array<std::string, input_streams.size()> lines_buf; 
        for (auto &config : configs)
        {
            const auto folder_name = task::createName(config);
            const auto path_to_file = data_folder / stat_folder / folder_name;
            fs::current_path(path_to_file);
            
            for (auto idx = 0u; idx < input_streams.size(); ++idx)
            {
                input_streams[idx] = std::ifstream(path_to_file / ("m_id=" + std::to_string(idx) + ".txt"), std::ios_base::in);
            }

            std::ofstream m_out{"m.txt"};

            m_out.flush();
            m_out.close();
        }
    }

    std::filesystem::path getResultStat();

    std::filesystem::path prepare_folder(std::filesystem::path path_to_result_folder, std::string_view raw_data_folder, std::string_view stat_folder) noexcept
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
}