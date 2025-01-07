//
// Created by sergey on 30.05.23.
//

#include "symbolicRing.h"
#include "sdpEncoder.h"
#include "programExpression.h"
#include "tokenizer.h"
#include "program.h"
#include "programParser.h"
#include "expressionParser.h"
#include "solver.h"
#include "combinatorics.h"
#include "automaitcComplexityEstimator.h"
#include "hacks.h"
#include "sdpProblem.h"
#include "templateEngine.h"

//
#include <gtest/gtest.h>

using namespace symbolic_ring;

template <typename T>
std::string toString(const T &m) {
    std::stringstream s;
    s << m;
    return s.str();
}

TEST(SymbolicTest, SymbolCreation) {
    auto env = SymbolicEnvironment();
    auto x = env.sym("x");

    EXPECT_EQ(x.getName(), "x");
}

TEST(SymbolicTest, CreatingExistingSymbol) {
    auto env = SymbolicEnvironment();
    auto x = env.sym("x");
    EXPECT_THROW(env.sym("x"), std::runtime_error);
}

TEST(SymbolicTest, SymbolEquality) {
    auto env = SymbolicEnvironment();
    auto x = env.sym("x");
    auto y = env.sym("y");
    auto x1 = x;

    EXPECT_EQ(x, x1);
    EXPECT_NE(x, y);
}

TEST(SymbolicTest, SymbolOrder) {
    auto env = SymbolicEnvironment();
    auto x = env.sym("x");
    auto y = env.sym("y");
    auto z = env.sym("z");

    EXPECT_LT(x, y);
    EXPECT_LT(y, z);
    EXPECT_LT(x, z);
}

TEST(SymbolicTest, MultiplyMonomials) {
    auto env = SymbolicEnvironment();
    auto x = env.sym("x");
    auto y = env.sym("y");

    auto p = QMonomial(x);
    auto q = QMonomial(y);

    auto r1 = mul(p, q);
    auto r2 = mul(mul(q, p), p);

    std::stringstream s;
    s << r1;
    EXPECT_EQ(std::string("(1/1)*x^(1)*y^(1)"), s.str());

    std::stringstream s1;
    s1 << r2;
    EXPECT_EQ(std::string("(1/1)*x^(2)*y^(1)"), s1.str());

}


TEST(SymbolicTest, CreateQPolynomial) {
    auto env = SymbolicEnvironment();
    auto x = env.sym("x");
    auto p = QPolynomial(x);

    std::stringstream s;
    s << p;

    EXPECT_EQ(std::string("(1/1)*x^(1)"), s.str());
}


TEST(SymbolicTest, AddPolynomial) {
    auto env = SymbolicEnvironment();
    auto px = QPolynomial(env.sym("x"));
    auto py = QPolynomial(env.sym("y"));

    auto p = add(px, py);

    std::stringstream s;
    s << p;

    EXPECT_EQ(std::string("(1/1)*x^(1) + (1/1)*y^(1)"), s.str());

    auto p2 = add(px, px);

    std::stringstream s2;
    s2 << p2;

    EXPECT_EQ(std::string("(2/1)*x^(1)"), s2.str());
}

TEST(SymbolicTest, MultiplyPolynomial) {
    auto env = SymbolicEnvironment();
    auto px = QPolynomial(env.sym("x"));
    auto py = QPolynomial(env.sym("y"));

    auto p = mul(px, py);

    std::stringstream s;
    s << p;

    EXPECT_EQ(std::string("(1/1)*x^(1)*y^(1)"), s.str());

    auto p2 = mul(px, px);

    std::stringstream s2;
    s2 << p2;

    EXPECT_EQ(std::string("(1/1)*x^(2)"), s2.str());
}

TEST(SymbolicTest, BinomialTest) {
    auto env = SymbolicEnvironment();
    auto px = QPolynomial(env.sym("x"));
    auto py = QPolynomial(env.sym("y"));

    auto p = add(px, py);

    auto ppow3 = env.qPolynomialOne();
    for (int i = 0; i < 3; ++i) {
        ppow3 = mul(ppow3, p);
    }
    ppow3.reduce();
    std::stringstream s;
    s << ppow3;

    EXPECT_EQ(std::string("(3/1)*x^(1)*y^(2) + (3/1)*x^(2)*y^(1) + (1/1)*x^(3) + (1/1)*y^(3)"), s.str());
}

TEST(SymbolicTest, PrintSymbolicMonomial) {
    auto env = SymbolicEnvironment();
    auto x = env.sym("x");
    auto y = env.sym("y");

    auto p = SymbolicMonomial(x);
    auto q = SymbolicMonomial(QPolynomial(y));

    std::cout << p << std::endl;
    std::cout << q << std::endl;
    std::stringstream ps, qs;
    ps << p;
    qs << q;

    EXPECT_EQ(std::string("(1/1)*x^(1)*[(1/1)]"), ps.str());
    EXPECT_EQ(std::string("(1/1)*[(1/1)*y^(1)]"), qs.str());
}

TEST(SymbolicTest, UnitizeSymbolicMonomial) {
    auto env = SymbolicEnvironment();
    auto x = env.sym("x");
    auto a = env.sym("a");
    auto b = env.sym("b");


    auto px = mul(3, QMonomial(x));
    auto pa = QPolynomial(a);
    auto pb = div(mul(2, QPolynomial(b)), 5);

    auto p = SymbolicMonomial(px, add(pa, pb));

    p.unitize();

    ASSERT_EQ(std::string("(1/1)*x^(1)*[(3/1)*a^(1) + (6/5)*b^(1)]"), toString(p));
}


TEST(SymbolicTest, SymbolicPolynomialAddition) {
    auto env = SymbolicEnvironment();
    auto x = env.sym("x");
    auto a = env.sym("a");
    auto b = env.sym("b");

    auto px = mul(3, QMonomial(x));
    auto pa = QPolynomial(a);
    auto pb = mul(2, QPolynomial(b));

    auto p = SymbolicPolynomial(SymbolicMonomial(px, pa));
    auto q = SymbolicPolynomial(SymbolicMonomial(px, pb));



    ASSERT_EQ(std::string("(3/1)*x^(1)*[(1/1)*a^(1)]"), toString(p));
    ASSERT_EQ(std::string("(3/1)*x^(1)*[(2/1)*b^(1)]"), toString(q));
    ASSERT_EQ(std::string("(1/1)*x^(1)*[(3/1)*a^(1) + (6/1)*b^(1)]"), toString(add(p, q, true)));

}

