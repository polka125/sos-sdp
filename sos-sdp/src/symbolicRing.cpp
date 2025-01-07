//
// Created by sergey on 30.05.23.
//

#include <sstream>
#include <iostream>

#include "symbolicRing.h"

//#define SYM_RING_CPP_DEBUG 1

namespace symbolic_ring {

    class my_exception : public std::runtime_error {
        std::string msg;
    public:
        my_exception(const std::string &arg, const char *file, int line) :
                std::runtime_error(arg) {
            std::ostringstream o;
            o << file << ":" << line << ": " << arg;
            msg = o.str();
        }

        ~my_exception() throw() {}

        const char *what() const throw() {
            return msg.c_str();
        }
    };

#define throw_line(arg) throw my_exception(arg, __FILE__, __LINE__);

    QMonomial mul(const QMonomial &l, const QMonomial &r) {
        if (l.viewEnvironment() != r.viewEnvironment()) {
            throw std::runtime_error("Different environments");
        }
        QMonomial result = l;
//    result.variables.clear();
//    result.powers.clear();
        result.variables_and_powers.clear();
        if (isMultiplicationSafe(result.enumerator, r.enumerator) && isMultiplicationSafe(result.denominator, r.denominator)) {
            result.enumerator *= r.enumerator;
            result.denominator *= r.denominator;
            auto gcd_result = gcd_pos(result.enumerator, result.denominator);
            if (gcd_result == 0) {
                throw_line("GCD is zero");
            }
            result.enumerator /= gcd_result;
            result.denominator /= gcd_result;
        } else {
            double resultCoeff = (double) result.enumerator / (double) result.denominator;
            double rCoeff = (double) r.enumerator / (double) r.denominator;
            double newCoeff = resultCoeff * rCoeff;
            Fractionizer::fractionize<double, long long>(newCoeff, result.enumerator, result.denominator);
        }
        // result.enumerator *= r.enumerator;
        // result.denominator *= r.denominator;

        // auto gcd_result = gcd_pos(result.enumerator, result.denominator);
        // if (gcd_result == 0) {
        //    throw_line("GCD is zero");
        //}
        // result.enumerator /= gcd_result;
        // result.denominator /= gcd_result;


        // early return if zero
        if (result.enumerator == 0) {
            return result;
        }

        std::set<std::string> all_variables;
        for (auto &var: l.variables_and_powers) {
            all_variables.insert(var.first);
        }
        //    all_variables.insert(l.variables.begin(), l.variables.end());
        for (auto &var: r.variables_and_powers) {
            all_variables.insert(var.first);
        }
        //all_variables.insert(r.variables.begin(), r.variables.end());

        result.variables_and_powers = std::vector<std::pair<std::string, int>>();

        std::map<std::string, int> index_of_variable;
        for (auto &var: all_variables) {
            result.variables_and_powers.emplace_back(var, 0);
            index_of_variable[var] = result.variables_and_powers.size() - 1;
        }

        for (auto &var: l.variables_and_powers) {
            result.variables_and_powers[index_of_variable[var.first]].second += var.second;
        }

        for (auto &var: r.variables_and_powers) {
            result.variables_and_powers[index_of_variable[var.first]].second += var.second;
        }

        return result;
    }


    QMonomial mul(const QMonomial &l, const Symbol &r) {
        return mul(l, QMonomial(r));
    }

    QMonomial mul(const Symbol &l, const QMonomial &r) {
        return mul(QMonomial(l), r);
    }

    QMonomial mul(const Symbol &l, const Symbol &r) {
        return mul(QMonomial(l), QMonomial(r));
    }

    QMonomial mul(const QMonomial &l, long long r) {
        auto result = l;
        if (isMultiplicationSafe(result.enumerator, r)) {
            result.enumerator *= r;
            auto gcd_result = gcd_pos(result.enumerator, result.denominator);
            if (gcd_result == 0) {
                throw_line("GCD is zero");
            }
            result.enumerator /= gcd_result;
            result.denominator /= gcd_result;
            return result;
        }

        double resultCoeff = (double) result.enumerator / (double) result.denominator;
        double rCoeff = (double) r;
        double newCoeff = resultCoeff * rCoeff;

        Fractionizer::fractionize<double, long long>(newCoeff, result.enumerator, result.denominator);

        return result;
    }

