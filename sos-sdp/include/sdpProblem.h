//
// Created by sergey on 02.07.23.
//
// this class is mainly for test purposes
//

#ifndef MYPROJECT_SDPPROBLEM_H
#define MYPROJECT_SDPPROBLEM_H

#include <vector>
#include <set>
#include <map>
#include <string>
#include <memory>
#include <sstream>

#include "fusion.h"


 #define SUPPRESSCHECKS 1

// use mosek::fusion as fus:

namespace fus = mosek::fusion;
using namespace monty;


//#define SDP_PROBLEM_DEBUG 1


enum class LinearMatrixExpressionType {
    GEQ,
    EQ,
    IN_RANGE,
    UNKNOWN
};

struct LinearMatrixExpression {
    explicit LinearMatrixExpression(int matrixSize): matrixSize(matrixSize), type(LinearMatrixExpressionType::UNKNOWN) {
    }

    void addMatrixEntry(int matrixIndex, int row, int col, double coefficient) {
        if (matrixCoefficients.find(matrixIndex) == matrixCoefficients.end()) {
            matrixCoefficients[matrixIndex] = std::vector<std::vector<double>>(matrixSize, std::vector<double>(matrixSize, 0.0));
        }
        matrixCoefficients[matrixIndex][row][col] += coefficient / 2;
        matrixCoefficients[matrixIndex][col][row] += coefficient / 2;
    }

    void addFreeCoefficients(int matrixIndex, double coefficient) {
        if (freeCoefficients.find(matrixIndex) == freeCoefficients.end()) {
            freeCoefficients[matrixIndex] = 0.0;
        }
        freeCoefficients[matrixIndex] += coefficient;
    }

    void addToConstantPart(double coefficient) {
        constantPart += coefficient;
    }

    void setType(LinearMatrixExpressionType type, double withinRange = 0.0) {
        if (type != LinearMatrixExpressionType::IN_RANGE && withinRange != 0.0) {
            throw std::runtime_error("Cannot set withinRange for non IN_RANGE type");
        }

        this->type = type;
        this->withinRange = withinRange;
    }


    int matrixSize;
    std::map<int, std::vector<std::vector<double>>> matrixCoefficients;
    std::map<int, double> freeCoefficients;
    double constantPart = 0.0;
    double withinRange = 0.0; // used only for IN_RANGE type

    LinearMatrixExpressionType type;
};



class SdpProblem {
public:
    explicit SdpProblem(int allMatricesSize) : allMatricesSize(allMatricesSize), objective(allMatricesSize) {
        M = new fus::Model("temp");
    }

    ~SdpProblem() {
        M->dispose();
    }

    fus::Model::t& getModel() {
        return M;
    }


    void startNewCondition() {
        if (state != READY_TO_START) {
            throw std::runtime_error("Cannot start new condition");
        }
        state = READY_TO_ADD;

        conditions.emplace_back(allMatricesSize);
    }

    void addSdpConstrainedVariable(int matrixIndex, int row, int col, double coefficient) {
        if (state != READY_TO_ADD) {
            throw std::runtime_error("Cannot add sdp constrained variable");
        }

        auto innerMatrixIndex = encodeOrCreateMatrixIndexAsInner(matrixIndex);
        conditions.back().addMatrixEntry(innerMatrixIndex, row, col, coefficient);

    }

    void addUnconstrainedVariable(std::string variableName, double coefficient) {
        if (state != READY_TO_ADD) {
            throw std::runtime_error("Cannot add sdp constrained variable");
        }

        auto innerIndex = encodeOrCreateUnconstrainedVariableNameAsInner(variableName);
        conditions.back().addFreeCoefficients(innerIndex, coefficient);
    }

    void addConstant(double constant) {
        if (state != READY_TO_ADD) {
            throw std::runtime_error("Cannot add sdp constrained variable");
        }
        conditions.back().addToConstantPart(constant);
    }

    void endCondition(LinearMatrixExpressionType type, double rhs = 0.0) {
        if (state != READY_TO_ADD) {
            throw std::runtime_error("Cannot end condition");
        }
        state = READY_TO_START;

        conditions.back().setType(type, rhs);
    }

