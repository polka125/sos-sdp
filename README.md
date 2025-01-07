# Build

0. `git clone TODO && cd TODO`
1. Obtain a MOSEK license key from https://www.mosek.com/products/academic-licenses/
2. Replace the `REPLACE_ME_WITH_mosek.lic` file with `mosek.lic` file with a key obtained on the previous step
3. `chmod +x build.sh run.sh`
4. `./build.sh` (it will install `sympy` package on the host computer and buld a docker container named `sos-sdp`)



# Test
In order to test the installation, run:

1. `./run.sh -inp examples/test/quadratic.req -deg 1`

If everythign is normal, it should produce a file which contains a solution and a correcness certificate

```
===========================================================
The solution is found
To check the solution certificate, run python examples/test/quadratic.req.cert.py [<nothing>|fast|answer]
```

In order to get the answer: 

2. python examples/test/quadratic.req.cert.py

it will produce 

```
f = 0.499999546085865*_function_arg_0**2 + 1.50000045457509*_function_arg_0 + 1.00000002273349
The program status is UNKNOWN, to check the program, run without "answer" argument
```

Additionally, we generate a certificate that found solution is correct, to check the certificate: 

3. python examples/test/quadratic.req.cert.py

Which will output 

```
The program is correct
```

# Program Parameters 

1. `-inp` a path to the input `*.req` file containing the problem description. A collection of `*.req` files is available in the examples folder, and the file format is specified below 
2. `-deg` the maximum degree of a monomial vector, should be a positive integer number, for example `-deg 3`. The higher this parameter, the more examples the program can handle (but it increases the runtime and space complexity).

# Project Layout
The project has the following structure: 

```
examples/
sos-sdp/
mosektoolslinux64x86.tar.bz2
Dockerfile
build.sh
run.sh
```

1. `Examples` folder contains the benchmark examples and tests 
2. `sos-sdp/` folder contains a `c++` implementation of the tool, the main entrypoint is `sos-sdp/main.cpp`, the bulding system used is `cmake`, the makefile is `sos-sdp/CMakeLists.txt`
3. `mosektoolslinux64x86.tar.bz2` are MOSEK artifacts
4. `Dockerfile` used by docker to create a reproducible platform-intependent environment
5. `build.sh` and `run.sh` are used to build and run the project



# `*.req` file format

Let us take a look at the example: 

```
real x;
function f[1, 2];  // a polynomial of one variable degree 2

if { x == 0 } => { f(x) >= 1 }
if { x == 0 } => { f(x) <= 1 }

if { x >= 1 } => { f(x) >= f(x - 1) + x + 1 }
if { x >= 1 } => { f(x) <= f(x - 1) + x + 1 }
```


Each file describes an implication system. Each expression is either real variable definition, function definition or an implication definition. 
### Real variable definition 
To define a real variable, the followin syntax is used: 
```
real VARNAME1, VARNAME2, ..., VARNAMEN;
```

each variable name should be alpha-numeric expression 

### Function variable definition 
Each function is a an uninterpret polynomial of n variables of degree m, the syntax: 

```
function FUNNAME1[NUMARGS1, MAXDEG1], FUNNAME2[NUMARGS2, MAXDEG2], ..., FUNNAMEN[NUMARGSN, MAXDEGN];
```

### Implications

Each implication has syntax 

```
if {CONDITION1; CONDITION2; CONDITIONN} => {CONCLUSION}
```
Each **CONDITION** is a polynomial equality or inequality
```
POLYNOMIAL_EXPR_LHS == POLYNOMIAL_EXPR_LHS |
POLYNOMIAL_EXPR_LHS >= POLYNOMIAL_EXPR_LHS |
POLYNOMIAL_EXPR_LHS <= POLYNOMIAL_EXPR_LHS

```

Each **CONCLUSION** must be a polynomial inequality (notice, the equality is not supported but each equality `A == B` can be reqritten as two inequalities `A >= B` and `A <= B`): 

```
POLYNOMIAL_EXPR_LHS >= POLYNOMIAL_EXPR_LHS |
POLYNOMIAL_EXPR_LHS <= POLYNOMIAL_EXPR_LHS 

```

