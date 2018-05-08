#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <fstream>
#include <ctime>
#include <queue>
#include <sys/time.h>
#include <unordered_map>
#include <unordered_set>

#include "minisat/core/Solver.h"
#include "minisat/utils/System.h"

#include "global.h"
#include "KISSParser.h"
#include "IncSpecSeq.h"
#include "DIMACSWriter.h"
#include "MachineBuilder.h"

using std::cout;
using std::endl;
using std::vector;
using std::map;
using std::unordered_map;
using std::unordered_set;
using std::set;
using std::pair;
using std::sort;
using std::queue;
using namespace Minisat;

void printStats(Solver& solver) {
    double cpu_time = cpuTime();
    double mem_used = memUsedPeak();
    printf("restarts              : %" PRIu64"\n", solver.starts);
    printf("conflicts             : %-12" PRIu64"   (%.0f /sec)\n", solver.conflicts   , solver.conflicts   /cpu_time);
    printf("decisions             : %-12" PRIu64"   (%4.2f %% random) (%.0f /sec)\n", solver.decisions, (float)solver.rnd_decisions*100 / (float)solver.decisions, solver.decisions   /cpu_time);
    printf("propagations          : %-12" PRIu64"   (%.0f /sec)\n", solver.propagations, solver.propagations/cpu_time);
    printf("conflict literals     : %-12" PRIu64"   (%4.2f %% deleted)\n", solver.tot_literals, (solver.max_literals - solver.tot_literals)*100 / (double)solver.max_literals);
    if (mem_used != 0) printf("Memory used           : %.2f MB\n", mem_used);
    printf("CPU time              : %g s\n", cpu_time);
}

void removeUnreachableStates(vector<vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > > >& machine, int& resetState);
void computePredecessorMap(vector<vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > > >& states, unordered_map<IncSpecSeq*,vector<int> > pred[]);
void computeIncompMatrix(vector<vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > > >& states, unordered_map<IncSpecSeq*,vector<int> > pred[], vector<bool>& incompMatrix);
vector<vector<bool> > getTransitivelyCompatibleStates(vector<vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > > >& states, vector<bool>& incompMatrix);
void splitTransitions(vector<vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > > >& states, vector<bool>& incompMatrix, vector<vector<int> >& newNextStates, vector<vector<IncSpecSeq*> >& newOutput, vector<IncSpecSeq>& inputIDToIncSpecSeq);
unordered_set<IncSpecSeq> getDisjointInputSet(vector<vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > > >& states, vector<bool>& eqClass);
void findPairwiseIncStates(vector<int>& pairwiseIncStates, vector<bool>& incompMatrix, int nStates);

int verbosity = 0;
bool firstStateReset = true;
bool noPartialSolutionInSat = false;
bool noLowerBound = false;

void usage() {
	cout << "Usage: ./MeMin [Options] <input.kiss>" << endl;
	cout << endl;
	cout << "Options:" << endl;
	cout << "  -r        if no reset state is specified, any state might be a reset state" << endl;
	cout <<	"            (otherwise, the first state is assumed to be the reset state)"<< endl;
	cout << "  -np       do not include the 'partial solution' in the SAT problem" << endl;
	cout << "  -nl       like -np, but does also not use the size of the 'partial solution'" << endl;
	cout << "            as a lower bound (i.e., does not need the partial solution at all)" << endl;
	cout << "  -v {0,1}  verbosity level" << endl;
}

