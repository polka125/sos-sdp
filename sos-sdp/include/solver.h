//
// Created by sergey on 06.06.23.
//

#ifndef MYPROJECT_SOLVER_H
#define MYPROJECT_SOLVER_H

#include "sdpEncoder.h"
#include "symbolicRing.h"
#include "stringRoutines.h"
#include "sdpProblem.h"

#include <iostream>
#include <vector>
#include <fusion.h>
#include <chrono>
#include <fstream>

#include "debugTools.h"


//#define debug_disabled 1

//#define SOLVER_DEBUG 1
//#define SOLVER_PRINT_SYSTEM 1

namespace fus = mosek::fusion;
using namespace monty;

// TODO: move to separate file
inline fus::Variable::t slice(fus::Variable::t X, int d, int j) {
    return
            X->slice(new_array_ptr<int,1>({j,0,0}), new_array_ptr<int,1>({j+1,d,d}))
                    ->reshape(new_array_ptr<int,1>({d,d}));
}


class SolverMosec {

private:
    struct LinearScalarExpression {

        static long long gcdll(long long a, long long b) {
            if (a == 0) {
                return b;
            }
            return gcdll(b % a, a);
        }

        static std::pair<long long, long long> addTwoFractions(std::pair<long long, long long> a, std::pair<long long, long long> b) {
            long long newEnumer = a.first * b.second + b.first * a.second;
            long long newDenom = a.second * b.second;
            long long gcd = gcdll(newEnumer, newDenom);
            return std::make_pair(newEnumer / gcd, newDenom / gcd);
        }

        int getVariableIndexOrCreate(std::string varName) {
            if (variableNameToCoefficient.find(varName) == variableNameToCoefficient.end()) {
                variableNameToCoefficient[varName] = variableNames.size();
                variableNames.push_back(varName);
                coefficients.push_back(std::make_pair(0, 1));
            }
            return variableNameToCoefficient[varName];
        }

        std::vector<std::string> getAllUsedVariables() {
            return variableNames;
        }

        std::pair<long long, long long> getCoefficientByVariableName(std::string varName) {
            return coefficients[variableNameToCoefficient[varName]];
        }

        void addCoeff(std::string varName, long long enumer, long long demon) {
            int varIdx = getVariableIndexOrCreate(varName);
            coefficients[varIdx] = addTwoFractions(coefficients[varIdx], std::make_pair(enumer, demon));
        }

        std::map<std::string, long long> variableNameToCoefficient;
        std::vector<std::string> variableNames;
        std::vector<std::pair<long long, long long>> coefficients;
    };


public:
    SolverMosec(int sos_dim, int linear_var_number, const std::string& instance_name) : sos_dim(sos_dim),
                                                                                                        linear_var_number(linear_var_number) {
//        M = new fus::Model(instance_name);
    }

    ~SolverMosec() {
//        M->dispose();
    }




    int getMaxLvarIndex(const QPolynomial& qpoly) {
        int maxLvarIndex = -1;

        for (const auto& monom: qpoly.getMonomials()) {
            if (monom.isConstant()) {
                continue;
            }
            if (!monom.isLinear()) {
                throw std::runtime_error("Not a linear variable");
            }

            std::string const name = monom.getNameIfLinear();
            if (name[0] != 'l' or name[1] != '_') {
                continue;
            }

            auto tokens = parseLVarName(name);
            maxLvarIndex = std::max(maxLvarIndex, tokens[0]);
        }

        if (maxLvarIndex == -1) {
            throw std::runtime_error("No linear variables");
        }
        return maxLvarIndex;
    }



