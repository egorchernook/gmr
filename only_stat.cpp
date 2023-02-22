#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

#include "stat.hpp"

int main()
{
    std::ifstream info{"info.txt"};
    std::vector<std::string> vals;
    std::string line{};
    std::getline(info, line);
    std::istringstream str{line};
    for (std::string value; std::getline(str, value, '\t');)
    {
        vals.push_back(value);
    }

    stat::stater::makeStat(std::filesystem::path{vals[0]}, vals[1]);
}