int main(int argc, char* argv[]) {
	timeval start, end;
	gettimeofday(&start, 0);

	int resetState;
	vector<vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > > > machine;
	int numInputBits;
	int numOutputBits;

	for (int argI=1; argI < argc-1; argI++) {
		char* arg = argv[argI];
		if (strcmp(arg,"-r")==0) {
			firstStateReset = false;
		} else if (strcmp(arg,"-np")==0) {
			noPartialSolutionInSat = true;
		} else if (strcmp(arg,"-nl")==0) {
			noLowerBound = true;
		} else if (strcmp(arg,"-v")==0) {
			argI++;
			verbosity = argv[argI][0]-'0';
			if (verbosity<0 || verbosity>9) {
				usage();
				return 1;
			}
		} else {
			usage();
			return 1;
		}
	}

	if ((argc <= 1) || (argv[argc - 1] == NULL) || (argv[argc - 1][0] == '-')) {
		usage();
		return 1;
	} else {
		parseKISSFile(argv[argc-1], machine, resetState, numInputBits, numOutputBits);
	}

	gettimeofday(&end, 0);
	if (verbosity>0) cout << "Parsing: "<< (end.tv_sec*1e6 + end.tv_usec) - (start.tv_sec*1e6 + start.tv_usec) << " usec" << endl;
	gettimeofday(&start, 0);

	if (resetState!=-1) {
		removeUnreachableStates(machine, resetState);
		gettimeofday(&end, 0);
		if (verbosity>0) cout << "Removing unreachable states: "<< (end.tv_sec*1e6 + end.tv_usec) - (start.tv_sec*1e6 + start.tv_usec) << " usec" << endl;
		gettimeofday(&start, 0);
	}

	int nStates = machine.size();

	//predecessors for each state and input
	unordered_map<IncSpecSeq*,vector<int> > pred[nStates];
	computePredecessorMap(machine, pred);

	gettimeofday(&end, 0);
	if (verbosity>0) cout << "Computing pred map: "<< (end.tv_sec*1e6 + end.tv_usec) - (start.tv_sec*1e6 + start.tv_usec) << " usec" << endl;
	gettimeofday(&start, 0);

	//0 if compatible, 1 if incompatible
	vector<bool> incompMatrix;
	incompMatrix.resize(nStates*nStates, false);
	computeIncompMatrix(machine, pred, incompMatrix);

	gettimeofday(&end, 0);
	if (verbosity>0) cout << "Computing IncompMatrix: "<< (end.tv_sec*1e6 + end.tv_usec) - (start.tv_sec*1e6 + start.tv_usec) << " usec" << endl;
	gettimeofday(&start, 0);

	vector<vector<int> > nextStatesMap(nStates);
	vector<vector<IncSpecSeq*> > outputsMap(nStates);
	vector<IncSpecSeq> inputIDToIncSpecSeq;
	splitTransitions(machine, incompMatrix, nextStatesMap, outputsMap, inputIDToIncSpecSeq);

	gettimeofday(&end, 0);
	if (verbosity>0) cout << "Splitting transitions: "<< (end.tv_sec*1e6 + end.tv_usec) - (start.tv_sec*1e6 + start.tv_usec) << " usec" << endl;
	gettimeofday(&start, 0);

	gettimeofday(&start, 0);
	vector<int> pairwiseIncStates;
	if (!noLowerBound) findPairwiseIncStates(pairwiseIncStates, incompMatrix, nStates);
	gettimeofday(&end, 0);
	if (verbosity>0) cout << "Finding pairwise incomp states: "<< (end.tv_sec*1e6 + end.tv_usec) - (start.tv_sec*1e6 + start.tv_usec) << " usec" << endl;
	gettimeofday(&start, 0);

	for (int nClasses=pairwiseIncStates.size(); nClasses>=0; nClasses++) {
		if (verbosity>0) cout << "Classes: " << nClasses << endl;
		if (noPartialSolutionInSat) pairwiseIncStates.clear();

		//if literal i is true, and literalToStateClass[i]=(s,c) then state s is in class c
		vector<pair<int, int> > literalToStateClass;

		timeval start2, end2;
		gettimeofday(&start2, 0);

		Solver S;
		buildCNF(&S, literalToStateClass, nClasses, nextStatesMap, incompMatrix, pairwiseIncStates, inputIDToIncSpecSeq.size()-1);

		gettimeofday(&end2, 0);
		if (verbosity>0) cout << "Building CNF: "<< (end2.tv_sec*1e6 + end2.tv_usec) - (start2.tv_sec*1e6 + start2.tv_usec) << " usec" << endl;
		gettimeofday(&start2, 0);

		vec<Lit> dummy;
		lbool ret = S.solveLimited(dummy);

		gettimeofday(&end2, 0);
		if (verbosity>0) cout << "Minisat: "<< (end2.tv_sec*1e6 + end2.tv_usec) - (start2.tv_sec*1e6 + start2.tv_usec) << " usec" << endl;

		if (verbosity>0) cout << (ret == l_True ? "SATISFIABLE\n" : ret == l_False ? "UNSATISFIABLE\n" : "INDETERMINATE\n");

		if (verbosity>1) printStats(S);

		if (ret == l_True){
			gettimeofday(&end, 0);
			if (verbosity>0) cout << "Total time for SAT: "<< (end.tv_sec*1e6 + end.tv_usec) - (start.tv_sec*1e6 + start.tv_usec) << " usec" << endl;
			gettimeofday(&start, 0);

			std::vector<int> dimacsOutput;

			printf("SAT\n");
			for (int i = 0; i < S.nVars(); i++) {
				if (S.model[i] != l_Undef) {
					int lit = i+1;
					if (S.model[i]==l_False) lit = -lit;
					dimacsOutput.push_back(lit);					}
			}

			int newResetState=-1;

			vector<vector<int> > newMachineNextStatesMap(nClasses);
			vector<vector<IncSpecSeq*> > newMachineOutputsMap(nClasses);
			buildMachine(newMachineNextStatesMap, newMachineOutputsMap, newResetState, nClasses, dimacsOutput, literalToStateClass, nextStatesMap, outputsMap, resetState, inputIDToIncSpecSeq.size()-1);

			gettimeofday(&end, 0);
			if (verbosity>0) cout << "Building machine: "<< (end.tv_sec*1e6 + end.tv_usec) - (start.tv_sec*1e6 + start.tv_usec) << " usec" << endl;
			gettimeofday(&start, 0);

			writeKISSFile(newMachineNextStatesMap, newMachineOutputsMap, newResetState, numInputBits, numOutputBits, inputIDToIncSpecSeq, "result.kiss");

			gettimeofday(&end, 0);
			if (verbosity>0) cout << "Writing to KISS file: "<< (end.tv_sec*1e6 + end.tv_usec) - (start.tv_sec*1e6 + start.tv_usec) << " usec" << endl;

			std::cerr << nClasses;
			cout << endl;
			exit(0);
		}
	}
	exit(0);
}

