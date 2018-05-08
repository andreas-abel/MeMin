#include <iostream>
#include <queue>
#include <set>
#include <list>
#include <sstream>
#include <string.h>

#include "KISSParser.h"
#include "IncSpecSeq.h"
#include "global.h"

using std::cout;
using std::endl;
using std::vector;
using std::map;
using std::set;
using std::list;
using std::queue;
using std::pair;
using std::make_pair;

bool firstStateReset=true;

string inputSeqToString(vector<IncSpecSeq> inputSequence) {
	std::stringstream s;
	s << "(";
	for (unsigned int i=0; i<inputSequence.size(); i++) {
		s << inputSequence[i];
		if (i<inputSequence.size()-1) {
			s << ", ";
		}
	}
	s << ")";
	return s.str();
}

//searches for an input that is either only applicable to machine2, or for which the output of the two machines is different
vector<IncSpecSeq> findDistinguishingInput(vector<vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > > > machine1, vector<vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > > > machine2, int resetState1, int resetState2) {
	map<pair<int,int>, vector<IncSpecSeq> > inputSequenceForStatePair;

	set<pair<int,int> > processedStatePairs;
	queue<pair<int,int> > worklist;

	processedStatePairs.insert(make_pair(resetState1, resetState2));
	worklist.push(make_pair(resetState1, resetState2));

	while (!worklist.empty()) {
		pair<int,int>& statePair = worklist.front();
		worklist.pop();

		vector<IncSpecSeq> inputSeq = inputSequenceForStatePair[statePair];

		vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >& trans1 = machine1[statePair.first];
		vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >& trans2 = machine2[statePair.second];

		for (vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >::iterator it2=trans2.begin(); it2 != trans2.end(); it2++) {
			list<IncSpecSeq> input2L;
			input2L.push_back(*(it2->first));

			for (vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > >::iterator it1=trans1.begin(); it1 != trans1.end(); it1++) {
				IncSpecSeq input1 = *(it1->first);

				for (list<IncSpecSeq>::iterator listIt=input2L.begin(); listIt!=input2L.end();) {
					IncSpecSeq input2 = *listIt;
					if (input2.isDisjoint(input1)) {
						listIt++;
						continue;
					}

					vector<IncSpecSeq> newInputSeq = inputSeq;
					newInputSeq.push_back(input1.intersect(input2));

					IncSpecSeq& o1 = *((it1->second).second);
					IncSpecSeq& o2 = *((it2->second).second);

					if (!o1.isSubset(o2)) {
						return newInputSeq;
//						cout << "False (different output), counterexample: " << inputSeqToString(newInputSeq) << ", expected output: "<<o2<<", actual output: "<<o1<< endl;
//						return 1;
					}

					int nextState1 = (it1->second).first;
					int nextState2 = (it2->second).first;

					pair<int,int> nextStatePair = make_pair(nextState1, nextState2);
					if (processedStatePairs.count(nextStatePair)==0) {
						processedStatePairs.insert(nextStatePair);
						worklist.push(nextStatePair);

						inputSequenceForStatePair[nextStatePair]=newInputSeq;
					}

					if (!input2.isSubset(input1)) {
						vector<IncSpecSeq> diff = input2.diff(input1);
						for (unsigned int i=0; i<diff.size(); i++) {
							input2L.push_back(diff[i]);
						}

					}
					listIt = input2L.erase(listIt);
				}
			}

			if (!input2L.empty()) {
				vector<IncSpecSeq> newSeq = inputSeq;
				newSeq.push_back(*(input2L.begin()));
				return newSeq;
//				cout << "False (missing transition), counterexample: " << inputSeqToString(newSeq) << endl;
//				return 1;
			}
		}
	}
	return vector<IncSpecSeq>();
}


void usage() {
	cout << "Usage: ./MealyCover [Options] <m1.kiss> <m2.kiss>" << endl;
	cout << "Checks whether m1 covers m2." << endl;
	cout << endl;
	cout << "Options:" << endl;
	cout << "  -r        if no reset state is specified, any state might be a reset state" << endl;
	cout <<	"            (otherwise, the first state is assumed to be the reset state)"<< endl;
}

int main(int argc, char *argv[]) {
	if (argc<3) {
		usage();
		return 1;
	}

	for (int argI=1; argI < argc-2; argI++) {
		char* arg = argv[argI];
		if (strcmp(arg,"-r")==0) {
			firstStateReset = false;
		} else {
			usage();
			return 1;
		}
	}


	int resetState1;
	int resetState2;
	vector<vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > > > machine1;
	vector<vector<pair<IncSpecSeq*, pair<int, IncSpecSeq*> > > > machine2;

	int numInputBits;
	int numOutputBits;
	parseKISSFile(argv[argc-2], machine1, resetState1, numInputBits, numOutputBits);
	parseKISSFile(argv[argc-1], machine2, resetState2, numInputBits, numOutputBits);

	if (machine1.size()==0 || machine2.size()==0) {
		std::cout << "False (invalid file)" << std::endl;
		return 1;
	}

	if ((resetState1==-1 || resetState2==-1) && resetState1!=resetState2) {
		std::cout << "False (reset state only specified in one machine)" << std::endl;
		return 1;
	}

	if (resetState1>-1) {
		vector<IncSpecSeq> counterExample = findDistinguishingInput(machine1, machine2, resetState1, resetState2);
		if (counterExample.empty()) {
			cout << "True" << endl;
			return 0;
		} else {
			cout << "False, counterexample: " << inputSeqToString(counterExample) << endl;
			return 1;
		}
	} else {
		for (unsigned int s2=0; s2<machine2.size(); s2++) {
			bool matchingStateFound = false;
			for (unsigned int s1=0; s1<machine1.size(); s1++) {
				vector<IncSpecSeq> counterExample = findDistinguishingInput(machine1, machine2, s1, s2);
				if (counterExample.empty()) {
					matchingStateFound = true;
					break;
				}
			}
			if (!matchingStateFound) {
				cout << "False, no matching reset state for state " << s2 << endl;
				return 1;
			}
		}
		cout << "True" << endl;
		return 0;
	}
}