TEST(SymbolicTest, SymbolicPolynomialMultiplication) {
    auto env = SymbolicEnvironment();
    auto x = env.sym("x");
    auto y = env.sym("y");
    auto a = env.sym("a");
    auto b = env.sym("b");

    auto px = mul(3, QMonomial(x));
    auto py = QMonomial(y);
    auto pa = QPolynomial(a);
    auto pb = mul(2, QPolynomial(b));

    auto p = SymbolicPolynomial(SymbolicMonomial(px, pa));
    auto q = SymbolicPolynomial(SymbolicMonomial(py, pb));

    ASSERT_EQ(std::string("(3/1)*x^(1)*y^(1)*[(2/1)*a^(1)*b^(1)]"), toString(mul(p, q)));

}

TEST(SymbolicTest, GetSos) {
    auto env = SymbolicEnvironment();
    auto x = QMonomial(env.sym("x"));
    auto y = QMonomial(env.sym("y"));

    auto p = getSos({x, y}, 1);
    std::cout << p << std::endl;

}


TEST(SymbolicTest, TestSubstitute) {
    auto env = SymbolicEnvironment();
    auto x = QMonomial(env.sym("x"));
    auto y = QMonomial(env.sym("y"));

    auto px = QMonomial(x);
    auto py = QPolynomial(y);


    px = mul(px, px);

    std::cout << px << std::endl;
    std::cout << substitute(px, env.getOrCreate("x"), add(x, py)) << std::endl;

}


TEST(SymbolicTest, SubstituteInSymbolicPolynomial) {
    auto env = SymbolicEnvironment();
    auto x = env.sym("x");
    auto y = env.sym("y");
    auto a = env.sym("a");
    auto b = env.sym("b");

    auto px = QMonomial(x);
    auto pa = QPolynomial(a);
    auto pb = QPolynomial(b);

    auto p = SymbolicPolynomial(SymbolicMonomial(px, pa)); // x * [a]
//    auto q = SymbolicPolynomial(SymbolicMonomial(px, pb));

    std::cout << p << std::endl;

    auto result = substituteInBase(p, env.getOrCreate("x"), add(QPolynomial(x), QPolynomial(y)));
    std::cout << result << std::endl;

    auto result2 = substituteInCoefficients(p, env.getOrCreate("a"), add(QPolynomial(a), QPolynomial(b)));
    std::cout << result2 << std::endl;

    auto fortytwo = mul(env.qPolynomialOne(), 42);
    auto result3 = substituteInCoefficients(result2, env.getOrCreate("a"), fortytwo);
    std::cout << result3 << std::endl;

}

TEST(ProgramExpressionTest, BasicProgramExpression1) {
    // transforms x >= y to x - y >= 0
    auto env = SymbolicEnvironment();
    auto x = QPolynomial(env.sym("x"));
    auto y = QPolynomial(env.sym("y"));

    auto exprx = std::make_unique<Variable>("x");
    auto expry = std::make_unique<Variable>("y");
    auto xgeqy = std::make_unique<BinaryRelation>(std::move(exprx), std::move(expry), ">=");

    auto str_repr = xgeqy->toString();

    std::cout << str_repr << std::endl;
    ASSERT_EQ(std::string("(x >= y)"), str_repr);
}

TEST(ProgramExpressionTest, TestFunction) {
    // transforms x >= y to x - y >= 0
    auto env = SymbolicEnvironment();
    auto x = QPolynomial(env.sym("x"));
    auto y = QPolynomial(env.sym("y"));

    auto exprx = std::make_unique<Variable>("x");
    auto expry = std::make_unique<Variable>("y");
    auto xgeqy = std::make_unique<BinaryRelation>(std::move(exprx), std::move(expry), ">=");

    auto str_repr = xgeqy->toString();

    std::cout << str_repr << std::endl;
    ASSERT_EQ(std::string("(x >= y)"), str_repr);
}


TEST(ProgramExpressionTest, SubstitutePart) {
    // transforms x >= y to x - y >= 0
    auto env = SymbolicEnvironment();
    auto x = QPolynomial(env.sym("x"));
    auto y = QPolynomial(env.sym("y"));
    auto z = QPolynomial(env.sym("z"));

    auto exprx = std::make_unique<Variable>("x");
    auto expry = std::make_unique<Variable>("y");
    auto exprz = std::make_unique<Variable>("z");

    auto xgeqy = std::make_unique<BinaryRelation>(std::move(exprx), std::move(expry), ">=");


    auto str_repr = xgeqy->toString();

    std::cout << str_repr << std::endl;
    ASSERT_EQ(std::string("(x >= y)"), str_repr);
}


TEST(ProgramExpressionTest, BasicProgramExpressionWithEvaluationContext) {
    // transforms x >= y to x - y >= 0
    auto env = SymbolicEnvironment();
    auto x = QPolynomial(env.sym("x"));
    auto y = QPolynomial(env.sym("y"));

    auto eval_ctx = EvaluationContext(&env);
    eval_ctx.setVariableQPolynomial("x", x);
    eval_ctx.setVariableQPolynomial("y", y);

    auto exprx = std::make_unique<Variable>("x");
    auto expry = std::make_unique<Variable>("y");
    auto xgeqy = std::make_unique<BinaryRelation>(std::move(exprx), std::move(expry), ">=");

    auto evaluation_result = xgeqy->evaluate(eval_ctx);
    auto str_repr = toString(evaluation_result.getSymbolicPolynomial());
//    std::cout << str_repr << std::endl;
    ASSERT_EQ(str_repr, std::string("(1/1)*x^(1)*[(1/1)] + (1/1)*y^(1)*[(-1/1)]"));
}

TEST(ProgramExpressionTest, EvaluationWithDivision) {
    // transforms x >= y to x - y >= 0
    auto env = SymbolicEnvironment();
    auto x = QPolynomial(env.sym("x"));
//    auto six = mul(env.qPolynomialOne(), 6);


    auto eval_ctx = EvaluationContext(&env);
    eval_ctx.setVariableQPolynomial("x", x);
//    eval_ctx.setVariableQPolynomial("y", six);

    auto exprx = std::make_unique<Variable>("x");
    auto exprsix = std::make_unique<Constant>(6);
    auto xdivsix = std::make_unique<BinaryOperation>(std::move(exprx), std::move(exprsix), "/");

    auto xdivdixgeqzero = std::make_unique<BinaryRelation>(std::move(xdivsix), std::make_unique<Constant>(1), ">=");
    auto evaluation_result = xdivdixgeqzero->evaluate(eval_ctx);
    auto str_repr = toString(evaluation_result.getSymbolicPolynomial());
    std::cout << str_repr << std::endl;
//    ASSERT_EQ(str_repr, std::string("(1/1)*x^(1)*[(1/1)] + (1/1)*y^(1)*[(-1/1)]"));
}