    QMonomial mul(long long l, const QMonomial &r) {
        return mul(r, l);
    }

    QMonomial div(const QMonomial &l, long long r) {
        if (isMultiplicationSafe(l.denominator, r)) {
            auto result = l;
            result.denominator *= r;
            auto gcd_result = gcd_pos(result.enumerator, result.denominator);
            if (gcd_result == 0) {
                throw_line("GCD is zero");
            }
            result.enumerator /= gcd_result;
            result.denominator /= gcd_result;
            return result;
        }
        auto result = l;
        double resultCoeff = (double) l.enumerator / (double) l.denominator;
        double rCoeff = 1.0 / (double) r;
        double newCoeff = resultCoeff * rCoeff;
        Fractionizer::fractionize<double, long long>(newCoeff, result.enumerator, result.denominator);
        return result;
    }

    bool isSimilar(const QMonomial &l, const QMonomial &r) {
        if (l.viewEnvironment() != r.viewEnvironment()) {
            throw std::runtime_error("Different environments");
        }
        if (l.getEnumerator() == 0 || r.getEnumerator() == 0) {
            return true;
        }
        auto l_vars = l.variables_and_powers;
        auto r_vars = r.variables_and_powers;
        std::sort(l_vars.begin(), l_vars.end());
        std::sort(r_vars.begin(), r_vars.end());
        return l_vars == r_vars;
    }

    QMonomial add(const QMonomial &l, const QMonomial &r) {
#ifdef SYM_RING_CPP_DEBUG
        std::cout << "add: " << l << " + " << r << std::endl;
#endif

        auto result = l;
        try {
            if (l.viewEnvironment() != r.viewEnvironment()) {
                throw std::runtime_error("Different environments");
            }

            if (!isSimilar(l, r)) {
                throw std::runtime_error("Different variables and powers");
            }

            if (l.getEnumerator() == 0) {
                return r;
            }

            if (r.getEnumerator() == 0) {
                return l;
            }

            // this part causing overflows
//
//        result.enumerator = l.enumerator * r.denominator + r.enumerator * l.denominator;
//        result.denominator = l.denominator * r.denominator;
//        auto gcd_result = gcd_pos(result.enumerator, result.denominator);
//        if (gcd_result == 0) {
//            throw_line("GCD is zero");
//        }
//
//        result.enumerator /= gcd_result;
//        result.denominator /= gcd_result;

// so instead we do this
            auto gcd_result = gcd_pos(l.denominator, r.denominator);
            if (gcd_result == 0) {
                throw_line("GCD is zero");
            }
            auto lcm_result = lcm_pos(l.denominator, r.denominator);
            if (lcm_result == 0) {
                throw_line("LCM is zero");
            }

            assertMultiplicationSafe(l.enumerator, lcm_result / l.denominator);
            assertMultiplicationSafe(r.enumerator, lcm_result / r.denominator);

            result.enumerator =
                    l.enumerator * (lcm_result / l.denominator) + r.enumerator * (lcm_result / r.denominator);
            result.denominator = lcm_result;
            gcd_result = gcd_pos(result.enumerator, result.denominator);
            if (gcd_result == 0) {
                throw_line("GCD is zero");
            }

            result.enumerator /= gcd_result;
            result.denominator /= gcd_result;

            return result;
        } catch (std::exception &e) {
            std::cerr << "precision of long long is not enough" << std::endl;
            if (l.viewEnvironment() != r.viewEnvironment()) {
                throw std::runtime_error("Different environments");
            }

            if (!isSimilar(l, r)) {
                throw std::runtime_error("Different variables and powers");
            }

            if (l.getEnumerator() == 0) {
                return r;
            }

            if (r.getEnumerator() == 0) {
                return l;
            }

            double lCoeff = (double) l.getEnumerator() / (double) l.getDenominator();
            double rCoeff = (double) r.getEnumerator() / (double) r.getDenominator();

            double resultCoeff = lCoeff + rCoeff;
            Fractionizer::fractionize<double, long long>(resultCoeff, result.enumerator, result.denominator);

            return result;
        }
    }