    void addLinearEqualityConstraint(const QPolynomial& lhs) {
        // TODO(@sergey): add auto remapping of lvars to not waste a space



        double total_rhs = 0;
        linear_vars_name_to_coefficients.emplace_back();
        linearMatrixCoefficients.emplace_back(sos_dim);
        linearScalarCoefficients.emplace_back();




        for (const auto& monom: lhs.getMonomials()) {
            if (monom.isConstant()) {
                total_rhs -= monom.getEnumerator() * 1.0 / monom.getDenominator();
                continue;
            }
            if (!monom.isLinear()) {
                throw std::runtime_error("Not a linear variable");
            }

            std::string const name = monom.getNameIfLinear();


            auto enumer = monom.getEnumerator();
            auto denom = monom.getDenominator();


            if (name[0] != 'l' or name[1] != '_') { // non-constrained variable (e.g. a), will be encoded as a_p - a_n = a, a_p >= 0, a_n >= 0
//                auto x  = std::make_shared<Variable::t>(M->variable(std::string(name)));
//                linear_vars.push_back(x);
//                linear_vars_name_to_var[name] = x;
                linear_vars_name_to_coefficients.back()[name] = {monom.getEnumerator(), monom.getDenominator()};
                linear_vars_names.insert(name);
                linearScalarCoefficients.back().addCoeff(name, enumer, denom);
                continue;
            }


            auto tokens = parseLVarName(name);

            debugStream << "name: " << name << std::endl;


            // TODO(@Sergei) multiply RHS by it
//            if (denom != 1) {
//                throw std::runtime_error("Not a linear variable");
//            }

            debugStream << "\nadding constraint " << tokens[0] << " " << tokens[1] << " " << tokens[2] << " " << enumer << std::endl;
            linearMatrixCoefficients.back().addToMatrix(tokens[0], tokens[1], tokens[2], enumer * 1.0 / denom);
            debugStream << "constraint added\n" << std::endl;

        }
        rhss.push_back(total_rhs);
    }



    bool is_feasible() {
        build();
//        M->setLogHandler([=](const std::string & msg) { std::cout << msg << std::flush; });

        std::cout << "Solver started" << std::endl;
        auto t_start = std::chrono::high_resolution_clock::now();

//        M->writeTask(M->getName() + ".cbf");
//        M->writeTask(M->getName() + ".ptf");

        //        M->writeTask(M->getName() + ".cplex");



//        M->solve();

        // TODO: eliminate double job
        sdpProblemRef->solveWithMosek();

        auto solutionMap = sdpProblemRef->getSolutionAsMap();

        for (auto varName_varValue: solutionMap) {
            std::cout << varName_varValue.first << " " << varName_varValue.second << std::endl;
        }


        auto t_end = std::chrono::high_resolution_clock::now();
        double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end-t_start).count();
        std::cout << "Solved: " << elapsed_time_ms << "ms" << std::endl;

//        M->writeTask("cancellation.ptf");

        return (sdpProblemRef->getModel()->getProblemStatus() == fus::ProblemStatus::PrimalAndDualFeasible) || (sdpProblemRef->getModel()->getProblemStatus() == fus::ProblemStatus::PrimalFeasible);
    }

    void print() {
        for (int displacement = 0; displacement < rhss.size(); displacement++) {
            int sos_number = sos_numbers[displacement];
            for (int i = 0; i < sos_number; i++) {
                for (int j = 0; j < sos_dim; j++) {
                    for (int k = 0; k < sos_dim; k++) {
                        // TODO: uncomment fix
//                        std::cout << (*linearMatrixCoefficients[displacement * sos_number + i])(j, k) << " ";
                    }
                    std::cout << std::endl;
                }
            }
            std::cout << " == " << rhss[displacement] << std::endl;
        }
    }
    std::shared_ptr<ndarray<int,1>> nint(const std::vector<int> &X)    { return new_array_ptr<int>(X); }


    std::map<std::string, double> getSolution2() {
        return sdpProblemRef->getSolutionAsMap();
    }