TEST(ProgramExpressionTest, EvaluationExpressionWithQPolynomial) {
    auto env = SymbolicEnvironment();
    auto x = QPolynomial(env.sym("x"));
    auto x_sq = symbolicPolynomialfromQPolynomialAsBase(mul(x, x));
    auto a = env.sym("a");
    auto free_var = SymbolicPolynomial(SymbolicMonomial(QPolynomial(a)));
    x_sq = mul(x_sq, free_var);
    std::cout << x_sq << std::endl;
    std::cout << free_var << std::endl;

    auto y = QPolynomial(env.sym("y"));

    auto eval_ctx = EvaluationContext(&env);
    eval_ctx.setVariableQPolynomial("x", x);
    eval_ctx.setVariableQPolynomial("y", y);
    eval_ctx.setSymbolicPolynomial("x_sq", x_sq, {"x"});

    auto s = std::make_unique<int>(5);

    auto exprx = std::make_unique<Variable>("x");
    auto expry = std::make_unique<Variable>("y");
    auto x_plus_y = std::make_unique<BinaryOperation>(std::move(exprx), std::move(expry), "+");
    auto x_plus_y_vec = std::vector<std::unique_ptr<ExpressionElement>>();
    x_plus_y_vec.push_back(std::move(x_plus_y));
    auto x_plus_y_sq = std::make_unique<Function>(std::move(x_plus_y_vec), std::move(std::string("x_sq")));



//    auto x_plus_y = Function(std::move(x_and_y), std::string("x_sq"), {"x"});
//    auto x_plus_y_squared = std::make_unique<Function>(std::move(x_plus_y_sq), std::move(std::string("x_sq")));

    auto result = x_plus_y_sq->evaluate(eval_ctx).getSymbolicPolynomial();

    auto str_result = toString(x_plus_y_sq->evaluate(eval_ctx).getSymbolicPolynomial());
//    std::cout << str_result << std::endl;
    ASSERT_EQ(str_result, std::string("(2/1)*x^(1)*y^(1)*[(1/1)*a^(1)] + (1/1)*x^(2)*[(1/1)*a^(1)] + (1/1)*y^(2)*[(1/1)*a^(1)]"));
////    auto x_plus_y_vec = std::vector<std::unique_ptr<ExpressionElement>>(std::move(x_plus_y));
//    auto x_plus_y_squared = Function(x_plus_y, std::string("x_sq"), {"x"});
//

}


TEST(ProgramExpressionTest, TestClone) {
    auto env = SymbolicEnvironment();
    auto ctx = EvaluationContext(&env);

    auto x = std::make_unique<Variable>("x");
    auto y = std::make_unique<Variable>("y");
    auto z = std::make_unique<Variable>("z");

    auto x_plus_y = std::make_unique<BinaryOperation>(std::move(x), std::move(y), "+");
    auto x_plus_y_vec = std::vector<std::unique_ptr<ExpressionElement>>();
    x_plus_y_vec.push_back(std::move(x_plus_y));

    auto f_of_x_plus_y = std::make_unique<Function>(std::move(x_plus_y_vec), std::string("f"));

    std::cout << f_of_x_plus_y->toString() << std::endl;
    auto f_of_x_plus_y_clone = f_of_x_plus_y->clone();

    auto z_vec = std::vector<std::unique_ptr<ExpressionElement>>();
    z_vec.push_back(std::move(z));


    ASSERT_EQ(f_of_x_plus_y->toString(), f_of_x_plus_y_clone->toString());
}

TEST(TokenizationTest, Test1) {
    std::string file_content = "real a, b, c; function T[2]; // some remark \n if(n*n >= 0; n==1)  ==> {T(n) == 1}";
    std::istringstream iss(file_content);
    std::vector<Token> tokens3 = tokenize(iss);
    std::cout << "Tokens in file3.aut:" << std::endl;
    printTokens(tokens3);
}

TEST(ProgramTest, TestDeclareReal) {
    auto p = Program();
    p.declareReal("a");
    p.declareReal("b");
    ASSERT_THROW(p.declareReal("a"), std::runtime_error);
}

TEST(ProgramTest, TestDeclareFunction) {
    auto p = Program();
    p.declareFunction("R", 1, 1);
    p.declareFunction("T", 1, 2);

    ASSERT_THROW(p.declareFunction("R", 3, 1), std::runtime_error);
}

TEST(ProgramTest, TestInsertIfThenCondition) {
    auto p = Program();

    auto exprx = std::make_unique<Variable>("x");
    auto exprzero = std::make_unique<Constant>(0);

    auto xgeqzero = std::make_unique<BinaryRelation>(std::move(exprx), std::move(exprzero), ">=");

    auto exprx2 = std::make_unique<Variable>("x");
    auto exprx3 = std::make_unique<Variable>("x");
    auto exprxtimesx = std::make_unique<BinaryOperation>(std::move(exprx2), std::move(exprx3), "*");

    auto exprzero2 = std::make_unique<Constant>(0);

    auto xsquaregeqzero = std::make_unique<BinaryRelation>(std::move(exprxtimesx), std::move(exprzero2), ">=");

    std::vector<std::unique_ptr<ExpressionElement>> xgeqzero_vec;
    xgeqzero_vec.push_back(std::move(xgeqzero));


    std::vector<std::unique_ptr<ExpressionElement>> xsquaregeqzero_vec;
    xsquaregeqzero_vec.push_back(std::move(xsquaregeqzero));

    p.addIfThenCondition(std::move(xgeqzero_vec), std::move(xsquaregeqzero_vec));

    std::cout << p.getConditions()[0].getConditions()[0]->toString() << std::endl;
    std::cout << p.getConditions()[0].getConclusions()[0]->toString() << std::endl;
}


TEST(TestParser, RealVariables1) {
//    const char* programText = "real a, b, c;"
//                  "function T[2,1];"
//                  "if {a >= 0} => {T[0] * a >= 1}";

    const char* programText = "real a, b, c;"
                             "real d, e;"
                             "real f;";

    std::istringstream iss(programText);
    auto programTokens = tokenize(iss);
    printTokens(programTokens);
    auto p = Program();
    parse(programTokens, p);
    auto vars = p.getTable().getDeclaredVariables();
    std::sort(vars.begin(), vars.end());
    std::vector<std::string> expectedVars = {"a", "b", "c", "d", "e", "f"};
    ASSERT_EQ(vars, expectedVars);
}


