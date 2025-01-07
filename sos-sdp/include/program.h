//
// Created by sergey on 17.06.23.
//

#ifndef MYPROJECT_PROGRAM_H
#define MYPROJECT_PROGRAM_H

#include "symbolicRing.h"
#include "programExpression.h"
#include "tokenizer.h"

using namespace symbolic_ring;


class ProgramFunction {
public:
    ProgramFunction(const std::string& name, int arity, int highestDegree) :
    name(name),
    arity(arity),
    highestDegree(highestDegree) {
    }

    ProgramFunction(const std::string& name, int arity): ProgramFunction(name, arity, -1) {
    }

    std::string getName() const {
        return name;
    }
    int getArity() const {
        return arity;
    }

    int getHighestDegree() const {
        return highestDegree;
    }

    bool operator<(const ProgramFunction &rhs) const {
        return name < rhs.name;
    }

    bool operator>(const ProgramFunction &rhs) const {
        return rhs < *this;
    }

    bool operator<=(const ProgramFunction &rhs) const {
        return !(rhs < *this);
    }

    bool operator>=(const ProgramFunction &rhs) const {
        return !(*this < rhs);
    }

    bool operator==(const ProgramFunction &rhs) const {
        return name == rhs.name;
    }

    bool operator!=(const ProgramFunction &rhs) const {
        return !(rhs == *this);
    }

private:
    const std::string name;
    int arity;
    int highestDegree;
};


class ProgramVariable {
public:


    explicit ProgramVariable(const std::string &name) : name(name) {}

    ProgramVariable& operator=(const ProgramVariable& other) {
        if (this != &other) {
            this->name = other.name;
        }
        return *this;
    }

    std::string getName() const {
        return name;
    }

    bool operator<(const ProgramVariable &rhs) const {
        return name < rhs.name;
    }

    bool operator>(const ProgramVariable &rhs) const {
        return rhs < *this;
    }

    bool operator<=(const ProgramVariable &rhs) const {
        return !(rhs < *this);
    }

    bool operator>=(const ProgramVariable &rhs) const {
        return !(*this < rhs);
    }

    bool operator==(const ProgramVariable &rhs) const {
        return name == rhs.name;
    }

    bool operator!=(const ProgramVariable &rhs) const {
        return !(rhs == *this);
    }

private:
    std::string name;
};

class ProgramTable {
public:
    void declareReal(const std::string& name) {
        if (_variables.find(name) != _variables.end()) {
            throw std::runtime_error("variable already declared");
        }
        _variables.insert(std::make_pair(name, ProgramVariable(name)));
    }

    void declareFunction(const std::string& name, int arity, int highestDegree=-1) {
        if (_functions.find(name) != _functions.end()) {
            throw std::runtime_error("function already declared");
        }
        _functions.insert(std::make_pair(name, ProgramFunction(name, arity, highestDegree)));
    }

    bool isFunctionDeclared(const std::string& name) const {
        return _functions.find(name) != _functions.end();
    }

    bool isVariableDeclared(const std::string& name) const {
        return _variables.find(name) != _variables.end();
    }

    std::vector<std::string> getDeclaredVariables() const {
        std::vector<std::string> result;
        for (const auto& pair : _variables) {
            result.push_back(pair.first);
        }
        return result;
    }

    std::vector<std::string> getDeclaredFunctions() const {
        std::vector<std::string> result;
        for (const auto& pair : _functions) {
            result.push_back(pair.first);
        }
        return result;
    }

    ProgramFunction getFunction(const std::string& name) {
        if (!isFunctionDeclared(name)) {
            throw std::runtime_error("function not declared");
        }
        return _functions.at(name);
    }

    ProgramVariable getVariable(const std::string& name) {
        if (!isVariableDeclared(name)) {
            throw std::runtime_error("variable not declared");
        }
        return _variables.at(name);
    }


private:
    std::map<std::string, ProgramVariable> _variables;
    std::map<std::string, ProgramFunction> _functions;

};


class IfThenCondition {
public:
    IfThenCondition() = default;
    IfThenCondition(std::vector<std::unique_ptr<ExpressionElement>> &&conditions,
                    std::vector<std::unique_ptr<ExpressionElement>> &&conclusions) :
                    _conditions(std::move(conditions)),
                    _conclusions(std::move(conclusions)) {
    }

    std::vector<std::unique_ptr<ExpressionElement>>& getConditions()  {
        return _conditions;
    }

    std::vector<std::unique_ptr<ExpressionElement>>& getConclusions() {
        return _conclusions;
    }

    void addCondition(std::unique_ptr<ExpressionElement> &&condition) {
        _conditions.push_back(std::move(condition));
    }

    void addConclusion(std::unique_ptr<ExpressionElement> &&conclusion) {
        _conclusions.push_back(std::move(conclusion));
    }
private:
    std::vector<std::unique_ptr<ExpressionElement>> _conditions;
    std::vector<std::unique_ptr<ExpressionElement>> _conclusions;
};

class Program {
public:
    Program() = default;
    void declareReal(const std::string& name) {
        _table.declareReal(name);
    }
    void declareFunction(const std::string& name, int arity, int highestDegree=-1) {
        _table.declareFunction(name, arity, highestDegree);
    }

    void addIfThenCondition(std::vector<std::unique_ptr<ExpressionElement>> &&conditions,
                            std::vector<std::unique_ptr<ExpressionElement>> &&conclusions) {
        _conditions.emplace_back(std::move(conditions), std::move(conclusions));
    }

    void startEmptyIfThenCondition() {
        _conditions.emplace_back();
    }

    std::vector<IfThenCondition>& getConditions() {
        return _conditions;
    }

    const ProgramTable& getTable()  {
        return _table;
    }

    std::vector<int> getFunctionSignature(const std::string& functionName)  {
        auto arity = _table.getFunction(functionName).getArity();
        auto hdegree = _table.getFunction(functionName).getHighestDegree();
        return {arity, hdegree};
    }

private:
    ProgramTable _table;
    std::vector<IfThenCondition> _conditions;
};


#endif //MYPROJECT_PROGRAM_H
