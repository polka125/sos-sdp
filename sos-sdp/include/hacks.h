//
// Created by sergey on 30.06.23.
//

#ifndef MYPROJECT_HACKS_H
#define MYPROJECT_HACKS_H

#include <tokenizer.h>
#include <cmath>

#include "stringRoutines.h"

struct PowerGroup {
    int groupStartIdx;
    int groupFinishIdx;
    int powerSymbolIdx;
    int powerValueIdx;
};

inline std::vector<int> getAllPowerIdxs(const std::vector<Token>& tokens) {
    std::vector<int> result;
    for (int i = 0; i < tokens.size(); ++i) {
        if (tokens[i].value == Arith::POW) {
            result.push_back(i);
        }
    }
    return result;
}

inline int getCorrespondingOpenBracket(const std::vector<Token>& tokens, int pos) {
    // check that tokens[pos] is a closing bracket
    if (tokens[pos].value != Braces::ROUND_CLOSE) {
        throw std::runtime_error("getCorrespondingOpenBracket: tokens[pos] is not a closing bracket");
    }

    int depth = 0;
    for (int i = pos; i >= 0; --i) {
        if (tokens[i].value == Braces::ROUND_CLOSE) {
            depth++;
        } else if (tokens[i].value == Braces::ROUND_OPEN) {
            depth--;
        }
        if (depth == 0) {
            return i;
        }
    }
    throw std::runtime_error("getCorrespondingOpenBracket: no corresponding open bracket found");
}

// finds all expression of the form a ^ b or (expr) ^ b and returns a vector of PowerGroup
inline std::vector<PowerGroup> getPowerGroups(const std::vector<Token>& tokens) {
    auto powerIdxs = getAllPowerIdxs(tokens);
    // for each powerIdxs create a PowerGroup
    std::vector<PowerGroup> result(powerIdxs.size());

    for (int i = 0; i < powerIdxs.size(); ++i) {
        result[i].powerSymbolIdx = powerIdxs[i];
        result[i].groupFinishIdx = powerIdxs[i];
        if (tokens[result[i].groupFinishIdx - 1].value == Braces::ROUND_CLOSE) {
            result[i].groupStartIdx = getCorrespondingOpenBracket(tokens, result[i].groupFinishIdx - 1);
        } else if (tokens[result[i].groupFinishIdx - 1].type == TokenType::NUMBER || tokens[result[i].groupFinishIdx - 1].type == TokenType::IDENTIFIER) {
            result[i].groupStartIdx = result[i].groupFinishIdx - 1;
        } else {
            throw std::runtime_error("getPowerGroups: invalid token before power symbol");
        }

        if (tokens[result[i].groupFinishIdx + 1].type == TokenType::NUMBER) {
            result[i].powerValueIdx = result[i].groupFinishIdx + 1;
        } else {
            throw std::runtime_error("getPowerGroups: invalid token after power symbol");
        }
    }
    return result;
}