    void addObjectiveVariable(std::string variableName, double coefficient) {
        if (state != READY_TO_ADD) {
            throw std::runtime_error("Cannot add sdp constrained variable");
        }

        auto innerIndex = encodeOrCreateUnconstrainedVariableNameAsInner(variableName);
        objective.addFreeCoefficients(innerIndex, coefficient);
    }


    int encodeOrCreateMatrixIndexAsInner(int index) {
        if (matrixIndices.count(index) == 0) {
            matrixIndices.insert(index);
            outerMatrixIndexToInnerMatrixIndex[index] = maxMatrixIndex;
            innerMatrixIndexToOuterMatrixIndex[maxMatrixIndex] = index;
            maxMatrixIndex++;
        }
        return outerMatrixIndexToInnerMatrixIndex[index];
    }

    int decodeMatrixIndexFromInner(int index) {
        if (innerMatrixIndexToOuterMatrixIndex.count(index) == 0) {
            throw std::runtime_error("Matrix index is not found");
        }
        return innerMatrixIndexToOuterMatrixIndex[index];
    }

    int encodeOrCreateUnconstrainedVariableNameAsInner(std::string variableName) {
        if (unconstrainedVariables.count(variableName) == 0) {
            unconstrainedVariables.insert(variableName);
            unconstrainedVariableNameToInnerIndex[variableName] = maxUnconstrainedVariableIndex;
            innerIndexToUnconstrainedVariableName[maxUnconstrainedVariableIndex] = variableName;
            maxUnconstrainedVariableIndex++;
        }
        return unconstrainedVariableNameToInnerIndex[variableName];
    }

    std::string decodeUnconstrainedVariableNameFromInner(int index) {
        if (innerIndexToUnconstrainedVariableName.count(index) == 0) {
            throw std::runtime_error("Unconstrained variable name is not found");
        }
        return innerIndexToUnconstrainedVariableName[index];
    }

    void printLinearMatrixExpression(std::ostream& os, const LinearMatrixExpression& expr, bool trueIndex = false) {
        for (const auto& matrixIndex_matrix : expr.matrixCoefficients) {
            auto matrixIndex = matrixIndex_matrix.first;
            if (trueIndex) {
                matrixIndex = decodeMatrixIndexFromInner(matrixIndex);
            }
            const auto& matrix = matrixIndex_matrix.second;

            os << "X_" << matrixIndex << std::endl << "@" << std::endl;
            for (int i = 0; i < expr.matrixSize; ++i) {
                for (int j = 0; j < expr.matrixSize; ++j) {
                    os << matrix[i][j] << " ";
                }
                os << std::endl;
            }
        }

        os << " + " << std::endl;
        for (const auto& matrixIndex_freeCoefficient : expr.freeCoefficients) {
            auto matrixIndex = matrixIndex_freeCoefficient.first;
            auto freeCoefficient = matrixIndex_freeCoefficient.second;

            os << " + (" << freeCoefficient << ") * " << "coef_" << matrixIndex;
        }
        os << " + " << expr.constantPart;

        switch (expr.type) {
            case LinearMatrixExpressionType::GEQ:
                os << ">= 0" << std::endl;
                break;
            case LinearMatrixExpressionType::EQ:
                os << "== 0" << std::endl;
                break;
            case LinearMatrixExpressionType::IN_RANGE:
                os << " IN_RANGE " << expr.withinRange << std::endl;
                break;
            case LinearMatrixExpressionType::UNKNOWN:
                os << "UNKNOWN";
                break;
        }
    }

    void printSystem(std::ostream& out, bool trueIndex = false) {
        for (const auto& condition : conditions) {
            printLinearMatrixExpression(out, condition, trueIndex);
        }
    }

    int getNumberOfSdpMatrices() {
        return matrixIndices.size();
    }

    int getNumberOfUnconstrainedVariables() {
        return unconstrainedVariables.size();
    }

    int getNumberOfConditions() {
        return conditions.size();
    }

    int getMatrixSize() {
        return allMatricesSize;
    }

    const std::vector<LinearMatrixExpression>& getConditions() {
        return conditions;
    }

