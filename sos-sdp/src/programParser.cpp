//
// Created by sergey on 17.06.23.
//
#include "programParser.h"
#include "program.h"
#include "expressionParser.h"
#include "hacks.h"

//#define debugStream std::cout

//#define PROGRAM_PARSER_DEBUG

#ifdef PROGRAM_PARSER_DEBUG
#define CONDITION true
#else
#define CONDITION false
#endif

#define debugStream if (CONDITION) {} else std::cout


enum ParserStates {
    START,
    REAL,
    REAL_IDENTIFIER_ACTION,
    REAL_COMMA,
    FUN,
    FUN_IDENTIFIER,
    FUN_LEFT_BRACKET,
    FUN_FIRST_ARG,
    FUN_COMMA,
    FUN_SECOND_ARG_ACTION,
    FUN_RIGHT_BRACKET_ONE_ARG_ACTION,
    FUN_RIGHT_BRACKET_TWO_ARGS,
    IFTHEN_ACTION,
    IFTHEN_EXPRIF_READ,
    IFTHEN_EXPRIF_SEMICOLON_ACTION,
    IFTHEN_EXPRIF_CLOSE_BRACKET_ACTION,
    IFTHEN_THEN,
    IFTHEN_EXPRTHEN_READ,
    IFTHEN_EXPRTHEN_SEMICOLON_ACTION,
    IFTHEN_EXPRTHEN_CLOSE_BRACKET_ACTION,
    END
};

std::string parserStateToString(ParserStates state) {
    switch(state) {
        case START:
            return "START";
        case REAL:
            return "REAL";
        case REAL_IDENTIFIER_ACTION:
            return "REAL_IDENTIFIER_ACTION";
        case REAL_COMMA:
            return "REAL_COMMA";
        case FUN:
            return "FUN";
        case FUN_IDENTIFIER:
            return "FUN_IDENTIFIER";
        case FUN_LEFT_BRACKET:
            return "FUN_LEFT_BRACKET";
        case FUN_FIRST_ARG:
            return "FUN_FIRST_ARG";
        case FUN_COMMA:
            return "FUN_COMMA";
        case FUN_SECOND_ARG_ACTION:
            return "FUN_SECOND_ARG_ACTION";
        case FUN_RIGHT_BRACKET_ONE_ARG_ACTION:
            return "FUN_RIGHT_BRACKET_ONE_ARG_ACTION";
        case FUN_RIGHT_BRACKET_TWO_ARGS:
            return "FUN_RIGHT_BRACKET_TWO_ARGS";
        case IFTHEN_ACTION:
            return "IFTHEN_ACTION";
        case IFTHEN_EXPRIF_READ:
            return "IFTHEN_EXPRIF_READ";
        case IFTHEN_EXPRIF_SEMICOLON_ACTION:
            return "IFTHEN_EXPRIF_SEMICOLON_ACTION";
        case IFTHEN_EXPRIF_CLOSE_BRACKET_ACTION:
            return "IFTHEN_EXPRIF_CLOSE_BRACKET_ACTION";
        case IFTHEN_THEN:
            return "IFTHEN_THEN";
        case IFTHEN_EXPRTHEN_READ:
            return "IFTHEN_EXPRTHEN_READ";
        case IFTHEN_EXPRTHEN_SEMICOLON_ACTION:
            return "IFTHEN_EXPRTHEN_SEMICOLON_ACTION";
        case IFTHEN_EXPRTHEN_CLOSE_BRACKET_ACTION:
            return "IFTHEN_EXPRTHEN_CLOSE_BRACKET_ACTION";
        case END:
            return "END";
        default:
            return "UNKNOWN";
    }
}

std::unique_ptr<ExpressionElement> parseExpression(const std::vector<Token>& tokens, Program& p) {
    auto expression = parseExpression(tokens);
    debugStream << "parsed expression: " << expression->toString() << std::endl;
    return expression;
}

std::string errorToken(const Token& token) {
    return "unexpected token: " + token.value + " at line " + std::to_string(token.line);
}

bool exressionToken(const Token& token) {
    return token.type == OPERATION ||
    token.value == "(" || token.value == ")" || token.value == "," || token.type == IDENTIFIER || token.type == NUMBER;
}