TEST(TestParser, Test1) {
    const char* programText = "real a, b, c;"
                  "function T[2,1], G[1]; \n"
                  "\n"
                  "// some lines\n"
                  "real m, n;\n"
                  "function R[1,44];\n";

    std::cout << programText << std::endl;
    std::istringstream iss(programText);
    auto programTokens = tokenize(iss);
    printTokens(programTokens);
    auto p = Program();
    parse(programTokens, p);

    auto vars = p.getTable().getDeclaredVariables();
    std::cout << "Declared variables:" << std::endl;
    for (auto& v : vars) {
        std::cout << v << " ";
    }
    std::cout << std::endl;

    auto funcs = p.getTable().getDeclaredFunctions();
    std::cout << "Declared functions:" << std::endl;
    for (auto& f : funcs) {
        std::cout << f << " arity: " << p.getFunctionSignature(f)[0] << " hdegree: " << p.getFunctionSignature(f)[1] << std::endl;
    }
    std::cout << std::endl;
}


TEST(TestParser, Test2) {
    const char* programText = "real a, b, c;\n"
                              "if {a > 0} => {b > 0} if {b >0 } => {c>0} \n"
                              "if {a >= 0; b > 0} => {-1 == 0} \n "
                              "if {T(x, y) >= 0; T(y) * x == 1} => {-1 - 1 < 2 + 0}";

    std::istringstream iss(programText);
    auto programTokens = tokenize(iss);
    printTokens(programTokens);
    auto p = Program();
    parse(programTokens, p);

}


TEST(TestExpressionParser, TestgetArgumentRangesOfFunctionCall) {
    auto functionCallExpression = std::istringstream("T(x, x + y, x * y)");
    auto functionCallTokens = tokenize(functionCallExpression);
    functionCallTokens.pop_back(); // remove EOF token
    printTokens(functionCallTokens);
    auto argumentRanges = getArgumentRangesOfFunctionCall(functionCallTokens,
                                                          IndexRange(0, static_cast<int>(functionCallTokens.size())));
    auto trueArgumentRanges = std::vector<IndexRange>{IndexRange(2, 3), IndexRange(4, 7), IndexRange(8, 11)};
    ASSERT_EQ(argumentRanges, trueArgumentRanges);
}

std::vector<Token> getExpressionTokensFromString(const std::string& str) {
    std::istringstream iss(str);
    auto tokens = tokenize(iss);
    tokens.pop_back(); // remove EOF token
    return tokens;
}

TEST(TestExpressionParser, TestParsePlus) {
    auto expression_tkns = getExpressionTokensFromString("x + y");
    printTokens(expression_tkns);
    auto expr = parseExpression(expression_tkns);
    ASSERT_EQ(expr->toString(), "(x + y)");
}

TEST(TestExpressionParser, TestParseMinus) {
    auto expression_tkns = getExpressionTokensFromString("x - y");
    printTokens(expression_tkns);
    auto expr = parseExpression(expression_tkns);
    ASSERT_EQ(expr->toString(), "(x - y)");
}

TEST(TestExpressionParser, TestParseUnaryMinus) {
    auto expression_tkns = getExpressionTokensFromString("-x");
    printTokens(expression_tkns);
    auto expr = parseExpression(expression_tkns);
    ASSERT_EQ(expr->toString(), "(-x)");
}

TEST(TestExpressionParser, TestParseUnaryPlus) {
    auto expression_tkns = getExpressionTokensFromString("+x");
    printTokens(expression_tkns);
    auto expr = parseExpression(expression_tkns);
    ASSERT_EQ(expr->toString(), "(+x)");
}

TEST(TestExpressionParser, TestParseMul) {
    auto expression_tkns = getExpressionTokensFromString("x * y");
    printTokens(expression_tkns);
    auto expr = parseExpression(expression_tkns);
    ASSERT_EQ(expr->toString(), "(x * y)");
}

TEST(TestExpressionParser, TestParseDiv) {
    auto expression_tkns = getExpressionTokensFromString("x / y");
    printTokens(expression_tkns);
    auto expr = parseExpression(expression_tkns);
    ASSERT_EQ(expr->toString(), "(x / y)");
}


TEST(TestExpressionParser, TestBallance) {
    auto expression_tkns = getExpressionTokensFromString("x * x * x * x * x * x * x * x");
    printTokens(expression_tkns);
    auto expr = parseExpression(expression_tkns);
    ASSERT_EQ(expr->toString(), "(((((((x * x) * x) * x) * x) * x) * x) * x)");
}


TEST(TestExpressionParser, TestPriority) {
    auto expression_tkns = getExpressionTokensFromString("x * y + z");
    auto expr = parseExpression(expression_tkns);
    auto stringExpr = expr->toString();
    ASSERT_EQ(stringExpr, "((x * y) + z)");
}

TEST(TestExpressionParser, TestBrace) {
    auto expression_tkns = getExpressionTokensFromString("(-(a + b))");
    auto expr = parseExpression(expression_tkns);
    auto stringExpr = expr->toString();
    ASSERT_EQ(stringExpr, "(-(a + b))");
}

TEST(TestExpressionParser, TestFunctionCall1) {
    auto expression_tkns = getExpressionTokensFromString("T(x, y)");
    auto expr = parseExpression(expression_tkns);
    auto stringExpr = expr->toString();
    ASSERT_EQ(stringExpr, "T(x, y)");
}


TEST(TestExpressionParser, TestFunctionCall2) {
    auto expression_tkns = getExpressionTokensFromString("T(x + y, y)");
    auto expr = parseExpression(expression_tkns);
    auto stringExpr = expr->toString();
    ASSERT_EQ(stringExpr, "T((x + y), y)");
}

TEST(TestExpressionParser, TestBinaryRelation) {
    auto expression_tkns = getExpressionTokensFromString("x + y > z");
    auto expr = parseExpression(expression_tkns);
    auto stringExpr = expr->toString();
    ASSERT_EQ(stringExpr, "((x + y) > z)");
}

TEST(TestExpressionParser, ComplexExpression) {
    auto expression_tkns = getExpressionTokensFromString("T(-x, y - x) * 1 - 0 > + 1-z");
    auto expr = parseExpression(expression_tkns);
    auto stringExpr = expr->toString();
    ASSERT_EQ(std::string("(((T((-x), (y - x)) * 1) - 0) > ((+1) - z))"), stringExpr);

}

TEST(TestExpressionParser, UnariyMinus2) {
    auto expression_tkns = getExpressionTokensFromString("((-(-x)))");
    auto expr = parseExpression(expression_tkns);
    auto stringExpr = expr->toString();
    std::cout << stringExpr << std::endl;
}