private:


    void build() {
//        int sos_number_sum = std::accumulate(sos_numbers.begin(), sos_numbers.end(), 0);


        allSosIndicies = std::set<int>();
        sosReindexFlat = std::map<int, int>();
        sosReindexFlat = std::map<int, int>();

        for (auto& linearMatrixCoefficient: linearMatrixCoefficients) {
            for (auto it: linearMatrixCoefficient.sosIndexToIndexInArr) {
                allSosIndicies.insert(it.first);
            }
        }

        int allSosCounter = 0;
        for (auto it: allSosIndicies) {
            sosReindexFlat[it] = allSosCounter;
            sosReindexBack[allSosCounter] = it;
            allSosCounter++;
        }


//        allSosMatrices = M->variable(fus::Domain::inPSDCone(sos_dim, allSosIndicies.size()));

//        fus::Variable::t hackVariable = M->variable(fus::Domain::greaterThan(0.0, 1));
//        fus::Variable::t hackVariable = M->variable(fus::Domain::unbounded(1));


        // TODO(@Sergei) Let's hope it knows how to encode it
//        positiveLinear = std::make_shared<fus::Variable::t>(M->variable(fus::Domain::greaterThan(0.0, 2 * linear_vars_names.size())));
//        positiveLinear = std::make_shared<fus::Variable::t>(M->variable(fus::Domain::unbounded(2 * linear_vars_names.size())));
        std::vector<std::string> linear_vars_names_list;

        for (auto& name: linear_vars_names) {
            linear_vars_name_to_index[name] = linear_vars_names_list.size();
            linear_vars_names_list.push_back(name);
        }


        sdpProblemRef = std::make_unique<SdpProblem>(sos_dim);
        auto& sdpProblem = *sdpProblemRef;

        for (int linearMatrixExpressionIdx = 0; linearMatrixExpressionIdx < linearMatrixCoefficients.size(); linearMatrixExpressionIdx++) {
            sdpProblem.startNewCondition();

            auto& linearMatrixExpression = linearMatrixCoefficients[linearMatrixExpressionIdx];
            auto scalarVariables = linearScalarCoefficients[linearMatrixExpressionIdx].getAllUsedVariables();
            auto constantPart = -rhss[linearMatrixExpressionIdx];

            auto sosIndicies = linearMatrixExpression.getSosIndicies();
            for (int sosIndicie : sosIndicies)  {
                auto currentMatrixCoefficient = linearMatrixExpression.getMatrixBySosIndex(sosIndicie);
                for (int row = 0; row < sos_dim; row++) {
                    for (int col = 0; col < sos_dim; col++) {
                        sdpProblem.addSdpConstrainedVariable(sosIndicie, row, col, (*currentMatrixCoefficient)(row, col));
                    }
                }
            }

            for (auto& scalarVariableName: scalarVariables) {
                auto coeff = linearScalarCoefficients[linearMatrixExpressionIdx].getCoefficientByVariableName(scalarVariableName);
                sdpProblem.addUnconstrainedVariable(scalarVariableName, coeff.first * 1.0 / coeff.second);
            }

            sdpProblem.addConstant(static_cast<double>(constantPart));

            sdpProblem.endCondition(LinearMatrixExpressionType::IN_RANGE, 1e-6);
//            sdpProblem.endCondition(LinearMatrixExpressionType::EQ);
        }

        // TODO: remove
//        sdpProblem.startNewCondition();
//        sdpProblem.addUnconstrainedVariable("_coeff_17_T", 1.0);
//        sdpProblem.addConstant(-0.0000001);
//        sdpProblem.endCondition(LinearMatrixExpressionType::GEQ);

        // add L1 consrtraints for linear variables


#ifdef SOLVER_DEBUG
        std::cout << "system v1" << std::endl;
        sdpProblem.printSystem(std::cout, true);

        std::cout << "\n\n=====================================\n=====================================\n\n";
#endif



    }


    std::vector<int> sos_numbers;

    int sos_dim;
    int linear_var_number;


    struct LinearMatrixExpression {
        explicit LinearMatrixExpression(int sos_dim): sos_dim(sos_dim) {}

        int getOrCreateAndGetIndexInArr(int sosInd) {
            if (sosIndexToIndexInArr.find(sosInd) == sosIndexToIndexInArr.end()) {
                sosIndexToIndexInArr[sosInd] = matrixCoeffitients.size();
                matrixCoeffitients.push_back(std::make_shared<ndarray<double, 2>>(shape(sos_dim, sos_dim)));
            }
            return sosIndexToIndexInArr[sosInd];
        }

        void addToMatrix(int sosInd, int i, int j, double val) {
            if (i == j) {
                (*matrixCoeffitients[getOrCreateAndGetIndexInArr(sosInd)])(i, j) += val;
            } else {
                (*matrixCoeffitients[getOrCreateAndGetIndexInArr(sosInd)])(i, j) += val / 2;
                (*matrixCoeffitients[getOrCreateAndGetIndexInArr(sosInd)])(j, i) += val / 2;
            }
        }

        std::shared_ptr<ndarray<double, 2>> getMatrixBySosIndex(int sosInd) {
            return matrixCoeffitients[sosIndexToIndexInArr[sosInd]];
        }

        std::vector<int> getSosIndicies() {
            std::vector<int> ans;
            for (auto it: sosIndexToIndexInArr) {
                ans.push_back(it.first);
            }
            return ans;
        }

        std::map<int, int> sosIndexToIndexInArr;
        std::vector<std::shared_ptr<ndarray<double, 2>>> matrixCoeffitients;
        int sos_dim;
    };




    std::vector<LinearMatrixExpression> linearMatrixCoefficients;
    std::vector<LinearScalarExpression> linearScalarCoefficients;


    std::vector<std::map<std::string, std::pair<long long, long long>>> linear_vars_name_to_coefficients;
    std::set<std::string> linear_vars_names;
    std::map<std::string, size_t> linear_vars_name_to_index;
    std::shared_ptr<fus::Variable::t> positiveLinear;
    std::vector<double> rhss;

    fus::Variable::t allSosMatrices;
    std::set<int> allSosIndicies;
    std::map<int, int> sosReindexFlat;
    std::map<int, int> sosReindexBack;

    std::unique_ptr<SdpProblem> sdpProblemRef;

};