void removeUnreachableStates(vector<vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > > >& machine, int& resetState) {
	bool reachableStates[machine.size()];
	for (unsigned int i=0; i<machine.size(); i++) reachableStates[i]=false;

	reachableStates[resetState] = true;

	queue<int> worklist;
	worklist.push(resetState);

	while (!worklist.empty()) {
		int state = worklist.front();
		worklist.pop();

		vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >& trans = machine[state];
		for (vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >::iterator it=trans.begin(); it != trans.end(); it++) {
			int nextState = (it->second).first;
			if (reachableStates[nextState]) continue;

			reachableStates[nextState]=true;
			worklist.push(nextState);
		}
	}

	int stateRemapping[machine.size()];

	for (unsigned int i=0; i<machine.size(); i++) {
		if (reachableStates[i]) {
			stateRemapping[i]=i;
			continue;
		}

		unsigned int lastReachableState;
		for (lastReachableState=machine.size()-1; lastReachableState>=0; lastReachableState--) {
			if (reachableStates[lastReachableState]) break;
		}

		if (lastReachableState>i) {
			stateRemapping[lastReachableState]=i;
			machine[i]=machine[lastReachableState];
			reachableStates[i]=true;
			reachableStates[lastReachableState]=false;
			machine.pop_back();
		} else {
			machine.resize(lastReachableState+1);
		}
	}

	for (unsigned int i=0; i<machine.size(); i++) {
		vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >& trans = machine[i];
		for (vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >::iterator it=trans.begin(); it != trans.end(); it++) {
			pair<int, IncSpecSeq*>& p = it->second;
			p.first = stateRemapping[p.first];
		}
	}

	resetState = stateRemapping[resetState];
}

void computePredecessorMap(vector<vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > > >& states, unordered_map<IncSpecSeq*,vector<int> > pred[]) {
	for (unsigned int s=0; s<states.size(); s++) {
		vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >& tMap = states[s];
		for (vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >::iterator it=tMap.begin(); it!=tMap.end(); ++it) {
			IncSpecSeq* input = it->first;
			pair<int, IncSpecSeq*>& p = it->second;
			int nextS = p.first;

			pred[nextS][input].push_back(s);
		}
	}
}