TEST(ProgramWithExpression, Test1) {
    const char * program = "if {a >= 0} => {b > -1}";
    std::istringstream iss(program);
    auto tokens = tokenize(iss);
    auto p = Program();

    parse(tokens, p);

    int cnt = 0;
    for (auto& s : p.getConditions()) {
        cnt++;
        std::cout << "If Then statement " << cnt << ":" << std::endl;
        for (auto& pre : s.getConditions()) {
            std::cout << pre->toString() << std::endl;
        }
        std::cout << "Then:" << std::endl;
        for (auto& post : s.getConclusions()) {
            std::cout << post->toString() << std::endl;
        }
    }

}

TEST(ProgramWithExpression, Test2) {
    const char * program =
            "real a, b, c;\n"
            "function T[1], G[2];\n"
            "if {a >= 0} => {b > -1; c > 0}"
            "if {T(a / 2) == a * a; a <= -1} => {-1 == 0}";
    std::istringstream iss(program);
    auto tokens = tokenize(iss);
    auto p = Program();

    parse(tokens, p);

    int cnt = 0;
    for (auto& s : p.getConditions()) {
        cnt++;
        std::cout << "If Then statement " << cnt << ":" << std::endl;
        for (auto& pre : s.getConditions()) {
            std::cout << pre->toString() << std::endl;
        }
        std::cout << "Then:" << std::endl;
        for (auto& post : s.getConclusions()) {
            std::cout << post->toString() << std::endl;
        }
    }

    std::cout << "Functions:" << std::endl;
    for (auto& f : p.getTable().getDeclaredFunctions()) {
        std::cout << f << "[" << p.getFunctionSignature(f)[0] << "]" << std::endl;
    }

    std::cout << "Variables:" << std::endl;
    for (auto& v : p.getTable().getDeclaredVariables()) {
        std::cout << v << std::endl;
    }
}



TEST(FullPass, QuickSort) {

    // ---------------- setting up the program ------------------------
    const char *program =
            "real n;\n"
            "function T[1, 1];\n"
            "if {n >= 1} => {T(n) == 2 * T(n / 2) + 2}\n"
            "if {n == 0} => {T(n) == 1}";


    std::istringstream iss(program);
    auto p = Program();
    parse(iss, p);

    auto& conditions = p.getConditions();

//    ASSERT_EQ(conditions.size(), 2);


    // ------------------- setting up the environment -----------------
    auto env = SymbolicEnvironment();

    auto n = QMonomial(env.sym("n"));

    auto a0 = QMonomial(env.sym("a0"));
    auto a1 = QMonomial(env.sym("a1"));
    auto a2 = QMonomial(env.sym("a2"));

    auto sn = SymbolicPolynomial(SymbolicMonomial(n));

    auto sa0 = SymbolicPolynomial(SymbolicMonomial(QPolynomial(a0)));
    auto sa1 = SymbolicPolynomial(SymbolicMonomial(QPolynomial(a1)));
    auto sa2 = SymbolicPolynomial(SymbolicMonomial(QPolynomial(a2)));

    auto sn2 = mul(sn, sn);
    auto Tn = add(add(sa0, mul(sa1, sn), true), mul(sa2, sn2), true);

    std::cout << "T(n) = " << Tn << std::endl;

    // ------------------- setting up evaluation context ------------------

    auto ctx = EvaluationContext(&env);
    ctx.setVariableQPolynomial("n", QPolynomial(n));
    ctx.setSymbolicPolynomial("T", Tn, {"n"});



//    std::cout << p.getConditions()[0].getConditions()[0]->evaluate(ctx).getSymbolicPolynomial();
//    std::cout << p.getConditions()[0].getConclusions()[0]->evaluate(ctx).getSymbolicPolynomial();


    int cnt = 0;
    for (auto& s : p.getConditions()) {
        cnt++;
        std::cout << "If tatements " << cnt << ":" << std::endl;
        for (auto& pre : s.getConditions()) {
            std::cout << "Tree form: " << pre->toString() << std::endl;
            auto ev_result = pre->evaluate(ctx);
            std::cout << "Evaluated form: " <<
            ((ev_result.getTypeTag() == TypeTag::QPOLYNOMIAL) ?
             (toString(ev_result.getQPolynomial())) : (toString(ev_result.getSymbolicPolynomial()))) << std::endl;
        }
        std::cout << "Then:" << std::endl;
        for (auto& post : s.getConclusions()) {
            std::cout << "Tree form: " << post->toString() << std::endl;
            auto ev_result = post->evaluate(ctx);
            std::cout << "Evaluated form: " <<
                      ((ev_result.getTypeTag() == TypeTag::QPOLYNOMIAL) ?
                       (toString(ev_result.getQPolynomial())) : (toString(ev_result.getSymbolicPolynomial()))) << std::endl;

        }
    }

//    // ------------------- setting up the conditions ------------------
//
//    auto monomialVec = getMonomialVecotor(x, y, 1);
//
//    std::cout << "Monomials:" << std::endl;
//    for (auto& it: monomialVec) {
//        std::cout << it << std::endl;
//    }
//
//    auto q0 = getSos(monomialVec, 0);
//    auto q1 = getSos(monomialVec, 1);
//    auto q2 = getSos(monomialVec, 2);
//    auto q3 = getSos(monomialVec, 3);

}

TEST(Combinatorics, GenerateBoundSum) {
    std::vector<int> vec = {0, 0, 0};
    int bound = 2;

    do {
        std::cout << vec[0] << " " << vec[1] << " " << vec[2] << std::endl;
    } while (getNextVectorBoundedSum(vec, bound));
}

TEST(ComplexityEstimator, EstimateSimple) {
    const char *program =
            "real n;\n"
            "function T[1, 1];\n"
            "if {n >= 1} => {T(n) >= T(n - 1) + 1}";
            "if {n >= 1} => {T(n) >= 2 * T(n / 2) + 2}\n";
            "if {n == 0} => {T(n) >= 1}";

    std::istringstream iss(program);
    auto p = Program();
    parse(iss, p);

    auto estimator = ComplexityEstimator(p, "EstimateSimple");
    auto config = SolverConfig();

    config.setMethod(AlgorithmFamily::PUTINAR);
    config.setHighMonomialDegree(0);

    estimator.configure(config);

    estimator.solveWithPutinarMosek();
}



