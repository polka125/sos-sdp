//
// Created by sergey on 30.06.23.
//

#ifndef MYPROJECT_PYTHONCODEGEN_H
#define MYPROJECT_PYTHONCODEGEN_H

#include <string>
#include "symbolicRing.h"
#include "programExpression.h"
#include "stringRoutines.h"

class PythonCodegen {
public:

    std::string constructFunctonArgByIndex(int index) const {
        check();

        return std::string("check.") + "_function_arg_" + std::to_string(index);
    }

    std::string programExpressionToPysym(const std::unique_ptr<ExpressionElement>& expr) {
        check();

        auto exprType = expr->getType();
        if (exprType == NodeType::BINARY_RELATION) {
            throw std::runtime_error("Binary relation is not supported");
        }

        if (exprType == NodeType::CONSTANT) {
            return expr->toString();
        }

        if (exprType == NodeType::VARIABLE) {
            return "check." + expr->toString();
        }

        if (exprType == NodeType::BINARY_OPERATION) {
            auto lhs = programExpressionToPysym(expr->getChildren()[0]);
            auto rhs = programExpressionToPysym(expr->getChildren()[1]);
            auto op = expr->getName();
            return "((" + lhs + ") " + op + " (" + rhs + "))";
        }

        if (exprType == NodeType::UNARY_OPERATION) {
            auto arg = programExpressionToPysym(expr->getChildren()[0]);
            auto op = expr->getName();
            return "(" + op + "(" + arg + "))";
        }


        if (exprType == NodeType::FUNCTION) {
            auto name = "check." + expr->getName();
            auto& args = expr->getChildren();
            std::vector<std::string> argsStr;
            for (auto& it: args) {
                argsStr.push_back(programExpressionToPysym(it));
            }
            std::stringstream answer;
            answer << name << ".subs({";
            for (int i = 0; i < argsStr.size(); i++) {
                if (i > 0) {
                    answer << ", ";
                }
                answer << constructFunctonArgByIndex(i) << ": " << argsStr[i];
            }
            answer << "})";
            return answer.str();
        }

        throw std::runtime_error("Unknown node type");
    }

    void IAdmitThatThisIsUnsafeAndShouldBeUsedOnlyWithTrusterInput() {
        agreeNotToUseWithUntrustedInput = true;
    }

    void check() const {
        if (!agreeNotToUseWithUntrustedInput) {
            throw std::runtime_error("PythonCodegen is not safe to use with untrusted input");
        }
    }

    // marks that the function is explicitly safe
    void safe() const {
        // the body is intentionally empty
    }

    std::string get_current_indent() const {
        return ind();
    }

    std::string pythonHeader() {
        check();
        // read arguments from /tmp/argumentsadfjnjadflnawgnwq.txt
        std::ifstream argumentsFile("/tmp/argumentsadfjnjadflnawgnwq.txt");
        std::string arguments;
        std::getline(argumentsFile, arguments);
        argumentsFile.close();

        // prepend arguments to the header
        std::string header = '#' + arguments + '\n';
        const char *rest_of_header =
                             "import sys\n"
                             "import sympy as sp\n"
                             "\n"
                             "\n"
                             "EPS_MATRIX_NORM = 1e-6\n"
                             "EPS_EIG = 1e-4\n"
                             "EPS_POLY_COEFF = 1e-3\n"
                             "\n"
                             "# check matrix norm\n"
                             "def check_matrix_symmetric(spmatrix):\n"
                             "    return (spmatrix - spmatrix.T).norm() < EPS_MATRIX_NORM\n"
                             "\n"
                             "\n"
                             "def check_matrix_psd(spmatrix):\n"
                             "    return check_matrix_symmetric(spmatrix) and min(spmatrix.eigenvals(multiple=True)) + EPS_EIG > 0\n"
                             "\n"
                             "\n"
                             "def get_poly_max_coeff(poly):\n"
                             "    return max(map(abs, sp.poly(sp.expand(poly)).coeffs()))\n"
                             "\n"
                             "\n";

        return std::string(header) + std::string(rest_of_header);
    }

    std::string block_of_code(std::vector<std::string> block_of_code) {
        check();

        std::stringstream answer;
        for (auto& it: block_of_code) {
            answer << ind() << it << "\n";
        }
        return answer.str();
    }