inline std::vector<Token> repeatPowerGroupsHack(const std::vector<Token>& tokens) {
    auto powerGroups = getPowerGroups(tokens);
    auto answer = std::vector<Token>();

    const int notInPowerGroup = -1;
    const int inPowerGroupNotFirst = -2;
    const int inPowerGroupFirst = -3;

    std::vector<int> isWithinPowerGoup(tokens.size(), notInPowerGroup);
    std::vector<int> linkToPowerGroupIdx(tokens.size(), -1);

    for (int i = 0; i < powerGroups.size(); ++i) {
        for (int j = powerGroups[i].groupStartIdx; j <= powerGroups[i].powerValueIdx; ++j) {
            isWithinPowerGoup[j] = inPowerGroupNotFirst;
            linkToPowerGroupIdx[j] = i;
        }
        isWithinPowerGoup[powerGroups[i].groupStartIdx] = inPowerGroupFirst;
    }

    Token multiplyToken = {TokenType::OPERATION, Arith::MULT, tokens[0].line};
    Token openBraceToken = {TokenType::BRACE, Braces::ROUND_OPEN, tokens[0].line};
    Token closeBraceToken = {TokenType::BRACE, Braces::ROUND_CLOSE, tokens[0].line};

    for (int i = 0; i < tokens.size(); ++i) {
        if (isWithinPowerGoup[i] == notInPowerGroup) {
            answer.push_back(tokens[i]);
        } else if (isWithinPowerGoup[i] == inPowerGroupFirst) {
            // repeat power group value times with "*" symbol in between
            auto& currentPowerGroup = powerGroups[linkToPowerGroupIdx[i]];

            // prepend "("
            answer.push_back(openBraceToken);

            auto powerValue = std::stoi(tokens[currentPowerGroup.powerValueIdx].value);

            if (powerValue <= 0) {
                throw std::runtime_error("repeatPowerGroupsHack: power value " + std::to_string(powerValue) + " at the position " + std::to_string(currentPowerGroup.powerValueIdx) + " is not positive");
            }
            // repeat it powerValue times
            for (int repeatCounter = 0; repeatCounter < powerValue; ++repeatCounter) {
                for (int indexWithinGroup = currentPowerGroup.groupStartIdx; indexWithinGroup < currentPowerGroup.groupFinishIdx; ++indexWithinGroup) {
                    answer.push_back(tokens[indexWithinGroup]);
                }
                if (repeatCounter != powerValue - 1) {
                    answer.push_back(multiplyToken);
                }
            }

            // append ")"
            answer.push_back(closeBraceToken);

        } else if (isWithinPowerGoup[i] == inPowerGroupNotFirst) {
            // do nothing
        } else {
            throw std::runtime_error("repeatPowerGroupsHack: invalid isWithinPowerGroup value");
        }
    }
    return answer;
}

inline bool isMultiplicationSafe(unsigned long long a, unsigned long long b) {
    return (a == 0 || b == 0 || (a * b) / b == a);
}

inline bool isMultiplicationSafe(long long a, long long b) {
    a = std::abs(a);
    b = std::abs(b);
    return isMultiplicationSafe((unsigned long long) a, (unsigned long long) b);
}

inline void assertMultiplicationSafe(long long a, long long b) {
    if (!isMultiplicationSafe(a, b)) {
        throw std::runtime_error("assertMultiplicationSafe: multiplication is not safe");
    }
}

class Fractionizer {
    // credits: https://github.com/ajneu/fractionizer/blob/master/src/fractionizer.h
public:
    template <typename Tfl, typename Tfl2> // Tfl should be a floating point type (preferably double or long double)
    static std::vector<Tfl> fractionize(const Tfl val, Tfl2 &num, Tfl2 &denom)
    {
        Tfl n;
        Tfl d;
        std::vector<Tfl> vec;
        Tfl i;
        Tfl v = val;
        goto label_begin;
        do {
            v = 1/v;
            label_begin:
            //std::cout << "v == " << v << '\t';
            v = std::modf(v, &i);
            //std::cout << "i == " << i << '\t';
            vec.push_back(i);
            //std::cout << "calc == " << Fractionizer::calc_frac<Tfl>(vec, n, d) << std::endl;
        }
            //while (std::abs((Fractionizer::calc_frac<Tfl>(vec, n, d) - val)/val) > numeric_limits<Tfl>::min());
        while (Fractionizer::calc_frac<Tfl>(vec, n, d) != val);
        //assert((n/d) == val);
        num   = n;
        denom = d;

        if (num * 1.0 / denom - val > 1e-5 || num * 1.0 / denom - val < -1e-5) {
            throw std::runtime_error("Factorizer::fractionize failure: " + doubleToString(val) + " != " + doubleToString(num) + "/" + doubleToString(denom) + " == " + doubleToString(num / denom));
        }
        return std::move(vec);
    }

    template <typename Tfl, typename Tfl2> // Tfl should be a floating point type (preferably double or long double)
    static Tfl calc_frac(const std::vector<Tfl> &vec, Tfl2 &num, Tfl2 &denom)
    // {2,     3,  4}
    //  2 + 1/(3+1/4)
    {
        if (!vec.empty()) {
            Tfl n = 1.0; // num
            Tfl d = 0.0; // denom

            auto it_end = vec.cend();
            const auto it_beg = vec.cbegin();
            do {
                --it_end;

                std::swap(n, d);
                n += *it_end * d;
            } while (it_end != it_beg);
            num   = n;
            denom = d;

            return n/d;
        }
        num   = 0.0;
        denom = 1.0;


        return 0.0;
    }

};



#endif //MYPROJECT_HACKS_H
