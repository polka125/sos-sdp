//
// Created by sergey on 19.06.23.
//

// takes a sequence of tokens as an input and produces an expression

#ifndef MYPROJECT_EXPRESSIONPARSER_H
#define MYPROJECT_EXPRESSIONPARSER_H

#include "programExpression.h"
#include "tokenizer.h" // TODO: extract tokenizer to a separate file
#include <memory>

using namespace symbolic_ring;

struct IndexRange {
    IndexRange() = default;
    IndexRange(int leftIndex, int rightIndex) : leftIndex(leftIndex), rightIndex(rightIndex) {}

    std::string toString() const {
        return std::to_string(leftIndex) + " " + std::to_string(rightIndex);
    }

    bool operator==(const IndexRange& rhs) const {
        return leftIndex == rhs.leftIndex && rightIndex == rhs.rightIndex;
    }

    int leftIndex;
    int rightIndex;
};

std::vector<IndexRange> getArgumentRangesOfFunctionCall(const std::vector<Token> &tokens, IndexRange range);

std::unique_ptr<ExpressionElement> parseExpression(const std::vector<Token>& tokens);


#endif //MYPROJECT_EXPRESSIONPARSER_H
