//
// Created by sergey on 25.06.23.
//

// takes a program as an input and produces possible solution

#ifndef MYPROJECT_AUTOMAITCCOMPLEXITYESTIMATOR_H
#define MYPROJECT_AUTOMAITCCOMPLEXITYESTIMATOR_H

// #define AUCOES_DEBUG

#include "program.h"
#include "solver.h"
#include "debugTools.h"
#include "combinatorics.h"
#include "symbolicRing.h"
#include "pythonCodeGen.h"
#include "stringRoutines.h"

enum class Feasibility {
    FEASIBLE,
    INFEASIBLE,
    UNKNOWN
};

enum class AlgorithmFamily {
    FARKAS,
    HANDELMAN,
    PUTINAR,
    NONE
};

class SolverConfig {
public:
    SolverConfig() {
        method_ = AlgorithmFamily::NONE;
    }

    explicit SolverConfig(AlgorithmFamily method) : method_(method) {
    }

    void setMethod(AlgorithmFamily method) {
        if (method == AlgorithmFamily::NONE) {
            throw std::runtime_error("Method should be specified");
        }
        if (method == AlgorithmFamily::PUTINAR) {
            method_ = AlgorithmFamily::PUTINAR;
            return;
        }
        throw std::runtime_error("Not implemented");
    }

    void setHighMonomialDegree(int hdegree) {
        if (hdegree < 0) {
            throw std::runtime_error("High monomial degree should be non-negative");
        }
        highMonomialDegree_ = hdegree;
    }

    int getHighMonomialDegree() const {
        return highMonomialDegree_;
    }

    void setAddAdditionalOneGeqZero(bool add) {
        addAdditionalOneGeqZero_ = add;
    }

    bool getNeedAdditionalOneGeqZero() const {
        return addAdditionalOneGeqZero_;
    }

    AlgorithmFamily method() const {
        return method_;
    }
private:
    int highMonomialDegree_ = -1;
    AlgorithmFamily method_;
    bool addAdditionalOneGeqZero_ = true;
};


class Solution {
public:
    std::map<std::string, QPolynomial> solution() const {
        return solution_;
    }
private:
    std::map<std::string, QPolynomial> solution_;
};

class ComplexityEstimator {
public:

    explicit ComplexityEstimator(Program& program, std::string instanceName="") : program_(program), instanceName_(instanceName) {
    }


    void configure(const SolverConfig& config = SolverConfig(AlgorithmFamily::PUTINAR)) {
        config_ = config;

        if (config_.method() == AlgorithmFamily::PUTINAR) {
            return;
        }
        throw std::runtime_error("Not implemented: " + std::to_string(static_cast<int>(config_.method())) + "");
    }

    Solution get() {
        return solution_;
    }

private:

    struct SolverMosecParams {
        int sos_dim;
        int linear_var_number;
        std::string instance_name;
    };

    SolverMosecParams getSolverParamsPutinarMosec() {

        throw std::runtime_error("Not implemented 68878");
    }

