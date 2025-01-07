//
// Created by sergey on 07.06.23.
//

#ifndef MYPROJECT_PROGRAMEXPRESSION_H
#define MYPROJECT_PROGRAMEXPRESSION_H

#include <utility>
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <iostream>

#include "symbolicRing.h"

using namespace symbolic_ring;

class SymbolicPolynomialWithOrderedArguments {
public:
SymbolicPolynomialWithOrderedArguments(SymbolicPolynomial symbolicPolynomial, std::vector<std::string> orderedArguments) :
            symbolicPolynomial(symbolicPolynomial),
            orderedArguments(std::move(orderedArguments)) {}

    SymbolicPolynomial getSymbolicPolynomial() const {
        return symbolicPolynomial;
    }

    const std::vector<std::string>& getOrderedArguments() const {
        return orderedArguments;
    }
private:
    SymbolicPolynomial symbolicPolynomial;
    std::vector<std::string> orderedArguments;
};

class EvaluationContext {
public:
    EvaluationContext(SymbolicEnvironment *const environment): environment(environment) {}

    symbolic_ring::SymbolicEnvironment& getEnvironment() {
        return *environment;
    }

    symbolic_ring::QPolynomial getVariableQPolynomial(const std::string& variable) const {
        if (variableValues.find(variable) == variableValues.end()) {
            throw std::runtime_error("Variable " + variable + " not found");
        }
        return variableValues.at(variable);
    }

    symbolic_ring::Symbol getSymobol(const std::string& variable) {
        return environment->getOrCreate(variable);
    }


    SymbolicPolynomialWithOrderedArguments getSymbolicPolynomialWithOrderedArguments(const std::string& functionName) const {
        if (functionValues.find(functionName) == functionValues.end()) {
            throw std::runtime_error("Function " + functionName + "not found");
        }
        return functionValues.at(functionName);
    }

    void setVariableQPolynomial(const std::string& variable, const symbolic_ring::QPolynomial& qPolynomial) {
        variableValues.insert({variable, qPolynomial});
    }

    void setSymbolicPolynomial(const std::string& functionName, const symbolic_ring::SymbolicPolynomial& symbolicPolynomial,
                               const std::vector<std::string>& orderedArguments) {
        functionValues.insert({functionName, {symbolicPolynomial, orderedArguments}});
    }

private:
    SymbolicEnvironment *const environment;

    std::set<std::string> variables;
    std::set<std::string> functions;

    std::map<std::string, symbolic_ring::QPolynomial> variableValues;
    std::map<std::string, SymbolicPolynomialWithOrderedArguments> functionValues;

};


enum NodeType {
    CONSTANT, VARIABLE, FUNCTION, BINARY_OPERATION, UNARY_OPERATION, BINARY_RELATION
};

enum OperationType {
    PLUS, MINUS, MUL, POW, UNARY_MINUS
};

enum PredicateType {
    EQ, LEQ, GEQ, LT, GT
};

enum TypeTag {
    QPOLYNOMIAL, SYMBOLIC_POLYNOMIAL, BOOLEAN // the type of expression we get after evaluating the node with the given context
};

class EvaluationResult {
public:
    EvaluationResult(bool boolean):
        boolean(boolean),
        typeTag(BOOLEAN) {}
    EvaluationResult(const symbolic_ring::QPolynomial& qPolynomial):
        qPolynomial(std::make_unique<symbolic_ring::QPolynomial>(qPolynomial)),
        typeTag(QPOLYNOMIAL) {}
    EvaluationResult(const symbolic_ring::SymbolicPolynomial& symbolicPolynomial):
        symbolicPolynomial(std::make_unique<symbolic_ring::SymbolicPolynomial>(symbolicPolynomial)),
        typeTag(SYMBOLIC_POLYNOMIAL) {}

    TypeTag getTypeTag() const {
        return typeTag;
    }

    const symbolic_ring::QPolynomial &getQPolynomial() const {
        if (typeTag != QPOLYNOMIAL) {
            throw std::runtime_error("Wrong type tag");
        }
        return *qPolynomial;
    }

    const symbolic_ring::SymbolicPolynomial &getSymbolicPolynomial() const {
        if (typeTag != SYMBOLIC_POLYNOMIAL) {
            throw std::runtime_error("Wrong type tag");
        }
        return *symbolicPolynomial;
    }

