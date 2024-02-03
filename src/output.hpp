#ifndef OUTPUT_HPP_INCLUDED
#define OUTPUT_HPP_INCLUDED

#include "config.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <string_view>
#include <type_traits>

class outputer_t {
    std::filesystem::path folder;
    const int id;

public:
    template<typename Arg>
    static std::string createName(std::string_view text, Arg&& value_var) noexcept
    {
        std::ostringstream stream{};
        stream << text << " " << value_var;
        return stream.str();
    }
    template<typename Arg>
    static std::string createName(std::string_view text, const Arg& value_var) noexcept
    {
        std::ostringstream stream{};
        stream << text << " " << value_var;
        return stream.str();
    }
    static std::string
    createName(std::string_view text, const task::base_config::spin_t::magn_t& value_var) noexcept
    {
        std::ostringstream stream{};
        stream << text << " " << to_string(value_var);
        return stream.str();
    }
    template<typename Arg, std::enable_if_t<std::is_integral_v<Arg>, bool> = true>
    static std::string createName(std::string_view text, const Arg& value_var) noexcept
    {
        std::ostringstream stream{};
        stream << text << " " << std::to_string(value_var);
        return stream.str();
    }

    outputer_t(const typename task::base_config::config_t& config, std::string_view current_dir)
        : folder{current_dir}
        , id{config.stat_id}
    {
        std::filesystem::current_path(current_dir);
    }

    outputer_t& EnterDirectory(std::string_view directory_name)
    {
        using std::filesystem::current_path;
        const auto dir = folder / directory_name;
        current_path(dir);
        folder = dir;
        return *this;
    }

    class output_file_t {
        std::ofstream out;

    public:
        output_file_t() = delete;
        output_file_t(const output_file_t& other) = delete;
        output_file_t(output_file_t&& other)
            : out{std::move(other.out)} {};
        output_file_t& operator=(output_file_t&& other)
        {
            out = std::move(other.out);
            return *this;
        }

        output_file_t(const std::ofstream& out_) = delete;
        output_file_t(std::ofstream&& out_)
            : out{std::move(out_)} {};
        output_file_t& operator=(const std::ofstream& out_) = delete;

        // агрументы разделяются табуляцией
        template<typename... Args>
        output_file_t& printLn(Args... args) noexcept
        {
            ((out << args << "\t"), ...);
            out << "\n";
            return *this;
        }

        template<typename Head, typename... Args>
        output_file_t& print(Head fst, Args... args) noexcept
        {
            out << fst;
            if (sizeof...(Args) == 0) {
                out << "\t";
            } else {
                ((out << "\t" << args), ...);
            }
            return *this;
        }

        ~output_file_t()
        {
            out.flush();
            out.close();
        }
    };

    output_file_t createFile(const std::string& name) const noexcept
    {
        std::ofstream res{folder / name};
        res << std::fixed;
        return res;
    }
};

#endif