#/home/sos-sdp/sos-sdp/build/sos-sdp -inp examples/test/quadratic.req -deg 1 
import sys
import sympy as sp


EPS_MATRIX_NORM = 1e-6
EPS_EIG = 1e-4
EPS_POLY_COEFF = 1e-3

# check matrix norm
def check_matrix_symmetric(spmatrix):
    return (spmatrix - spmatrix.T).norm() < EPS_MATRIX_NORM


def check_matrix_psd(spmatrix):
    return check_matrix_symmetric(spmatrix) and min(spmatrix.eigenvals(multiple=True)) + EPS_EIG > 0


def get_poly_max_coeff(poly):
    return max(map(abs, sp.poly(sp.expand(poly)).coeffs()))


def check_polynomial_almost_zero(poly):
    max_abs_coeff = max(map(abs, sp.poly(sp.expand(poly)).coeffs()))
    return max_abs_coeff < EPS_POLY_COEFF



def check(need_check_matrix_psd, is_only_answer):
    check._coeff_0_f = sp.symbols('_coeff_0_f')
    check._coeff_0_f = 1.000000022733492421878054
    _coeff_0_f = check._coeff_0_f
    check._coeff_1_f = sp.symbols('_coeff_1_f')
    check._coeff_1_f = 1.500000454575086239472625
    _coeff_1_f = check._coeff_1_f
    check._coeff_2_f = sp.symbols('_coeff_2_f')
    check._coeff_2_f = 0.4999995460858651097346694
    _coeff_2_f = check._coeff_2_f
    
    check._function_arg_0 = sp.symbols('_function_arg_0')
    _function_arg_0 = check._function_arg_0
    check._function_arg_1 = sp.symbols('_function_arg_1')
    _function_arg_1 = check._function_arg_1
    check._function_arg_2 = sp.symbols('_function_arg_2')
    _function_arg_2 = check._function_arg_2
    check._function_arg_3 = sp.symbols('_function_arg_3')
    _function_arg_3 = check._function_arg_3
    check._function_arg_4 = sp.symbols('_function_arg_4')
    _function_arg_4 = check._function_arg_4
    check._function_arg_5 = sp.symbols('_function_arg_5')
    _function_arg_5 = check._function_arg_5
    check._function_arg_6 = sp.symbols('_function_arg_6')
    _function_arg_6 = check._function_arg_6
    check._function_arg_7 = sp.symbols('_function_arg_7')
    _function_arg_7 = check._function_arg_7
    check._function_arg_8 = sp.symbols('_function_arg_8')
    _function_arg_8 = check._function_arg_8
    check._function_arg_9 = sp.symbols('_function_arg_9')
    _function_arg_9 = check._function_arg_9
    check.f = (1/1)*((1/1)*_coeff_0_f**(1)) + (1/1)*_function_arg_0**(1)*((1/1)*_coeff_1_f**(1)) + (1/1)*_function_arg_0**(2)*((1/1)*_coeff_2_f**(1))
    if is_only_answer:
        print(f"f = {sp.simplify(check.f)}")

    if is_only_answer:
        return

    # all psd matrices
    check.l_0 = sp.matrices.Matrix([[0.05114125399396732274404087, 0.0006447720145182109056924968], [0.0006447720145182109056924968, 0.1096833947481219290187582]])
    check.l_1 = sp.matrices.Matrix([[1.551113674562379252108713, 0.2040857633656734393401422], [0.2040857633656734393401422, 0.1096833943968300861815379]])
    check.l_2 = sp.matrices.Matrix([[4.271072190637444053428731e-07, 1.402429868640951695030281e-05], [1.402429868640951695030281e-05, 0.09311745550149112815585539]])
    check.l_3 = sp.matrices.Matrix([[1.545823130707110193426956, 0.2853308464439567870840619], [0.2853308464439567870840619, 0.119495145159324661632283]])
    check.l_4 = sp.matrices.Matrix([[0.04585539870566449072164517, -0.004724543819291201172949624], [-0.004724543819291201172949624, 0.1194951464807712526727812]])
    check.l_5 = sp.matrices.Matrix([[4.564236965526853125067394e-07, -1.637126093473302169815399e-05], [-1.637126093473302169815399e-05, 0.08011116247030487802494036]])
    check.l_6 = sp.matrices.Matrix([[6.450271185518623739984546e-06, -1.156183977920335649512934e-06], [-1.156183977920335649512934e-06, 7.104582009466458274797823e-07]])
    check.l_7 = sp.matrices.Matrix([[7.653033231091819417230178e-06, -4.595090797255784283636623e-06], [-4.595090797255784283636623e-06, 3.571653653056579206772269e-06]])
    check.l_8 = sp.matrices.Matrix([[1.123993487747742582583133e-05, -2.0381041454631357187589e-06], [-2.0381041454631357187589e-06, 7.486601814205605435564818e-07]])
    check.l_9 = sp.matrices.Matrix([[1.073970258391793766153052e-05, -6.991992924335090776454416e-06], [-6.991992924335090776454416e-06, 5.38198019567423879632416e-06]])
    
    check.l_all = [check.l_0, check.l_1, check.l_2, check.l_3, check.l_4, check.l_5, check.l_6, check.l_7, check.l_8, check.l_9]
    
    # check matrices are positive semidefinite
    if need_check_matrix_psd:
        for l in check.l_all:
            assert(check_matrix_psd(l))

    check.x = sp.symbols('x')
    x = check.x
    
    check.monomial_vector = sp.matrices.Matrix([(1/1), (1/1)*x**(1)])
    
    check.sos_all = []
    for l in check.l_all:
        check.sos_all.append((check.monomial_vector.transpose() * l * check.monomial_vector)[0, 0])
    
    
    # Verifying condition: 1
    
    check.cond_1 = [
    ((0) - (check.x)), # >= 0
    ((check.x) - (0)), # >= 0
    1 # >= 0
    ]
    # Implies
    check.conc_1 = ((check.f.subs({check._function_arg_0: check.x})) - (1)) # >= 0
    
    check.sos_1 = check.sos_all[0:3]
    check.res_1 = sum(x * y for x, y in zip(check.cond_1, check.sos_1)) - check.conc_1
    # ############# PROOF OF CONDITION 1 #####################
    assert(check_polynomial_almost_zero(check.res_1))
    # Verifying condition: 2
    
    check.cond_2 = [
    ((0) - (check.x)), # >= 0
    ((check.x) - (0)), # >= 0
    1 # >= 0
    ]
    # Implies
    check.conc_2 = ((1) - (check.f.subs({check._function_arg_0: check.x}))) # >= 0
    
    check.sos_2 = check.sos_all[3:6]
    check.res_2 = sum(x * y for x, y in zip(check.cond_2, check.sos_2)) - check.conc_2
    # ############# PROOF OF CONDITION 2 #####################
    assert(check_polynomial_almost_zero(check.res_2))
    # Verifying condition: 3
    
    check.cond_3 = [
    ((check.x) - (1)), # >= 0
    1 # >= 0
    ]
    # Implies
    check.conc_3 = ((check.f.subs({check._function_arg_0: check.x})) - (((((check.f.subs({check._function_arg_0: ((check.x) - (1))})) + (check.x))) + (1)))) # >= 0
    
    check.sos_3 = check.sos_all[6:8]
    check.res_3 = sum(x * y for x, y in zip(check.cond_3, check.sos_3)) - check.conc_3
    # ############# PROOF OF CONDITION 3 #####################
    assert(check_polynomial_almost_zero(check.res_3))
    # Verifying condition: 4
    
    check.cond_4 = [
    ((check.x) - (1)), # >= 0
    1 # >= 0
    ]
    # Implies
    check.conc_4 = ((((((check.f.subs({check._function_arg_0: ((check.x) - (1))})) + (check.x))) + (1))) - (check.f.subs({check._function_arg_0: check.x}))) # >= 0
    
    check.sos_4 = check.sos_all[8:10]
    check.res_4 = sum(x * y for x, y in zip(check.cond_4, check.sos_4)) - check.conc_4
    # ############# PROOF OF CONDITION 4 #####################
    assert(check_polynomial_almost_zero(check.res_4))

if __name__ == '__main__':
    is_fast_check = len(sys.argv) >= 2 and sys.argv[1] == "fast"
    is_only_answer = len(sys.argv) >= 2 and sys.argv[1] == "answer"
    
    try:
        check(not is_fast_check, is_only_answer)
        if not is_only_answer:
            print("The program is \033[92mcorrect\033[0m")
        else:
            print("The program status is \033[92mUNKNOWN\033[0m, to check the program, run without \"answer\" argument")
    except AssertionError:
        print("The program is \033[91mINCORRECT\033[0m")