void propagateIncompStates(int s1, int s2, int nStates, unordered_map<IncSpecSeq*,vector<int> > pred[], vector<bool>& incompMatrix) {
	unordered_map<IncSpecSeq*,vector<int> >& pred1 = pred[s1];
	unordered_map<IncSpecSeq*,vector<int> >& pred2 = pred[s2];

	for (unordered_map<IncSpecSeq*,vector<int> >::iterator it1=pred1.begin(); it1!=pred1.end(); it1++) {
		const IncSpecSeq& input1 = *(it1->first);
		vector<int>& predStates1 = it1->second;

		for (unordered_map<IncSpecSeq*,vector<int> >::iterator it2=pred2.begin(); it2!=pred2.end(); it2++) {
			const IncSpecSeq& input2 = *(it2->first);

			if (input1.isDisjoint(input2)) continue;

			vector<int>& predStates2 = it2->second;

			for (unsigned int i1 = 0; i1<predStates1.size(); i1++) {
				int predS1 = predStates1[i1];
				for (unsigned int i2 = 0; i2<predStates2.size(); i2++) {
					int predS2 = predStates2[i2];
					if (incompMatrix[ai(predS1,predS2,nStates)]) continue;

					incompMatrix[ai(predS1,predS2,nStates)]=true;
					incompMatrix[ai(predS2,predS1,nStates)]=true;
					propagateIncompStates(predS1, predS2, nStates, pred, incompMatrix);
				}
			}
		}
	}
}

void computeIncompMatrix(vector<vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > > >& states, unordered_map<IncSpecSeq*,vector<int> > pred[], vector<bool>& incompMatrix) {
	int nStates = states.size();

	for (int s1=0; s1<nStates; s1++) {
		vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >& succMap1 = states[s1];
		for (int s2=s1; s2<nStates; s2++) {
			if (incompMatrix[ai(s1,s2,nStates)]) continue;

			vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >& succMap2 = states[s2];

			for (vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >::iterator it1=succMap1.begin(); it1!=succMap1.end(); ++it1) {
				const IncSpecSeq& input1 = *(it1->first);

				bool incompOutputFound = false;
				for (vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >::iterator it2=succMap2.begin(); it2!=succMap2.end(); ++it2) {
					const IncSpecSeq& input2 = *(it2->first);

					if (input1.isDisjoint(input2)) continue;

					pair<int, IncSpecSeq*>& p1 = it1->second;
					pair<int, IncSpecSeq*>& p2 = it2->second;

					IncSpecSeq& o1 = *(p1.second);
					IncSpecSeq& o2 = *(p2.second);

					if (o1.isCompatible(o2)) continue;

					incompMatrix[ai(s1,s2,nStates)]=true;
					incompMatrix[ai(s2,s1,nStates)]=true;

					propagateIncompStates(s1, s2, nStates, pred, incompMatrix);

					incompOutputFound=true;
					break;
				}
				if (incompOutputFound) break;
			}
		}
	}
}

//partitions the set of states into equivalence classes, s.t. two states are in the same class if they are transitively compatible
//ret[i][s]==true iff state s is in class i
vector<vector<bool> > getTransitivelyCompatibleStates(vector<vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > > >& states, vector<bool>& incompMatrix) {
	vector<vector<bool> > ret;

	int nStates = states.size();
	vector<bool> processedStates;
	processedStates.resize(nStates, false);

	for (int s=0; s<nStates; s++) {
		if (processedStates[s]) continue;
		processedStates[s] = true;

		vector<bool> curSet;
		curSet.resize(nStates, 0);
		curSet[s]=true;

		queue<int> worklist;
		worklist.push(s);

		while (!worklist.empty()) {
			int curS = worklist.front();
			worklist.pop();

			for (int i=s; i<nStates; i++) {
				if (processedStates[i]) continue;
				if (incompMatrix[ai(curS,i,nStates)]) continue;

				worklist.push(i);
				processedStates[i] = true;
				curSet[i]=true;
			}
		}
		ret.push_back(curSet);
	}

	return ret;
}