TEST(ComplexityEstimator, EstimateSimple2) {
    const char *program =
            "real n;\n"
            "function T[1, 2];\n"
            "if {n >= 1} => {T(n) >= 2 * T(n / 2) + 2}\n"
            "if {n == 0} => {T(n) >= 1}";

    std::istringstream iss(program);
    auto p = Program();
    parse(iss, p);

    auto estimator = ComplexityEstimator(p);
    auto config = SolverConfig();

    config.setMethod(AlgorithmFamily::PUTINAR);
    config.setHighMonomialDegree(2);

    estimator.configure(config);

    estimator.solveWithPutinarMosek();
}

TEST(ComplexityEstimator, EstimateSimple3) {
    const char *program =
            "real n, m1, m2;\n"
            "function T[1, 1];\n"
            "if {n >= 1; n * n == m1; (n / 2) * (n / 2) == m2 } => {T(m1) >= 2 * T(m2) + 2}\n"
            "if {n == 0} => {T(n) >= 1}";

    std::istringstream iss(program);
    auto p = Program();
    parse(iss, p);

    auto estimator = ComplexityEstimator(p);
    auto config = SolverConfig();

    config.setMethod(AlgorithmFamily::PUTINAR);
    config.setHighMonomialDegree(2);

    estimator.configure(config);

    estimator.solveWithPutinarMosek();
}

TEST(TokenizationTest, BehaviorWithPow) {
    const char *program =
            "a ^ 3 + ((a) + b)^1 / 5 ^ 5\n";
    std::istringstream iss(program);
    auto tokens = tokenize(iss);
    printTokens(tokens);

    auto powerGroups = getPowerGroups(tokens);
    for (auto& it: powerGroups) {
        std::cout << it.groupStartIdx << " " << it.groupFinishIdx << " " << it.powerValueIdx << std::endl;
    }

    auto powerGroupsRepeated = repeatPowerGroupsHack(tokens);
    std::string newString = "";
    for (auto& it: powerGroupsRepeated) {
        newString += it.value + " ";
    }


    std::cout << newString << std::endl;


}


TEST(ComplexityEstimator, EstimateSimple4) {
    const char *program =
            "real n;\n"
            "if {1 >= 0} => {n * n >= 0}\n";

    std::istringstream iss(program);
    auto p = Program();
    parse(iss, p);

    auto estimator = ComplexityEstimator(p);
    auto config = SolverConfig();

    config.setMethod(AlgorithmFamily::PUTINAR);
    config.setHighMonomialDegree(1);
    config.setAddAdditionalOneGeqZero(false);

    estimator.configure(config);

    estimator.solveWithPutinarMosek();
}

TEST(ComplexityEstimator, EstimateSimple5) {
    std::cout << std::to_string(1e-4l) << std::endl;


    const char *program =
            "real n, m1, m2;\n"
            "function T[1, 1];\n"
            "if {n >= 1; n ^ 2 == m1; (n / 2) ^ 2 == m2 } => {T(m1) >= 2 * T(m2) + 2}\n"
            "if {n == 0} => {T(n) >= 1}";

    std::istringstream iss(program);
    auto p = Program();
    parse(iss, p, ParseConfig(), true);

    auto estimator = ComplexityEstimator(p);
    auto config = SolverConfig();

    config.setMethod(AlgorithmFamily::PUTINAR);
    config.setHighMonomialDegree(1);

    estimator.configure(config);

    estimator.IAdmitThatThisIsUnsafeAndShouldBeUsedOnlyWithTrustedInput();

    estimator.solveWithPutinarMosek();


    // open file sol.cert.py for writing
    std::ofstream certFile;
    certFile.open("sol.cert.py");

    estimator.getCertifiedSolutionPy(certFile);

}


TEST(ComplexityEstimator, Estimate6) {

    const char *program =
            "real n, m1, m2;\n"
            "function T[1, 1];\n"
            "if {n >= 1; n ^ 5 == m1; (n / 2) ^ 5 == m2 } => {T(m1) >= 2 * T(m2) + 2}\n"
            "if {n == 0} => {T(n) >= 1}";

    std::istringstream iss(program);
    auto p = Program();
    parse(iss, p, ParseConfig(), true);

    auto estimator = ComplexityEstimator(p);
    auto config = SolverConfig();

    config.setMethod(AlgorithmFamily::PUTINAR);
    config.setHighMonomialDegree(2);

    estimator.configure(config);

    estimator.IAdmitThatThisIsUnsafeAndShouldBeUsedOnlyWithTrustedInput();

    estimator.solveWithPutinarMosek();


    // open file sol.cert.py for writing
    std::ofstream certFile;
    certFile.open("sol.cert.py");

    estimator.getCertifiedSolutionPy(certFile);

}


TEST(ComplexityEstimator, Estimate7) {

    const char *program =
            "real n, m1, m2;\n"
            "function G[1, 17];\n"
            "if { n >= 2; m1 ^ 6 == n; m2 ^ 6 == (n / 2); m1 >= 1; m2 >= 1 ; n ^ 2 + m1 ^ 2 + m2 ^ 2 < 1000000 } => { G(m1) >= 7 * G(m2) + n * n + 10 }\n"
            "if { n >= 0; n <= 1 } => {G(n) >= 1}";

    std::istringstream iss(program);
    auto p = Program();
    parse(iss, p, ParseConfig(), true);

    auto estimator = ComplexityEstimator(p);
    auto config = SolverConfig();

    config.setMethod(AlgorithmFamily::PUTINAR);
    config.setHighMonomialDegree(3);

    estimator.configure(config);

    estimator.IAdmitThatThisIsUnsafeAndShouldBeUsedOnlyWithTrustedInput();

    estimator.solveWithPutinarMosek();

    std::cout << "solved" << std::endl;


    // open file sol.cert.py for writing
    std::ofstream certFile;
    certFile.open("sol7a.cert.py");

    estimator.getCertifiedSolutionPy(certFile);

}


