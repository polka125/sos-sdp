//
// Created by sergey on 30.06.23.
//



#include <stdexcept>
#include <sstream>
#include <iomanip>
#include "stringRoutines.h"

bool isLVar(const std::string& name) {
    std::string prefix = "l_";
    if (name.substr(0, prefix.size()) != prefix) {
        return false;
    }
    return true;
}

std::vector<int> parseLVarName(std::string name) {
    // l_1_2_3 -> (1, 2, 3)
    std::string prefix = "l_";
    if (name.substr(0, prefix.size()) != prefix) {
        throw std::runtime_error("Not a linear variable");
    }
    name = name.substr(prefix.size());
    std::vector<int> tokens;
    std::string token;
    std::istringstream tokenStream(name);
    while (std::getline(tokenStream, token, '_')) {
        tokens.push_back(std::stoi(token));
    }
    if (tokens.size() != 3) {
        throw std::runtime_error("Not a linear variable");
    }
    return {tokens[0], tokens[1], tokens[2]};
}

void replaceAllInplace(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

std::string replaceAll(const std::string& str, const std::string& from, const std::string& to) {
    std::string answer = str;
    replaceAllInplace(answer, from, to);
    return answer;
}

std::string doubleToString(double x) {
    // high precision double to string
    std::stringstream ss;
    ss << std::setprecision(25) << x;
    return ss.str();
}