    static double
    evaluateLhsForLinearMatrixExpression(
            const std::vector<std::vector<std::vector<double>>>& matrices,
            const std::vector<double>& unconstrainedVariables, const LinearMatrixExpression& expr) {
        double result = 0.0;

        for (auto matrixIndex_matrix : expr.matrixCoefficients) {
            auto matrixIndex = matrixIndex_matrix.first;
            const auto& matrix = matrixIndex_matrix.second;
            result += twoMatricesProduct(matrices[matrixIndex], matrix);
        }

        for (auto coeffIndex_freeCoefficient : expr.freeCoefficients) {
            auto coeffIndex = coeffIndex_freeCoefficient.first;
            auto freeCoefficient = coeffIndex_freeCoefficient.second;
            result += freeCoefficient * unconstrainedVariables[coeffIndex];
        }

        result += expr.constantPart;
        return result;
    }

    void setSolution(const std::vector<std::vector<std::vector<double>>>& matrices,
                     const std::vector<double>& unconstrainedVariables) {
        if (matrices.size() != getNumberOfSdpMatrices()) {
            throw std::runtime_error("Wrong number of matrices");
        }
        if (unconstrainedVariables.size() != getNumberOfUnconstrainedVariables()) {
            throw std::runtime_error("Wrong number of unconstrained variables");
        }

        int conditionCounter = 0;
        for (auto& condition : conditions) {
            conditionCounter++;
            auto evaluationResult = evaluateLhsForLinearMatrixExpression(matrices, unconstrainedVariables, condition);
#ifndef SUPPRESSCHECKS
            switch (condition.type) {
                case LinearMatrixExpressionType::GEQ:
                    if (evaluationResult < 0) {
                        throw std::runtime_error("Solution is not correct, GEQ condition " +
                        std::to_string(conditionCounter) + "/" + std::to_string(conditions.size()) + " is not satisfied");
                    }
                    break;
                case LinearMatrixExpressionType::EQ:
                    if (std::abs(evaluationResult) > allowedError) {
                        throw std::runtime_error("Solution is not correct, EQ condition " +
                        std::to_string(conditionCounter) + "/" + std::to_string(conditions.size()) + " is not satisfied:" +
                        "expected to be less than" + doubleToString(allowedError) + ", but got " + doubleToString(evaluationResult) + " instead");
                    }
                    break;
                case LinearMatrixExpressionType::IN_RANGE:
                    if (evaluationResult < -(condition.withinRange + allowedError)|| evaluationResult > (condition.withinRange + allowedError)) {
                        throw std::runtime_error("Solution is not correct, IN_RANGE condition " +
                        std::to_string(conditionCounter) + "/" + std::to_string(conditions.size()) + " is not satisfied:" +
                        "expected to be in [" + std::to_string(-condition.withinRange) + ", " + std::to_string(condition.withinRange) +
                        "], but got " + doubleToString(evaluationResult) + " instead");
                    }
                    break;
                case LinearMatrixExpressionType::UNKNOWN:
                    throw std::runtime_error("UNKNOWN condition is not supported");
            }
#endif
        }

        solutionMatrices = matrices;
        solutionUnconstrainedVariables = unconstrainedVariables;

        setupSolution();
    }

    std::map<int, int> csdpSosIdxToBlock;
    std::map<int, int> csdpUnconstrainedIdxToBlock;

    bool doubleIsZero(double x) {
        return x == 0.0;
    }

    void writeCsdpHeader(std::ostream& os) {
        // writing number of constraints
        int numberOfConstraints = 0;
        for (const auto& condition : conditions) {
            if (condition.type != LinearMatrixExpressionType::EQ) {
                throw std::runtime_error("Only EQ conditions are supported for csdp");
            }
            numberOfConstraints += 1;
        }

        os << numberOfConstraints << std::endl;
        os << getNumberOfSdpMatrices() + getNumberOfUnconstrainedVariables() << std::endl; // number of blocks

        // block matrices for each sdp
        int blockCnt = 1;
        for (int i = 0; i < getNumberOfSdpMatrices(); ++i) {
            os <<  getMatrixSize() << " ";
            csdpSosIdxToBlock[i] = blockCnt;
            blockCnt += 1;
        }

        // 2x2 matrix for each unconstrained variable
        for (int i = 0; i < getNumberOfUnconstrainedVariables(); ++i) {
            os << -2 << " ";
            csdpUnconstrainedIdxToBlock[i] = blockCnt;
            blockCnt += 1;
        }
        os << std::endl;

    }