TEST(ComplexityEstimator, Estimate8) {

    // H(n) = n ^ 1/2
    // H(n) >= 2 * H(n/4) + 1
    // G(n) = H(n ^ 2)
    // G(n ^ 1/2) = H(n)
    // G(n ^ 1/2) >= 2 * G((n / 4) ^ 1/2) + 1
    const char *program =
            "real n, m1, m2;\n"
            "function G[1, 1];\n"
            "if { n >= 2; m1 * m1 == n; m2 * m2 == (n / 4); n ^ 2 + m1 ^ 2 + m2 ^ 2 < 10000; m1 >= 0; m2 >= 0 } => { G(m1) >= 2 * G(m2) + 1}\n"
            "if { n <= 1; n >= 0; n ^ 2 + m1 ^ 2 + m2 ^ 2 < 10000 } => { G(n) >= 0 }";

    std::istringstream iss(program);
    auto p = Program();
    parse(iss, p, ParseConfig(), true);

    auto estimator = ComplexityEstimator(p);
    auto config = SolverConfig();

    config.setMethod(AlgorithmFamily::PUTINAR);
    config.setHighMonomialDegree(3);

    estimator.configure(config);

    estimator.IAdmitThatThisIsUnsafeAndShouldBeUsedOnlyWithTrustedInput();

    estimator.solveWithPutinarMosek();

    std::cout << "solved" << std::endl;


    // open file sol.cert.py for writing
    std::ofstream certFile;
    certFile.open("sol8.cert.py");

    estimator.getCertifiedSolutionPy(certFile);

}



namespace mosectest {
    std::shared_ptr<ndarray<int, 1>> nint(const std::vector<int> &X) { return new_array_ptr<int>(X); }

    std::shared_ptr<ndarray<double, 1>> ndou(const std::vector<double> &X) { return new_array_ptr<double>(X); }

    TEST(MosecTest, Example2) {
        using namespace mosek::fusion;
        using namespace monty;


        std::vector<int> C1_k = {0, 2};
        std::vector<int> C1_l = {0, 2};
        std::vector<double> C1_v = {1, 6};
        std::vector<int> A1_k = {0, 2, 0, 2};
        std::vector<int> A1_l = {0, 0, 2, 2};
        std::vector<double> A1_v = {1, 1, 1, 2};
        std::vector<int> C2_k = {0, 1, 0, 1, 2};
        std::vector<int> C2_l = {0, 0, 1, 1, 2};
        std::vector<double> C2_v = {1, -3, -3, 2, 1};
        std::vector<int> A2_k = {1, 0, 1, 3};
        std::vector<int> A2_l = {0, 1, 1, 3};
        std::vector<double> A2_v = {1, 1, -1, -3};
        double b = 23;
        double k = -3;

        // Convert input data into Fusion sparse matrices
        auto C1 = Matrix::sparse(3, 3, nint(C1_k), nint(C1_l), ndou(C1_v));
        auto C2 = Matrix::sparse(4, 4, nint(C2_k), nint(C2_l), ndou(C2_v));
        auto A1 = Matrix::sparse(3, 3, nint(A1_k), nint(A1_l), ndou(A1_v));
        auto A2 = Matrix::sparse(4, 4, nint(A2_k), nint(A2_l), ndou(A2_v));

        // Create model
        Model::t M = new Model("sdo2");
        auto _M = finally([&]() { M->dispose(); });

        // Two semidefinite variables
        auto X1 = M->variable(Domain::inPSDCone(3));
        auto X2 = M->variable(Domain::inPSDCone(4));

        // Objective
        M->objective(ObjectiveSense::Minimize, Expr::add(Expr::dot(C1, X1), Expr::dot(C2, X2)));

        // Equality constraint
        M->constraint(Expr::add(Expr::dot(A1, X1), Expr::dot(A2, X2)), Domain::equalsTo(b));

        // Inequality constraint
        M->constraint(X2->index(nint({0, 1})), Domain::lessThan(k));

        // Solve
        M->setLogHandler([=](const std::string &msg) { std::cout << msg << std::flush; });
        M->solve();

        M->writeTask("sdo2.ptf");


        // Retrieve solution
        std::cout << "Solution (vectorized) : " << std::endl;
        std::cout << "  X1 = " << *(X1->level()) << std::endl;
        std::cout << "  X2 = " << *(X2->level()) << std::endl;
    }

}




TEST(TestsdpProblem, Test1) {

    auto problem = SdpProblem(3);

    problem.startNewCondition();

    problem.addSdpConstrainedVariable(0, 0, 1, 1.0);
    problem.addSdpConstrainedVariable(3, 1, 1, 1.0);
    problem.addSdpConstrainedVariable(0, 2, 1, 2.0);

    problem.addUnconstrainedVariable("a", 1.0);
    problem.addUnconstrainedVariable("b", 1.0);
    problem.addUnconstrainedVariable("a", 3.0);

    problem.addConstant(3.3);

    problem.endCondition(LinearMatrixExpressionType::GEQ);

    problem.startNewCondition();
    problem.addSdpConstrainedVariable(1, 1, 1, 1.2);
    problem.addSdpConstrainedVariable(3, 2, 1, 3.4);

    problem.addUnconstrainedVariable("a", 1.0);
    problem.addConstant(-12.34);
    problem.endCondition(LinearMatrixExpressionType::GEQ);

    problem.printSystem(std::cout);

    problem.solveWithMosek();
}

TEST(TestsdpProblem, Test2) {

    auto problem = SdpProblem(1);

    problem.startNewCondition();

    problem.addSdpConstrainedVariable(0, 0, 0, 2.0);

    problem.addUnconstrainedVariable("a", 1.0);
//    problem.addUnconstrainedVariable("b", 1.0);
//    problem.addUnconstrainedVariable("a", 3.0);

    problem.addConstant(-3.0);

    problem.endCondition(LinearMatrixExpressionType::EQ);

    problem.solveWithMosek();

    std::cout << "Problem formulation:" << std::endl;
    problem.printSystem(std::cout);

    std::cout << "Solution:" << std::endl;
    problem.printSolution(std::cout);
}

TEST(TemplateEngineTest, Test1) {

    std::string input = "$a + $b = 4";

    auto engine = TemplateEngine(input);

    engine.addVariableAssignment("a", "1");
    engine.addVariableAssignment("b", "3");


    ASSERT_EQ(toString(engine.evaluate()), "1 + 3 = 4");

}


TEST(Csdp, CsdpWrite1) {

    SdpProblem problem(2);

    problem.startNewCondition();
    problem.addSdpConstrainedVariable(0, 0, 0, 1.0);
    problem.addSdpConstrainedVariable(0, 0, 1, -2.0);
    problem.addSdpConstrainedVariable(1, 1, 1, 3.0);
    problem.addUnconstrainedVariable("a", 4.0);
    problem.addConstant(5.0);
    problem.endCondition(LinearMatrixExpressionType::EQ);

//    problem.startNewCondition();
//    problem.addUnconstrainedVariable("a", 1.0);
//    problem.addConstant(-2.0);
//    problem.endCondition(LinearMatrixExpressionType::EQ);

    problem.writeCsdp(std::cout);
    // soution, received from csdp
//    2 1 1 1 5.505013257045722241e+01
//    2 1 1 2 5.455463930822634921e+00
//    2 1 2 2 6.050559650127985378e+01
//    2 2 1 1 6.009489220428435630e+01
//    2 2 2 2 4.619272619378943290e+01
//    2 3 1 1 4.320148804493977224e+01
//    2 3 2 2 9.013083386748482440e+01

    double l0_00 = 5.505013257045722241e+01;
    double l0_01 = 5.455463930822634921e+00;
    double l0_11 = 6.050559650127985378e+01;

    double l1_00 = 6.009489220428435630e+01;
    double l1_11 = 4.619272619378943290e+01;
    double l1_01 = 0.0;

    double a = 4.320148804493977224e+01 - 9.013083386748482440e+01;

    problem.setSolution({{{l0_00, l0_01}, {l0_01, l0_11}}, {{l1_00, l1_01}, {l1_01, l1_11}}}, {a});
}