    // TODO: extract it to separate file as a factory method
    SymbolicPolynomial createSymbolicPolynomial(const std::string& functionName,
                                                const std::vector<QMonomial>& arguments,
                                                SymbolicEnvironment& env) {
        const auto& functionSignature = program_.getFunctionSignature(functionName);
        const int functionArity = functionSignature[0];
        const int functionDegree = functionSignature[1];

        if (functionSignature.size() > 2) {
            throw std::runtime_error("Function signature should have no more than two arguments");
        }

        std::vector<std::vector<int>> allCombinations;
        std::vector<int> current(functionArity, 0);
        do {
            allCombinations.push_back(current);
        } while (getNextVectorBoundedSum(current, functionDegree));

        std::vector<QMonomial> monomials;
        for (const auto& combination : allCombinations) {
            QMonomial monomial = env.qmonomialOne();
            for (int i = 0; i < functionArity; i++) {
                for (int power = 0; power < combination[i]; power++)
                monomial = mul(monomial, arguments[i]);
            }
            monomials.push_back(monomial);
        }

        std::vector<SymbolicPolynomial> monomialsSym;

        for (const auto& it : monomials) {
            monomialsSym.push_back(SymbolicPolynomial(SymbolicMonomial(it)));
        }

        std::vector<SymbolicPolynomial> symbolicCoeffitients;

        auto vecToStr = [](const std::vector<int>& vec) {
            std::string res;
            for (const auto& it : vec) {
                res += std::to_string(it) + "_";
            }
            return res;
        };

        for (int i = 0; i < monomials.size(); i++) {
            auto coeff = env.sym("_coeff_" + vecToStr(allCombinations[i]) + functionName);
            auto coeff_sym = SymbolicPolynomial(SymbolicMonomial(QPolynomial(coeff)));
            symbolicCoeffitients.push_back(coeff_sym);
        }

        SymbolicPolynomial res = env.symbolicPolynomialZero();

        for (int i = 0; i < monomials.size(); i++) {
            const auto& lhs = symbolicCoeffitients[i];
            const auto& rhs = monomialsSym[i];
            res = symbolic_ring::add(res, symbolic_ring::mul(lhs, rhs), true);
        }
        return res;
    }

public:
    void solveWithHandelmanCsdp(int highMonomialDegree) {
        // setting up the context
        auto& programTable = program_.getTable();
        auto& ifthenConditions = program_.getConditions();

//        codegen.IAdmitThatThisIsUnsafeAndShouldBeUsedOnlyWithTrusterInput();

        // move everything to the lhs
        for (auto& ifThen: ifthenConditions) {
            auto& conditioins = ifThen.getConditions();

            for (auto & conditioin : conditioins) {
                conditioin = moveToGreaterSide(conditioin->clone());
            }

            auto& conclusions = ifThen.getConclusions();
            for (auto& conclusion: conclusions) {
                conclusion = moveToGreaterSide(conclusion->clone());
            }

        }

        for (auto& ifThen: ifthenConditions) {
            auto& conditions = ifThen.getConditions();
            if (conditions.empty()) {
                continue;
            }

            std::vector<std::unique_ptr<ExpressionElement>> newConditions;
            std::vector<std::vector<int>> powers;
            std::vector<int> degrees(conditions.size(), 0);

            // generate all possible combinations of powers with sum >= 2 and <= highMonomialDegree
            do {
                // check sum >= 2
                int sum = 0;
                for (auto& it: degrees) {
                    sum += it;
                }
                if (sum < 2) {
                    continue;
                }
                powers.push_back(degrees);
            } while (getNextVectorBoundedSum(degrees, highMonomialDegree));

            for (const auto& power: powers) {
                std::unique_ptr<ExpressionElement> newTerm = std::make_unique<Constant>(1);
                for (int condIdx = 0; condIdx < power.size(); condIdx++) {
                    for (int powerIdx = 0; powerIdx < power[condIdx]; powerIdx++) {
                        newTerm = std::make_unique<BinaryOperation>(std::move(newTerm), conditions[condIdx]->getChildren()[0]->clone(), "*");
                    }
                }
                newTerm = std::make_unique<BinaryRelation>(std::move(newTerm), std::make_unique<Constant>(0), ">=");
                newConditions.push_back(std::move(newTerm));
            }

            // add new conditions
            for (auto& newCondition: newConditions) {
                conditions.push_back(std::move(newCondition));
            }
        }

        // add artificial "1 >= 0" condition
        //TODO: generate monomid
        if (config_.getNeedAdditionalOneGeqZero()) {
            for (auto& ifThen: ifthenConditions) {
                ifThen.addCondition(std::make_unique<BinaryRelation>(std::make_unique<Constant>(1), std::make_unique<Constant>(0), ">="));
            }
        }


#ifdef AUCOES_DEBUG
        std::cout << "\n===========================\nIf-then conditions, not evaluated:\n";
        for (int condIdx = 0; condIdx < ifthenConditions.size(); condIdx++) {
            for (int conclIdx = 0; conclIdx < ifthenConditions[condIdx].getConclusions().size(); conclIdx++) {
                std::cout << "Condition #" << condIdx + 1 << " conclusion #" << conclIdx + 1 << ": " << std::endl;
                std::cout << "Conditions:" << std::endl;
                for (auto& cond: ifthenConditions[condIdx].getConditions()) {
//                    std::cout << cond->toString() << std::endl;
                    std::cout << codegen.programExpressionToPysym(cond->getChildren()[0]) << std::endl;
                }
                std::cout << "Conclusion:" << std::endl;
//                std::cout << ifthenConditions[condIdx].getConclusions()[conclIdx]->toString() << std::endl;

                std::cout << codegen.programExpressionToPysym(
                        ifthenConditions[condIdx].getConclusions()[conclIdx]->
                            getChildren()[0]) << std::endl;
            }
        }
        std::cout << "\n===========================\n";
#endif


        auto env = SymbolicEnvironment();
        auto ctx = EvaluationContext(&env);

        allRationalVariablesNames = programTable.getDeclaredVariables();
        const auto& functions = programTable.getDeclaredFunctions();

        std::map<std::string, QPolynomial> varToPolynomial;
        for (const auto& varName : allRationalVariablesNames) {
            varToPolynomial.insert({varName, QPolynomial(env.sym(varName))});
        }

        int maxArgNumber = 0;
        for (const auto& functionName : functions) {
            maxArgNumber = std::max(maxArgNumber, program_.getFunctionSignature(functionName)[0]);
        }
        std::vector<QMonomial> functionArguments;
        std::vector<std::string> functionArguments_names;

        for (int i = 0; i < maxArgNumber; i++) {
            functionArguments.push_back(QMonomial(env.sym("_function_arg_" + std::to_string(i))));
            functionArguments_names.push_back("_function_arg_" + std::to_string(i));
        }


        //// create functions from its signature
//        std::map<std::string, SymbolicPolynomial> functionNameToSymbolicPolynomial;

        for (const auto& functionName: functions) {
            functionNameToSymbolicPolynomial.insert({functionName, createSymbolicPolynomial(functionName, functionArguments, env)});
        }



#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll registered functions: \n";
        for (auto& it: functionNameToSymbolicPolynomial) {
            std::cout << "Name: " << it.first << std::endl;
            std::cout << "Polynomial: " << it.second << std::endl;
        }
        std::cout << "End of registered functions\n================================\n";
#endif

        //// register function to ctx
        for (const auto& functionName : functions) {
            ctx.setSymbolicPolynomial(
                    functionName,
                    functionNameToSymbolicPolynomial.at(functionName),
                    std::vector<std::string>(
                            functionArguments_names.begin(),
                            functionArguments_names.begin() + program_.getFunctionSignature(functionName)[0]));
        }

        //// create all rational variables
        allRationalVariables;

        for (const auto& varName : allRationalVariablesNames) {
            allRationalVariables.push_back(QMonomial(env.getOrCreate(varName)));
        }

        //// register all variables to ctx
        for (int i = 0; i < allRationalVariablesNames.size(); i++) {
            ctx.setVariableQPolynomial(allRationalVariablesNames[i], allRationalVariables[i]);
        }



        // using the context to evaluate all if-then conditions
        struct IfThenConditionSymbolic {
            std::vector<SymbolicPolynomial> conditions;
            std::vector<SymbolicPolynomial> conclusions;
        };

        auto pritntIfThenCondition = [](const IfThenConditionSymbolic& condition, std::string pref = "") {
            std::cout << pref << "Conditions: (" << condition.conditions.size() << ") \n";
            int cnt = 0;
            for (const auto& it : condition.conditions) {
                std::cout << pref << ++cnt << "/" << condition.conditions.size() << ": " << it << std::endl;
            }
            std::cout << std::endl;
            std::cout << pref << "Conclusions: (" << condition.conclusions.size() << ") \n";
            cnt = 0;
            for (const auto& it : condition.conclusions) {
                std::cout << pref << ++cnt << "/" << condition.conclusions.size() << ": " << it << std::endl;
            }
        };

        std::vector<IfThenConditionSymbolic> ifThenConditionsSymbolic;

        for (auto& condition : ifthenConditions) {
            IfThenConditionSymbolic conditionSymbolic;
            for (auto& it : condition.getConditions()) {
                conditionSymbolic.conditions.push_back(it->evaluate(ctx).getSymbolicPolynomial());
            }
            for (auto& it : condition.getConclusions()) {
                conditionSymbolic.conclusions.push_back(it->evaluate(ctx).getSymbolicPolynomial());
            }


            ifThenConditionsSymbolic.push_back(conditionSymbolic);
        }

#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll registered if-then conditions: \n";
        int cnt = 0;
        for (auto& it: ifThenConditionsSymbolic) {
            std::cout << "\n[" << ++cnt << "/" << ifThenConditionsSymbolic.size() << "]: \n";
            pritntIfThenCondition(it, "\t");
        }
        std::cout << "End of registered if-then conditions\n================================\n";
#endif



        // create monomials for sos
        std::vector<std::vector<int>> allCombinations;
        auto current = std::vector<int>(allRationalVariablesNames.size(), 0);
        allCombinations.push_back(current); // patching to have sos with degree 0
//        do {
//            allCombinations.push_back(current);
//            for (auto& it : current) {
//                std::cout << it << " ";
//            }
//            std::cout << std::endl;
//        } while (getNextVectorBoundedSum(current, config_.getHighMonomialDegree()));


        sosMonomials;

        for (const auto& combination : allCombinations) {
            QMonomial monomial = env.qmonomialOne();
            for (int i = 0; i < allRationalVariablesNames.size(); i++) {
                for (int power = 0; power < combination[i]; power++)
                    monomial = symbolic_ring::mul(monomial, allRationalVariables[i]);
            }
            sosMonomials.push_back(monomial);
        }

#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll sos monomials: \n";
        for (auto& it: sosMonomials) {
            std::cout << it << std::endl;
        }
        std::cout << "End of sos monomials\n================================\n";
#endif

        std::vector<std::vector<SymbolicPolynomial>> sosPolynomials;
        std::vector<std::vector<SymbolicPolynomial>> symbolicPolynoimalRepresentation;


        // for each if-then condition create #conditions * #conclusion soses
        int sosCounter = -1;
        for (auto& ifThen: ifThenConditionsSymbolic) {
            sosPolynomials.emplace_back();
            symbolicPolynoimalRepresentation.emplace_back();

            for (auto ifThenConclusion: ifThen.conclusions) {
                auto currentSymbolicPolynomialEncodingIfThen = env.symbolicPolynomialZero();
                for (auto ifThenCondition: ifThen.conditions) {
                    sosCounter += 1;
                    std::cout << "getting sos: " << sosCounter << std::endl;
                    auto toPush = getSos(sosMonomials, sosCounter);
                    std::cout << "sos received." << std::endl;

                    sosPolynomials.back().push_back(toPush);
                    currentSymbolicPolynomialEncodingIfThen =
                            symbolic_ring::add(currentSymbolicPolynomialEncodingIfThen,
                                               symbolic_ring::mul(toPush, ifThenCondition), false);
                }
                currentSymbolicPolynomialEncodingIfThen = symbolic_ring::add(
                        currentSymbolicPolynomialEncodingIfThen,
                        symbolic_ring::mul(ifThenConclusion, -1), false);
                currentSymbolicPolynomialEncodingIfThen.reduce();
                symbolicPolynoimalRepresentation.back().push_back(currentSymbolicPolynomialEncodingIfThen);
            }
        }

#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll sos polynomials: \n";
        for (auto& it: sosPolynomials) {
            for (auto& it2: it) {
                std::cout << it2 << std::endl;
            }
        }
        std::cout << "End of sos polynomials\n================================\n";
#endif

#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll symbolic polynomials: \n";
        for (auto& it: symbolicPolynoimalRepresentation) {
            for (auto& it2: it) {
                std::cout << it2 << std::endl;
            }
        }
        std::cout << "End of symbolic polynomials\n================================\n";
#endif

        std::vector<QPolynomial> linearPolynomialsShouldBeZero;
        for (int i = 0; i < symbolicPolynoimalRepresentation.size(); i++) {
            for (int j = 0; j < symbolicPolynoimalRepresentation[i].size(); j++) {
                auto allMonomials = symbolicPolynoimalRepresentation[i][j].getReducedMonomials();
                // iterating over monomials and setting the coefficients to zero
                for (auto& it: allMonomials) {
                    linearPolynomialsShouldBeZero.push_back(it.getQcoefficient());
                }
            }
        }

#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll linear polynomials that should be zero: \n";
        for (auto& it: linearPolynomialsShouldBeZero) {
            std::cout << it << std::endl;
        }
        std::cout << "End of all linear polynomials that should be zero\n================================\n";
#endif


        solverCsdp_ = std::make_unique<SolverCsdp>(sosMonomials.size(), 0, instanceName_ + std::to_string(config_.getHighMonomialDegree()));

        int dbg_cnt = 0;
        for (auto&it : linearPolynomialsShouldBeZero) {
#ifdef AUCOES_DEBUG
            std::cout << "Adding(3): " << it << std::endl;
            dbg_cnt += 1;
#endif
            solverCsdp_->addLinearEqualityConstraint(it);
        }


        auto feasibility = solverCsdp_->is_feasible();
        hasSolution = feasibility;

        if (feasibility) {
            is_feasible_ = Feasibility::FEASIBLE;
        } else {
            is_feasible_ = Feasibility::INFEASIBLE;
        }


        std::cout << "The system is feasible: " << (feasibility ? "YES" : "NO") << std::endl;

        if (feasibility) {
            auto sol = solverCsdp_->getSolution2();
            for (auto& it: sol) {
                std::cout << it.first << " " << it.second << std::endl;
            }
        }


        solution = solverCsdp_->getSolution2();
    }