    bool QMonomial::operator==(const QMonomial &rhs) const {

        std::vector<std::pair<std::string, int>> variables_and_powers_lhs = this->variables_and_powers;
        std::vector<std::pair<std::string, int>> variables_and_powers_rhs = rhs.variables_and_powers;

        std::sort(variables_and_powers_lhs.begin(), variables_and_powers_lhs.end());
        std::sort(variables_and_powers_rhs.begin(), variables_and_powers_rhs.end());

        return variables_and_powers_lhs == variables_and_powers_rhs &&
               enumerator * rhs.denominator == denominator * rhs.enumerator &&
               this->viewEnvironment() == rhs.viewEnvironment();
    }

    bool QMonomial::operator!=(const QMonomial &rhs) const {
        std::vector<std::pair<std::string, int>> variables_and_powers_lhs = this->variables_and_powers;
        std::vector<std::pair<std::string, int>> variables_and_powers_rhs = rhs.variables_and_powers;

        std::sort(variables_and_powers_lhs.begin(), variables_and_powers_lhs.end());
        std::sort(variables_and_powers_rhs.begin(), variables_and_powers_rhs.end());

        return !(variables_and_powers_lhs == variables_and_powers_rhs);
    }

    bool QMonomial::operator<(const QMonomial &rhs) const {
        std::vector<std::pair<std::string, int>> variables_and_powers_lhs = this->variables_and_powers;
        std::vector<std::pair<std::string, int>> variables_and_powers_rhs = rhs.variables_and_powers;

        std::sort(variables_and_powers_lhs.begin(), variables_and_powers_lhs.end());
        std::sort(variables_and_powers_rhs.begin(), variables_and_powers_rhs.end());

        return variables_and_powers_lhs < variables_and_powers_rhs;
    }


    bool QMonomial::operator>(const QMonomial &rhs) const {
        return rhs < *this;
    }

    bool QMonomial::operator<=(const QMonomial &rhs) const {
        return !(rhs < *this);
    }

    bool QMonomial::operator>=(const QMonomial &rhs) const {
        return !(*this < rhs);
    }

    long long int QMonomial::getEnumerator() const {
        return enumerator;
    }

    long long int QMonomial::getDenominator() const {
        return denominator;
    }


    std::ostream &operator<<(std::ostream &os, const QMonomial &monomial) {
        os << "(" << monomial.enumerator << "/" << monomial.denominator << ")";
        for (auto &var: monomial.variables_and_powers) {
            os << "*" << var.first << "**(" << var.second << ")";
        }

        return os;
    }

    const std::vector<std::pair<std::string, int>> &QMonomial::getVariablesAndPowers() const {
        return variables_and_powers;
    }

    QMonomial rename(const QMonomial &l, const std::string &old_name, const std::string &new_name) {
        auto result = l;
        auto new_var = l.getEnvironment()->getOrCreate(new_name); // creating new variable

        for (auto &var: result.variables_and_powers) {
            if (var.first == new_name) {
                throw std::runtime_error("Renaming clash");
            }

            if (var.first == old_name) {
                var.first = new_name;
            }
        }
        return result;
    }


    std::ostream &operator<<(std::ostream &os, const QPolynomial &polynomial) {

        for (int i = 0; i < polynomial.monomials.size(); i++) {
            auto monomial = polynomial.monomials[i];
            os << monomial;
            if (i != polynomial.monomials.size() - 1) {
                os << " + ";
            }
        }
        return os;
    }

    void QPolynomial::reduce() {
        auto monomials_copy = monomials;
        std::sort(monomials_copy.begin(), monomials_copy.end());
        monomials.clear();

        if (monomials_copy.empty()) {
            return;
        }


        if (monomials_copy[0].getEnumerator() == 0) {
            monomials.push_back(this->getEnvironment()->qmonomialZero());
        } else {
            monomials.push_back(monomials_copy[0]);
        }

        for (int i = 1; i < monomials_copy.size(); ++i) {
            if (monomials_copy[i].getOrderedVariablesAndPowers() ==
                monomials_copy[i - 1].getOrderedVariablesAndPowers()) {
                monomials.back() = add(monomials.back(), monomials_copy[i]);
            } else {
                monomials.push_back(monomials_copy[i]);
            }
        }
    }


