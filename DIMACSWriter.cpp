/*
 * DIMACSWriter.cpp
 *
 *  Created on: 02.03.2015
 *      Author: Andreas Abel
 */

#include "DIMACSWriter.h"
#include "global.h"

void computeReducedInputAlphabet(unordered_set<int>& reducedInputAlphabet, int maxInput, vector<vector<int> >& machineNextState) {
	unordered_map<int, vector<vector<int> > > hashmap;

	for (int input=0; input<=maxInput; input++) {

		int hash = 0;
		vector<int> nextStatesList;

		for (unsigned int i=0; i<machineNextState.size(); i++) {
			vector<int>& curNextStates = machineNextState[i];
			int succ = curNextStates[input];

			if (succ==-1) {
				nextStatesList.push_back(-1);
				hash = 31*hash;
			} else {
				nextStatesList.push_back(succ);
				hash = 31*hash + succ;
			}
		}

		unordered_map<int, vector<vector<int> > >::iterator hashmapEntry = hashmap.find(hash);

		if (hashmapEntry==hashmap.end()) {
			hashmap[hash].push_back(nextStatesList);
			reducedInputAlphabet.insert(input);
		} else {
			vector<vector<int> >& statesListsWithSameHash = hashmapEntry->second;

			bool contained = false;

			for (vector<vector<int> >::iterator it=statesListsWithSameHash.begin(); it!=statesListsWithSameHash.end(); it++) {
				vector<int>& curStateList = *it;

				bool equal = true;
				for (unsigned int i=0; i<machineNextState.size(); i++) {
					if (nextStatesList[i]!=curStateList[i]) {
						equal = false;
						break;
					}
				}

				if (equal) {
					contained = true;
					break;
				}
			}

			if (!contained) {
				hashmap[hash].push_back(nextStatesList);
				reducedInputAlphabet.insert(input);
			}
		}
	}
}

Solver* S;

int* stateClassToLiteral;
int curLiteral;
int getStateLiteral(int state, int sClass, int numClasses) {
	int key = ai(state, sClass,numClasses);

	int retLiteral = stateClassToLiteral[key];
	if (retLiteral==-1) {
		retLiteral = curLiteral;
		stateClassToLiteral[key] = curLiteral++;
	}

	return retLiteral;
}

int* auxLiteralsMap;
int getAuxLiteral(int j) {
	int key = j;

	int retLiteral = auxLiteralsMap[key];
	if (retLiteral==-1) {
		retLiteral = curLiteral;
		auxLiteralsMap[key] = curLiteral++;
	}

	return retLiteral;
}

void addLitToCurrentClause(int lit) {
	if (S!=NULL) {
		int var = abs(lit)-1;
		while (var >= (S->nVars())) {
			S->newVar();
		}
		curMinisatClause.push((lit>0) ? mkLit(var) : ~mkLit(var));
	} else {
		curClause.push_back(lit);
	}
}

void addClause() {
	if (S!=NULL) {
		S->addClause(curMinisatClause);
		curMinisatClause.clear();
	} else {
		clauses.push_back(curClause);
		curClause.clear();
	}
}

vector<vector<int> > clauses;
vec<Lit> curMinisatClause;
vector<int> curClause;