    void solveWithHandelmanMosek(int highMonomialDegree) {
        // setting up the context
        auto& programTable = program_.getTable();
        auto& ifthenConditions = program_.getConditions();

//        codegen.IAdmitThatThisIsUnsafeAndShouldBeUsedOnlyWithTrusterInput();

        // move everything to the lhs
        for (auto& ifThen: ifthenConditions) {
            auto& conditioins = ifThen.getConditions();

            for (auto & conditioin : conditioins) {
                conditioin = moveToGreaterSide(conditioin->clone());
            }

            auto& conclusions = ifThen.getConclusions();
            for (auto& conclusion: conclusions) {
                conclusion = moveToGreaterSide(conclusion->clone());
            }

        }

        for (auto& ifThen: ifthenConditions) {
            auto& conditions = ifThen.getConditions();
            if (conditions.empty()) {
                continue;
            }

            std::vector<std::unique_ptr<ExpressionElement>> newConditions;
            std::vector<std::vector<int>> powers;
            std::vector<int> degrees(conditions.size(), 0);

            // generate all possible combinations of powers with sum >= 2 and <= highMonomialDegree
            do {
                // check sum >= 2
                int sum = 0;
                for (auto& it: degrees) {
                    sum += it;
                }
                if (sum < 2) {
                    continue;
                }
                powers.push_back(degrees);
            } while (getNextVectorBoundedSum(degrees, highMonomialDegree));

            for (const auto& power: powers) {
                std::unique_ptr<ExpressionElement> newTerm = std::make_unique<Constant>(1);
                for (int condIdx = 0; condIdx < power.size(); condIdx++) {
                    for (int powerIdx = 0; powerIdx < power[condIdx]; powerIdx++) {
                        newTerm = std::make_unique<BinaryOperation>(std::move(newTerm), conditions[condIdx]->getChildren()[0]->clone(), "*");
                    }
                }
                newTerm = std::make_unique<BinaryRelation>(std::move(newTerm), std::make_unique<Constant>(0), ">=");
                newConditions.push_back(std::move(newTerm));
            }

            // add new conditions
            for (auto& newCondition: newConditions) {
                conditions.push_back(std::move(newCondition));
            }
        }

        // add artificial "1 >= 0" condition
        //TODO: generate monomid
        if (config_.getNeedAdditionalOneGeqZero()) {
            for (auto& ifThen: ifthenConditions) {
                ifThen.addCondition(std::make_unique<BinaryRelation>(std::make_unique<Constant>(1), std::make_unique<Constant>(0), ">="));
            }
        }


#ifdef AUCOES_DEBUG
        std::cout << "\n===========================\nIf-then conditions, not evaluated:\n";
        for (int condIdx = 0; condIdx < ifthenConditions.size(); condIdx++) {
            for (int conclIdx = 0; conclIdx < ifthenConditions[condIdx].getConclusions().size(); conclIdx++) {
                std::cout << "Condition #" << condIdx + 1 << " conclusion #" << conclIdx + 1 << ": " << std::endl;
                std::cout << "Conditions:" << std::endl;
                for (auto& cond: ifthenConditions[condIdx].getConditions()) {
//                    std::cout << cond->toString() << std::endl;
                    std::cout << codegen.programExpressionToPysym(cond->getChildren()[0]) << std::endl;
                }
                std::cout << "Conclusion:" << std::endl;
//                std::cout << ifthenConditions[condIdx].getConclusions()[conclIdx]->toString() << std::endl;

                std::cout << codegen.programExpressionToPysym(
                        ifthenConditions[condIdx].getConclusions()[conclIdx]->
                            getChildren()[0]) << std::endl;
            }
        }
        std::cout << "\n===========================\n";
#endif


        auto env = SymbolicEnvironment();
        auto ctx = EvaluationContext(&env);

        allRationalVariablesNames = programTable.getDeclaredVariables();
        const auto& functions = programTable.getDeclaredFunctions();

        std::map<std::string, QPolynomial> varToPolynomial;
        for (const auto& varName : allRationalVariablesNames) {
            varToPolynomial.insert({varName, QPolynomial(env.sym(varName))});
        }

        int maxArgNumber = 0;
        for (const auto& functionName : functions) {
            maxArgNumber = std::max(maxArgNumber, program_.getFunctionSignature(functionName)[0]);
        }
        std::vector<QMonomial> functionArguments;
        std::vector<std::string> functionArguments_names;

        for (int i = 0; i < maxArgNumber; i++) {
            functionArguments.push_back(QMonomial(env.sym("_function_arg_" + std::to_string(i))));
            functionArguments_names.push_back("_function_arg_" + std::to_string(i));
        }


        //// create functions from its signature
//        std::map<std::string, SymbolicPolynomial> functionNameToSymbolicPolynomial;

        for (const auto& functionName: functions) {
            functionNameToSymbolicPolynomial.insert({functionName, createSymbolicPolynomial(functionName, functionArguments, env)});
        }



#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll registered functions: \n";
        for (auto& it: functionNameToSymbolicPolynomial) {
            std::cout << "Name: " << it.first << std::endl;
            std::cout << "Polynomial: " << it.second << std::endl;
        }
        std::cout << "End of registered functions\n================================\n";
#endif

        //// register function to ctx
        for (const auto& functionName : functions) {
            ctx.setSymbolicPolynomial(
                    functionName,
                    functionNameToSymbolicPolynomial.at(functionName),
                    std::vector<std::string>(
                            functionArguments_names.begin(),
                            functionArguments_names.begin() + program_.getFunctionSignature(functionName)[0]));
        }

        //// create all rational variables
        allRationalVariables;

        for (const auto& varName : allRationalVariablesNames) {
            allRationalVariables.push_back(QMonomial(env.getOrCreate(varName)));
        }

        //// register all variables to ctx
        for (int i = 0; i < allRationalVariablesNames.size(); i++) {
            ctx.setVariableQPolynomial(allRationalVariablesNames[i], allRationalVariables[i]);
        }



        // using the context to evaluate all if-then conditions
        struct IfThenConditionSymbolic {
            std::vector<SymbolicPolynomial> conditions;
            std::vector<SymbolicPolynomial> conclusions;
        };

        auto pritntIfThenCondition = [](const IfThenConditionSymbolic& condition, std::string pref = "") {
            std::cout << pref << "Conditions: (" << condition.conditions.size() << ") \n";
            int cnt = 0;
            for (const auto& it : condition.conditions) {
                std::cout << pref << ++cnt << "/" << condition.conditions.size() << ": " << it << std::endl;
            }
            std::cout << std::endl;
            std::cout << pref << "Conclusions: (" << condition.conclusions.size() << ") \n";
            cnt = 0;
            for (const auto& it : condition.conclusions) {
                std::cout << pref << ++cnt << "/" << condition.conclusions.size() << ": " << it << std::endl;
            }
        };

        std::vector<IfThenConditionSymbolic> ifThenConditionsSymbolic;

        for (auto& condition : ifthenConditions) {
            IfThenConditionSymbolic conditionSymbolic;
            for (auto& it : condition.getConditions()) {
                conditionSymbolic.conditions.push_back(it->evaluate(ctx).getSymbolicPolynomial());
            }
            for (auto& it : condition.getConclusions()) {
                conditionSymbolic.conclusions.push_back(it->evaluate(ctx).getSymbolicPolynomial());
            }


            ifThenConditionsSymbolic.push_back(conditionSymbolic);
        }

#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll registered if-then conditions: \n";
        int cnt = 0;
        for (auto& it: ifThenConditionsSymbolic) {
            std::cout << "\n[" << ++cnt << "/" << ifThenConditionsSymbolic.size() << "]: \n";
            pritntIfThenCondition(it, "\t");
        }
        std::cout << "End of registered if-then conditions\n================================\n";
#endif



        // create monomials for sos
        std::vector<std::vector<int>> allCombinations;
        auto current = std::vector<int>(allRationalVariablesNames.size(), 0);
        allCombinations.push_back(current); // patching to have sos with degree 0
//        do {
//            allCombinations.push_back(current);
//            for (auto& it : current) {
//                std::cout << it << " ";
//            }
//            std::cout << std::endl;
//        } while (getNextVectorBoundedSum(current, config_.getHighMonomialDegree()));


        sosMonomials;

        for (const auto& combination : allCombinations) {
            QMonomial monomial = env.qmonomialOne();
            for (int i = 0; i < allRationalVariablesNames.size(); i++) {
                for (int power = 0; power < combination[i]; power++)
                    monomial = symbolic_ring::mul(monomial, allRationalVariables[i]);
            }
            sosMonomials.push_back(monomial);
        }

#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll sos monomials: \n";
        for (auto& it: sosMonomials) {
            std::cout << it << std::endl;
        }
        std::cout << "End of sos monomials\n================================\n";
#endif

        std::vector<std::vector<SymbolicPolynomial>> sosPolynomials;
        std::vector<std::vector<SymbolicPolynomial>> symbolicPolynoimalRepresentation;


        // for each if-then condition create #conditions * #conclusion soses
        int sosCounter = -1;
        for (auto& ifThen: ifThenConditionsSymbolic) {
            sosPolynomials.emplace_back();
            symbolicPolynoimalRepresentation.emplace_back();
            // within this block the currentSymbolicPolynomialEncodingIfThen reduced only once to optimize the performance
            for (auto ifThenConclusion: ifThen.conclusions) {
                auto currentSymbolicPolynomialEncodingIfThen = env.symbolicPolynomialZero();
                for (auto ifThenCondition: ifThen.conditions) {
                    sosCounter += 1;
                    std::cout << "getting sos: " << sosCounter << std::endl;
                    auto toPush = getSos(sosMonomials, sosCounter);
                    std::cout << "sos received." << std::endl;

                    sosPolynomials.back().push_back(toPush);
                    currentSymbolicPolynomialEncodingIfThen =
                            symbolic_ring::add(currentSymbolicPolynomialEncodingIfThen,
                                               symbolic_ring::mul(toPush, ifThenCondition), false);
                }
                currentSymbolicPolynomialEncodingIfThen = symbolic_ring::add(
                        currentSymbolicPolynomialEncodingIfThen,
                        symbolic_ring::mul(ifThenConclusion, -1), false);
                currentSymbolicPolynomialEncodingIfThen.reduce();

                symbolicPolynoimalRepresentation.back().push_back(currentSymbolicPolynomialEncodingIfThen);
            }
        }

#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll sos polynomials: \n";
        for (auto& it: sosPolynomials) {
            for (auto& it2: it) {
                std::cout << it2 << std::endl;
            }
        }
        std::cout << "End of sos polynomials\n================================\n";
#endif

#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll symbolic polynomials: \n";
        for (auto& it: symbolicPolynoimalRepresentation) {
            for (auto& it2: it) {
                std::cout << it2 << std::endl;
            }
        }
        std::cout << "End of symbolic polynomials\n================================\n";
#endif

        std::vector<QPolynomial> linearPolynomialsShouldBeZero;
        for (int i = 0; i < symbolicPolynoimalRepresentation.size(); i++) {
            for (int j = 0; j < symbolicPolynoimalRepresentation[i].size(); j++) {
                auto allMonomials = symbolicPolynoimalRepresentation[i][j].getReducedMonomials();
                // iterating over monomials and setting the coefficients to zero
                for (auto& it: allMonomials) {
                    linearPolynomialsShouldBeZero.push_back(it.getQcoefficient());
                }
            }
        }

#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll linear polynomials that should be zero: \n";
        for (auto& it: linearPolynomialsShouldBeZero) {
            std::cout << it << std::endl;
        }
        std::cout << "End of all linear polynomials that should be zero\n================================\n";
#endif


        solver_ = std::make_unique<SolverMosec>(sosMonomials.size(), 0, instanceName_ + std::to_string(config_.getHighMonomialDegree()));

        int dbg_cnt = 0;
        for (auto&it : linearPolynomialsShouldBeZero) {
#ifdef AUCOES_DEBUG
            std::cout << "Adding(4): " << it << std::endl;
            dbg_cnt += 1;
#endif
            solver_->addLinearEqualityConstraint(it);
        }


        auto feasibility = solver_->is_feasible();
        hasSolution = feasibility;

        if (feasibility) {
            is_feasible_ = Feasibility::FEASIBLE;
        } else {
            is_feasible_ = Feasibility::INFEASIBLE;
        }


        std::cout << "The system is feasible: " << (feasibility ? "YES" : "NO") << std::endl;

        if (feasibility) {
            auto sol = solver_->getSolution2();
            for (auto& it: sol) {
                std::cout << it.first << " " << it.second << std::endl;
            }
        }


        solution = solver_->getSolution2();
    }