    void writeCsdpCmatrix(std::ostream& os) {
        for (const auto& condition : conditions) {
            os << doubleToString(-condition.constantPart) << " ";
        }
        os << std::endl;
    }

    void writeCsdpLinearMatrixExpression(std::ostream& os, const LinearMatrixExpression& expr, int expressionIdx) {
        for (const auto& index_doubleMatrix : expr.matrixCoefficients) {
            const auto& index = index_doubleMatrix.first;
            const auto& matrix = index_doubleMatrix.second;

            for (int i = 0; i < getMatrixSize(); ++i) {
                for (int j = i; j < getMatrixSize(); ++j) {
                    if (doubleIsZero(matrix[i][j]))
                        continue;
                    os << expressionIdx + 1 << " " // A_i index
                    << csdpSosIdxToBlock[index] << " " // block index
                    << i + 1 << " " // row index
                    << j + 1 << " " // column index
                    << doubleToString(matrix[i][j]) << std::endl;
                }
            }
        }

        for (const auto& index_Value: expr.freeCoefficients) {
            const auto& index = index_Value.first;
            const auto& value = index_Value.second;

            if (doubleIsZero(value))
                continue;

            os << expressionIdx + 1 << " " //
            << csdpUnconstrainedIdxToBlock[index] << " " // block index
            << 1 << " " // row index
            << 1 << " " // column index
            << doubleToString(value) << std::endl;
            os << expressionIdx + 1 << " " //
               << csdpUnconstrainedIdxToBlock[index] << " " // block index
               << 2 << " " // row index
               << 2 << " " // column index
               << doubleToString(-value) << std::endl;
        }
    }

    void writeCsdp(std::ostream& os) {
        writeCsdpHeader(os);
        writeCsdpCmatrix(os);

        int conditionIdx = 0;
        for (const auto& condition : conditions) {
            writeCsdpLinearMatrixExpression(os, condition, conditionIdx);
            conditionIdx += 1;
        }
    }

    std::pair<std::vector<std::vector<std::vector<double>>>, std::vector<double> > readCsdp(std::istream& inp) {
//        std::map<int, int> csdpSosIdxToBlock;
//        std::map<int, int> csdpUnconstrainedIdxToBlock;
        std::map<int, int> blockIdxToCsdpSosIdx;
        std::map<int, int> blockIdxToCsdpUnconstrainedIdx;
        std::set<int> inMatrix;
        std::set<int> inUnconstrained;

        // reversing maps
        for (const auto& idx_block : csdpSosIdxToBlock) {
            blockIdxToCsdpSosIdx[idx_block.second] = idx_block.first;
            inMatrix.insert(idx_block.second);
        }
        for (const auto& idx_block : csdpUnconstrainedIdxToBlock) {
            blockIdxToCsdpUnconstrainedIdx[idx_block.second] = idx_block.first;
            inUnconstrained.insert(idx_block.second);
        }

        std::vector<std::vector<std::vector<double>>> matrices(getNumberOfSdpMatrices());
        for (auto& matrix: matrices) {
            matrix = std::vector<std::vector<double>>(getMatrixSize(), std::vector<double>(getMatrixSize(), 0.0));
        }
        std::vector<double> variables = std::vector<double>(getNumberOfUnconstrainedVariables(), 0.0);

        std::vector<double> dualsY = std::vector<double>(getNumberOfConditions(), 0.0);

        for (auto& dual: dualsY) {
            inp >> dual;
        }

        int option;
        const int optionIgnore = 1;
        const int optionToMatrix = 2;
        int blockIdx, rowIdx, colIdx;
        double entryVal;

        while (inp >> option >> blockIdx >> rowIdx >> colIdx >> entryVal) {
            if (option == optionIgnore)
                continue;

            if (inMatrix.find(blockIdx) != inMatrix.end()) { // we are sos entry
                int rowIdxZeroBased = rowIdx - 1;
                int colIdxZeroBased = colIdx - 1;
                int sosIdx = blockIdxToCsdpSosIdx[blockIdx];
                matrices[sosIdx][rowIdxZeroBased][colIdxZeroBased] = entryVal;
                if (rowIdx != colIdx) {
                    matrices[sosIdx][colIdxZeroBased][rowIdxZeroBased] = entryVal;
                }
            } else {
                int unconstrainedIdx = blockIdxToCsdpUnconstrainedIdx[blockIdx];
                if (rowIdx == 1 && colIdx == 1) {
                    variables[unconstrainedIdx] += entryVal;
                } else if (rowIdx == 2 && colIdx == 2) {
                    variables[unconstrainedIdx] -= entryVal;
                } else {
                    throw std::runtime_error("unexpected rowIdx and colIdx");
                }
            }
        }

        return std::make_pair(matrices, variables);

    }


