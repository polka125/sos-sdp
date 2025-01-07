//
// Created by sergey on 30.05.23.
//

#ifndef MYPROJECT_SYMBOLICRING_H
#define MYPROJECT_SYMBOLICRING_H

#include <string>
#include <utility>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <ostream>
#include <algorithm>
#include <numeric>
#include <string>
#include <sstream>

#include "hacks.h"

inline long long gcd_pos(long long a, long long b) {
    while (b != 0) {
        long long t = b;
        b = a % b;
        a = t;
    }
    if (a < 0) {
        return -a;
    }
    return a;
}

inline long long lcm_pos(long long a, long long b) {
    long long tmp1 = a / gcd_pos(a, b);
    long long tmp2 = b;
    assertMultiplicationSafe(tmp1, tmp2);
    return tmp1 * tmp2;
}

#define throw_line(msg) \
    throw std::runtime_error(msg " " __FILE__ ":" std::to_string(__LINE__))
namespace symbolic_ring {


    class SymbolicEnvironment;

    class HasSymbolicEnvironment {
    public:
        explicit HasSymbolicEnvironment(SymbolicEnvironment *const environment) : environment(environment) {}

        SymbolicEnvironment *getEnvironment() const {
            return environment;
        }

        SymbolicEnvironment *viewEnvironment() const {
            return environment;
        }


        bool operator==(const HasSymbolicEnvironment &rhs) const {
            return environment == rhs.environment;
        }

        bool operator!=(const HasSymbolicEnvironment &rhs) const {
            return !(rhs == *this);
        }

        HasSymbolicEnvironment(const HasSymbolicEnvironment &other) : environment(other.viewEnvironment()) {}

        // copy assignment operator

    private:
        SymbolicEnvironment *const environment;
    };

    class HasRationalCoefficient {
    public:
        virtual long long getEnumerator() const = 0;

        virtual long long getDenominator() const = 0;
    };

    class Symbol : public HasSymbolicEnvironment {
        friend class SymbolicEnvironment;

    public:
        std::string getName() const {
            return name;
        }

//    const SymbolicEnvironment* getEnvironment() const {
//        return environment;
//    }

        bool operator==(const Symbol &rhs) const {
            return name == rhs.name &&
                   this->viewEnvironment() == rhs.viewEnvironment();
        }

        bool operator!=(const Symbol &rhs) const {
            return !(rhs == *this);
        }

        bool operator<(const Symbol &rhs) const {
            if (name < rhs.name)
                return true;
            if (rhs.name < name)
                return false;
            return this->viewEnvironment() < rhs.viewEnvironment();
        }

        bool operator>(const Symbol &rhs) const {
            return rhs < *this;
        }

        bool operator<=(const Symbol &rhs) const {
            return !(rhs < *this);
        }

        bool operator>=(const Symbol &rhs) const {
            return !(*this < rhs);
        }

        friend std::ostream &operator<<(std::ostream &os, const Symbol &symbol) {
            os << symbol.name;
            return os;
        }

    private:
        Symbol(std::string name, SymbolicEnvironment *environment) : name(std::move(name)),
                                                                     HasSymbolicEnvironment(environment) {}

        const std::string name;
//    const SymbolicEnvironment* environment;
    };


    class QMonomial : public HasSymbolicEnvironment, public HasRationalCoefficient {
    public:
        friend class SymbolicEnvironment;

        friend QMonomial mul(const QMonomial &l, const QMonomial &r);

        friend QMonomial mul(const QMonomial &l, const Symbol &r);

        friend QMonomial mul(const Symbol &l, const Symbol &r);

        friend QMonomial mul(const Symbol &l, const QMonomial &r);

        friend QMonomial mul(const QMonomial &l, long long r);

        friend QMonomial mul(long long l, const QMonomial &r);

        friend QMonomial div(const QMonomial &l, long long r);

        friend QMonomial add(const QMonomial &l, const QMonomial &r);

        friend bool isSimilar(const QMonomial &l, const QMonomial &r);

        friend QMonomial rename(const QMonomial &l, const std::string &old_name, const std::string &new_name);