    void solveWithPutinarCsdp() {
        // setting up the context
        auto& programTable = program_.getTable();
        auto& ifthenConditions = program_.getConditions();

//        codegen.IAdmitThatThisIsUnsafeAndShouldBeUsedOnlyWithTrusterInput();

        for (auto& ifThen: ifthenConditions) {
            auto& conditioins = ifThen.getConditions();

            for (auto & conditioin : conditioins) {
                conditioin = moveToGreaterSide(conditioin->clone());
            }

            auto& conclusions = ifThen.getConclusions();
            for (auto& conclusion: conclusions) {
                conclusion = moveToGreaterSide(conclusion->clone());
            }

        }

        // add artificial "1 >= 0" condition
        if (config_.getNeedAdditionalOneGeqZero()) {
            for (auto& ifThen: ifthenConditions) {
                ifThen.addCondition(std::make_unique<BinaryRelation>(std::make_unique<Constant>(1), std::make_unique<Constant>(0), ">="));
            }
        }


        #ifdef AUCOES_DEBUG
        std::cout << "\n===========================\nIf-then conditions, not evaluated:\n";
        for (int condIdx = 0; condIdx < ifthenConditions.size(); condIdx++) {
            for (int conclIdx = 0; conclIdx < ifthenConditions[condIdx].getConclusions().size(); conclIdx++) {
                std::cout << "Condition #" << condIdx + 1 << " conclusion #" << conclIdx + 1 << ": " << std::endl;
                std::cout << "Conditions:" << std::endl;
                for (auto& cond: ifthenConditions[condIdx].getConditions()) {
//                    std::cout << cond->toString() << std::endl;
                    std::cout << codegen.programExpressionToPysym(cond->getChildren()[0]) << std::endl;
                }
                std::cout << "Conclusion:" << std::endl;
//                std::cout << ifthenConditions[condIdx].getConclusions()[conclIdx]->toString() << std::endl;

                std::cout << codegen.programExpressionToPysym(
                        ifthenConditions[condIdx].getConclusions()[conclIdx]->
                            getChildren()[0]) << std::endl;
            }
        }
        std::cout << "\n===========================\n";
        #endif


        auto env = SymbolicEnvironment();
        auto ctx = EvaluationContext(&env);

        allRationalVariablesNames = programTable.getDeclaredVariables();
        const auto& functions = programTable.getDeclaredFunctions();

        std::map<std::string, QPolynomial> varToPolynomial;
        for (const auto& varName : allRationalVariablesNames) {
            varToPolynomial.insert({varName, QPolynomial(env.sym(varName))});
        }

        int maxArgNumber = 0;
        for (const auto& functionName : functions) {
            maxArgNumber = std::max(maxArgNumber, program_.getFunctionSignature(functionName)[0]);
        }
        std::vector<QMonomial> functionArguments;
        std::vector<std::string> functionArguments_names;

        for (int i = 0; i < maxArgNumber; i++) {
            functionArguments.push_back(QMonomial(env.sym("_function_arg_" + std::to_string(i))));
            functionArguments_names.push_back("_function_arg_" + std::to_string(i));
        }


        //// create functions from its signature
//        std::map<std::string, SymbolicPolynomial> functionNameToSymbolicPolynomial;

        for (const auto& functionName: functions) {
            functionNameToSymbolicPolynomial.insert({functionName, createSymbolicPolynomial(functionName, functionArguments, env)});
        }



        #ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll registered functions: \n";
        for (auto& it: functionNameToSymbolicPolynomial) {
            std::cout << "Name: " << it.first << std::endl;
            std::cout << "Polynomial: " << it.second << std::endl;
        }
        std::cout << "End of registered functions\n================================\n";
        #endif

        //// register function to ctx
        for (const auto& functionName : functions) {
            ctx.setSymbolicPolynomial(
                    functionName,
                    functionNameToSymbolicPolynomial.at(functionName),
                    std::vector<std::string>(
                            functionArguments_names.begin(),
                            functionArguments_names.begin() + program_.getFunctionSignature(functionName)[0]));
        }

        //// create all rational variables
        allRationalVariables;

        for (const auto& varName : allRationalVariablesNames) {
            allRationalVariables.push_back(QMonomial(env.getOrCreate(varName)));
        }

        //// register all variables to ctx
        for (int i = 0; i < allRationalVariablesNames.size(); i++) {
            ctx.setVariableQPolynomial(allRationalVariablesNames[i], allRationalVariables[i]);
        }



        // using the context to evaluate all if-then conditions
        struct IfThenConditionSymbolic {
            std::vector<SymbolicPolynomial> conditions;
            std::vector<SymbolicPolynomial> conclusions;
        };

        auto pritntIfThenCondition = [](const IfThenConditionSymbolic& condition, std::string pref = "") {
            std::cout << pref << "Conditions: (" << condition.conditions.size() << ") \n";
            int cnt = 0;
            for (const auto& it : condition.conditions) {
                std::cout << pref << ++cnt << "/" << condition.conditions.size() << ": " << it << std::endl;
            }
            std::cout << std::endl;
            std::cout << pref << "Conclusions: (" << condition.conclusions.size() << ") \n";
            cnt = 0;
            for (const auto& it : condition.conclusions) {
                std::cout << pref << ++cnt << "/" << condition.conclusions.size() << ": " << it << std::endl;
            }
        };

        std::vector<IfThenConditionSymbolic> ifThenConditionsSymbolic;

        for (auto& condition : ifthenConditions) {
            IfThenConditionSymbolic conditionSymbolic;
            for (auto& it : condition.getConditions()) {
                conditionSymbolic.conditions.push_back(it->evaluate(ctx).getSymbolicPolynomial());
            }
            for (auto& it : condition.getConclusions()) {
                conditionSymbolic.conclusions.push_back(it->evaluate(ctx).getSymbolicPolynomial());
            }


            ifThenConditionsSymbolic.push_back(conditionSymbolic);
        }

        #ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll registered if-then conditions: \n";
        int cnt = 0;
        for (auto& it: ifThenConditionsSymbolic) {
            std::cout << "\n[" << ++cnt << "/" << ifThenConditionsSymbolic.size() << "]: \n";
            pritntIfThenCondition(it, "\t");
        }
        std::cout << "End of registered if-then conditions\n================================\n";
        #endif



        // create monomials for sos
        std::vector<std::vector<int>> allCombinations;
        auto current = std::vector<int>(allRationalVariablesNames.size(), 0);
        do {
            allCombinations.push_back(current);
            for (auto& it : current) {
                std::cout << it << " ";
            }
            std::cout << std::endl;
        } while (getNextVectorBoundedSum(current, config_.getHighMonomialDegree()));


        sosMonomials;

        for (const auto& combination : allCombinations) {
            QMonomial monomial = env.qmonomialOne();
            for (int i = 0; i < allRationalVariablesNames.size(); i++) {
                for (int power = 0; power < combination[i]; power++)
                    monomial = symbolic_ring::mul(monomial, allRationalVariables[i]);
            }
            sosMonomials.push_back(monomial);
        }

        #ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll sos monomials: \n";
        for (auto& it: sosMonomials) {
            std::cout << it << std::endl;
        }
        std::cout << "End of sos monomials\n================================\n";
        #endif

        std::vector<std::vector<SymbolicPolynomial>> sosPolynomials;
        std::vector<std::vector<SymbolicPolynomial>> symbolicPolynoimalRepresentation;


        // for each if-then condition create #conditions * #conclusion soses
        int sosCounter = -1;
        for (auto& ifThen: ifThenConditionsSymbolic) {
            sosPolynomials.emplace_back();
            symbolicPolynoimalRepresentation.emplace_back();

            for (auto ifThenConclusion: ifThen.conclusions) {
                auto currentSymbolicPolynomialEncodingIfThen = env.symbolicPolynomialZero();
                for (auto ifThenCondition: ifThen.conditions) {
                    sosCounter += 1;
                    std::cout << "getting sos: " << sosCounter << std::endl;
                    auto toPush = getSos(sosMonomials, sosCounter);
                    std::cout << "sos received." << std::endl;

                    sosPolynomials.back().push_back(toPush);
                    currentSymbolicPolynomialEncodingIfThen =
                            symbolic_ring::add(currentSymbolicPolynomialEncodingIfThen,
                                               symbolic_ring::mul(toPush, ifThenCondition), true);
                }
                currentSymbolicPolynomialEncodingIfThen = symbolic_ring::add(
                        currentSymbolicPolynomialEncodingIfThen,
                        symbolic_ring::mul(ifThenConclusion, -1), true);

                symbolicPolynoimalRepresentation.back().push_back(currentSymbolicPolynomialEncodingIfThen);
            }
        }

        #ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll sos polynomials: \n";
        for (auto& it: sosPolynomials) {
            for (auto& it2: it) {
                std::cout << it2 << std::endl;
            }
        }
        std::cout << "End of sos polynomials\n================================\n";
        #endif

        #ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll symbolic polynomials: \n";
        for (auto& it: symbolicPolynoimalRepresentation) {
            for (auto& it2: it) {
                std::cout << it2 << std::endl;
            }
        }
        std::cout << "End of symbolic polynomials\n================================\n";
        #endif

        std::vector<QPolynomial> linearPolynomialsShouldBeZero;
        for (int i = 0; i < symbolicPolynoimalRepresentation.size(); i++) {
            for (int j = 0; j < symbolicPolynoimalRepresentation[i].size(); j++) {
                auto allMonomials = symbolicPolynoimalRepresentation[i][j].getReducedMonomials();
                // iterating over monomials and setting the coefficients to zero
                for (auto& it: allMonomials) {
                    linearPolynomialsShouldBeZero.push_back(it.getQcoefficient());
                }
            }
        }

        #ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll linear polynomials that should be zero: \n";
        for (auto& it: linearPolynomialsShouldBeZero) {
            std::cout << it << std::endl;
        }
        std::cout << "End of all linear polynomials that should be zero\n================================\n";
        #endif


        solverCsdp_ = std::make_unique<SolverCsdp>(sosMonomials.size(), 0, instanceName_ + std::to_string(config_.getHighMonomialDegree()));

        int dbg_cnt = 0;
        for (auto&it : linearPolynomialsShouldBeZero) {
        #ifdef AUCOES_DEBUG
            std::cout << "Adding(1): " << it << std::endl;
            dbg_cnt += 1;
        #endif
            solverCsdp_->addLinearEqualityConstraint(it);
        }


        auto feasibility = solverCsdp_->is_feasible();
        hasSolution = feasibility;

        if (feasibility) {
            is_feasible_ = Feasibility::FEASIBLE;
        } else {
            is_feasible_ = Feasibility::INFEASIBLE;
        }


        std::cout << "The system is feasible: " << (feasibility ? "YES" : "NO") << std::endl;

        if (feasibility) {
            auto sol = solverCsdp_->getSolution2();
            for (auto& it: sol) {
                std::cout << it.first << " " << it.second << std::endl;
            }
        }


        solution = solverCsdp_->getSolution2();
    }


