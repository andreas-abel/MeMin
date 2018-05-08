# MeMin
SAT-based exact minimization of incompletely specified Mealy machines

## Introduction

MeMin is a tool for minimizing incompletely specified Mealy machines, i.e., Mealy machines where one or more outputs or next states might not be specified. Minimization of a machine M in this context means finding a machine M' with the minimal number of states that has the same input/output behavior on all input sequences, on which the behavior of M is defined (but M' might be defined on additional input sequences on which M is not defined). While the problem of minimizing Mealy machines is efficiently solvable for fully specified machines, it is NP-complete for incompletely specified machines.

Most existing exact minimization tools are based on the enumeration of sets of compatible states, and the solution of a covering problem. MeMin uses a different approach. First, a polynomial-time algorithm is used to compute a partial solution. This partial solution is then extended to a minimum-size complete solution by solving a series of Boolean satisfiability (SAT) problems.

MeMin is written in C++, and it uses MiniSat as SAT solver. Like many existing tools, MeMin accepts inputs in the widely used [KISS2 input format](http://ddd.fit.cvut.cz/prj/Benchmarks/LGSynth91.pdf). In this format, Mealy machines are essentially described by a set of 4-tuples of the form (input; currentState; nextState; output).

## How to use MeMin

Simply run ./MeMin [Options] <input.kiss>. The output is written to a file named result.kiss in the same folder.

The following options can be specified:

    -r:         if no reset state is specified, any state might be a reset state (otherwise, the first state is assumed to be the reset state)
    -np:        do not include the 'partial solution' in the SAT problem
    -nl:        like -np, but does also not use the size of the 'partial solution' as a lower bound (i.e., does not need the partial solution at all)
    -v {0,1}:   verbosity level

## Evaluation Results

We have compared the performance of our implementation to two other exact approaches: [BICA](http://www.inesc-id.pt/pt/indicadores/Ficheiros/963.pdf) is based on Angluin’s learning algorithm, and [STAMINA (exact mode)](http://web.cecs.pdx.edu/~mperkows/CLASS_573/Asynchr_Febr_2007/00259940.pdf) is a popular implementation of the explicit version of the two-step standard approach. Furthermore, we have also compared our tool with [STAMINA (heuristic mode)](http://web.cecs.pdx.edu/~mperkows/CLASS_573/Asynchr_Febr_2007/00259940.pdf), and [COSME](http://www.degruyter.com/dg/viewarticle.fullcontentlink:pdfeventlink/$002fj$002fcomp.2013.3.issue-2$002fs13537-013-0106-0$002fs13537-013-0106-0.pdf?t:ac=j$002fcomp.2013.3.issue-2$002fs13537-013-0106-0$002fs13537-013-0106-0.xml), which is another, recently proposed, heuristic technique.

We used the same sets of benchmarks that were used previously in the literature. These benchmarks come from several sources, e.g., logic synthesis, learning problems, and networks of interacting FSMs. On a number of hard benchmarks, our approach outperforms the other exact minimization techniques by several orders of magnitude; it is even competitive with state-of-the-art heuristic approaches on most benchmarks.

A table with all benchmark results can be found [here](http://embedded.cs.uni-saarland.de/tools/MeMin/results.pdf).

## References
Please see the following paper if you are interested in how MeMin works:

>  MeMin: SAT-based Exact Minimization of Incompletely Specified Mealy Machines  
>  A. Abel and J. Reineke  
>  ICCAD, 2015. [doi](http://dx.doi.org/10.1109/ICCAD.2015.7372555)  [pdf](http://embedded.cs.uni-saarland.de/publications/iccad15.pdf)
