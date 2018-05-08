/*
 * DIMACSWriter.h
 *
 *  Created on: 02.03.2015
 *      Author: Andreas Abel
 */

#ifndef DIMACSWRITER_H_
#define DIMACSWRITER_H_

#include <vector>
#include <map>
#include "IncSpecSeq.h"
#include "minisat/core/Solver.h"
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <sstream>

#include <sys/time.h>
#include "IncSpecSeq.h"

using namespace Minisat;

using std::cout;
using std::endl;
using std::vector;
using std::map;
using std::set;
using std::unordered_map;
using std::unordered_set;
using std::pair;
using std::string;
using std::make_pair;

//array index
inline int ai(int x, int y, int ySize) {
	return x*ySize+y;
}

extern Solver* S;

extern int* stateClassToLiteral;
extern int curLiteral;
int getStateLiteral(int state, int sClass, int numClasses);

extern int* auxLiteralsMap;
int getAuxLiteral(int i, int j, int numClasses);

extern vector<vector<int> > clauses;
extern vec<Lit> curMinisatClause;
extern vector<int> curClause;

void addClause();
void addLitToCurrentClause(int lit);


template <class InputType>
void computeInputAlphabet(set<InputType>& inputAlphabet, vector<map<InputType, pair<int, IncSpecSeq> > >& states) {
	for (typename vector<map<InputType, pair<int, IncSpecSeq> > >::iterator vIt = states.begin(); vIt != states.end(); vIt++) {
		for (typename map<InputType, pair<int, IncSpecSeq> >::iterator mIt = vIt->begin(); mIt!=vIt->end(); mIt++) {
			inputAlphabet.insert(mIt->first);
		}
	}
}

template <class InputType>
void computeReducedInputAlphabet(set<InputType>& reducedInputAlphabet, map<InputType, vector<int> >& reducedInputAlphabetMap, set<InputType>& inputAlphabet, vector<map<InputType, pair<int, IncSpecSeq> > >& states) {
	map<int, vector<vector<int> > > hashmap;

	for (typename set<InputType>::iterator inputIt=inputAlphabet.begin(); inputIt != inputAlphabet.end(); inputIt++) {
		InputType input = *inputIt;

		int hash = 0;
		vector<int> nextStatesList;

		for (unsigned int i=0; i<states.size(); i++) {
			map<InputType, pair<int, IncSpecSeq> >& transitions = states[i];
			typename map<InputType, pair<int, IncSpecSeq> >::iterator succ = transitions.find(input);

			if (succ==transitions.end()) {
				nextStatesList.push_back(-1);
				hash = 31*hash;
			} else {
				int succState = (succ->second).first;
				nextStatesList.push_back(succState);
				hash = 31*hash + succState;
			}
		}

		map<int, vector<vector<int> > >::iterator hashmapEntry = hashmap.find(hash);

		if (hashmapEntry==hashmap.end()) {
			hashmap[hash].push_back(nextStatesList);
			reducedInputAlphabet.insert(input);
			reducedInputAlphabetMap[input]=nextStatesList;
		} else {
			vector<vector<int> >& statesListsWithSameHash = hashmapEntry->second;

			bool contained = false;

			for (vector<vector<int> >::iterator it=statesListsWithSameHash.begin(); it!=statesListsWithSameHash.end(); it++) {
				vector<int>& curStateList = *it;

				bool equal = true;
				for (unsigned int i=0; i<states.size(); i++) {
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
				reducedInputAlphabetMap[input]=nextStatesList;
			}
		}
	}
}

void buildCNF(Solver* solver, vector<pair<int, int> >& literalToStateClass, unsigned int numClasses, vector<vector<int> >& machineNextState, vector<bool>& incompMatrix, vector<int>& pairwiseIncStates, int maxInput);

#endif /* DIMACSWRITER_H_ */