    void solveWithPutinarMosek() {
        // setting up the context
        auto& programTable = program_.getTable();
        auto& ifthenConditions = program_.getConditions();

//        codegen.IAdmitThatThisIsUnsafeAndShouldBeUsedOnlyWithTrusterInput();

        for (auto& ifThen: ifthenConditions) {
            auto& conditioins = ifThen.getConditions();

            for (auto & conditioin : conditioins) {
                conditioin = moveToGreaterSide(conditioin->clone());
            }

            auto& conclusions = ifThen.getConclusions();
            for (auto& conclusion: conclusions) {
                conclusion = moveToGreaterSide(conclusion->clone());
            }

        }

        // add artificial "1 >= 0" condition
        if (config_.getNeedAdditionalOneGeqZero()) {
            for (auto& ifThen: ifthenConditions) {
                ifThen.addCondition(std::make_unique<BinaryRelation>(std::make_unique<Constant>(1), std::make_unique<Constant>(0), ">="));
            }
        }


#ifdef AUCOES_DEBUG
        std::cout << "\n===========================\nIf-then conditions, not evaluated:\n";
        for (int condIdx = 0; condIdx < ifthenConditions.size(); condIdx++) {
            for (int conclIdx = 0; conclIdx < ifthenConditions[condIdx].getConclusions().size(); conclIdx++) {
                std::cout << "Condition #" << condIdx + 1 << " conclusion #" << conclIdx + 1 << ": " << std::endl;
                std::cout << "Conditions:" << std::endl;
                for (auto& cond: ifthenConditions[condIdx].getConditions()) {
//                    std::cout << cond->toString() << std::endl;
                    std::cout << codegen.programExpressionToPysym(cond->getChildren()[0]) << std::endl;
                }
                std::cout << "Conclusion:" << std::endl;
//                std::cout << ifthenConditions[condIdx].getConclusions()[conclIdx]->toString() << std::endl;

                std::cout << codegen.programExpressionToPysym(
                        ifthenConditions[condIdx].getConclusions()[conclIdx]->
                            getChildren()[0]) << std::endl;
            }
        }
        std::cout << "\n===========================\n";
#endif


        auto env = SymbolicEnvironment();
        auto ctx = EvaluationContext(&env);

        allRationalVariablesNames = programTable.getDeclaredVariables();
        const auto& functions = programTable.getDeclaredFunctions();

        std::map<std::string, QPolynomial> varToPolynomial;
        for (const auto& varName : allRationalVariablesNames) {
            varToPolynomial.insert({varName, QPolynomial(env.sym(varName))});
        }

        int maxArgNumber = 0;
        for (const auto& functionName : functions) {
            maxArgNumber = std::max(maxArgNumber, program_.getFunctionSignature(functionName)[0]);
        }
        std::vector<QMonomial> functionArguments;
        std::vector<std::string> functionArguments_names;

        for (int i = 0; i < maxArgNumber; i++) {
            functionArguments.push_back(QMonomial(env.sym("_function_arg_" + std::to_string(i))));
            functionArguments_names.push_back("_function_arg_" + std::to_string(i));
        }


        //// create functions from its signature
//        std::map<std::string, SymbolicPolynomial> functionNameToSymbolicPolynomial;

        for (const auto& functionName: functions) {
            functionNameToSymbolicPolynomial.insert({functionName, createSymbolicPolynomial(functionName, functionArguments, env)});
        }



#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll registered functions: \n";
        for (auto& it: functionNameToSymbolicPolynomial) {
            std::cout << "Name: " << it.first << std::endl;
            std::cout << "Polynomial: " << it.second << std::endl;
        }
        std::cout << "End of registered functions\n================================\n";
#endif

        //// register function to ctx
        for (const auto& functionName : functions) {
            ctx.setSymbolicPolynomial(
                    functionName,
                    functionNameToSymbolicPolynomial.at(functionName),
                    std::vector<std::string>(
                            functionArguments_names.begin(),
                            functionArguments_names.begin() + program_.getFunctionSignature(functionName)[0]));
        }

        //// create all rational variables
        allRationalVariables;

        for (const auto& varName : allRationalVariablesNames) {
            allRationalVariables.push_back(QMonomial(env.getOrCreate(varName)));
        }

        //// register all variables to ctx
        for (int i = 0; i < allRationalVariablesNames.size(); i++) {
            ctx.setVariableQPolynomial(allRationalVariablesNames[i], allRationalVariables[i]);
        }



        // using the context to evaluate all if-then conditions
        struct IfThenConditionSymbolic {
            std::vector<SymbolicPolynomial> conditions;
            std::vector<SymbolicPolynomial> conclusions;
        };

        auto pritntIfThenCondition = [](const IfThenConditionSymbolic& condition, std::string pref = "") {
            std::cout << pref << "Conditions: (" << condition.conditions.size() << ") \n";
            int cnt = 0;
            for (const auto& it : condition.conditions) {
                std::cout << pref << ++cnt << "/" << condition.conditions.size() << ": " << it << std::endl;
            }
            std::cout << std::endl;
            std::cout << pref << "Conclusions: (" << condition.conclusions.size() << ") \n";
            cnt = 0;
            for (const auto& it : condition.conclusions) {
                std::cout << pref << ++cnt << "/" << condition.conclusions.size() << ": " << it << std::endl;
            }
        };

        std::vector<IfThenConditionSymbolic> ifThenConditionsSymbolic;

        for (auto& condition : ifthenConditions) {
            IfThenConditionSymbolic conditionSymbolic;
            for (auto& it : condition.getConditions()) {
                conditionSymbolic.conditions.push_back(it->evaluate(ctx).getSymbolicPolynomial());
            }
            for (auto& it : condition.getConclusions()) {
                conditionSymbolic.conclusions.push_back(it->evaluate(ctx).getSymbolicPolynomial());
            }


            ifThenConditionsSymbolic.push_back(conditionSymbolic);
        }

#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll registered if-then conditions: \n";
        int cnt = 0;
        for (auto& it: ifThenConditionsSymbolic) {
            std::cout << "\n[" << ++cnt << "/" << ifThenConditionsSymbolic.size() << "]: \n";
            pritntIfThenCondition(it, "\t");
        }
        std::cout << "End of registered if-then conditions\n================================\n";
#endif



        // create monomials for sos
        std::vector<std::vector<int>> allCombinations;
        auto current = std::vector<int>(allRationalVariablesNames.size(), 0);
        do {
            allCombinations.push_back(current);
            for (auto& it : current) {
                std::cout << it << " ";
            }
            std::cout << std::endl;
        } while (getNextVectorBoundedSum(current, config_.getHighMonomialDegree()));


        sosMonomials;

        for (const auto& combination : allCombinations) {
            QMonomial monomial = env.qmonomialOne();
            for (int i = 0; i < allRationalVariablesNames.size(); i++) {
                for (int power = 0; power < combination[i]; power++)
                    monomial = symbolic_ring::mul(monomial, allRationalVariables[i]);
            }
            sosMonomials.push_back(monomial);
        }

#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll sos monomials: \n";
        for (auto& it: sosMonomials) {
            std::cout << it << std::endl;
        }
        std::cout << "End of sos monomials\n================================\n";
#endif

        std::vector<std::vector<SymbolicPolynomial>> sosPolynomials;
        std::vector<std::vector<SymbolicPolynomial>> symbolicPolynoimalRepresentation;


        // for each if-then condition create #conditions * #conclusion soses
        int sosCounter = -1;
        for (auto& ifThen: ifThenConditionsSymbolic) {
            sosPolynomials.emplace_back();
            symbolicPolynoimalRepresentation.emplace_back();

            for (auto ifThenConclusion: ifThen.conclusions) {
                auto currentSymbolicPolynomialEncodingIfThen = env.symbolicPolynomialZero();
                for (auto ifThenCondition: ifThen.conditions) {
                    sosCounter += 1;
                    std::cout << "getting sos: " << sosCounter << std::endl;
                    auto toPush = getSos(sosMonomials, sosCounter);
                    std::cout << "sos received." << std::endl;

                    sosPolynomials.back().push_back(toPush);
                    currentSymbolicPolynomialEncodingIfThen =
                            symbolic_ring::add(currentSymbolicPolynomialEncodingIfThen,
                                               symbolic_ring::mul(toPush, ifThenCondition), false);
                }
                currentSymbolicPolynomialEncodingIfThen = symbolic_ring::add(
                        currentSymbolicPolynomialEncodingIfThen,
                        symbolic_ring::mul(ifThenConclusion, -1), false);

                currentSymbolicPolynomialEncodingIfThen.reduce();

                symbolicPolynoimalRepresentation.back().push_back(currentSymbolicPolynomialEncodingIfThen);
            }
        }

#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll sos polynomials: \n";
        for (auto& it: sosPolynomials) {
            for (auto& it2: it) {
                std::cout << it2 << std::endl;
            }
        }
        std::cout << "End of sos polynomials\n================================\n";
#endif

#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll symbolic polynomials: \n";
        for (auto& it: symbolicPolynoimalRepresentation) {
            for (auto& it2: it) {
                std::cout << it2 << std::endl;
            }
        }
        std::cout << "End of symbolic polynomials\n================================\n";
#endif

        std::vector<QPolynomial> linearPolynomialsShouldBeZero;
        for (int i = 0; i < symbolicPolynoimalRepresentation.size(); i++) {
            for (int j = 0; j < symbolicPolynoimalRepresentation[i].size(); j++) {
                auto allMonomials = symbolicPolynoimalRepresentation[i][j].getReducedMonomials();
                // iterating over monomials and setting the coefficients to zero
                for (auto& it: allMonomials) {
                    linearPolynomialsShouldBeZero.push_back(it.getQcoefficient());
                }
            }
        }

#ifdef AUCOES_DEBUG
        std::cout << "\n================================\nAll linear polynomials that should be zero: \n";
        for (auto& it: linearPolynomialsShouldBeZero) {
            std::cout << it << std::endl;
        }
        std::cout << "End of all linear polynomials that should be zero\n================================\n";
#endif


        solver_ = std::make_unique<SolverMosec>(sosMonomials.size(), 0, instanceName_ + std::to_string(config_.getHighMonomialDegree()));

        int dbg_cnt = 0;
        for (auto&it : linearPolynomialsShouldBeZero) {
#ifdef AUCOES_DEBUG
            std::cout << "Adding(2): " << it << std::endl;
            dbg_cnt += 1;
#endif
            solver_->addLinearEqualityConstraint(it);
        }


        auto feasibility = solver_->is_feasible();
        hasSolution = feasibility;

        if (feasibility) {
            is_feasible_ = Feasibility::FEASIBLE;
        } else {
            is_feasible_ = Feasibility::INFEASIBLE;
        }


        std::cout << "The system is feasible: " << (feasibility ? "YES" : "NO") << std::endl;

        if (feasibility) {
            auto sol = solver_->getSolution2();
            for (auto& it: sol) {
                std::cout << it.first << " " << it.second << std::endl;
            }
        }

        solution = solver_->getSolution2();
    }