std::vector<QMonomial> getMonomialVecotor(const QMonomial& x, const QMonomial& y, const int highestPower);

class SolverCsdp {
private:
    struct LinearScalarExpression {

        static long long gcdll(long long a, long long b) {
            if (a == 0) {
                return b;
            }
            return gcdll(b % a, a);
        }

        static std::pair<long long, long long> addTwoFractions(std::pair<long long, long long> a, std::pair<long long, long long> b) {
            long long newEnumer = a.first * b.second + b.first * a.second;
            long long newDenom = a.second * b.second;
            long long gcd = gcdll(newEnumer, newDenom);
            return std::make_pair(newEnumer / gcd, newDenom / gcd);
        }

        int getVariableIndexOrCreate(std::string varName) {
            if (variableNameToCoefficient.find(varName) == variableNameToCoefficient.end()) {
                variableNameToCoefficient[varName] = variableNames.size();
                variableNames.push_back(varName);
                coefficients.push_back(std::make_pair(0, 1));
            }
            return variableNameToCoefficient[varName];
        }

        std::vector<std::string> getAllUsedVariables() {
            return variableNames;
        }

        std::pair<long long, long long> getCoefficientByVariableName(std::string varName) {
            return coefficients[variableNameToCoefficient[varName]];
        }

        void addCoeff(std::string varName, long long enumer, long long demon) {
            int varIdx = getVariableIndexOrCreate(varName);
            coefficients[varIdx] = addTwoFractions(coefficients[varIdx], std::make_pair(enumer, demon));
        }

        std::map<std::string, long long> variableNameToCoefficient;
        std::vector<std::string> variableNames;
        std::vector<std::pair<long long, long long>> coefficients;
    };


public:
    SolverCsdp(int sos_dim, int linear_var_number, const std::string& instance_name) : sos_dim(sos_dim),
                                                                                        linear_var_number(linear_var_number) {
    }

    ~SolverCsdp() {
    }




    int getMaxLvarIndex(const QPolynomial& qpoly) {
        int maxLvarIndex = -1;

        for (const auto& monom: qpoly.getMonomials()) {
            if (monom.isConstant()) {
                continue;
            }
            if (!monom.isLinear()) {
                throw std::runtime_error("Not a linear variable");
            }

            std::string const name = monom.getNameIfLinear();
            if (name[0] != 'l' or name[1] != '_') {
                continue;
            }

            auto tokens = parseLVarName(name);
            maxLvarIndex = std::max(maxLvarIndex, tokens[0]);
        }

        if (maxLvarIndex == -1) {
            throw std::runtime_error("No linear variables");
        }
        return maxLvarIndex;
    }



    void addLinearEqualityConstraint(const QPolynomial& lhs) {
        // TODO(@sergey): add auto remapping of lvars to not waste a space



        double total_rhs = 0;
        linear_vars_name_to_coefficients.emplace_back();
        linearMatrixCoefficients.emplace_back(sos_dim);
        linearScalarCoefficients.emplace_back();




        for (const auto& monom: lhs.getMonomials()) {
            if (monom.isConstant()) {
                total_rhs -= monom.getEnumerator() * 1.0 / monom.getDenominator();
                continue;
            }
            if (!monom.isLinear()) {
                throw std::runtime_error("Not a linear variable");
            }

            std::string const name = monom.getNameIfLinear();


            auto enumer = monom.getEnumerator();
            auto denom = monom.getDenominator();


            if (name[0] != 'l' or name[1] != '_') { // non-constrained variable (e.g. a), will be encoded as a_p - a_n = a, a_p >= 0, a_n >= 0
//                auto x  = std::make_shared<Variable::t>(M->variable(std::string(name)));
//                linear_vars.push_back(x);
//                linear_vars_name_to_var[name] = x;
                linear_vars_name_to_coefficients.back()[name] = {monom.getEnumerator(), monom.getDenominator()};
                linear_vars_names.insert(name);
                linearScalarCoefficients.back().addCoeff(name, enumer, denom);
                continue;
            }


            auto tokens = parseLVarName(name);

            debugStream << "name: " << name << std::endl;


            // TODO(@Sergei) multiply RHS by it
//            if (denom != 1) {
//                throw std::runtime_error("Not a linear variable");
//            }

            debugStream << "\nadding constraint " << tokens[0] << " " << tokens[1] << " " << tokens[2] << " " << enumer << std::endl;
            linearMatrixCoefficients.back().addToMatrix(tokens[0], tokens[1], tokens[2], enumer * 1.0 / denom);
            debugStream << "constraint added\n" << std::endl;

        }
        rhss.push_back(total_rhs);
    }



