#ifndef LINE_T_HPP_INCLUDED
#define LINE_T_HPP_INCLUDED

#include <algorithm>
#include <execution>
#include <fstream>
#include <vector>

namespace stat
{
struct line_t final : private std::vector<double>
{
    using data_t = std::vector<double>;
    using data_t::begin;
    using data_t::cbegin;
    using data_t::cend;
    using data_t::crbegin;
    using data_t::crend;
    using data_t::end;
    using data_t::rbegin;
    using data_t::rend;

    using data_t::push_back;
    using data_t::operator[];
    using data_t::clear;
    using data_t::size;

    line_t() : data_t{}
    {
        this->reserve(10);
    }

    line_t &operator+=(const line_t &other) noexcept
    {
        if (this->empty())
        {
            *this = other;
        }
        else
        {
            for (auto idx = 0u; idx < this->size(); ++idx)
            {
                if (idx < other.size())
                {
                    this->at(idx) += other[idx];
                }
            }
        }
        return *this;
    }
    line_t &operator-=(const line_t &other) noexcept
    {
        if (this->empty())
        {
            *this = other;
        }
        else
        {
            for (auto idx = 0u; idx < this->size(); ++idx)
            {
                if (idx < other.size())
                {
                    this->at(idx) -= other[idx];
                }
            }
        }
        return *this;
    }
    line_t &operator/=(double value) noexcept
    {
        std::for_each(std::execution::par_unseq, this->begin(), this->end(),
                      [&value](auto &elem) noexcept -> void { elem /= value; });
        return *this;
    }
};

inline line_t operator+(line_t lhs, const line_t &rhs) noexcept
{
    return lhs += rhs;
}
inline line_t operator-(line_t lhs, const line_t &rhs) noexcept
{
    return lhs -= rhs;
}
inline line_t sqr(line_t value)
{
    std::for_each(std::execution::par_unseq, value.begin(), value.end(),
                  [](auto &elem) noexcept -> void { elem *= elem; });
    return value;
}
inline std::ostream &operator<<(std::ostream &out, const line_t &line) noexcept
{
    out << "line : [ ";
    for (auto iter = line.begin(); iter != line.end(); ++iter)
    {
        const auto value = *iter;
        out << value;
        if (iter + 1 != line.end())
        {
            out << ", ";
        }
    }
    out << "]";
    return out;
}
} // namespace stat
#endif