        QMonomial(const Symbol &symbol, unsigned int pow = 1, long long enumerator = 1, long long decominator = 1)
                : HasSymbolicEnvironment(symbol.viewEnvironment()), enumerator(enumerator), denominator(decominator) {
//        variables.push_back(symbol.getName());
//        powers.push_back(1);
            variables_and_powers.emplace_back(symbol.getName(), pow);
        }

        QMonomial(const QMonomial &other) : HasSymbolicEnvironment(other.viewEnvironment()) {
            variables_and_powers = other.variables_and_powers;
            enumerator = other.enumerator;
            denominator = other.denominator;
        }

        QMonomial &operator=(const QMonomial &other) {
            if (this != &other) {
                variables_and_powers = other.variables_and_powers;
                enumerator = other.enumerator;
                denominator = other.denominator;
            }
            return *this;
        }

//    const SymbolicEnvironment* getEnvironment() const {
//        return environment;
//    }

        bool operator==(const QMonomial &rhs) const;

        bool operator!=(const QMonomial &rhs) const;

        std::vector<std::pair<std::string, int>> getOrderedVariablesAndPowers() const {
            auto variables_and_powers_copy = std::vector<std::pair<std::string, int>>(variables_and_powers.begin(),
                                                                                      variables_and_powers.end());
            std::sort(variables_and_powers_copy.begin(), variables_and_powers_copy.end());
            return variables_and_powers_copy;
        }

        bool operator<(const QMonomial &rhs) const;

        bool operator>(const QMonomial &rhs) const;

        bool operator<=(const QMonomial &rhs) const;

        bool operator>=(const QMonomial &rhs) const;

        long long int getEnumerator() const override;

        long long int getDenominator() const override;

        bool isUnitary() const {
            return getEnumerator() == getDenominator();
        }

        friend std::ostream &operator<<(std::ostream &os, const QMonomial &monomial);

        const std::vector<std::pair<std::string, int>> &getVariablesAndPowers() const;

        bool isLinear() const {
            return variables_and_powers.size() == 1 && variables_and_powers[0].second == 1;
        }

        bool isConstant() const {
            return variables_and_powers.empty();
        }

        std::string toString() const {
            std::stringstream self;
            self << *this;
            return self.str();
        }


        std::string getNameIfLinear() const {
            if (variables_and_powers.size() == 1 && variables_and_powers[0].second == 1) {
                return variables_and_powers[0].first;
            }
            throw std::runtime_error("Not linear " + toString());
        }

    private:
        QMonomial(long long enumerator, long long denominator, SymbolicEnvironment *const environment) :
                enumerator(enumerator),
                denominator(denominator),
                HasSymbolicEnvironment(environment) {}

//    std::vector<std::string> variables;
//    std::vector<int> powers;
        std::vector<std::pair<std::string, int>> variables_and_powers;
        long long enumerator;
        long long denominator;
//    const SymbolicEnvironment* environment;
    };

    QMonomial mul(const QMonomial &l, const QMonomial &r);

    QMonomial mul(const QMonomial &l, const Symbol &r);

    QMonomial mul(const Symbol &l, const Symbol &r);

    QMonomial mul(const Symbol &l, const QMonomial &r);

    QMonomial mul(const QMonomial &l, long long r);

    QMonomial mul(long long l, const QMonomial &r);

    QMonomial div(const QMonomial &l, long long r);

    QMonomial add(const QMonomial &l, const QMonomial &r);

    bool isSimilar(const QMonomial &l, const QMonomial &r);

    QMonomial rename(const QMonomial &l, const std::string &old_name, const std::string &new_name);

//    std::ostream &operator<<(std::ostream &os, const QMonomial &monomial);



    class QPolynomial : public HasSymbolicEnvironment {
    public:


        friend class SymbolicEnvironment;

        friend QPolynomial mul(const QPolynomial &l, const QPolynomial &r);

        friend QPolynomial mul(const QPolynomial &l, long long r);

        friend QPolynomial mul(long long l, const QPolynomial &r);

        friend QPolynomial div(const QPolynomial &l, long long r);

