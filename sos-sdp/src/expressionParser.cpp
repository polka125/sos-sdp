//
// Created by sergey on 19.06.23.
//
#include <stack>
#include "expressionParser.h"

#define TODO() throw std::runtime_error("TODO")

#define debugStream if (false) {} else std::cout


int getPriority(const std::string &operation) {
    if (operation == Arith::GE || operation == Arith::LE || operation == Arith::GEQ ||
        operation == Arith::LEQ || operation == Arith::EQ) {
        return 0;
    } else if (operation == Arith::PLUS || operation == Arith::MINUS) {
        return 1;
    } else if (operation == Arith::MULT || operation == Arith::DIV) {
        return 2;
    } else {
        throw std::runtime_error("Unknown operation: " + operation);
    }
}


struct OperationInfo {
    std::string operation;
    int priority;
    int index;
};

int getSplitOperation(const std::vector<Token> &tokens, IndexRange range) {
    std::vector<OperationInfo> firstLevelOperationIndices;
    int currentDepth = 0;

    for (int i = range.leftIndex; i < range.rightIndex; i++) {
        if (tokens[i].value == Braces::ROUND_OPEN) {
            currentDepth++;
        } else if (tokens[i].value == Braces::ROUND_CLOSE) {
            currentDepth--;
        } else if (currentDepth == 0 && tokens[i].type == TokenType::OPERATION) {
            firstLevelOperationIndices.push_back({tokens[i].value, getPriority(tokens[i].value), i});
        }
    }

    if (firstLevelOperationIndices.empty()) {
        return range.rightIndex;
    }

    int minPriority = firstLevelOperationIndices[0].priority;
    int minPriorityIndex = 0;
//    int medianIndex = (range.leftIndex + range.rightIndex) / 2;

    // find the operation with the highest priority most close to (range.leftIndex + range.rightIndex) / 2

    for (int i = 1; i < firstLevelOperationIndices.size(); i++) {
        if (firstLevelOperationIndices[i].priority <= minPriority) {
            minPriority = firstLevelOperationIndices[i].priority;
            minPriorityIndex = i;
        }
//        else if (firstLevelOperationIndices[i].priority == minPriority) {
//            if (std::abs(firstLevelOperationIndices[i].index - medianIndex) < std::abs(firstLevelOperationIndices[minPriorityIndex].index - medianIndex)) {
//                minPriorityIndex = i;
//            }
//        }
    }

    return firstLevelOperationIndices[minPriorityIndex].index;
}

std::unique_ptr<ExpressionElement> parseSingleExpression(const Token &token) {
    if (token.type == TokenType::NUMBER) {
        return std::make_unique<Constant>(std::stoi(token.value));
    } else if (token.type == TokenType::IDENTIFIER) {
        return std::make_unique<Variable>(token.value);
    } else {
        throw std::runtime_error("Unexpected token: " + token.value);
    }
}

std::vector<IndexRange> getArgumentRangesOfFunctionCall(const std::vector<Token> &tokens, IndexRange range) {
    // 1. find all commas
    std::vector<int> commaIndices;
    // TODO: check is it the problem
    for (int i = range.leftIndex; i < range.rightIndex; i++) {
        if (tokens[i].value == ",") {
            commaIndices.push_back(i);
        }
    }
    // 2. split the range into subranges
    std::vector<IndexRange> result;
    int leftIndex = range.leftIndex + 2;
    for (int commaIndex : commaIndices) {
        result.emplace_back(leftIndex, commaIndex);
        leftIndex = commaIndex + 1;
    }
    result.emplace_back(leftIndex, range.rightIndex - 1); // -1 for the closing bracket
    return result;
}