    std::string start_function(std::string name, std::vector<std::string> args) {
        check();

        std::stringstream answer;
        answer << ind();
        answer << "def " << name << "(";
        for (int i = 0; i < args.size(); i++) {
            answer << args[i];
            if (i + 1 < args.size()) {
                answer << ", ";
            }
        }
        answer << "):\n";
        indent++;
        return answer.str();
    }

    std::string end_function() {
        check();

        indent--;
        return "";
    }

    std::string strart_if(std::string condition) {
        check();

        std::stringstream answer;
        answer << ind() << "if " << condition << ":\n";
        indent++;
        return answer.str();
    }

    std::string end_if() {
        check();

        indent--;
        return "";
    }

    std::string comment(const std::string& comment) {
        check();

        return ind() + "# " + comment;
    }

    std::string create_sym(const std::string& name) {
        check();

        auto globalAssign =  ind() + std::string("check.") + name + " = " + getSymPyMethod("symbols") + "('" + name + "')";
        return globalAssign;
    }

    std::string assign_sym(const std::string& name, double value) {
        check();

        return ind() + std::string("check.") + name + " = " + doubleToString(value) + "\n"
        + ind() + name + " = " + std::string("check.") + name;
    }

    std::string generate_matrix(const std::string& name, const std::vector<std::vector<double>>& matrix) {
        check();

        std::stringstream answer;
        answer << ind() <<  std::string("check.") + name << " = " << getSymPyMethod("matrices.Matrix") << "([";
        for (int i = 0; i < matrix.size(); i++) {
            answer << "[";
            for (int j = 0; j < matrix[i].size(); j++) {
                answer << doubleToString(matrix[i][j]);
                if (j + 1 < matrix[i].size()) {
                    answer << ", ";
                }
            }
            answer << "]";
            if (i + 1 < matrix.size()) {
                answer << ", ";
            }
        }
        answer << "])";
        return answer.str();
    }

    std::string ifnamemain() {
        check();

        auto ans = ind() + "if __name__ == '__main__':";
        indent++;
        return ans;
    }

    std::string arbitrary_code(const std::string& code) {
        check();

        return ind() + code;
    }

    std::string new_line() {
        check();

        return ind();
    }

    std::string to_str(const SymbolicPolynomial& poly) {
        check();

        std::stringstream poly_str_stream;
        poly_str_stream << poly;
        std::string poly_str = poly_str_stream.str();

        std::map<std::string, std::string> replacement = {
                {"[", "("},
                {"]", ")"},
                {"^", "**"}
        };

        for (auto& it: replacement) {
            std::string from = it.first;
            std::string to = it.second;
            poly_str = replaceAll(poly_str, from, to);
        }
        return poly_str;
    }

    std::string to_str(const QPolynomial& poly) {
        check();

        std::stringstream poly_str_stream;
        poly_str_stream << poly;
        std::string poly_str = poly_str_stream.str();

        std::map<std::string, std::string> replacement = {
                {"[", "("},
                {"]", ")"},
                {"^", "**"}
        };

        for (auto& it: replacement) {
            std::string from = it.first;
            std::string to = it.second;
            poly_str = replaceAll(poly_str, from, to);
        }
        return poly_str;
    }


    std::string assign_poly(const std::string& poly_name, const SymbolicPolynomial& poly) {
        check();

        std::stringstream answer;

        std::string poly_str = to_str(poly);

        answer << ind() << std::string("check.") + poly_name << " = " << poly_str;
        return answer.str();
    }

private:

    std::string ind() const {
        safe();

        if (indent < 0) {
            throw std::runtime_error("Indentation level is negative");
        }

        std::string answer;
        for (int i = 0; i < indent; i++) {
            answer += "    ";
        }
        return answer;
    }

    std::string getSymPyMethod(std::string method) {
        check();

        if (sympy_import_alias.empty()) {
            return method;
        }
        return sympy_import_alias + "." + method;
    }

    std::string sympy_import_alias = "sp";
    std::string numpy_import_alias = "np";

    int indent = 0;

    bool agreeNotToUseWithUntrustedInput = false;

};

#endif //MYPROJECT_PYTHONCODEGEN_H