    bool isBoolean() const {
        if (typeTag != BOOLEAN) {
            throw std::runtime_error("Wrong type tag");
        }
        return boolean;
    }
private:
    TypeTag typeTag;
    std::unique_ptr<symbolic_ring::QPolynomial> qPolynomial;
    std::unique_ptr<symbolic_ring::SymbolicPolynomial> symbolicPolynomial;
    bool boolean;
};

class ExpressionElement {
public:
    ExpressionElement() = default;
    virtual ~ExpressionElement() = default;

    virtual NodeType getType() const = 0;
    virtual std::string toString() const = 0;
    virtual std::string toStringEnd() const = 0;
    virtual int getArity() const = 0;
    virtual const std::vector<std::unique_ptr<ExpressionElement>>& getChildren() const = 0;
    virtual std::string getName() const = 0;
    virtual TypeTag getTypeTag() const = 0;
    virtual EvaluationResult evaluate(EvaluationContext& context) = 0;

    virtual std::unique_ptr<ExpressionElement> clone() const = 0;

};



class Constant : public ExpressionElement {
public:
    explicit Constant(long long val): value(val), children(), stringValue(std::to_string(value)) {}

    NodeType getType() const override {
        return CONSTANT;
    }
    std::string toString() const override {
        return stringValue;
    }
    std::string toStringEnd() const override {
        return "";
    }
    int getArity() const override {
        return 0;
    }
    const std::vector<std::unique_ptr<ExpressionElement>>& getChildren() const override {
        return children;
    }

    std::string getName() const override {
        return stringValue;
    }

    TypeTag getTypeTag() const override {
        return QPOLYNOMIAL;
    }

    EvaluationResult evaluate(EvaluationContext &context)  override {
        return {mul(context.getEnvironment().qPolynomialOne(), value)};
    }

    std::unique_ptr<ExpressionElement> clone() const override {
        return std::make_unique<Constant>(value);
    }

private:
    const long long value;
    const std::string stringValue;

    const std::vector<std::unique_ptr<ExpressionElement>> children;

};

class Variable : public ExpressionElement {
public:
    explicit Variable(std::string variableName): variableName(std::move(variableName)), children()
    {}

    NodeType getType() const override {
        return VARIABLE;
    }

    std::string toString() const override {
        return variableName;
    }

    std::string toStringEnd() const override {
        return "";
    }

    int getArity() const override {
        return 0;
    }

    const std::vector<std::unique_ptr<ExpressionElement>>& getChildren() const override {
        return children;
    }

    std::string getName() const override {
        return variableName;
    }

    TypeTag getTypeTag() const override {
        return QPOLYNOMIAL;
    }

    EvaluationResult evaluate(EvaluationContext &context)  override {
        return {context.getVariableQPolynomial(variableName)};
    }

    std::unique_ptr<ExpressionElement> clone() const override {
        return std::make_unique<Variable>(variableName);
    }

private:
    std::string variableName;
    const std::vector<std::unique_ptr<ExpressionElement>> children;
};




class Function : public ExpressionElement {
public:
    explicit Function(std::vector<std::unique_ptr<ExpressionElement>> &&children, std::string functionName):
    children(std::move(children)),
    functionName(functionName) {}


    Function(Function &&other) noexcept {
        children = std::move(other.children);
        functionName = std::move(other.functionName);
    }
    NodeType getType() const override {
        return FUNCTION;
    }

    std::string toString() const override {
        std::string repr = functionName + "(";
        for (int i = 0; i < children.size(); ++i) {
            repr += children[i]->toString();
            if (i != children.size() - 1) {
                repr += ", ";
            }
        }
        repr += ")";
        return repr;
    }

    std::string toStringEnd() const override {
        return "";
    }

    int getArity() const override {
        return children.size();
    }

    const std::vector<std::unique_ptr<ExpressionElement>> &getChildren() const override {
        return children;
    }

    std::string getName() const override {
        return functionName;
    }

    TypeTag getTypeTag() const override {
        return SYMBOLIC_POLYNOMIAL;
    }

    EvaluationResult evaluate(EvaluationContext &context) override {
        auto env = context.getEnvironment();
        auto ans = context.getSymbolicPolynomialWithOrderedArguments(functionName).getSymbolicPolynomial();
        auto args_ordering = context.getSymbolicPolynomialWithOrderedArguments(functionName).getOrderedArguments();
        for (int i = 0; i < args_ordering.size(); ++i) {
            ans = substituteInBase(ans, context.getSymobol(args_ordering[i]), children[i]->evaluate(context).getQPolynomial());
        }
        return EvaluationResult(ans);
    }