std::pair<std::unique_ptr<ExpressionElement>, std::unique_ptr<ExpressionElement>>
splitEqualityToTwoInequalities(std::unique_ptr<ExpressionElement> equality) {
    if (equality->getName() != "==") {
        throw std::runtime_error("splitEqualityToTwoInequalities: equality expected");
    }
    auto lhs1 = equality->getChildren()[0]->clone();
    auto rhs1 = equality->getChildren()[1]->clone();

    auto lhs2 = equality->getChildren()[0]->clone();
    auto rhs2 = equality->getChildren()[1]->clone();

    auto condition1 = std::make_unique<BinaryRelation>(std::move(lhs1), std::move(rhs1), "<=");
    auto condition2 = std::make_unique<BinaryRelation>(std::move(lhs2), std::move(rhs2), ">=");

    return std::make_pair(std::move(condition1), std::move(condition2));
}

const bool ADD_TO_CONDITION = true;
const bool ADD_TO_CONCLUSION = false;

void addConditionToProgram(const std::vector<Token>& tokens, Program& program, bool addToCondition,
                           const ParseConfig& config) {
    auto condition = parseExpression(tokens, program);
    if (config.rewriteEqualAsTwoInequalities && condition->getName() == "==") {
        auto conditions = splitEqualityToTwoInequalities(std::move(condition));
        if (addToCondition) {
            program.getConditions().back().addCondition(std::move(conditions.first));
            program.getConditions().back().addCondition(std::move(conditions.second));
        } else {
            program.getConditions().back().addConclusion(std::move(conditions.first));
            program.getConditions().back().addConclusion(std::move(conditions.second));
        }
    } else {
        if (addToCondition) {
            program.getConditions().back().addCondition(std::move(condition));
        } else {
            program.getConditions().back().addConclusion(std::move(condition));
        }
    }
}

