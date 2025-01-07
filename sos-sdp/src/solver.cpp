//
// Created by sergey on 06.06.23.
//

#include "solver.h"
#include "fusion.h"


std::vector<QMonomial> getMonomialVecotor(const QMonomial& x, const QMonomial& y, const int highestPower) {
    auto env = x.viewEnvironment();
    auto one = env->qmonomialOne();

    std::vector<QMonomial> result;


    for (int ydeg = 0; ydeg <= highestPower; ydeg++) {
        for (int xdeg = 0; xdeg <= highestPower; xdeg++) {
            if (xdeg + ydeg > highestPower) {
                continue;
            }

            auto monomial = one;
            for (int i = 0; i < xdeg; i++) {
                monomial = mul(monomial, x);
            }
            for (int i = 0; i < ydeg; i++) {
                monomial = mul(monomial, y);
            }
            result.push_back(monomial);
        }
    }
    return result;
}


//
//
//Variable::t slice(Variable::t X, int d, int j) {
//    return
//            X->slice(new_array_ptr<int,1>({j,0,0}), new_array_ptr<int,1>({j+1,d,d}))
//                    ->reshape(new_array_ptr<int,1>({d,d}));
//}