void trimLeftRightOpenBrackets(const std::vector<Token> &tokens, IndexRange &range) {
    std::vector<int> closeBracketMatchesOpenWithIndex(range.rightIndex - range.leftIndex);

    typedef int BracketType;
    const BracketType openBracket = 1;
    const BracketType closeBracket = -1;

    std::stack<std::pair<BracketType, int>> brackets;

    for (int i = range.leftIndex; i < range.rightIndex; i++) {
        if (tokens[i].value == Braces::ROUND_OPEN) {
            brackets.push({openBracket, i});
        } else if (tokens[i].value == Braces::ROUND_CLOSE) {
            if (brackets.empty()) {
                throw std::runtime_error("Unmatched closing bracket");
            }
            auto top = brackets.top();
            brackets.pop();
            if (top.first == openBracket) {
                closeBracketMatchesOpenWithIndex[i - range.leftIndex] = top.second - range.leftIndex;
            } else {
                throw std::runtime_error("Unmatched closing bracket");
            }
        }
    }

    debugStream << "closeBracketMatchesOpenWithIndex: " << std::endl;
    for (int i = 0; i < closeBracketMatchesOpenWithIndex.size(); i++) {
        debugStream << ""<< closeBracketMatchesOpenWithIndex[i] << " ";
    }

    if (!brackets.empty()) {
        throw std::runtime_error("Unmatched opening bracket");
    }

    int leftIndex = 0;
    int rightIndex = range.rightIndex - range.leftIndex;

    while (leftIndex < rightIndex
            && tokens[leftIndex + range.leftIndex].value == Braces::ROUND_OPEN
            && tokens[rightIndex - 1 + range.leftIndex].value == Braces::ROUND_CLOSE
            && closeBracketMatchesOpenWithIndex[rightIndex - 1] == leftIndex) {
        leftIndex++;
        rightIndex--;
    }

    range.leftIndex += leftIndex;
    range.rightIndex -= leftIndex;

    debugStream << "Got tokens: " << std::endl;
    for (int i = range.leftIndex; i < range.rightIndex; i++) {
        debugStream << "{"<< tokens[i].value << "} ";
    }
    debugStream << std::endl;

    debugStream << "With range: " << range.leftIndex << " " << range.rightIndex << std::endl;
    debugStream << "Trimmed brackets: " << range.leftIndex << " " << range.rightIndex << std::endl;

}

std::unique_ptr<ExpressionElement> parseExpression(const std::vector<Token> &tokens, IndexRange range) {

    auto expressionToString = [&]() {
        std::stringstream stream;
        for (int i = range.leftIndex; i < range.rightIndex; i++) {
            stream << "" << tokens[i].value << " ";
        }
        stream << std::endl;
        return stream.str();
    };

    debugStream << "Before trimming brackets: " << expressionToString() << std::endl;
    trimLeftRightOpenBrackets(tokens, range);
    debugStream << "After trimming brackets: " << expressionToString() << std::endl;

    if (range.leftIndex == range.rightIndex) {
        throw std::runtime_error("Empty subexpression");
    }

    if (range.leftIndex + 1 == range.rightIndex) {
        return parseSingleExpression(tokens[range.leftIndex]);
    }

    int splitOperation = getSplitOperation(tokens, range);

    if (splitOperation == range.rightIndex) {
        if (tokens[range.leftIndex].type == IDENTIFIER && tokens[range.leftIndex + 1].value == Braces::ROUND_OPEN
            && tokens[range.rightIndex - 1].value == Braces::ROUND_CLOSE) { // function case
            auto argumentRanges = getArgumentRangesOfFunctionCall(tokens, range);
            std::vector<std::unique_ptr<ExpressionElement>> arguments;
            for (auto argumentRange : argumentRanges) {
                auto subexpr = parseExpression(tokens, argumentRange);
                arguments.push_back(std::move(subexpr));
            }
            return std::make_unique<Function>(std::move(arguments), tokens[range.leftIndex].value);
        } else {
            throw std::runtime_error("[A] Invalid expression " + expressionToString());
        }
    }

    if (splitOperation == range.leftIndex) { // unary operation
        auto subexpr = parseExpression(tokens, {range.leftIndex + 1, range.rightIndex});
        return std::make_unique<UnaryOperation>(std::move(subexpr), tokens[range.leftIndex].value);
    }

    // binary operation
    auto leftSubexpr = parseExpression(tokens, {range.leftIndex, splitOperation});
    auto rightSubexpr = parseExpression(tokens, {splitOperation + 1, range.rightIndex});

    if (tokens[splitOperation].value == Arith::GE ||
        tokens[splitOperation].value ==  Arith::LE ||
        tokens[splitOperation].value ==  Arith::EQ ||
        tokens[splitOperation].value ==  Arith::GEQ ||
        tokens[splitOperation].value ==  Arith::LEQ) {
        return std::make_unique<BinaryRelation>(std::move(leftSubexpr), std::move(rightSubexpr), tokens[splitOperation].value);
    } else if (
            tokens[splitOperation].value == Arith::PLUS ||
            tokens[splitOperation].value == Arith::MINUS ||
            tokens[splitOperation].value == Arith::MULT ||
            tokens[splitOperation].value == Arith::DIV
            ) {
        return std::make_unique<BinaryOperation>(std::move(leftSubexpr), std::move(rightSubexpr), tokens[splitOperation].value);
    } else {
        throw std::runtime_error("[B] Invalid expression " + expressionToString());
    }
}

std::unique_ptr<ExpressionElement> parseExpression(const std::vector<Token>& tokens) {
    return parseExpression(tokens, {0, static_cast<int>(tokens.size())});
}