void parse(const std::vector<Token>& tokens, Program& program,  const ParseConfig& config) {
    int tokenPosition = 0;


    auto state = START;
    auto tokenBuffer = std::vector<Token>();
    auto clearBuffer = false;
    auto ignoreToken = false;
    auto ended = false;

    while (tokenPosition < tokens.size()) {
        if (ended) {
            break;
        }

        debugStream << "state: " << parserStateToString(state) << std::endl;
        debugStream << "tokenBuffer: ";
        for (auto& token : tokenBuffer) {
            debugStream << token.value << " ";
        }
        debugStream << std::endl;

        while (tokens[tokenPosition].type == COMMENT) {
            tokenPosition++;
        }

//        tokenBuffer.push_back(tokens[tokenPosition]);
//        tokenPosition++;
        auto lastToken = tokens[tokenPosition];

        switch(state) {
            case START:
                if (lastToken.value == "real") {
                    state = REAL;
                } else if (lastToken.value == "function") {
                    state = FUN;
                    clearBuffer = true;
                } else if (lastToken.value == "if") {
                    state = IFTHEN_ACTION;
                } else if (lastToken.type == _EOF) {
                    state = END;
                    ended = true;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case REAL:
                if (lastToken.type == IDENTIFIER) {
                    state = REAL_IDENTIFIER_ACTION;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case REAL_IDENTIFIER_ACTION:
                program.declareReal(tokenBuffer[tokenBuffer.size() - 1].value);
                tokenBuffer.clear();
                if (lastToken.value == ";") {
                    state = START;
                    clearBuffer = true;
                } else if (lastToken.value == ",") {
                    state = REAL_COMMA;
                    clearBuffer = true;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case REAL_COMMA:
                if (lastToken.type == IDENTIFIER) {
                    state = REAL_IDENTIFIER_ACTION;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case FUN:
                if (lastToken.type == IDENTIFIER) {
                    state = FUN_IDENTIFIER;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case FUN_IDENTIFIER:
                if (lastToken.value == "[") {
                    state = FUN_LEFT_BRACKET;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case FUN_LEFT_BRACKET:
                if (lastToken.type == TokenType::NUMBER) {
                    state = FUN_FIRST_ARG;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case FUN_FIRST_ARG:
                if (lastToken.value == ",") {
                    state = FUN_COMMA;
                } else if (lastToken.value == "]") {
                    state = FUN_RIGHT_BRACKET_ONE_ARG_ACTION;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case FUN_COMMA:
                if (lastToken.type == TokenType::NUMBER) {
                    state = FUN_SECOND_ARG_ACTION;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case FUN_RIGHT_BRACKET_ONE_ARG_ACTION:
                program.declareFunction(tokenBuffer[0].value, std::stoi(tokenBuffer[2].value));
                tokenBuffer.clear();
                if (lastToken.value == ";") {
                    state = START;
                    clearBuffer = true;
                } else if (lastToken.value == ",") {
                    state = FUN;
                    clearBuffer = true;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case FUN_SECOND_ARG_ACTION:
                program.declareFunction(tokenBuffer[0].value, std::stoi(tokenBuffer[2].value), std::stoi(tokenBuffer[4].value));
                tokenBuffer.clear();
                if (lastToken.value == "]") {
                    state = FUN_RIGHT_BRACKET_TWO_ARGS;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case FUN_RIGHT_BRACKET_TWO_ARGS:
                if (lastToken.value == ";") {
                    state = START;
                    clearBuffer = true;
                } else if (lastToken.value == ",") {
                    state = FUN;
                    clearBuffer = true;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case IFTHEN_ACTION:
                program.startEmptyIfThenCondition();
                if (lastToken.value == "{") {
                    state = IFTHEN_EXPRIF_READ;
                    clearBuffer = true;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case IFTHEN_EXPRIF_READ:
                if (exressionToken(lastToken)) {
                    state = IFTHEN_EXPRIF_READ;
                } else if (lastToken.value == "}") {
                    state = IFTHEN_EXPRIF_CLOSE_BRACKET_ACTION;
                    ignoreToken = true;
                } else if (lastToken.value == ";") {
                    state = IFTHEN_EXPRIF_SEMICOLON_ACTION;
                    ignoreToken = true;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case IFTHEN_EXPRIF_SEMICOLON_ACTION:
//                program.getConditions().back().addCondition(parseExpression(tokenBuffer, program));
                addConditionToProgram(tokenBuffer, program, ADD_TO_CONDITION, config);
                tokenBuffer.clear();
                if (exressionToken(lastToken)) {
                    state = IFTHEN_EXPRIF_READ;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case IFTHEN_EXPRIF_CLOSE_BRACKET_ACTION:

//                program.getConditions().back().addCondition(parseExpression(tokenBuffer, program));
                addConditionToProgram(tokenBuffer, program, ADD_TO_CONDITION, config);
                tokenBuffer.clear();
                if (lastToken.value == "=>") {
                    state = IFTHEN_THEN;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case IFTHEN_THEN:
                if (lastToken.value == "{") {
                    state = IFTHEN_EXPRTHEN_READ;
                    clearBuffer = true;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case IFTHEN_EXPRTHEN_READ:
                if (exressionToken(lastToken)) {
                    state = IFTHEN_EXPRTHEN_READ;
                } else if (lastToken.value == "}") {
                    state = IFTHEN_EXPRTHEN_CLOSE_BRACKET_ACTION;
                    ignoreToken = true;
                } else if (lastToken.value == ";") {
                    state = IFTHEN_EXPRTHEN_SEMICOLON_ACTION;
                    ignoreToken = true;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case IFTHEN_EXPRTHEN_SEMICOLON_ACTION:
                //program.getConditions().back().addConclusion(parseExpression(tokenBuffer, program));
                addConditionToProgram(tokenBuffer, program, ADD_TO_CONCLUSION, config);
                tokenBuffer.clear();
                if (exressionToken(lastToken)) {
                    state = IFTHEN_EXPRTHEN_READ;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case IFTHEN_EXPRTHEN_CLOSE_BRACKET_ACTION:
//                program.getConditions().back().addConclusion(parseExpression(tokenBuffer, program));
                addConditionToProgram(tokenBuffer, program, ADD_TO_CONCLUSION, config);
                tokenBuffer.clear();
                if (lastToken.value == "if") {
                    state = IFTHEN_ACTION;
                    clearBuffer = true;
                } else if (lastToken.type == _EOF) {
                    state = END;
                    ended = true;
                } else {
                    throw std::runtime_error(errorToken(lastToken));
                }
                break;
            case END:
                ended = true;
                break;
        }
        if (ended) {
            break;
        }
        if (clearBuffer) {
            tokenBuffer.clear();
            clearBuffer = false;
        } else {
            if (!ignoreToken) {
                tokenBuffer.push_back(lastToken);
            } else {
                ignoreToken = false;
            }
        }


         tokenPosition++;

    }
}

void parse(std::istream &input, Program &program, const ParseConfig& config, bool enableRepeatPowerGroupsHack) {
    auto tokens = tokenize(input);

    if (enableRepeatPowerGroupsHack) {
        tokens = repeatPowerGroupsHack(tokens);
    }

    for (auto& it : tokens) {
        std::cout << "" << it.value << " ";
    }

    std::cout << std::endl;
    parse(tokens, program, config);
}