    fus::Variable::t slice(fus::Variable::t X, int d, int j) {
        return
                X->slice(new_array_ptr<int,1>({j,0,0}), new_array_ptr<int,1>({j+1,d,d}))
                        ->reshape(new_array_ptr<int,1>({d,d}));
    }
    void solveWithMosek() {
        // the code taken from https://docs.mosek.com/latest/cxxfusion/tutorial-sdo-shared.html and modified

        const auto& conditions = getConditions();

        int n = getNumberOfSdpMatrices(), d = getMatrixSize(), k = getNumberOfConditions();

        std::vector<double> b(k, 0.0);
        for (int i = 0; i < k; ++i) {
            if (conditions[i].type == LinearMatrixExpressionType::IN_RANGE) {
                b[i] = conditions[i].withinRange;
            }
        }

//        std::map<std::pair<int, int>, std::shared_ptr<ndarray<double,2>>> A;
        std::vector<std::map<int, std::shared_ptr<ndarray<double,2>>>> A;
        for (int coditionIndex = 0; coditionIndex < k; ++coditionIndex) {
            A.emplace_back();

            for (auto matrixIndex_matrix : conditions[coditionIndex].matrixCoefficients) {
                auto matrixIndex = matrixIndex_matrix.first;
                const auto& matrix = matrixIndex_matrix.second;

                auto A_ij = std::make_shared<ndarray<double,2>>(shape(d,d));
                for (int i = 0; i < d; ++i) {
                    for (int j = 0; j < d; ++j) {
                        (*A_ij)(i,j) = matrix[i][j];
                    }
                }
                A.back()[matrixIndex] = A_ij;
            }
        }


        // Create a model with n semidefinite variables of dimension d x d


        fus::Variable::t X = M->variable(fus::Domain::inPSDCone(d, n));
        fus::Variable::t unconstrained = M->variable(fus::Domain::unbounded(getNumberOfUnconstrainedVariables()));



        // Each constraint is a sum of inner products
        // Each semidefinite variable is a slice of X
        for(int i=0; i<k; i++) {
            std::vector<fus::Expression::t> sumlist;

            for (auto matrixIndex_matrix : A[i]) {
                auto matrixIndex = matrixIndex_matrix.first;
                const auto& matrix = matrixIndex_matrix.second;
                sumlist.push_back(fus::Expr::dot(matrix, slice(X, d, matrixIndex)));
            }

            for (auto coeffIndex_freeCoefficient : conditions[i].freeCoefficients) {
                auto coeffIndex = coeffIndex_freeCoefficient.first;
                auto freeCoefficient = coeffIndex_freeCoefficient.second;
                sumlist.push_back(fus::Expr::mul(unconstrained->index(coeffIndex), freeCoefficient));
            }

            sumlist.push_back(fus::Expr::constTerm(conditions[i].constantPart));

            if (conditions[i].type == LinearMatrixExpressionType::GEQ)
                M->constraint(fus::Expr::add(new_array_ptr(sumlist)), fus::Domain::greaterThan(b[i]));
            else if (conditions[i].type == LinearMatrixExpressionType::EQ)
                M->constraint(fus::Expr::add(new_array_ptr(sumlist)), fus::Domain::equalsTo(b[i]));
            else if (conditions[i].type == LinearMatrixExpressionType::IN_RANGE)
                M->constraint(fus::Expr::add(new_array_ptr(sumlist)), fus::Domain::inRange(-b[i], b[i]));
        }

        // set all unconstrained variavles less than the objectiveVariable

//        fus::Variable::t objectiveVariable = M->variable(fus::Domain::unbounded(1));
//        for (int i = 0; i < getNumberOfUnconstrainedVariables(); ++i) {
//            M->constraint(fus::Expr::sub(unconstrained->index(i), objectiveVariable), fus::Domain::lessThan(0.0));
//            M->constraint(fus::Expr::sub(fus::Expr::mul(unconstrained->index(i), -1.0), objectiveVariable), fus::Domain::lessThan(0.0));
//        }


//        M->objective(fus::ObjectiveSense::Minimize, objectiveVariable);
        M->objective(fus::ObjectiveSense::Minimize, fus::Expr::constTerm(0.0));

        // Solve
        M->setLogHandler([ = ](const std::string & msg) { std::cout << msg << std::flush; } );            // Add logging
        M->writeTask("sdosdo.ptf");                // Save problem in readable format

//        printSystem(std::cerr, true);


        M->setSolverParam("presolveUse", "off");
        M->solve();

#ifdef SDP_PROBLEM_DEBUG
        // print inner to outer

        outerMatrixIndexToInnerMatrixIndex;
        innerMatrixIndexToOuterMatrixIndex;
        std::cout << "Inner to Outer:" << std::endl;
        for (auto inner_outer : innerMatrixIndexToOuterMatrixIndex) {
            std::cout << inner_outer.first << " -> " << inner_outer.second << std::endl;
        }
        std::cout << "Outer to Inner:" << std::endl;
        for (auto outer_inner : outerMatrixIndexToInnerMatrixIndex) {
            std::cout << outer_inner.first << " -> " << outer_inner.second << std::endl;
        }
#endif

        // Get results. Each variable is a slice of X

        std::vector<std::vector<std::vector<double>>> matrices(n, std::vector<std::vector<double>>(d, std::vector<double>(d)));
        std::vector<double> unconstrainedVariables(getNumberOfUnconstrainedVariables());

        for(int j=0; j<n; j++) {
            auto Xj = *(slice(X, d, j)->level());
            for(int s1=0; s1<d; s1++) {
                for(int s2=0; s2<d; s2++) {
                    matrices[j][s1][s2] = Xj[s1*d+s2];
                }
            }
        }

        for (int i = 0; i < getNumberOfUnconstrainedVariables(); ++i) {
            unconstrainedVariables[i] = (*unconstrained->level())[i];
        }

        setSolution(matrices, unconstrainedVariables);
        setupSolution();

#ifdef SDP_PROBLEM_DEBUG
        std::cerr << "Solution, form spdProblem.h:" << std::endl;
        printSolution(std::cout);
#endif
    }




