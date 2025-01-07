//
// Created by sergey on 17.06.23.
// parses a sequence of tokens into a program

#ifndef MYPROJECT_PROGRAMPARSER_H
#define MYPROJECT_PROGRAMPARSER_H

#include "program.h"
#include "tokenizer.h"
#include "programExpression.h"

struct ParseConfig {
    ParseConfig() = default;
    ParseConfig(bool rewriteEqualAsTwoInequalities, bool moveAllConsequncesToLHS) :
    rewriteEqualAsTwoInequalities(rewriteEqualAsTwoInequalities),
    moveAllConsequncesToLHS(moveAllConsequncesToLHS) {
    }

    bool rewriteEqualAsTwoInequalities = true;
    bool moveAllConsequncesToLHS = false;
};

void parse(const std::vector<Token>& tokens, Program& program, const ParseConfig& config = ParseConfig());

void parse(std::istream &input, Program& program, const ParseConfig& config = ParseConfig(), bool enableRepeatPowerGroupsHack=false);

#endif //MYPROJECT_PROGRAMPARSER_H
