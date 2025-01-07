//
// Created by sergey on 30.06.23.
//

#ifndef MYPROJECT_STRINGROUTINES_H
#define MYPROJECT_STRINGROUTINES_H

#include <vector>
#include <string>

bool isLVar(const std::string& name);

std::vector<int> parseLVarName(std::string name);

void replaceAllInplace(std::string& str, const std::string& from, const std::string& to);

std::string replaceAll(const std::string& str, const std::string& from, const std::string& to);

std::string doubleToString(double x);

#endif //MYPROJECT_STRINGROUTINES_H