        friend QPolynomial add(const QPolynomial &l, const QPolynomial &r);


//    const SymbolicEnvironment* getEnvironment() const {
//        return environment;
//    }

//    QPolynomial(const std::vector<QMonomial>& monomials){
//        if (monomials.empty()) {
//            throw std::runtime_error("Empty monomials");
//        }
//        this->environment = monomials[0].getEnvironment();
//        for (const auto& monomial: monomials) {
//            if (monomial.getEnvironment() != monomials[0].getEnvironment()) {
//                throw std::runtime_error("Different environments");
//            }
//        }
//        this->monomials = monomials;
//    }
        QPolynomial(const QMonomial &monomial) : HasSymbolicEnvironment(monomial.viewEnvironment()) {
            monomials.push_back(monomial);

        }

        QPolynomial(const QPolynomial &other) : HasSymbolicEnvironment(other.viewEnvironment()) {
            monomials = other.monomials;
        }

        QPolynomial &operator=(const QPolynomial &other) {
            if (this != &other) {
                monomials = other.monomials;
            }
            return *this;
        }

        void reduce();

        friend std::ostream &operator<<(std::ostream &os, const QPolynomial &polynomial);

        bool operator<(const QPolynomial &rhs) const;

        bool operator>(const QPolynomial &rhs) const;

        bool operator<=(const QPolynomial &rhs) const;

        bool operator>=(const QPolynomial &rhs) const;

        const std::vector<QMonomial> &getMonomials() const;

    private:
        std::vector<QMonomial> monomials;
    };

    QPolynomial substitute(const QMonomial &monomial, const Symbol &to_substitute, const QPolynomial &polynomial);


    class SymbolicMonomial : public HasSymbolicEnvironment {
    public:
        friend class SymbolicEnvironment;

        friend SymbolicMonomial mul(const SymbolicMonomial &l, const SymbolicMonomial &r);

        friend SymbolicMonomial mul(const SymbolicMonomial &l, long long r);

        friend SymbolicMonomial mul(long long l, const SymbolicMonomial &r);

        friend SymbolicMonomial div(const SymbolicMonomial &l, long long r);

        friend SymbolicMonomial add(const SymbolicMonomial &l, const SymbolicMonomial &r);

        SymbolicMonomial(const QMonomial &qmonomial, const QPolynomial &qpolynomial) :
                qmonomial(qmonomial), qcoefficient(qpolynomial), HasSymbolicEnvironment(qmonomial.viewEnvironment()) {}

        explicit SymbolicMonomial(const QPolynomial &qpolynomial);

        explicit SymbolicMonomial(const QMonomial &qmonomial);

        bool isUnitary() const {
            return qmonomial.isUnitary();
        }

        void unitize() {
            auto enumer = qmonomial.getEnumerator();
            auto denom = qmonomial.getDenominator();

            qmonomial = QMonomial(div(mul(qmonomial, denom), enumer));
            qcoefficient = QPolynomial(div(mul(qcoefficient, enumer), denom));
        }

        SymbolicMonomial &operator=(const SymbolicMonomial &other) {
            if (this != &other) {
                qmonomial = other.qmonomial;
                qcoefficient = other.qcoefficient;

            }
            return *this;
        }

        friend std::ostream &operator<<(std::ostream &os, const SymbolicMonomial &monomial);

        bool operator==(const SymbolicMonomial &rhs) const;

        bool operator!=(const SymbolicMonomial &rhs) const;

        bool operator<(const SymbolicMonomial &rhs) const;

        bool operator>(const SymbolicMonomial &rhs) const;

        bool operator<=(const SymbolicMonomial &rhs) const;

        bool operator>=(const SymbolicMonomial &rhs) const;

        const QMonomial &getQmonomial() const;

        const QPolynomial &getQcoefficient() const;


    private:
        QMonomial qmonomial;
        QPolynomial qcoefficient;
    };

    class SymbolicPolynomial : HasSymbolicEnvironment {
    public:
        explicit SymbolicPolynomial(const SymbolicMonomial &monomial) : monomials({monomial}), HasSymbolicEnvironment(
                monomial.viewEnvironment()) {}


        void reduce();

        friend std::ostream &operator<<(std::ostream &os, const SymbolicPolynomial &polynomial);

        friend SymbolicPolynomial mul(const SymbolicPolynomial &l, const SymbolicPolynomial &r);