    QPolynomial mul(const QPolynomial &l, const QPolynomial &r) {
        if (l.viewEnvironment() != r.viewEnvironment()) {
            throw std::runtime_error("Different environments");
        }
        QPolynomial result = l;
        result.monomials.clear();

        for (auto &monomial_l: l.monomials) {
            for (auto &monomial_r: r.monomials) {
                result.monomials.push_back(mul(monomial_l, monomial_r));
            }
        }
        result.reduce();
        return result;
    }

    QPolynomial add(const QPolynomial &l, const QPolynomial &r) {
        if (l.viewEnvironment() != r.viewEnvironment()) {
            throw std::runtime_error("Different environments");
        }
        QPolynomial result = l;
        result.monomials.insert(result.monomials.end(), r.monomials.begin(), r.monomials.end());

        result.reduce();
        return result;
    }

    bool QPolynomial::operator<(const QPolynomial &rhs) const {
        if (static_cast<const HasSymbolicEnvironment &>(*this) != static_cast<const HasSymbolicEnvironment &>(rhs))
            throw std::runtime_error("Different environments");
        // TODO: avoid copying and sorting
        auto this_monomials = this->monomials;
        auto rhs_monomials = rhs.monomials;

        std::sort(this_monomials.begin(), this_monomials.end());
        std::sort(rhs_monomials.begin(), rhs_monomials.end());

        return this_monomials < rhs_monomials;
    }

    bool QPolynomial::operator>(const QPolynomial &rhs) const {
        return rhs < *this;
    }

    bool QPolynomial::operator<=(const QPolynomial &rhs) const {
        return !(rhs < *this);
    }

    bool QPolynomial::operator>=(const QPolynomial &rhs) const {
        return !(*this < rhs);
    }

    QPolynomial mul(const QPolynomial &l, long long int r) {
        if (r == 0) {
            return l.viewEnvironment()->qPolynomialZero();
        }
        QPolynomial result = l;
        for (auto &monomial: result.monomials) {
            monomial = mul(monomial, r);
        }
        return result;
    }

    QPolynomial mul(long long int l, const QPolynomial &r) {
        return mul(r, l);
    }

    QPolynomial div(const QPolynomial &l, long long int r) {
        if (r == 0) {
            throw std::runtime_error("Division by zero");
        }
        QPolynomial result = l;
        for (auto &monomial: result.monomials) {
            monomial = div(monomial, r);
        }
        return result;
    }

    const std::vector<QMonomial> &QPolynomial::getMonomials() const {
        return monomials;
    }



    SymbolicMonomial::SymbolicMonomial(const QPolynomial &qpolynomial) : SymbolicMonomial(
            qpolynomial.viewEnvironment()->qmonomialOne(), qpolynomial) {}

    SymbolicMonomial::SymbolicMonomial(const QMonomial &qmonomial) : SymbolicMonomial(qmonomial,
                                                                                      qmonomial.viewEnvironment()->qPolynomialOne()) {}

    std::ostream &operator<<(std::ostream &os, const SymbolicMonomial &monomial) {
        os << monomial.qmonomial << "*[" << monomial.qcoefficient << "]";
        return os;
    }

    bool SymbolicMonomial::operator==(const SymbolicMonomial &rhs) const {
        if (static_cast<const HasSymbolicEnvironment &>(*this) != static_cast<const HasSymbolicEnvironment &>(rhs))
            throw std::runtime_error("Different environments");
        return static_cast<const HasSymbolicEnvironment &>(*this) == static_cast<const HasSymbolicEnvironment &>(rhs) &&
               qmonomial == rhs.qmonomial &&
               qcoefficient == rhs.qcoefficient;
    }

    bool SymbolicMonomial::operator!=(const SymbolicMonomial &rhs) const {
        return !(rhs == *this);
    }

    bool SymbolicMonomial::operator<(const SymbolicMonomial &rhs) const {
        if (static_cast<const HasSymbolicEnvironment &>(*this) != static_cast<const HasSymbolicEnvironment &>(rhs))
            throw std::runtime_error("Different environments");

        if (qmonomial < rhs.qmonomial)
            return true;
        if (rhs.qmonomial < qmonomial)
            return false;
        return qcoefficient < rhs.qcoefficient;
    }

    bool SymbolicMonomial::operator>(const SymbolicMonomial &rhs) const {
        return rhs < *this;
    }

    bool SymbolicMonomial::operator<=(const SymbolicMonomial &rhs) const {
        return !(rhs < *this);
    }