//compatible states must not have transitions with overlapping inputs
void splitTransitions(vector<vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > > >& states, vector<bool>& incompMatrix, vector<vector<int> >& newNextStates, vector<vector<IncSpecSeq*> >& newOutput, vector<IncSpecSeq>& inputIDToIncSpecSeq) {
	vector<vector<bool> > tcs = getTransitivelyCompatibleStates(states, incompMatrix);

	vector<unordered_set<IncSpecSeq> >  disjInputsForTCS(tcs.size());

	unordered_map<IncSpecSeq, int> incSpecSeqToInputID;

	for (unsigned int i=0; i<tcs.size(); i++) {
		unordered_set<IncSpecSeq> disjInputs = getDisjointInputSet(states,tcs[i]);
		disjInputsForTCS[i] = disjInputs;

		for (unordered_set<IncSpecSeq>::iterator it=disjInputs.begin(); it!=disjInputs.end(); it++) {
			IncSpecSeq disjInput = *it;

			if (incSpecSeqToInputID.count(disjInput)>0) continue;

			incSpecSeqToInputID[disjInput] = inputIDToIncSpecSeq.size();
			inputIDToIncSpecSeq.push_back(disjInput);
		}
	}

	for (unsigned int i=0; i<tcs.size(); i++) {
		unordered_set<IncSpecSeq>& disjInputs = disjInputsForTCS[i];

		vector<bool>& tcsi = tcs[i];
		for (unsigned int curTcs=0; curTcs<states.size(); curTcs++) {
			if (!tcsi[curTcs]) continue;
			vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >& curMap = states[curTcs];

			vector<int>& curNextState = newNextStates[curTcs];
			vector<IncSpecSeq*>& curOutput = newOutput[curTcs];
			curNextState.resize(inputIDToIncSpecSeq.size());
			curOutput.resize(inputIDToIncSpecSeq.size());
			for (unsigned int j=0; j<inputIDToIncSpecSeq.size(); j++) {
				curNextState[j]=-1;
			}

			for (vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >::iterator mIt=curMap.begin(); mIt!=curMap.end(); mIt++) {
				const IncSpecSeq& input = *(mIt->first);
				const pair<int, IncSpecSeq*>& trans = mIt->second;

				if (disjInputs.count(input)>0) {
					int inputID = incSpecSeqToInputID[input];
					curNextState[inputID] = trans.first;
					curOutput[inputID] = trans.second;
				} else {
					for (unordered_set<IncSpecSeq>::iterator sIt=disjInputs.begin(); sIt!=disjInputs.end(); sIt++) {
						const IncSpecSeq& disjInput = *sIt;
						if (disjInput.isSubset(input)) {
							int inputID = incSpecSeqToInputID[disjInput];
							curNextState[inputID] = trans.first;
							curOutput[inputID] = trans.second;
						}
					}
				}
			}
		}
	}
}

struct IncSpecSeqPtrComp {
	bool operator()(const IncSpecSeq* lhs, const IncSpecSeq* rhs) const  {
		return (*lhs)==(*rhs);
	}
};

