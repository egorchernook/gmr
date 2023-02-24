#ifndef OUTPUT_HPP_INCLUDED
#define OUTPUT_HPP_INCLUDED

#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <string_view>
#include <type_traits>

#include "config.hpp"

class outputer_t
{
    std::filesystem::path folder;
    const int id;

  public:
    static std::string createName(std::string_view text, auto &&value_var) noexcept
    {
        std::ostringstream stream{};
        stream << text << " " << value_var;
        return stream.str();
    }
    static std::string createName(std::string_view text, const auto &value_var) noexcept
    {
        std::ostringstream stream{};
        stream << text << " " << value_var;
        return stream.str();
    }
    static std::string createName(std::string_view text, const task::base_config::spin_t::magn_t &value_var) noexcept
    {
        std::ostringstream stream{};
        stream << text << " " << to_string(value_var);
        return stream.str();
    }
    static std::string createName(std::string_view text, const std::integral auto &value_var) noexcept
    {
        std::ostringstream stream{};
        stream << text << " " << std::to_string(value_var);
        return stream.str();
    }

    outputer_t(const typename task::base_config::config_t &config, std::string_view current_dir)
        : folder{current_dir}, id{config.stat_id}
    {
        std::filesystem::current_path(current_dir);
    }

    static void create_directories(std::string_view directory_name)
    {
        using std::filesystem::exists;
        if (!exists(directory_name))
        {
            std::filesystem::create_directories(directory_name);
        }
    }

    outputer_t &createDirectoriesAndEnter(std::string_view directory_name) noexcept
    {
        using std::filesystem::current_path;
        const auto dir = current_path() / directory_name;
        create_directories(dir.string());
        current_path(dir);
        folder = current_path();
        return *this;
    }

    class output_file_t
    {
        std::ofstream out;

      public:
        output_file_t() = delete;
        output_file_t(std::ofstream &&out_) : out{std::move(out_)} {};
        output_file_t(const std::ofstream &out_) = delete;
        output_file_t &operator=(const std::ofstream &out_) = delete;

        // агрументы разделяются \t
        template <typename... Args> output_file_t &printLn(Args &...args) noexcept
        {
            ((out << std::forward<Args>(args) << "\t"), ...);
            out << "\n";
            return *this;
        }
        template <typename... Args> output_file_t &printLn(Args &&...args) noexcept
        {
            ((out << std::forward<Args>(args) << "\t"), ...);
            out << "\n";
            return *this;
        }

        template <typename Head, typename... Args> output_file_t &print(Head &fst, Args &...args) noexcept
        {
            out << fst;
            ((out << "\t" << std::forward<Args>(args)), ...);
            return *this;
        }
        template <typename Head, typename... Args> output_file_t &print(Head &&fst, Args &...args) noexcept
        {
            out << fst;
            ((out << "\t" << std::forward<Args>(args)), ...);
            return *this;
        }
        template <typename Head, typename... Args> output_file_t &print(Head &fst, Args &&...args) noexcept
        {
            out << fst;
            ((out << "\t" << std::forward<Args>(args)), ...);
            return *this;
        }
        template <typename Head, typename... Args> output_file_t &print(Head &&fst, Args &&...args) noexcept
        {
            out << fst;
            ((out << "\t" << std::forward<Args>(args)), ...);
            return *this;
        }

        ~output_file_t() noexcept
        {
            out.flush();
            out.close();
        }
    };

    // id стат прогонки и расширение ставится автоматически
    output_file_t createFile(const std::string &name) const noexcept
    {
        std::ofstream res{name + "_id=" + std::to_string(id) + ".txt"};
        res << std::fixed;
        return res;
    }
};

#endif