    bool is_feasible() {
        build();
//        M->setLogHandler([=](const std::string & msg) { std::cout << msg << std::flush; });

        // open file csdp.dat-s for writing
        std::ofstream csdpFile(".csdp.dat-s");
        sdpProblemRef->writeCsdp(csdpFile);
        csdpFile.close();

        // run csdp
        std::string csdpCommand = "csdp .csdp.dat-s .csdp.result";

        std::cout << "Running CSDP" << std::endl;
        std::cout << csdpCommand << std::endl;
        std::system(csdpCommand.c_str());
        std::cout << "CSDP finished" << std::endl;

        // read csdp result
        std::ifstream csdpResultFile(".csdp.result");
        auto answer = sdpProblemRef->readCsdp(csdpResultFile);

        // TODO: increase precision
        sdpProblemRef->setAllowedError(1e-4);
        sdpProblemRef->setSolution(answer.first, answer.second);


        // if not true, setSolution would throw an exception
        return true;
    }

    void print() {
        for (int displacement = 0; displacement < rhss.size(); displacement++) {
            int sos_number = sos_numbers[displacement];
            for (int i = 0; i < sos_number; i++) {
                for (int j = 0; j < sos_dim; j++) {
                    for (int k = 0; k < sos_dim; k++) {
                        // TODO: uncomment fix
//                        std::cout << (*linearMatrixCoefficients[displacement * sos_number + i])(j, k) << " ";
                    }
                    std::cout << std::endl;
                }
            }
            std::cout << " == " << rhss[displacement] << std::endl;
        }
    }
    std::shared_ptr<ndarray<int,1>> nint(const std::vector<int> &X)    { return new_array_ptr<int>(X); }


    std::map<std::string, double> getSolution2() {
        return sdpProblemRef->getSolutionAsMap();
    }


private:


