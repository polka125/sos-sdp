//
// Created by sergey on 18.06.23.
//

#include "tokenizer.h"

std::string spaceInserter(const std::string& line) {
    using namespace Arith;
    std::string preprocessedLine = surrounder(line, Arith::GEQ, " "); // >=
    preprocessedLine = surrounder(preprocessedLine, Arith::LEQ, " "); // <=
    preprocessedLine = surrounder(preprocessedLine, Arith::GE, " "); // >
    preprocessedLine = surrounder(preprocessedLine, Arith::LE, " "); // <
    preprocessedLine = surrounder(preprocessedLine, Arith::EQ, " "); // ==

    preprocessedLine = surrounder(preprocessedLine, Braces::CURLY_OPEN, " "); // {
    preprocessedLine = surrounder(preprocessedLine, Braces::CURLY_CLOSE, " "); // }
    preprocessedLine = surrounder(preprocessedLine, Braces::ROUND_OPEN, " "); // (
    preprocessedLine = surrounder(preprocessedLine, Braces::ROUND_CLOSE, " "); // )
    preprocessedLine = surrounder(preprocessedLine, Braces::SQUARE_OPEN, " "); // [
    preprocessedLine = surrounder(preprocessedLine, Braces::SQUARE_CLOSE, " "); // ]

    preprocessedLine = surrounder(preprocessedLine, Arith::MULT, " "); // *
    preprocessedLine = surrounder(preprocessedLine, Arith::DIV, " "); // /
    preprocessedLine = surrounder(preprocessedLine, Arith::PLUS, " "); // +
    preprocessedLine = surrounder(preprocessedLine, Arith::MINUS, " "); // -
    preprocessedLine = surrounder(preprocessedLine, Arith::POW, " "); // ^

    preprocessedLine = surrounder(preprocessedLine, Separators::COMMA, " "); // ,
    preprocessedLine = surrounder(preprocessedLine, Separators::SEMICOLON, " "); // ;

    return preprocessedLine;
}

std::string comment_cutter(const std::string& line) {
    std::string preprocessedLine = line;
    size_t commentPos = preprocessedLine.find("//");
    if (commentPos != std::string::npos) {
        preprocessedLine = preprocessedLine.substr(0, commentPos);
    }
    return preprocessedLine;
}
