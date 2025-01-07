//
// Created by sergey on 27.06.23.
//

#ifndef MYPROJECT_COMBINATORICS_H
#define MYPROJECT_COMBINATORICS_H

#include <vector>

bool getNext(std::vector<int>& current, int n) {
    int i = 0;
    while (i < current.size() && current[i] == n) {
        current[i] = 0;
        i++;
    }
    if (i == current.size()) {
        return false;
    }
    current[i]++;
    return true;
}

bool getNextVectorBoundedSum(std::vector<int>& current, int bound) {
    if (bound < 0) {
        throw std::runtime_error("Bound must be non-negative");
    }
    while (true) {
        if (!getNext(current, bound)) {
            return false;
        }
        int sum = 0;
        for (auto& it: current) {
            sum += it;
        }
        if (sum <= bound) {
            return true;
        }
    }
}



#endif //MYPROJECT_COMBINATORICS_H