    struct Solution {
        std::map<int, std::vector<std::vector<double>>> matrices;
        std::map<std::string, double> unconstrainedVariables;

        std::map<int, int> innerMatrixIndexToOuterMatrixIndex;
        std::map<int, int> outerMatrixIndexToInnerMatrixIndex;

    };

    const Solution& getSolution() {
//        Solution solution;

        if (solutionState != SOLVED) {
            throw std::runtime_error("Solution is not available");
        }

        return solution;
    }

    void setIgnore(std::string ignoredVariableName) {
        ignoredInnerVariables.insert(ignoredVariableName);
    }

    std::map<std::string, double> getSolutionAsMap(const std::set<std::string>& igoredVariables = {}) {
        if (solutionState != SOLVED) {
            throw std::runtime_error("Solution is not available");
        }

        std::map<std::string, double> answer;
        for (auto matrixIndex_Matrix2d: solution.matrices) {
            auto matrixIndex = matrixIndex_Matrix2d.first;

            // TODO(@Sergei): wtf
            matrixIndex = solution.innerMatrixIndexToOuterMatrixIndex[matrixIndex];
            matrixIndex = solution.innerMatrixIndexToOuterMatrixIndex[matrixIndex];

//            matrixIndex = solution.outerMatrixIndexToInnerMatrixIndex[matrixIndex];

            auto matrixName = matrixIndex_Matrix2d.second;
            auto prefix = "l_" + std::to_string(matrixIndex) + "_";
            for (int i = 0; i < matrixName.size(); ++i) {
                for (int j = 0; j < matrixName[i].size(); ++j) {
                    answer[prefix + std::to_string(i) + "_" + std::to_string(j)] = matrixName[i][j];
                }
            }
        }
        for (auto unconstrainedVariableName_value : solution.unconstrainedVariables) {
            auto unconstrainedVariableName = unconstrainedVariableName_value.first;
            auto value = unconstrainedVariableName_value.second;
            answer[unconstrainedVariableName] = value;
        }

        // delete ignored variables from the answer
        for (auto ignoredVariable : igoredVariables) {
            answer.erase(ignoredVariable);
        }
        for (auto ignoredVariable: ignoredInnerVariables) {
            answer.erase(ignoredVariable);
        }

        return answer;
    }

