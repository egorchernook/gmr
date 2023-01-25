#ifndef OUTPUT_HPP_INCLUDED
#define OUTPUT_HPP_INCLUDED

#include <string>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string_view>

#include "config.hpp"

class outputer_t
{
    const std::filesystem::path folder;
    const int id;

public:
    static std::string createName(std::string_view text, auto &&value_var) noexcept {
        std::ostringstream stream{};
        stream << text << value_var;
        return stream.str();
    }
    static std::string createName(std::string_view text, const auto &value_var) noexcept {
        std::ostringstream stream{};
        stream << text << value_var;
        return stream.str();
    }

    outputer_t(const typename task::config_t &config, const std::string_view current_dir)
        : folder{current_dir}, id{config.stat_id}
    {
        using namespace std::filesystem;
        if (!exists(folder))
        {
            create_directories(folder);
        }
    }

    outputer_t& createDirectoryAndEnter(std::string name) noexcept {
        using std::filesystem::current_path;
        const auto dir = current_path() / name;
        if (!exists(dir))
        {
            create_directories(dir);
        }
        current_path(dir);
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
        template <typename... Args>
        output_file_t &printLn(Args &...args) noexcept
        {
            ((out << std::forward<Args>(args) << "\t"), ...);
            out << "\n";
            return *this;
        }
        template <typename... Args>
        output_file_t &printLn(Args &&...args) noexcept
        {
            ((out << std::forward<Args>(args) << "\t"), ...);
            out << "\n";
            return *this;
        }

        template <typename Head, typename... Args>
        output_file_t &print(Head &fst, Args &...args) noexcept
        {
            out << fst;
            ((out << "\t" << std::forward<Args>(args)), ...);
            return *this;
        }
        template <typename Head, typename... Args>
        output_file_t &print(Head &&fst, Args &...args) noexcept
        {
            out << fst;
            ((out << "\t" << std::forward<Args>(args)), ...);
            return *this;
        }
        template <typename Head, typename... Args>
        output_file_t &print(Head &fst, Args &&...args) noexcept
        {
            out << fst;
            ((out << "\t" << std::forward<Args>(args)), ...);
            return *this;
        }
        template <typename Head, typename... Args>
        output_file_t &print(Head &&fst, Args &&...args) noexcept
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