    void IAdmitThatThisIsUnsafeAndShouldBeUsedOnlyWithTrustedInput() {
        codegen.IAdmitThatThisIsUnsafeAndShouldBeUsedOnlyWithTrusterInput();
    }

    bool isFeasible() {
        if (is_feasible_ == Feasibility::UNKNOWN) {
            throw std::runtime_error("The system is not solved yet. Please call solve() first.");
        }
        return hasSolution;
    }

    void getCertifiedSolutionPy(std::ostream &os) {

        if (is_feasible_ == Feasibility::UNKNOWN) {
            throw std::runtime_error("The system is not solved yet. Please call solve() first.");
        }
        if (is_feasible_ == Feasibility::INFEASIBLE) {
            throw std::runtime_error("The system is infeasible. No solution exists.");
        }

//        auto solution = solver_->getSolution2();

        // preparaion
        using doubleMatrix = std::vector<std::vector<double>>;
        std::map<std::string, doubleMatrix> sosNameToMatrix;
        std::map<std::string, double> variableNameToDouble;

        auto sosDim = sosMonomials.size();

        for (auto& it: solution) {
            if (isLVar(it.first)) {
                auto ijk = parseLVarName(it.first);
                auto sosId = ijk[0];
                auto rowIdx = ijk[1];
                auto colIdx = ijk[2];

                auto sosName = "l_" + std::to_string(sosId);

                if (sosNameToMatrix.find(sosName) == sosNameToMatrix.end()) {
                    sosNameToMatrix[sosName] = doubleMatrix(sosDim, std::vector<double>(sosDim, 0.0));
                }

                sosNameToMatrix[sosName][rowIdx][colIdx] += it.second;
                continue;
            }
            variableNameToDouble[it.first] = it.second;
        }

        // code generation
        os << codegen.pythonHeader();

        os << codegen.block_of_code({
                                            "def check_polynomial_almost_zero(poly):",
                                            "    max_abs_coeff = max(map(abs, sp.poly(sp.expand(poly)).coeffs()))",
                                            "    return max_abs_coeff < EPS_POLY_COEFF"
                                    }) << std::endl;

        os << codegen.new_line() << "\n" << codegen.new_line() << std::endl;

        os << codegen.start_function("check", {"need_check_matrix_psd", "is_only_answer"});


        // generate variable assignent code
        for (auto& it: variableNameToDouble) {
            os << codegen.create_sym(it.first) << std::endl;
            os << codegen.assign_sym(it.first, it.second) << std::endl;
        }
        os << codegen.new_line() << std::endl;

        for (int i = 0; i < 10; i++) {
            os << codegen.create_sym("_function_arg_" + std::to_string(i)) << std::endl;
            os << codegen.arbitrary_code("_function_arg_" + std::to_string(i) + " = " + "check." + "_function_arg_" + std::to_string(i)) << std::endl;
        }

        for (auto& it: functionNameToSymbolicPolynomial) {
            os << codegen.assign_poly(it.first, it.second) << std::endl;
            os << codegen.block_of_code({
                "if is_only_answer:",
                "    print(f\"" + it.first + " = {sp.simplify(" + "check." + it.first + ")}\")"
            }) << std::endl;
        }

        os << codegen.block_of_code({
            "if is_only_answer:",
            "    return"
        }) << std::endl;

        os << codegen.comment("all psd matrices") << std::endl;

        // generate sos code
        std::vector<std::string> allSosNames;

        int sosNameToMatrixSize = sosNameToMatrix.size();
        for (int sosNameIdx = 0; sosNameIdx < sosNameToMatrixSize; sosNameIdx++) {
            auto sosName = "l_" + std::to_string(sosNameIdx);
            allSosNames.push_back(sosName);
            os << codegen.generate_matrix(sosName, sosNameToMatrix[sosName]) << std::endl;
        }

//        for (auto& sosNameAndMatrix: sosNameToMatrix) {
//            std::cout << "Generating sos code for " << sosNameAndMatrix.first << std::endl;
//            std::cout << "Matrix:" << std::endl;
//            for (int i = 0; i < sosNameAndMatrix.second.size(); i++) {
//                for (int j = 0; j < sosNameAndMatrix.second[i].size(); j++) {
//                    std::cout << sosNameAndMatrix.second[i][j] << " ";
//                }
//                std::cout << std::endl;
//            }
//            allSosNames.push_back(sosNameAndMatrix.first);
//
//            os << codegen.generate_matrix(sosNameAndMatrix.first, sosNameAndMatrix.second) << std::endl;
//        }

        std::stringstream allSosNamesStrCommaSeparated;
        for (int i = 0; i < allSosNames.size(); i++) {
            allSosNamesStrCommaSeparated << "check." << allSosNames[i];
            if (i != allSosNames.size() - 1) {
                allSosNamesStrCommaSeparated << ", ";
            }
        }

        os << codegen.new_line() << std::endl;
        os << codegen.arbitrary_code("check.l_all = [" + allSosNamesStrCommaSeparated.str() + "]") << std::endl;
        os << codegen.new_line() << std::endl;

        os << codegen.comment("check matrices are positive semidefinite") << std::endl;

        // check matrices are positive semidefinite
        os << codegen.block_of_code({
            "if need_check_matrix_psd:",
            "    for l in check.l_all:",
            "        assert(check_matrix_psd(l))"
        }) << std::endl;



        // generate all variables definitions
        for (auto& it: allRationalVariablesNames) {
            os << codegen.create_sym(it) << std::endl;
            os << codegen.arbitrary_code(it + " = " + "check." + it) << std::endl;
        }

        // generate monomial vector
        std::vector<std::string> monomialVectorString;
        for (auto& it: sosMonomials) {
            monomialVectorString.push_back(codegen.to_str(QPolynomial(it)));
        }

        std::stringstream monomialVectorStringCommaSeparated;
        for (int i = 0; i < monomialVectorString.size(); i++) {
            monomialVectorStringCommaSeparated << monomialVectorString[i];
            if (i != monomialVectorString.size() - 1) {
                monomialVectorStringCommaSeparated << ", ";
            }
        }

        os << codegen.new_line() << std::endl;
        os << codegen.arbitrary_code("check.monomial_vector = sp.matrices.Matrix([" + monomialVectorStringCommaSeparated.str() + "])")
        << std::endl;
        os << codegen.new_line() << std::endl;

        // generate all sos polynomials
        os << codegen.block_of_code({
            "check.sos_all = []",
            "for l in check.l_all:",
            "    check.sos_all.append((check.monomial_vector.transpose() * l * check.monomial_vector)[0, 0])",
            ""
        });



        auto& ifthenConditions = program_.getConditions();

        os << codegen.new_line() << std::endl;

        int conditionCounter = 1;
        int sosIndexCounter = 0;
        for (auto& it: ifthenConditions) {
            std::vector<int> sosIndicesForCondition;

            os << codegen.comment("Verifying condition: " + std::to_string(conditionCounter)) << std::endl
            << codegen.new_line() << std::endl;

            const auto& conditions = it.getConditions();
            const auto& conclusions = it.getConclusions();
            if (conclusions.size() != 1) {
                throw std::runtime_error("Only one conclusion is supported");
            }

            std::vector<std::string> conditionsStrings;
            for (int i = 0; i < conditions.size(); i++) {
                sosIndicesForCondition.push_back(sosIndexCounter);
                sosIndexCounter++;
                std::string expr = codegen.programExpressionToPysym(conditions[i]->getChildren()[0]);
                conditionsStrings.push_back(expr);
            }

            os << codegen.arbitrary_code(std::string("check.") + "cond_" + std::to_string(conditionCounter) + " = [") << std::endl;

            for (int i = 0; i < conditionsStrings.size(); i++) {


                os << codegen.arbitrary_code(conditionsStrings[i]);
                if (i + 1 != conditionsStrings.size()) {
                    os << ", # >= 0";
                } else {
                    os << " # >= 0";
                }
                os << std::endl;
            }
            os << codegen.new_line() << "]" << std::endl;
            os << codegen.comment("Implies") << std::endl;
            std::string conclusionName = "conc_" + std::to_string(conditionCounter);
            std::string concl = codegen.programExpressionToPysym(conclusions[0]->getChildren()[0]);
            os << codegen.arbitrary_code(std::string("check.") + conclusionName + " = " + concl) << " # >= 0"<< std::endl;

            os << codegen.new_line() << std::endl;

            std::string sosUsedArrName = "sos_" + std::to_string(conditionCounter);
            os << codegen.arbitrary_code(std::string("check.") + sosUsedArrName +
                                         " " +
                                         "= " + std::string("check.") + "sos_all[" +
                                         std::to_string(sosIndicesForCondition[0]) + ":" +
                                         std::to_string(sosIndicesForCondition.back() + 1) + "]") << std::endl;

            os << codegen.arbitrary_code(
                std::string("check.") + "res_" + std::to_string(conditionCounter) + " = sum(x * y for x, y in zip(check.cond_" + std::to_string(conditionCounter) + ", " + "check." + sosUsedArrName + ")) - " + "check." + conclusionName) << std::endl;

            os << codegen.comment("############# PROOF OF CONDITION " + std::to_string(conditionCounter) + " #####################") << std::endl;
            os << codegen.arbitrary_code("assert(check_polynomial_almost_zero(" + (std::string("check.") + "res_" + std::to_string(conditionCounter)) + "))") << std::endl;

            conditionCounter++;
        }


        os << codegen.end_function() << std::endl;


        os << codegen.ifnamemain() << std::endl;

        os << codegen.block_of_code({
            "is_fast_check = len(sys.argv) >= 2 and sys.argv[1] == \"fast\"",
            "is_only_answer = len(sys.argv) >= 2 and sys.argv[1] == \"answer\"",
            "",
            "try:",
            "    check(not is_fast_check, is_only_answer)",
            "    if not is_only_answer:",
            "        print(\"The program is \\033[92mcorrect\\033[0m\")",
            "    else:",
            "        print(\"The program status is \\033[92mUNKNOWN\\033[0m, to check the program, run without \\\"answer\\\" argument\")",
            "except AssertionError:",
            "    print(\"The program is \\033[91mINCORRECT\\033[0m\")"
        }) << std::endl;


    }

private:

    Program& program_;

    std::unique_ptr<SolverMosec> solver_;
    std::unique_ptr<SolverCsdp> solverCsdp_;

    Solution solution_;
    bool hasSolution = false;


    SolverConfig config_;
    std::vector<QMonomial> sosMonomials;
    std::vector<QMonomial> allRationalVariables;
    std::vector<std::string> allRationalVariablesNames;

    std::map<std::string, SymbolicPolynomial> functionNameToSymbolicPolynomial;
    Feasibility is_feasible_ = Feasibility::UNKNOWN;
    PythonCodegen codegen = PythonCodegen();

    std::string instanceName_;

    std::map<std::string, double> solution;
};

#endif //MYPROJECT_AUTOMAITCCOMPLEXITYESTIMATOR_H
