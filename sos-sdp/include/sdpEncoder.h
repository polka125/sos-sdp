//
// Created by sergey on 31.05.23.
//

#ifndef MYPROJECT_SDPENCODER_H
#define MYPROJECT_SDPENCODER_H

#include "symbolicRing.h"
#include <vector>
#include <iostream>

using namespace symbolic_ring;

inline SymbolicPolynomial getSos(const std::vector<QMonomial>& monomials, int id) {
    if (monomials.empty()) {
        throw std::runtime_error("Empty list");
    }
    auto env = monomials[0].viewEnvironment();
    auto n = monomials.size();

    auto monomials_as_symbolic_polynomials = std::vector<SymbolicPolynomial>();

    for (const auto& monomial: monomials) {
        monomials_as_symbolic_polynomials.push_back(SymbolicPolynomial(SymbolicMonomial(monomial)));
    }

    auto x = std::vector<SymbolicPolynomial>();

    for (auto i = 0; i < n; i++) {
        for (auto j = 0; j < n; j++) {
            auto l_ij = QPolynomial(env->getOrCreate(
                    "l_" + std::to_string(id) + "_" +
                    std::to_string(std::min(i, j)) + "_" +
                    std::to_string(std::max(i, j))));
            auto q_ij = SymbolicMonomial(l_ij);
            auto s_ij = SymbolicPolynomial(q_ij);
            x.push_back(s_ij);
        }
    }


    // TODO: can we optimize it with openmp?
    auto row_matrix = std::vector<SymbolicPolynomial>();
    for (int col = 0; col < n; col++) {
        auto new_term = env->symbolicPolynomialZero();
        for (int row = 0; row < n; row++) {
            new_term = add(new_term, mul(monomials_as_symbolic_polynomials[row], x[row * n + col]), false);
        }
        new_term.reduce();

        row_matrix.push_back(new_term);
    }

//    for (auto& it: row_matrix) {
//        std::cout << "row_matrix: " << it << std::endl;
//    }


    auto result = env->symbolicPolynomialZero();
    for (int row = 0; row < n; row++) {
        result = add(result, mul(row_matrix[row], monomials_as_symbolic_polynomials[row]), false);
//        std::cout << "result it(" << row << "):" << result << std::endl;
    }
    result.reduce();
    return result;
}

#endif //MYPROJECT_SDPENCODER_H