        friend SymbolicPolynomial mul(const SymbolicPolynomial &l, long long r);

        friend SymbolicPolynomial mul(const long long l, const SymbolicPolynomial &r);

        friend SymbolicPolynomial div(const SymbolicPolynomial &l, const long long r);


        friend SymbolicPolynomial add(const SymbolicPolynomial &l, const SymbolicPolynomial &r, bool needReduce);

        SymbolicPolynomial &operator=(const SymbolicPolynomial &other) {
            if (this != &other) {
                monomials = other.monomials;
            }
            return *this;
        }

        std::vector<SymbolicMonomial> getReducedMonomials() const;

    private:
        std::vector<SymbolicMonomial> monomials;
    };

    SymbolicPolynomial mul(const SymbolicPolynomial &l, const SymbolicPolynomial &r);

    SymbolicPolynomial mul(const SymbolicPolynomial &l, long long r);

    SymbolicPolynomial mul(const long long l, const SymbolicPolynomial &r);

    SymbolicPolynomial div(const SymbolicPolynomial &l, const long long r);

    SymbolicPolynomial add(const SymbolicPolynomial &l, const SymbolicPolynomial &r, bool needReduce);



    SymbolicPolynomial
    substituteInBase(SymbolicPolynomial polynomial, const Symbol &to_substitute, const QPolynomial &substitution);

    SymbolicPolynomial
    substituteInCoefficients(SymbolicPolynomial polynomial, const Symbol &to_substitute, const QPolynomial &substitution);

    SymbolicPolynomial symbolicPolynomialfromQPolynomialAsBase(const QPolynomial &qpolynomial);

    class SymbolicEnvironment {
    public:
        SymbolicEnvironment() = default;

        Symbol sym(std::string name) {
            add(name);
            return {std::move(name), this};
        }

        Symbol getFreeSymbol(std::string prefix) {
            int freeSymbolCounter = 0;
            std::string candidate = prefix + std::to_string(freeSymbolCounter);
            while (variables.find(candidate) != variables.end()) {
                freeSymbolCounter++;
                candidate = prefix + std::to_string(freeSymbolCounter);
            }
            return sym(candidate);
        }

        Symbol getOrCreate(std::string name) {
            forceAdd(name);
            return {name, this};
        }

        QMonomial qmonomialOne() {
            return QMonomial(1, 1, this); //{1, 1, this};
        }

        QMonomial qmonomialZero() {
            return {0, 1, this};
        }

        QPolynomial qPolynomialZero() {
            return QPolynomial(qmonomialZero());
        }

        QPolynomial qPolynomialOne() {
            return QPolynomial(qmonomialOne());
        }

        SymbolicMonomial symbolicMonomialOne() {
            return SymbolicMonomial(qmonomialOne());
        }

        SymbolicMonomial symbolicMonomialZero() {
            return SymbolicMonomial(qmonomialZero());
        }

        SymbolicPolynomial symbolicPolynomialZero() {
            return SymbolicPolynomial(symbolicMonomialZero());
        }

        SymbolicPolynomial symbolicPolynomialOne() {
            return SymbolicPolynomial(symbolicMonomialOne());
        }


    private:
        std::set<std::string> variables;

        bool isExist(const std::string &name) const {
            return variables.find(name) != variables.end();
        }

        void forceAdd(const std::string &name) {
            variables.insert(name);
        }

        void forceAdd(const std::vector<std::string> &names) {
            for (const auto &name: names) {
                forceAdd(name);
            }
        }

        bool add(const std::string &name) {
            if (isExist(name)) {
                throw std::runtime_error("Variable " + name + " already exists");
            }
            forceAdd(name);
            return true;
        }

        bool add(const std::vector<std::string> &names) {
            for (const auto &name: names) {
                add(name);
            }
            return true;
        }
    };

//SymbolicPolynomial substituteVar(const SymbolicMonomial& polynomial, const Symbol& var, const SymbolicPolynomial& value);
//SymbolicPolynomial substituteVar(const SymbolicPolynomial& polynomial, const Symbol& var, const SymbolicPolynomial& value);

} // end of symbolic_ring namespace

#endif //MYPROJECT_SYMBOLICRING_H
