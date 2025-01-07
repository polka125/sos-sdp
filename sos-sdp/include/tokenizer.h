//
// Created by sergey on 17.06.23.
//

#ifndef MYPROJECT_TOKENIZER_H
#define MYPROJECT_TOKENIZER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "debugTools.h"

enum TokenType {
    IDENTIFIER,
    KEYWORD,
    DELIMETER,
    OPERATION, // + - * ^ = == >= <= > <
    NUMBER,
    COMMENT,
    BRACE,
    _EOF
};


namespace Arith {
    const std::string GEQ = ">=";
    const std::string LEQ = "<=";
    const std::string GE = ">";
    const std::string LE = "<";
    const std::string EQ = "==";
    const std::string ASSIGN = "=";
    const std::string POW = "^";
    const std::string PLUS = "+";
    const std::string MINUS = "-";
    const std::string MULT = "*";
    const std::string DIV = "/";
    const std::string FUNCTION_CALL = "op()";
}

namespace Braces {
    const std::string ROUND_OPEN = "(";
    const std::string ROUND_CLOSE = ")";
    const std::string CURLY_OPEN = "{";
    const std::string CURLY_CLOSE = "}";
    const std::string SQUARE_OPEN = "[";
    const std::string SQUARE_CLOSE = "]";
}

namespace Separators {
    const std::string COMMA = ",";
    const std::string SEMICOLON = ";";
}



struct Token {
    TokenType type;
    std::string value;
    int line;
};



inline std::string surrounder(const std::string& line, const std::string& target, const std::string& surround) {
    std::string surroundedLine = line;
    size_t pos = surroundedLine.find(target);
    size_t targetLength = target.length();

    while (pos != std::string::npos) {
        surroundedLine.insert(pos, surround);
        surroundedLine.insert(pos + targetLength + surround.length(), surround);
        pos = surroundedLine.find(target, pos + targetLength + surround.length() * 2);
    }

    return surroundedLine;
}

std::string spaceInserter(const std::string& line);


std::string comment_cutter(const std::string& line);

inline std::vector<Token> pretokenize(std::istream &iss) {
    std::vector<Token> tokens;
    std::string line;
    int lineNum = 0;
    while (std::getline(iss, line)) {
        lineNum++;
        std::string preprocessedLine = comment_cutter(line);
        preprocessedLine = spaceInserter(preprocessedLine);
        std::istringstream iss_line(preprocessedLine);
        std::string token;

        while (iss_line >> token) {
            Token t;
            t.line = lineNum;

            if (token == "//") {
                t.type = COMMENT;
                t.value = line.substr(line.find(token) + token.length());
                tokens.push_back(t);
                break;
            } else if (token == ";" || token == "," || token == "[" || token == "]" || token == "(" || token == ")" ||
            token == "{" || token == "}") {
                t.type = DELIMETER;
                t.value = token;
            } else if (token == "real" || token == "function" || token == "if" || token == "=>") {
                t.type = KEYWORD;
                t.value = token;
            } else if (token == "+" || token == "-" || token == "*" || token == "/" || token == "^" || token == "=" || token == "==" ||
            token == ">=" || token == "<=" || token == ">" || token == "<") {
                t.type = OPERATION;
                t.value = token;
            } else if (token.find_first_not_of("0123456789.") == std::string::npos) {
                t.type = NUMBER;
                t.value = token;
            } else {
                t.type = IDENTIFIER;
                t.value = token;
            }

            tokens.push_back(t);
        }
    }

    return tokens;
}

// merges tokens with should be one token like > = to >=
inline std::vector<Token> mergeTokens(std::vector<Token> tokens) {
    std::vector<Token> mergedTokens;
    Token t;

    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].value == Arith::GE && tokens[i + 1].value == Arith::ASSIGN) {
            t.type = OPERATION;
            t.value = Arith::GEQ;
            mergedTokens.push_back(t);
            i++;
        } else if (tokens[i].value == Arith::LE && tokens[i + 1].value == Arith::ASSIGN) {
            t.type = OPERATION;
            t.value = Arith::LEQ;
            mergedTokens.push_back(t);
            i++;
        } else if (tokens[i].value == Arith::ASSIGN && tokens[i + 1].value == Arith::ASSIGN) {
            t.type = OPERATION;
            t.value = "==";
            mergedTokens.push_back(t);
            i++;
        } else if (tokens[i].value == Arith::ASSIGN && tokens[i + 1].value == Arith::GE) {
            t.type = KEYWORD;
            t.value = "=>";
            mergedTokens.push_back(t);
            i++;
        } else if (tokens[i].value == Arith::DIV && tokens[i + 1].value == Arith::DIV) {
            t.type = COMMENT;
            t.value = "//";
            mergedTokens.push_back(t);
            i++;
        } else {
            mergedTokens.push_back(tokens[i]);
        }
    }

    return mergedTokens;
}

inline std::vector<Token> tokenize(std::istream &iss) {
    std::vector<Token> tokens = pretokenize(iss);
    auto merged = mergeTokens(tokens);
    auto last_line = merged[merged.size() - 1].line;
    merged.push_back(Token{_EOF, "", last_line});
    return merged;
}


inline void printTokens(const std::vector<Token>& tokens, std::ostream& stream = std::cout) {
    for (const Token& token : tokens) {
        stream << "Type: ";

        switch (token.type) {
            case IDENTIFIER:
                stream << "Identifier";
                break;
            case KEYWORD:
                stream << "Keyword";
                break;
            case DELIMETER:
                stream << "Delimeter";
                break;
            case NUMBER:
                stream << "Number";
                break;
            case COMMENT:
                stream << "Comment";
                break;
            case BRACE:
                stream << "Brace";
                break;
            case OPERATION:
                stream << "Operation";
                break;
            case _EOF:
                stream << "EOF";
                break;
            default:
                stream << "Unknown";
                break;
        }

        stream << ", Value: " << token.value << std::endl;
    }
}

#endif //MYPROJECT_TOKENIZER_H
