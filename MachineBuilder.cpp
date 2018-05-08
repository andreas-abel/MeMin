/*
 * MachineBuilder.cpp
 *
 *  Created on: 06.03.2015
 *      Author: Andreas Abel
 */
#include "MachineBuilder.h"
#include <sys/time.h>


void buildMachine(vector<vector<int> >& newMachineNextState, vector<vector<IncSpecSeq*> >& newMachineOutput, int& newResetState, int nClasses, vector<int>& dimacsOutput, vector<pair<int, int> >& literalToStateClass,  vector<vector<int> >& origMachineNextState, vector<vector<IncSpecSeq*> >& origMachineOutput, int origResetState, int maxInput) {
	vector<vector<int> > newStates(nClasses);
	vector<vector<bool> > classesForOrigState(origMachineNextState.size());
	for (unsigned int i=0; i<origMachineNextState.size(); i++) classesForOrigState[i].resize(nClasses);

	newMachineNextState.resize(nClasses);
	newMachineOutput.resize(nClasses);

	for (unsigned int litI=0; litI<dimacsOutput.size(); litI++) {
		int lit = dimacsOutput[litI];
		if (lit<=0) continue;

		pair<int,int>& stateClass = literalToStateClass[lit];
		if (stateClass.first==-1) continue;

		newStates[stateClass.second].push_back(stateClass.first);

		classesForOrigState[stateClass.first][stateClass.second]=true;

		if (stateClass.first==origResetState) {
			newResetState = stateClass.second;
		}
	}

	for (int stateI=0; stateI<nClasses; stateI++) {
		vector<int>& state = newStates[stateI];

		newMachineNextState[stateI].resize(maxInput+1);
		newMachineOutput[stateI].resize(maxInput+1);

		for (int a=0; a<=maxInput; a++) {
			newMachineNextState[stateI][a]=-1;
			vector<int> succ;
			IncSpecSeq* output = NULL;

			for (vector<int>::iterator stateIt=state.begin(); stateIt != state.end(); stateIt++) {
				int oldState = *stateIt;

				int nextState = origMachineNextState[oldState][a];
				if (nextState==-1) continue;


				if (succ.empty()) {
					output = origMachineOutput[oldState][a];
				} else {
					IncSpecSeq* origOutput = origMachineOutput[oldState][a];
					if (!output->equals(*origOutput)) {
						output = new IncSpecSeq(output->intersect(*origOutput));
					}
				}

				succ.push_back(nextState);
			}

			if (succ.empty()) continue;

			//find class that contains all successors
			int firstSucc=succ[0];
			vector<bool>& classesForFirstSucc = classesForOrigState[firstSucc];

			for (int succClass=0; succClass<nClasses; succClass++) {
				if (!classesForFirstSucc[succClass]) continue;

				bool classForAllStates = true;
				for (unsigned int succI=1; succI<succ.size(); succI++) {
					int otherSucc = succ[succI];

					if (!classesForOrigState[otherSucc][succClass]) {
						classForAllStates=false;
						break;
					}
				}

				if (classForAllStates) {
					newMachineNextState[stateI][a] = succClass;
					newMachineOutput[stateI][a] = output;
					break;
				}

				if (succClass==nClasses-1) {
					std::cerr << "No successor class found\n";
					exit(1);
				}
			}
		}
	}
}

void writeKISSFile(vector<vector<int> >& machineNextState, vector<vector<IncSpecSeq*> >& machineOutput, int resetState, int inputLength, int outputLength, vector<IncSpecSeq>& inputIDToIncSpecSeq, string filename) {
	vector<char*> stateToStr;
	stateToStr.resize(machineNextState.size());
	for (unsigned int state=0; state<machineNextState.size(); state++) {
		char* stateStr = new char[10];
		sprintf(stateStr, "S%d ", state);
		stateToStr[state] = stateStr;
	}

	vector<char*> lines;
	int bufferSize = inputLength+outputLength+25;

	for (unsigned int state=0; state<machineNextState.size(); state++) {
		vector<int>& curNextState = machineNextState[state];
		vector<IncSpecSeq*>& curOutput = machineOutput[state];

		for (unsigned int a=0; a<inputIDToIncSpecSeq.size(); a++) {
			if (curNextState[a]==-1) continue;

			char* buffer = new char[bufferSize];
			int bi=0;

			int ti=0;
			char* inputStr = inputIDToIncSpecSeq[a].toCString();
			while (inputStr[ti]!='\0') {
				buffer[bi] = inputStr[ti];
				bi++;
				ti++;
			}

			buffer[bi++] = ' ';
			ti=0;
			char* startStr = stateToStr[state];
			while (startStr[ti]!='\0') {
				buffer[bi] = startStr[ti];
				bi++;
				ti++;
			}

			ti=0;
			char* endStr = stateToStr[curNextState[a]];
			while (endStr[ti]!='\0') {
				buffer[bi] = endStr[ti];
				bi++;
				ti++;
			}

			ti=0;
			char* outputStr = curOutput[a]->toCString();
			while (outputStr[ti]!='\0') {
				buffer[bi] = outputStr[ti];
				bi++;
				ti++;
			}

			buffer[bi++] = '\0';
			lines.push_back(buffer);
		}
	}

	std::ofstream file(filename.c_str());
	if (!file.is_open()) {
		cout << "Unable to open file\n";
		return;
	}

	file << ".i " << inputLength << endl;
	file << ".o " << outputLength << endl;
	file << ".p " << lines.size() << endl;
	file << ".s " << machineNextState.size() << endl;
	if (resetState>-1) file << ".r S" << resetState << endl;

	for (vector<char*>::iterator it=lines.begin(); it != lines.end(); it++) {
		file << (*it) << endl;
	}

	file.close();
}