    std::unique_ptr<ExpressionElement> clone() const override {
        std::vector<std::unique_ptr<ExpressionElement>> newChildren;
        for (const auto& child: children) {
            newChildren.push_back(child->clone());
        }
        return std::make_unique<Function>(std::move(newChildren), functionName);
    }

    void setChildren(std::vector<std::unique_ptr<ExpressionElement>> &&newChildren) {
        if (newChildren.size() != children.size()) {
            throw std::runtime_error("Wrong number of children");
        }
        children = std::move(newChildren);
    }

private:
    std::string functionName;
    std::vector<std::unique_ptr<ExpressionElement>> children;
};

class BinaryOperation : public ExpressionElement {
public:
    explicit BinaryOperation(std::unique_ptr<ExpressionElement> &&left, std::unique_ptr<ExpressionElement> &&right,
                             std::string operationName):
    children(),
    operationName(operationName) {
        std::set<std::string> listOfSupportedOperations = {"+", "-", "*", "/"}; // TODO: add pow support, "^"};

        if (listOfSupportedOperations.find(operationName) == listOfSupportedOperations.end()) {
            throw std::runtime_error("Unsupported operation");
        }
        if (operationName == "^") {
            if (right->getType() != CONSTANT) {
                throw std::runtime_error("Power is not constant");
            }
        }
        children.push_back(std::move(left));
        children.push_back(std::move(right));
    }

    NodeType getType() const override {
        return BINARY_OPERATION;
    }

    std::string toString() const override {
        return "(" + children[0]->toString() + " " + operationName + " " + children[1]->toString() + ")";
    }

    std::string toStringEnd() const override {
        return "";
    }

    int getArity() const override {
        return 2;
    }

    const std::vector<std::unique_ptr<ExpressionElement>> &getChildren() const override {
        return children;
    }

    std::string getName() const override {
        return operationName;
    }

    TypeTag getTypeTag() const override {
        if (children[0]->getTypeTag() == SYMBOLIC_POLYNOMIAL || children[1]->getTypeTag() == SYMBOLIC_POLYNOMIAL) {
            return SYMBOLIC_POLYNOMIAL;
        }
        return QPOLYNOMIAL;
    }


    EvaluationResult evaluate(EvaluationContext &context) override {
        auto leftResult = children[0]->evaluate(context);
        auto rightResult = children[1]->evaluate(context);

        auto currentType = this->getTypeTag();
        if (currentType == QPOLYNOMIAL) {
            if (operationName == "+") {
                return {add(leftResult.getQPolynomial(), rightResult.getQPolynomial())};
            } else if (operationName == "-") {
                return {add(leftResult.getQPolynomial(), mul(rightResult.getQPolynomial(), -1))};
            } else if (operationName == "*") {
                return {mul(leftResult.getQPolynomial(), rightResult.getQPolynomial())};
            } else if (operationName == "/") {
                auto monomials = rightResult.getQPolynomial().getMonomials();
                if (monomials.size() != 1) {
                    throw std::runtime_error("Division by non-monomial");
                }
                if (!monomials[0].isConstant()) {
                    throw std::runtime_error("Division by non-constant");
                }
                auto monomial = monomials[0];
                auto leftQPolynomial = leftResult.getQPolynomial();
                auto leftQPolynomislMultEnum = mul(leftQPolynomial, monomial.getDenominator());
                auto leftQPolynomialDivDenom = div(leftQPolynomislMultEnum, monomial.getEnumerator());
                return {leftQPolynomialDivDenom};
            }
        } else if (currentType == SYMBOLIC_POLYNOMIAL) {
            auto leftResultCastToSymbolicPolynomial = context.getEnvironment().symbolicPolynomialZero();
            if (leftResult.getTypeTag() == SYMBOLIC_POLYNOMIAL) {
                leftResultCastToSymbolicPolynomial = leftResult.getSymbolicPolynomial();
            } else if (leftResult.getTypeTag() == QPOLYNOMIAL) {
                leftResultCastToSymbolicPolynomial = symbolicPolynomialfromQPolynomialAsBase(leftResult.getQPolynomial());
            }
            auto rightResultCastToSymbolicPolynomial = context.getEnvironment().symbolicPolynomialZero();
            if (rightResult.getTypeTag() == SYMBOLIC_POLYNOMIAL) {
                rightResultCastToSymbolicPolynomial = rightResult.getSymbolicPolynomial();
            } else if (rightResult.getTypeTag() == QPOLYNOMIAL) {
                rightResultCastToSymbolicPolynomial = symbolicPolynomialfromQPolynomialAsBase(rightResult.getQPolynomial());
            }
            if (operationName == "+") {
                return {add(leftResultCastToSymbolicPolynomial, rightResultCastToSymbolicPolynomial, true)};
            } else if (operationName == "-") {
                return {add(leftResultCastToSymbolicPolynomial, mul(rightResultCastToSymbolicPolynomial, -1), true)};
            } else if (operationName == "*") {
                return {mul(leftResultCastToSymbolicPolynomial, rightResultCastToSymbolicPolynomial)};
            } else if (operationName == "/") {
                auto monomials = rightResult.getQPolynomial().getMonomials();
                if (monomials.size() != 1) {
                    throw std::runtime_error("Division by non-monomial");
                }
                if (!monomials[0].isConstant()) {
                    throw std::runtime_error("Division by non-constant");
                }
                auto monomial = monomials[0];
                return {div(mul(leftResult.getQPolynomial(), monomial.getDenominator()), monomial.getEnumerator())};
            }
        }
        throw std::runtime_error("Cannot evaluate binary operation with type tag " + std::to_string(currentType));
    }