TEST(Csdp, CsdpWrite2Unfeasible) {

    SdpProblem problem(0);

    problem.startNewCondition();
    problem.addUnconstrainedVariable("a", 4.0);
    problem.addConstant(5.0);
    problem.endCondition(LinearMatrixExpressionType::EQ);

    problem.startNewCondition();
    problem.addUnconstrainedVariable("a", 2.0);
    problem.addConstant(15.0);
    problem.endCondition(LinearMatrixExpressionType::EQ);


    problem.writeCsdp(std::cout);

}

TEST(Csdp, CsdpRead1) {

        SdpProblem problem(2);

        problem.startNewCondition();
        problem.addSdpConstrainedVariable(0, 0, 0, 1.0);
        problem.addSdpConstrainedVariable(0, 0, 1, -2.0);
        problem.addSdpConstrainedVariable(1, 1, 1, 3.0);
        problem.addUnconstrainedVariable("a", 4.0);
        problem.addConstant(5.0);
        problem.endCondition(LinearMatrixExpressionType::EQ);


        problem.writeCsdp(std::cout);

        std::cout << "Solution:" << std::endl;

        std::string solutionString = "0.000000000000000000e+00 \n"
                                     "1 1 1 1 2.041241452319315405e-11 \n"
                                     "1 1 2 2 2.041241452319315405e-11 \n"
                                     "1 2 1 1 2.041241452319315405e-11 \n"
                                     "1 2 2 2 2.041241452319315405e-11 \n"
                                     "1 3 1 1 2.041241452319315405e-11 \n"
                                     "1 3 2 2 2.041241452319315405e-11 \n"
                                     "2 1 1 1 5.505013257045722241e+01 \n"
                                     "2 1 1 2 5.455463930822634921e+00 \n"
                                     "2 1 2 2 6.050559650127985378e+01 \n"
                                     "2 2 1 1 6.009489220428435630e+01 \n"
                                     "2 2 2 2 4.619272619378943290e+01 \n"
                                     "2 3 1 1 4.320148804493977224e+01 \n"
                                     "2 3 2 2 9.013083386748482440e+01 \n";
        std::stringstream solutionStream(solutionString);

        auto ans = problem.readCsdp(solutionStream);

        for (auto& matrix: ans.first) {
            std::cout << "Matrix:" << std::endl;
            for (auto& row: matrix) {
                for (auto& elem: row) {
                    std::cout << elem << " ";
                }
                std::cout << std::endl;
            }
        }
        std::cout << "Variables:" << std::endl;
        for (auto& elem: ans.second) {
            std::cout << elem << " ";
        }
        std::cout << std::endl;

        problem.setSolution(ans.first, ans.second);


}


TEST(ComplexityEstimatorCsdp, Estimate1) {

    const char *program =
            "real n, m1, m2;\n"
            "function T[1, 1];\n"
            "if {n >= 1; n ^ 5 == m1; (n / 2) ^ 5 == m2 } => {T(m1) >= 2 * T(m2) + 2}\n"
            "if {n == 0} => {T(n) >= 1}";

    std::istringstream iss(program);
    auto p = Program();
    parse(iss, p, ParseConfig(), true);

    auto estimator = ComplexityEstimator(p);
    auto config = SolverConfig();

    config.setMethod(AlgorithmFamily::PUTINAR);
    config.setHighMonomialDegree(2);

    estimator.configure(config);

    estimator.IAdmitThatThisIsUnsafeAndShouldBeUsedOnlyWithTrustedInput();

    estimator.solveWithPutinarCsdp();


    // open file sol.cert.py for writing
    std::ofstream certFile;
    certFile.open("sol.cert.py");

    estimator.getCertifiedSolutionPy(certFile);

}


TEST(ComplexityEstimatorHandelman, Estimate1) {

    const char *program =
            "real n, m;\n"
            "if {n >= 0; m >= 0} => {3 * n + 2 * m + n * m >= 0}";

    std::istringstream iss(program);
    auto p = Program();
    parse(iss, p, ParseConfig(), true);

    auto estimator = ComplexityEstimator(p);
    auto config = SolverConfig();

    config.setMethod(AlgorithmFamily::PUTINAR);
    config.setHighMonomialDegree(2);

    estimator.configure(config);

    estimator.IAdmitThatThisIsUnsafeAndShouldBeUsedOnlyWithTrustedInput();

    estimator.solveWithHandelmanMosek(2);
//    estimator.solveWithPutinarMosek();


    // open file sol.cert.py for writing
    std::ofstream certFile;
    certFile.open("sol.cert.py");

    estimator.getCertifiedSolutionPy(certFile);

}



TEST(ComplexityEstimatorHandelman, Estimate2) {

    const char *program =
            "real n;\n"
            "function T[1, 1];\n"
            "if {n >= 0} => { T(n + 1) >= T(n) + 1}";

    std::istringstream iss(program);
    auto p = Program();
    parse(iss, p, ParseConfig(), true);

    auto estimator = ComplexityEstimator(p);
    auto config = SolverConfig();

    config.setMethod(AlgorithmFamily::PUTINAR);
    config.setHighMonomialDegree(2);

    estimator.configure(config);

    estimator.IAdmitThatThisIsUnsafeAndShouldBeUsedOnlyWithTrustedInput();

    estimator.solveWithHandelmanCsdp(2);
//    estimator.solveWithPutinarMosek();


    // open file sol.cert.py for writing
    std::ofstream certFile;
    certFile.open("sol.cert.py");

    estimator.getCertifiedSolutionPy(certFile);

}


TEST(FractionizerTest, Test1) {
    long long a = 1;
    long long b = 2;

    Fractionizer::fractionize<double, long long>(123123.2415125, a, b);
    std::cout << a << " " << b << std::endl;
}

