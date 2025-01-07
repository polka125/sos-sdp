#include <iostream>
#include <random>
#include <fstream>
#include "program.h"
#include "programParser.h"
#include "automaitcComplexityEstimator.h"


// A helper function which returns a slice corresponding to j-th variable
//Variable::t slice(Variable::t X, int d, int j) {
//    return
//            X->slice(new_array_ptr<int,1>({j,0,0}), new_array_ptr<int,1>({j+1,d,d}))
//                    ->reshape(new_array_ptr<int,1>({d,d}));
//}

int main(int argc, char ** argv) {
    // save arguments to a file /tmp/argumentsadfjnjadflnawgnwq.txt
    std::ofstream argumentsFile("/tmp/argumentsadfjnjadflnawgnwq.txt");
    for (int i = 0; i < argc; ++i) {
        argumentsFile << argv[i] << " ";
    }
    argumentsFile << std::endl;
    argumentsFile.close();

    // Initialize the input file name from the argv

    // if -help or --help is passed, print help and exit
    if (argc == 2 && (std::string(argv[1]) == "-help" || std::string(argv[1]) == "--help")) {
        std::string help = "The usage: -inp <filename> -deg <integer> -met [mosek|csdp] -eng [mosek|csdp]"
                           "\n\t-inp <filename> - the name of the input file"
                           "\n\t-deg <integer> - the degree, in the case of putinar used for generating the "
                           "monomial vector, in the case of handelman used for generating the monoid, default = 2"
                           "\n\t-eng [mosek|csdp] - the method to use for solving the SDP, default = mosek"
                           "\n\t-met [putinar|handelman] - the method to use for solving the SDP, default = putinar";
        std::cout << help << std::endl;
        return 0;
    }

    std::set<std::string> possibleEngines = {"mosek", "csdp"};
    std::set<std::string> possibleMethods = {"putinar", "handelman"};

    bool inputFileFound = false;
    std::string inputFileName;
    const std::string inputFileNamePrefix = "-inp";

    bool highDegreeMonomialFound = false;
    int highDegreeMonomial = 2; // default value
    const std::string highDegreeMonomialPrefix = "-deg";

    bool methodFound = false;
    std::string method = "putinar";
    const std::string methodPrefix = "-met";

    bool solverEngineFound = false;
    std::string solverEngine = "mosek";
    const std::string solverEnginePrefix = "-eng";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.substr(0, inputFileNamePrefix.size()) == inputFileNamePrefix) {
            inputFileName = argv[i + 1];
            inputFileFound = true;
        }
        if (arg.substr(0, highDegreeMonomialPrefix.size()) == highDegreeMonomialPrefix) {
            highDegreeMonomial = std::stoi(argv[i + 1]);
            highDegreeMonomialFound = true;
        }
        if (arg.substr(0, methodPrefix.size()) == methodPrefix) {
            method = argv[i + 1];
            methodFound = true;
        }
        if (arg.substr(0, solverEnginePrefix.size()) == solverEnginePrefix) {
            solverEngine = argv[i + 1];
            solverEngineFound = true;
        }
    }

    if (possibleEngines.count(solverEngine) == 0) {
        std::cout << "Unknown solver engine: " << solverEngine << std::endl;
        std::cout << "Possible solver engines: ";
        for (auto & engine : possibleEngines) {
            std::cout << engine << " ";
        }
        std::cout << std::endl;
    }

    if (possibleMethods.count(method) == 0) {
        std::cout << "Unknown method: " << method << std::endl;
        std::cout << "Possible methods: ";
        for (auto & m : possibleMethods) {
            std::cout << m << " ";
        }
        std::cout << std::endl;
    }

    // Open inputFileName:
    if (!inputFileFound) {
        std::cout << "Input file name not found. Run --help to get more info" << std::endl;
        return 1;
    }

    std::ifstream inputFile(inputFileName);

    auto p = Program();
    parse(inputFile, p, ParseConfig(), true);

    auto estimator = ComplexityEstimator(p, inputFileName);
    auto config = SolverConfig();

    // redundant, does not change anything, TODO: remove
    config.setMethod(AlgorithmFamily::PUTINAR);

    if (highDegreeMonomialFound) {
        config.setHighMonomialDegree(highDegreeMonomial);
    } else {
        std::cout << "Using default value for high degree monomial: " << highDegreeMonomial << std::endl;
        std::cout << "To change it, use -deg <value>" << std::endl;
        config.setHighMonomialDegree(highDegreeMonomial);
    }


    estimator.configure(config);

    estimator.IAdmitThatThisIsUnsafeAndShouldBeUsedOnlyWithTrustedInput();

    if (solverEngine == "mosek" && method == "putinar") {
        estimator.solveWithPutinarMosek();
    } else if (solverEngine == "mosek" && method == "handelman") {
        estimator.solveWithHandelmanMosek(highDegreeMonomial);
    } else if (solverEngine == "csdp" && method == "putinar") {
        estimator.solveWithPutinarCsdp();
    } else if (solverEngine == "csdp" && method == "handelman") {
        estimator.solveWithHandelmanCsdp(highDegreeMonomial);
    } else {
        std::cout << "Unknown method or solver engine: " << method << " " << solverEngine << std::endl;
        return 1;
    }


    if (!estimator.isFeasible()) {
        std::cout << "\n\n===========================================================\n";
        std::cout << "Infeasible" << std::endl;
        std::cout << "The system is either infeasible or the specified degree is too small" << std::endl;
        std::cout << "Try to increase the degree using -deg <value>" << std::endl;
        return 0;
    }

    std::cout << "\n\n===========================================================\n";
    std::cout << "The solution is found" << std::endl;
    std::cout << "To print the solution, run python " << inputFileName << ".cert.py answer" << std::endl;
    std::cout << "To check the correcteness of the solution, run python " << inputFileName << ".cert.py" << std::endl;

    // to remove directory build/ accidentally added to git by git add .
    // run git rm -r --cached build/

    // open file sol.cert.py for writing
    std::ofstream certFile;
    certFile.open(inputFileName + ".cert.py");

    estimator.getCertifiedSolutionPy(certFile);


    return 0;
}