//computes a set of non-overlapping input sequences s.t. all transitions for states in eqClass are covered
unordered_set<IncSpecSeq> getDisjointInputSet(vector<vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > > >& states, vector<bool>& eqClass) {
	int nStates = states.size();
	unordered_set<IncSpecSeq*, std::hash<IncSpecSeq*>, IncSpecSeqPtrComp> disjointInputs;

	int curS=0;
	for (; curS<nStates; curS++) {
		if (eqClass[curS]) break;
	}
	if (curS>=nStates) return unordered_set<IncSpecSeq>();

	bool nonFullySpecInputFound = false;

	vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >& firstMap = states[curS];
	for (vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >::iterator mIp=firstMap.begin(); mIp!=firstMap.end(); mIp++) {
		IncSpecSeq* input = mIp->first;
		if (!input->isFullySpecified()) nonFullySpecInputFound=true;
		disjointInputs.insert(input);
	}


	unordered_set<IncSpecSeq*, std::hash<IncSpecSeq*>, IncSpecSeqPtrComp> alreadyInQueue;
	queue<IncSpecSeq*> remainingInputs;
	for (; curS<nStates; curS++) {
		if (!eqClass[curS]) continue;
		vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >& curMap = states[curS];
		for (vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >::iterator mIp=curMap.begin(); mIp!=curMap.end(); mIp++) {
			IncSpecSeq* input = mIp->first;
			if (!input->isFullySpecified()) nonFullySpecInputFound=true;
			if (alreadyInQueue.count(input)==0) {
				remainingInputs.push(input);
				alreadyInQueue.insert(input);
			}
		}
	}

	if (!nonFullySpecInputFound) {
		while (!remainingInputs.empty()) {
			IncSpecSeq* curInput = remainingInputs.front();
			remainingInputs.pop();
			disjointInputs.insert(curInput);
		}
		unordered_set<IncSpecSeq> ret;
		for (unordered_set<IncSpecSeq*, std::hash<IncSpecSeq*>, IncSpecSeqPtrComp>::iterator it=disjointInputs.begin(); it!=disjointInputs.end(); it++) {
			ret.insert(**it);
		}
		return ret;
	}

	while (!remainingInputs.empty()) {
		IncSpecSeq* curInput = remainingInputs.front();
		remainingInputs.pop();

		if (disjointInputs.count(curInput)>0) continue;

		int intersectingInputFound=false;
		for (unordered_set<IncSpecSeq*>::iterator dIt=disjointInputs.begin(); dIt != disjointInputs.end(); dIt++) {
			IncSpecSeq* disjInput = *dIt;

			if (!disjInput->isDisjoint(*curInput)) {
				intersectingInputFound = true;

				if (curInput->isSubset(*disjInput)) {
					IncSpecSeq inters = disjInput->intersect(*curInput);
					vector<IncSpecSeq> diff = disjInput->diff(*curInput);
					disjointInputs.erase(disjInput);
					disjointInputs.insert(new IncSpecSeq(inters));
					for (unsigned int i=0; i<diff.size(); i++) {
						disjointInputs.insert(new IncSpecSeq(diff[i]));
					}
				} else if (disjInput->isSubset(*curInput)) {
					vector<IncSpecSeq> diff = curInput->diff(*disjInput);

					for (unsigned int i=0; i<diff.size(); i++) {
						remainingInputs.push(new IncSpecSeq(diff[i]));
					}
				} else {
					IncSpecSeq inters = disjInput->intersect(*curInput);
					vector<IncSpecSeq> diff = disjInput->diff(*curInput);

					disjointInputs.insert(new IncSpecSeq(inters));

					for (unsigned int i=0; i<diff.size(); i++) {
						disjointInputs.insert(new IncSpecSeq(diff[i]));
					}

					vector<IncSpecSeq> diff2 = curInput->diff(*disjInput);
					for (unsigned int i=0; i<diff2.size(); i++) {
						remainingInputs.push(new IncSpecSeq(diff2[i]));
					}

					disjointInputs.erase(disjInput);
				}
				break;
			}
		}
		if (!intersectingInputFound) {
			disjointInputs.insert(curInput);
		}
	}

	unordered_set<IncSpecSeq> ret;
	for (unordered_set<IncSpecSeq*, std::hash<IncSpecSeq*>, IncSpecSeqPtrComp>::iterator it=disjointInputs.begin(); it!=disjointInputs.end(); it++) {
		ret.insert(**it);
	}
	return ret;
}

class incStateComp {
	int* nIncomp;
public:
	incStateComp(vector<bool>& incompMatrix, int nStates) {
		nIncomp = new int[nStates];
		for (int i=0; i<nStates; i++) {
			int c=0;
			for (int j=0; j<nStates; j++) {
				if (incompMatrix[ai(i,j,nStates)]) c++;
			}
			nIncomp[i]=c;
		}
	}

	bool operator() (int i,int j) {
		return (nIncomp[i]>nIncomp[j]);
	}
};

void findPairwiseIncStates(vector<int>& pairwiseIncStates, vector<bool>& incompMatrix, int nStates) {
	incStateComp comp(incompMatrix, nStates);

	vector<int> states;
	for (int i=0; i<nStates; i++) {
		states.push_back(i);
	}

	sort(states.begin(), states.end(), comp);

	for (int i=0; i<nStates; i++) {
		int s1 = states[i];
		bool compStateFound = false;
		for (unsigned int j=0; j<pairwiseIncStates.size(); j++) {
			int s2 = pairwiseIncStates[j];
			if (!incompMatrix[ai(s1,s2,nStates)]) {
				compStateFound = true;
				break;
			}
		}
		if (compStateFound) continue;
		pairwiseIncStates.push_back(s1);
	}
}