void buildCNF(Solver* solver, vector<pair<int, int> >& literalToStateClass, unsigned int numClasses, vector<vector<int> >& machineNextState, vector<bool>& incompMatrix, vector<int>& pairwiseIncStates, int maxInput) {
	S = solver;
	clauses.clear();
	curClause.clear();
	curMinisatClause.clear();

	int nStates = machineNextState.size();
	curLiteral = 1;

	stateClassToLiteral = new int[nStates*numClasses];
	for (unsigned int i=0; i<nStates*numClasses; i++) stateClassToLiteral[i]=-1;

	auxLiteralsMap = new int[numClasses];

	//add pairwise incompatible states to different classes
	for (unsigned int i=0; i<pairwiseIncStates.size(); i++) {
		int s = pairwiseIncStates[i];
		addLitToCurrentClause(getStateLiteral(s,i,numClasses));
		addClause();
	}

	vector<vector<int> > statesThatCanBeInClass;
	statesThatCanBeInClass.resize(numClasses);
	for (unsigned int i=0; i<numClasses; i++) {
		vector<int>& curVector = statesThatCanBeInClass[i];
		for (int s=0; s<nStates; s++) {
			if (i<pairwiseIncStates.size() && incompMatrix[ai(s, pairwiseIncStates[i], nStates)]) continue;
			curVector.push_back(s);
		}
	}

	//each state must be in at least one class
	for (int s=0; s<nStates; s++) {
		for (unsigned int i=0; i<numClasses; i++) {
			if (i<pairwiseIncStates.size() && incompMatrix[ai(s, pairwiseIncStates[i], nStates)]) continue;
			addLitToCurrentClause(getStateLiteral(s,i,numClasses));
		}
		addClause();
	}

	//incompatible states must not be in the same class
	for (unsigned int i=0; i<pairwiseIncStates.size(); i++) {
		int s = pairwiseIncStates[i];

		for (int incompS=0; incompS<nStates; incompS++) {
			if (!incompMatrix[ai(s, incompS, nStates)]) continue;
			addLitToCurrentClause(-getStateLiteral(incompS,i,numClasses));
			addClause();
		}
	}

	for (int s=0; s<nStates; s++) {
		for (unsigned int i=0; i<numClasses; i++) {
			if (i<pairwiseIncStates.size() && incompMatrix[ai(s, pairwiseIncStates[i], nStates)]) continue;
			for (int incompS=s+1; incompS<nStates; incompS++) {
				if (!incompMatrix[ai(s, incompS, nStates)]) continue;
				addLitToCurrentClause(-getStateLiteral(s,i,numClasses));
				addLitToCurrentClause(-getStateLiteral(incompS,i,numClasses));
				addClause();
			}
		}
	}

	unordered_set<int> reducedInputAlphabet;
	computeReducedInputAlphabet(reducedInputAlphabet, maxInput, machineNextState);

	timeval start, end;
	gettimeofday(&start, 0);

	vector<bool> possibleSuccClasses;
	possibleSuccClasses.resize(numClasses,false);

	//closure constraints
	for (int a=0; a<=maxInput; a++) {
		if (reducedInputAlphabet.count(a)==0) continue;

		for (unsigned int i=0; i<numClasses; i++) {
			//clear auxLiteralsMap and possibleSuccClasses
			for (unsigned int j=0; j<numClasses; j++) {
				auxLiteralsMap[j]=-1;
			}
			possibleSuccClasses.assign(numClasses,false);

			unsigned int smallestSuccClass = numClasses+1;
			unsigned int largestSuccClass = 0;

			vector<int>& statesThatCanBeInClassI = statesThatCanBeInClass[i];
			for (vector<int>::iterator sIt=statesThatCanBeInClassI.begin(); sIt!=statesThatCanBeInClassI.end(); sIt++) {
				int s = *sIt;
				int succS = machineNextState[s][a];
				if (succS == -1) continue;

				for (unsigned int j=0; j<numClasses; j++) {
					if (j<pairwiseIncStates.size() && incompMatrix[ai(succS, pairwiseIncStates[j], nStates)]) continue;
					possibleSuccClasses[j]=true;
					if (j<smallestSuccClass) smallestSuccClass=j;
					if (j>largestSuccClass) largestSuccClass=j;
				}
			}

			//auxOr
			for (unsigned int j=smallestSuccClass; j<=largestSuccClass; j++) {
				if (possibleSuccClasses[j]) {
					addLitToCurrentClause(getAuxLiteral(j));
				}
			}

			if (curMinisatClause.size()==0 && curClause.size()==0) continue;
			addClause();

			for (vector<int>::iterator sIt=statesThatCanBeInClassI.begin(); sIt!=statesThatCanBeInClassI.end(); sIt++) {
				int s = *sIt;
				int succS = machineNextState[s][a];
				if (succS == -1) continue;

				for (unsigned int j=smallestSuccClass; j<=largestSuccClass; j++) {
					if (!possibleSuccClasses[j]) continue;

					addLitToCurrentClause(-getAuxLiteral(j));
					addLitToCurrentClause(-getStateLiteral(s,i,numClasses));
					addLitToCurrentClause(getStateLiteral(succS,j,numClasses));
					addClause();
				}
			}
		}
	}

	gettimeofday(&end, 0);
	if (verbosity>=1) cout << "Closure Constraints: "<< (end.tv_usec-start.tv_usec) << " usec" << endl;

	pair<int, int> defaultPair(-1,-1);

	literalToStateClass.resize(curLiteral);
	for (int i=0; i<curLiteral; i++) literalToStateClass[i]=defaultPair;

	for (int i=0; i<nStates; i++) {
		for (unsigned int j=0; j<numClasses; j++) {
			if (stateClassToLiteral[ai(i,j,numClasses)]>-1) {
				literalToStateClass[stateClassToLiteral[ai(i,j,numClasses)]] = make_pair(i,j);
			}
		}
	}
}