    bool SymbolicMonomial::operator>=(const SymbolicMonomial &rhs) const {
        return !(*this < rhs);
    }

    SymbolicMonomial mul(const SymbolicMonomial &l, const SymbolicMonomial &r) {
        if (l.viewEnvironment() != r.viewEnvironment()) {
            throw std::runtime_error("Different environments");
        }
        SymbolicMonomial result = l;
        result.qmonomial = mul(l.qmonomial, r.qmonomial);
        result.qcoefficient = mul(l.qcoefficient, r.qcoefficient);
        return result;
    }

    SymbolicMonomial add(const SymbolicMonomial &l, const SymbolicMonomial &r) {
        if (l.viewEnvironment() != r.viewEnvironment()) {
            throw std::runtime_error("Different environments");
        }

        if (!isSimilar(l.qmonomial, r.qmonomial)) {
            throw std::runtime_error("Different monomials");
        }

        if (l.qmonomial.getEnumerator() == 0) {
            return r;
        }

        if (r.qmonomial.getEnumerator() == 0) {
            return l;
        }

        SymbolicMonomial lhs = l;
        SymbolicMonomial rhs = r;

        lhs.unitize();
        rhs.unitize();

        lhs.qcoefficient = add(lhs.qcoefficient, rhs.qcoefficient);
        return lhs;
    }

    const QMonomial &SymbolicMonomial::getQmonomial() const {
        return qmonomial;
    }

    const QPolynomial &SymbolicMonomial::getQcoefficient() const {
        return qcoefficient;
    }

    SymbolicMonomial mul(const SymbolicMonomial &l, long long int r) {
        SymbolicMonomial result = l;
        result.qcoefficient = mul(l.qcoefficient, r);
        return result;
    }

    SymbolicMonomial mul(long long int l, const SymbolicMonomial &r) {
        return mul(r, l);
    }

    SymbolicMonomial div(const SymbolicMonomial &l, long long int r) {
        SymbolicMonomial result = l;
        result.qcoefficient = div(l.qcoefficient, r);
        return result;
    }


    void SymbolicPolynomial::reduce() {
        auto monomials_copy = monomials;
        std::sort(monomials_copy.begin(), monomials_copy.end());
        monomials.clear();

        if (monomials_copy.empty()) {
            return;
        }


        monomials.push_back(monomials_copy[0]);
        for (int i = 1; i < monomials_copy.size(); ++i) {
            if (isSimilar(monomials.back().getQmonomial(), monomials_copy[i].getQmonomial())) {
                auto result = add(monomials.back(), monomials_copy[i]);
                monomials.back() = result;
            } else {
                monomials.push_back(monomials_copy[i]);
            }
        }
    }

    std::ostream &operator<<(std::ostream &os, const SymbolicPolynomial &polynomial) {
        for (int i = 0; i < polynomial.monomials.size(); ++i) {
            os << polynomial.monomials[i];
            if (i != polynomial.monomials.size() - 1) {
                os << " + ";
            }
        }
        return os;
    }

    SymbolicPolynomial add(const SymbolicPolynomial &l, const SymbolicPolynomial &r, bool needReduce) {
        if (l.viewEnvironment() != r.viewEnvironment()) {
            throw std::runtime_error("Different environments");
        }

        SymbolicPolynomial result = l;
        result.monomials.insert(result.monomials.end(), r.monomials.begin(), r.monomials.end());
        if (needReduce) {
            result.reduce();
        }
        return result;
    }

    SymbolicPolynomial mul(const SymbolicPolynomial &l, const SymbolicPolynomial &r) {
        if (l.viewEnvironment() != r.viewEnvironment()) {
            throw std::runtime_error("Different environments");
        }

        SymbolicPolynomial result = l.viewEnvironment()->symbolicPolynomialZero();
        for (auto &monomial: l.monomials) {
            for (auto &monomial1: r.monomials) {
                result.monomials.push_back(mul(monomial, monomial1));
            }
        }
        result.reduce();
        return result;
    }

    SymbolicPolynomial mul(const SymbolicPolynomial &l, const long long int r) {
        SymbolicPolynomial result = l;
        for (auto &monomial: result.monomials) {
            monomial = mul(monomial, r);
        }
        return result;
    }