    void printSolution(std::ostream& os) {
        auto solution = getSolution();

        for (auto matrixIndex_matrix : solution.matrices) {
            auto matrixIndex = matrixIndex_matrix.first;
            const auto& matrix = matrixIndex_matrix.second;

            os << "Matrix " << matrixIndex << ":\n";
            for (int i = 0; i < matrix.size(); ++i) {
                for (int j = 0; j < matrix[i].size(); ++j) {
                    os << matrix[i][j] << " ";
                }
                os << "\n";
            }
            os << "\n";
        }

        for (auto unconstrainedVariableName_value : solution.unconstrainedVariables) {
            auto unconstrainedVariableName = unconstrainedVariableName_value.first;
            auto value = unconstrainedVariableName_value.second;

            os << "Unconstrained variable " << unconstrainedVariableName << ": " << value << "\n";
        }
    }

    void setAllowedError(double allowedError) {
        this->allowedError = allowedError;
    }


private:
    void setupSolution() {
        solutionState = SOLVED;

        for (auto matrixIndex : matrixIndices) {
            solution.matrices[matrixIndex] = solutionMatrices[decodeMatrixIndexFromInner(matrixIndex)];
        }

        for (auto unconstrainedVariableName : unconstrainedVariables) {
            solution.unconstrainedVariables[unconstrainedVariableName] =
                    solutionUnconstrainedVariables[unconstrainedVariableNameToInnerIndex[unconstrainedVariableName]];
        }

        solution.innerMatrixIndexToOuterMatrixIndex = innerMatrixIndexToOuterMatrixIndex;
        solution.outerMatrixIndexToInnerMatrixIndex = outerMatrixIndexToInnerMatrixIndex;

    }

    int allMatricesSize = 0;

    std::vector<LinearMatrixExpression> conditions;
    LinearMatrixExpression objective;

    Solution solution;
    std::set<int> matrixIndices;
    std::map<int, int> outerMatrixIndexToInnerMatrixIndex;
    std::map<int, int> innerMatrixIndexToOuterMatrixIndex;
    int maxMatrixIndex = 0;

    std::set<std::string> unconstrainedVariables;
    std::map<std::string, int> unconstrainedVariableNameToInnerIndex;
    std::map<int, std::string> innerIndexToUnconstrainedVariableName;
    int maxUnconstrainedVariableIndex = 0;

    static const int READY_TO_START = -1;
    static const int READY_TO_ADD = 0;

    int state = READY_TO_START;


    static const int NOT_SOLVED = -1;
    static const int SOLVED = 0;
    static const int UNFEASIBLE = 1;

    int solutionState = NOT_SOLVED;

    std::vector<std::vector<std::vector<double>>> solutionMatrices;
    std::vector<double> solutionUnconstrainedVariables;


    static double twoMatricesProduct(const std::vector<std::vector<double>>& matrix1, const std::vector<std::vector<double>>& matrix2) {
        double result = 0;
        for (int i = 0; i < matrix1.size(); ++i) {
            for (int j = 0; j < matrix1[i].size(); ++j) {
                result += matrix1[i][j] * matrix2[i][j];
            }
        }
        return result;
    }

    double allowedError = 1e-6;

    std::set<std::string> ignoredInnerVariables;

    fus::Model::t M;

};

#endif //MYPROJECT_SDPPROBLEM_H