    std::unique_ptr<ExpressionElement> clone() const override {
        return std::make_unique<BinaryOperation>(children[0]->clone(), children[1]->clone(), operationName);
    }

private:

    std::string operationName;
    std::vector<std::unique_ptr<ExpressionElement>> children;
};

class UnaryOperation : public ExpressionElement {
public:
    explicit UnaryOperation(std::unique_ptr<ExpressionElement> &&child, const std::string& operationName):
    children(),
    operationName(operationName) {
        std::set<std::string> listOfSupportedOperations = {"-", "+"};

        if (listOfSupportedOperations.find(operationName) == listOfSupportedOperations.end()) {
            throw std::runtime_error("Unsupported operation");
        }
        children.push_back(std::move(child));
    }

    NodeType getType() const override {
        return UNARY_OPERATION;
    }

    std::string toString() const override {
        return "(" + operationName + children[0]->toString() + ")";
    }

    std::string toStringEnd() const override {
        return "";
    }

    int getArity() const override {
        return 1;
    }

    const std::vector<std::unique_ptr<ExpressionElement>> &getChildren() const override {
        return children;
    }

    std::string getName() const override {
        return operationName;
    }

    TypeTag getTypeTag() const override {
        if (children[0]->getTypeTag() == SYMBOLIC_POLYNOMIAL) {
            return SYMBOLIC_POLYNOMIAL;
        }
        return QPOLYNOMIAL;
    }

    EvaluationResult evaluate(EvaluationContext &context) override {
        auto childResult = children[0]->evaluate(context);

        auto currentType = this->getTypeTag();
        if (currentType == QPOLYNOMIAL) {
            if (operationName == "-") {
                return {mul(childResult.getQPolynomial(), -1)};
            } else if (operationName == "+") {
                return {childResult.getQPolynomial()};
            }
        } else if (currentType == SYMBOLIC_POLYNOMIAL) {
            auto childResultCastToSymbolicPolynomial = context.getEnvironment().symbolicPolynomialZero();
            if (childResult.getTypeTag() == SYMBOLIC_POLYNOMIAL) {
                childResultCastToSymbolicPolynomial = childResult.getSymbolicPolynomial();
            } else if (childResult.getTypeTag() == QPOLYNOMIAL) {
                childResultCastToSymbolicPolynomial = symbolicPolynomialfromQPolynomialAsBase(childResult.getQPolynomial());
            }
            if (operationName == "-") {
                return {mul(childResultCastToSymbolicPolynomial, -1)};
            } else if (operationName == "+") {
                return {childResultCastToSymbolicPolynomial};
            }
        }
        throw std::runtime_error("Cannot evaluate unary operation with type tag " + std::to_string(currentType));
    }

    std::unique_ptr<ExpressionElement> clone() const override {
        return std::make_unique<UnaryOperation>(children[0]->clone(), operationName);
    }

private:
    std::string operationName;
    std::vector<std::unique_ptr<ExpressionElement>> children;
};

