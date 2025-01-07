//
// Created by sergey on 03.07.23.
//

#ifndef MYPROJECT_TEMPLATEENGINE_H
#define MYPROJECT_TEMPLATEENGINE_H
#include <string>
#include <vector>
#include <map>
#include <set>


class TemplateEngine {
    // the purpose of the class is to substitute $VAR to specific value in the string template

public:
    explicit TemplateEngine(const std::string& templateString) : templateString(templateString) {
        tokenize();

        for (auto& it: tokens) {
            if (it.type == TokenType::var) {
                variablesSet.insert(it.value);
            }
        }
    }

    void addVariableAssignment(const std::string& varName, const std::string& varValue) {
        variablesAssignment[varName] = varValue;
    }

    void addVariableAssignments(const std::map<std::string, std::string>& variables) {
        for (auto& it: variables) {
            addVariableAssignment(it.first, it.second);
        }
    }

    bool isAllVariablesAssigned() {
        for (auto& it: variablesSet) {
            if (variablesAssignment.find(it) == variablesAssignment.end()) {
                return false;
            }
        }
        return true;
    }

    std::string evaluate() {
        std::string result;
        for (auto& it: tokens) {
            if (it.type == TokenType::str) {
                result += it.value;
            } else {
                result += variablesAssignment[it.value];
            }
        }
        return result;
    }

    void printTokens() {
        for (auto& it: tokens) {
            if (it.type == TokenType::str) {
                std::cout << "str: [" << it.value << "]" << std::endl;
            } else {
                std::cout << "var: [" << it.value << "]" << std::endl;
            }
        }
    }

private:
    enum class TokenType {
        str, var
    };

    struct Token {
        TokenType type;
        std::string value;
    };

    std::string templateString;
    std::vector<Token> tokens;

    std::map<std::string, std::string> variablesAssignment;
    std::set<std::string> variablesSet;

    bool isSpace(char c) {
        return c == ' ' || c == '\t' || c == '\n';
    }

    void tokenize() {

        std::string currentToken;
        bool isVariable = false;

        if (templateString.empty()) {
            return;
        }

        if (templateString[0] == '$') {
            isVariable = true;
        } else {
            currentToken += templateString[0];
        }

        for (int i = 1; i < templateString.size(); i++) {
            if (isVariable) {
                if (isSpace(templateString[i])) {
                    tokens.push_back({TokenType::var, currentToken});
                    currentToken = templateString[i];
                    isVariable = false;
                } else {
                    currentToken += templateString[i];
                }
            } else {
                if (templateString[i] == '$') {
                    isVariable = true;
                    tokens.push_back({TokenType::str, currentToken});
                    currentToken = "";
                } else {
                    currentToken += templateString[i];
                }
            }
        }

        if (!currentToken.empty()) {
            tokens.push_back({isVariable ? TokenType::var : TokenType::str, currentToken});
        }

    }

};

#endif //MYPROJECT_TEMPLATEENGINE_H