    SymbolicPolynomial mul(const long long int l, const SymbolicPolynomial &r) {
        return mul(r, l);
    }

    SymbolicPolynomial div(const SymbolicPolynomial &l, const long long int r) {
        if (r == 0) {
            throw std::runtime_error("Division by zero");
        }

        SymbolicPolynomial result = l;
        for (auto &monomial: result.monomials) {
            monomial = div(monomial, r);
        }
        return result;
    }

    QPolynomial substitute(const QMonomial &monomial, const Symbol &to_substitute, const QPolynomial &polynomial) {
        auto env = polynomial.getEnvironment();
        auto &varsAndPowers = monomial.getVariablesAndPowers();

        auto result = env->qPolynomialOne();
        result = mul(result, monomial.getEnumerator());
        result = div(result, monomial.getDenominator());

        for (auto &varAndPower: varsAndPowers) {
            auto &var = varAndPower.first;
            auto &power = varAndPower.second;

            if (var == to_substitute.getName()) {
                for (int i = 0; i < power; ++i) {
                    result = mul(result, polynomial);
                }
            } else {
                auto varMonomial = QMonomial(env->getOrCreate(var), power);
                result = mul(result, varMonomial);
            }
        }
        result.reduce();
        return result;
    }

    SymbolicPolynomial
    substituteInBase(const SymbolicMonomial &monomial, const Symbol &to_substitute, const QPolynomial &polynomial) {
        auto env = polynomial.getEnvironment();
        auto base = monomial.getQmonomial();
        auto coeff = monomial.getQcoefficient();

        auto base_substituted = substitute(base, to_substitute, polynomial);

        std::vector<SymbolicMonomial> init_vector;

        auto result = env->symbolicPolynomialZero();

        for (const auto &monomial_it: base_substituted.getMonomials()) {
            result = add(result, SymbolicPolynomial(SymbolicMonomial(monomial_it, coeff)), true);
        }

        return result;
    }

    SymbolicMonomial
    substituteInCoefficient(const SymbolicMonomial &monomial, const Symbol &to_substitute, const QPolynomial &polynomial) {
        auto env = polynomial.getEnvironment();
        auto base = monomial.getQmonomial();
        auto coeff = monomial.getQcoefficient();

        auto coeff_monomials = coeff.getMonomials();
        auto coeff_substituted = env->qPolynomialZero();

        for (auto &monomial_it: coeff_monomials) {
            coeff_substituted = add(coeff_substituted, substitute(monomial_it, to_substitute, polynomial));
        }

        return {base, coeff_substituted};
    }


    SymbolicPolynomial
    substituteInBase(SymbolicPolynomial polynomial, const Symbol &to_substitute, const QPolynomial &substitution) {

        auto env = substitution.viewEnvironment();
        auto monomials = polynomial.getReducedMonomials();

        SymbolicPolynomial result = env->symbolicPolynomialZero();

        for (auto &monomial: monomials) {
            auto substituted = substituteInBase(monomial, to_substitute, substitution);
            result = add(result, substituted, true);
        }
        return result;
    }

    SymbolicPolynomial symbolicPolynomialfromQPolynomialAsBase(const QPolynomial &qpolynomial) {
        auto env = qpolynomial.getEnvironment();
        auto monomials = qpolynomial.getMonomials();

        SymbolicPolynomial result = env->symbolicPolynomialZero();

        for (auto &monomial: monomials) {
            result = add(result, SymbolicPolynomial(SymbolicMonomial(monomial)), true);
        }
        return result;
    }

    SymbolicPolynomial substituteInCoefficients(SymbolicPolynomial polynomial, const Symbol &to_substitute,
                                                const QPolynomial &substitution) {

        auto env = substitution.viewEnvironment();
        auto monomials = polynomial.getReducedMonomials();

        SymbolicPolynomial result = env->symbolicPolynomialZero();

        for (auto &monomial: monomials) {
            auto substituted = substituteInCoefficient(monomial, to_substitute, substitution);
            result = add(result, SymbolicPolynomial(substituted), true);
        }
        return result;
    }


    std::vector<SymbolicMonomial> SymbolicPolynomial::getReducedMonomials() const {
        auto poly = *this;
        poly.reduce();
        return poly.monomials;
    }

} // namespace symbolic_ring