class BinaryRelation : public ExpressionElement {
public:
    explicit BinaryRelation(std::unique_ptr<ExpressionElement> &&left, std::unique_ptr<ExpressionElement> &&right,
                            const std::string &operationName) :
            children(),
            operationName(operationName) {
        std::set<std::string> listOfSupportedOperations = {"<", ">", "<=", ">=", "=="};

        if (listOfSupportedOperations.find(operationName) == listOfSupportedOperations.end()) {
            throw std::runtime_error("Unsupported operation");
        }
        children.push_back(std::move(left));
        children.push_back(std::move(right));
    }

    NodeType getType() const override {
        return BINARY_RELATION;
    }

    std::string toString() const override {
        return "(" + children[0]->toString() + " " + operationName + " " + children[1]->toString() + ")";
    }

    std::string toStringEnd() const override {
        return "";
    }

    int getArity() const override {
        return 2;
    }

    const std::vector<std::unique_ptr<ExpressionElement>> &getChildren() const override {
        return children;
    }

    std::string getName() const override {
        return operationName;
    }

    TypeTag getTypeTag() const override {
        return QPOLYNOMIAL;
    }

    EvaluationResult evaluate(EvaluationContext &context) override { // by the evaluation here we mean a symbolic polynomial p which must be >= 0, > 0, = 0
        auto leftResult = children[0]->evaluate(context);
        auto rightResult = children[1]->evaluate(context);

        auto currentType = this->getTypeTag();
        if (currentType == QPOLYNOMIAL) {
            auto leftResultCastToQPolynomial = context.getEnvironment().symbolicPolynomialZero();
            if (leftResult.getTypeTag() == SYMBOLIC_POLYNOMIAL) {
                leftResultCastToQPolynomial = leftResult.getSymbolicPolynomial();
            } else if (leftResult.getTypeTag() == QPOLYNOMIAL) {
                leftResultCastToQPolynomial = symbolicPolynomialfromQPolynomialAsBase(leftResult.getQPolynomial());
            }
            auto rightResultCastToQPolynomial = context.getEnvironment().symbolicPolynomialZero();
            if (rightResult.getTypeTag() == SYMBOLIC_POLYNOMIAL) {
                rightResultCastToQPolynomial = rightResult.getSymbolicPolynomial();
            } else if (rightResult.getTypeTag() == QPOLYNOMIAL) {
                rightResultCastToQPolynomial = symbolicPolynomialfromQPolynomialAsBase(rightResult.getQPolynomial());
            }

            if (operationName == "<") {
                return {add(rightResultCastToQPolynomial, mul(leftResultCastToQPolynomial, -1), true)};
            } else if (operationName == ">") {
                return {add(leftResultCastToQPolynomial, mul(rightResultCastToQPolynomial, -1), true)};
            } else if (operationName == "<=") {
                return {add(rightResultCastToQPolynomial, mul(leftResultCastToQPolynomial, -1), true)};
            } else if (operationName == ">=") {
                return {add(leftResultCastToQPolynomial, mul(rightResultCastToQPolynomial, -1), true)};
            } else if (operationName == "==") {
                return {add(leftResultCastToQPolynomial, mul(rightResultCastToQPolynomial, -1), true)};
            }
        }
        throw std::runtime_error("Cannot evaluate binary relation with type tag " + std::to_string(currentType));
    }

    std::unique_ptr<ExpressionElement> clone() const override {
        return std::make_unique<BinaryRelation>(children[0]->clone(), children[1]->clone(), operationName);
    }

private:
    std::string operationName;
    std::vector<std::unique_ptr<ExpressionElement>> children;
};


inline std::unique_ptr<BinaryRelation> moveToGreaterSide (std::unique_ptr<ExpressionElement> rel) {
    auto left = rel->getChildren()[0]->clone();
    auto right = rel->getChildren()[1]->clone();
    auto operationName = rel->getName();

    if (operationName == "<" || operationName == "<=") {
        auto rhsMinusLhs = std::make_unique<BinaryOperation>(std::move(right), std::move(left), "-");
        auto answer = std::make_unique<BinaryRelation>(std::move(rhsMinusLhs), std::make_unique<Constant>(0), ">=");
        return answer;
    } else if (operationName == ">" || operationName == ">=") {
        auto lhsMinusRhs = std::make_unique<BinaryOperation>(std::move(left), std::move(right), "-");
        auto answer = std::make_unique<BinaryRelation>(std::move(lhsMinusRhs), std::make_unique<Constant>(0), ">=");
        return answer;
    } else if (operationName == "==") {
        throw std::runtime_error("unsupported operation");
    }
    throw std::runtime_error("Cannot move to greater side");
}


#endif //MYPROJECT_PROGRAMEXPRESSION_H