    void build() {
//        int sos_number_sum = std::accumulate(sos_numbers.begin(), sos_numbers.end(), 0);


        allSosIndicies = std::set<int>();
        sosReindexFlat = std::map<int, int>();
        sosReindexFlat = std::map<int, int>();

        for (auto& linearMatrixCoefficient: linearMatrixCoefficients) {
            for (auto it: linearMatrixCoefficient.sosIndexToIndexInArr) {
                allSosIndicies.insert(it.first);
            }
        }

        int allSosCounter = 0;
        for (auto it: allSosIndicies) {
            sosReindexFlat[it] = allSosCounter;
            sosReindexBack[allSosCounter] = it;
            allSosCounter++;
        }


//        allSosMatrices = M->variable(fus::Domain::inPSDCone(sos_dim, allSosIndicies.size()));

//        fus::Variable::t hackVariable = M->variable(fus::Domain::greaterThan(0.0, 1));
//        fus::Variable::t hackVariable = M->variable(fus::Domain::unbounded(1));


        // TODO(@Sergei) Let's hope it knows how to encode it
//        positiveLinear = std::make_shared<fus::Variable::t>(M->variable(fus::Domain::greaterThan(0.0, 2 * linear_vars_names.size())));
//        positiveLinear = std::make_shared<fus::Variable::t>(M->variable(fus::Domain::unbounded(2 * linear_vars_names.size())));
        std::vector<std::string> linear_vars_names_list;

        for (auto& name: linear_vars_names) {
            linear_vars_name_to_index[name] = linear_vars_names_list.size();
            linear_vars_names_list.push_back(name);
        }


        sdpProblemRef = std::make_unique<SdpProblem>(sos_dim);
        auto& sdpProblem = *sdpProblemRef;

        for (int linearMatrixExpressionIdx = 0; linearMatrixExpressionIdx < linearMatrixCoefficients.size(); linearMatrixExpressionIdx++) {
            sdpProblem.startNewCondition();

            auto& linearMatrixExpression = linearMatrixCoefficients[linearMatrixExpressionIdx];
            auto scalarVariables = linearScalarCoefficients[linearMatrixExpressionIdx].getAllUsedVariables();
            auto constantPart = -rhss[linearMatrixExpressionIdx];

            auto sosIndicies = linearMatrixExpression.getSosIndicies();
            for (int sosIndicie : sosIndicies)  {
                auto currentMatrixCoefficient = linearMatrixExpression.getMatrixBySosIndex(sosIndicie);
                for (int row = 0; row < sos_dim; row++) {
                    for (int col = 0; col < sos_dim; col++) {
                        sdpProblem.addSdpConstrainedVariable(sosIndicie, row, col, (*currentMatrixCoefficient)(row, col));
                    }
                }
            }

            for (auto& scalarVariableName: scalarVariables) {
                auto coeff = linearScalarCoefficients[linearMatrixExpressionIdx].getCoefficientByVariableName(scalarVariableName);
                sdpProblem.addUnconstrainedVariable(scalarVariableName, coeff.first * 1.0 / coeff.second);
            }

            sdpProblem.addConstant(static_cast<double>(constantPart));

            sdpProblem.endCondition(LinearMatrixExpressionType::EQ);
        }

        // TODO: remove
//        sdpProblem.startNewCondition();
//        sdpProblem.addUnconstrainedVariable("_coeff_17_T", 1.0);
//        sdpProblem.addConstant(-0.0000001);
//        sdpProblem.endCondition(LinearMatrixExpressionType::GEQ);

        // add L1 consrtraints for linear variables


#ifdef SOLVER_DEBUG
        std::cout << "system v1" << std::endl;
        sdpProblem.printSystem(std::cout, true);

        std::cout << "\n\n=====================================\n=====================================\n\n";
#endif

    }

    std::vector<int> sos_numbers;

    int sos_dim;
    int linear_var_number;

    struct LinearMatrixExpression {
        explicit LinearMatrixExpression(int sos_dim): sos_dim(sos_dim) {}

        int getOrCreateAndGetIndexInArr(int sosInd) {
            if (sosIndexToIndexInArr.find(sosInd) == sosIndexToIndexInArr.end()) {
                sosIndexToIndexInArr[sosInd] = matrixCoeffitients.size();
                matrixCoeffitients.push_back(std::make_shared<ndarray<double, 2>>(shape(sos_dim, sos_dim)));
            }
            return sosIndexToIndexInArr[sosInd];
        }

        void addToMatrix(int sosInd, int i, int j, double val) {
            if (i == j) {
                (*matrixCoeffitients[getOrCreateAndGetIndexInArr(sosInd)])(i, j) += val;
            } else {
                (*matrixCoeffitients[getOrCreateAndGetIndexInArr(sosInd)])(i, j) += val / 2;
                (*matrixCoeffitients[getOrCreateAndGetIndexInArr(sosInd)])(j, i) += val / 2;
            }
        }

        std::shared_ptr<ndarray<double, 2>> getMatrixBySosIndex(int sosInd) {
            return matrixCoeffitients[sosIndexToIndexInArr[sosInd]];
        }

        std::vector<int> getSosIndicies() {
            std::vector<int> ans;
            for (auto it: sosIndexToIndexInArr) {
                ans.push_back(it.first);
            }
            return ans;
        }

        std::map<int, int> sosIndexToIndexInArr;
        std::vector<std::shared_ptr<ndarray<double, 2>>> matrixCoeffitients;
        int sos_dim;
    };




    std::vector<LinearMatrixExpression> linearMatrixCoefficients;
    std::vector<LinearScalarExpression> linearScalarCoefficients;


    std::vector<std::map<std::string, std::pair<long long, long long>>> linear_vars_name_to_coefficients;
    std::set<std::string> linear_vars_names;
    std::map<std::string, size_t> linear_vars_name_to_index;

    // TODO: remove csdp dependecy on fusion
    std::shared_ptr<fus::Variable::t> positiveLinear;
    std::vector<double> rhss;

    fus::Variable::t allSosMatrices;
    std::set<int> allSosIndicies;
    std::map<int, int> sosReindexFlat;
    std::map<int, int> sosReindexBack;

    std::unique_ptr<SdpProblem> sdpProblemRef;

};



#endif //MYPROJECT_SOLVER_H
