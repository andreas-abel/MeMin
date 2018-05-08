/*
 * KISSParser.cpp
 *
 *  Created on: Feb 28, 2015
 *      Author: Andreas Abel
 */
#include "KISSParser.h"
#include <cstdlib>
#include <fstream>
#include <vector>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <unordered_map>

#include "global.h"

using std::string;
using std::cout;
using std::vector;
using std::map;
using std::unordered_map;
using std::set;
using std::queue;
using std::pair;
using std::make_pair;

void parseKISSFile(string filename, vector<vector<pair<IncSpecSeq*,pair<int, IncSpecSeq*> > > >& states, int& resetState, int& numInputBits, int& numOutputBits) {
	resetState=-1;
	std::ifstream myfile(filename.c_str());

	unordered_map<string, int> stateToInt;
	unordered_map<string, IncSpecSeq*> stringToIncSpecSeq;

	if (myfile.is_open()) {
		string line;

		vector<pair<IncSpecSeq*,pair<int, IncSpecSeq*> > > undefinedFromStates;

		while (getline(myfile, line)) {
			if (line[0]=='.') {
				if (line[1]=='r') {
					string resetStateStr = line.substr(3, line.find_last_not_of(" \t\f\v\n\r")-2);

					if (stateToInt.count(resetStateStr)==0) {
						stateToInt[resetStateStr] = states.size();
						states.resize(states.size()+1);
					}
					resetState = stateToInt[resetStateStr];
				} else if (line[1]=='i') {
					string inputStr = line.substr(3, line.find_last_not_of(" \t\f\v\n\r")-2);
					std::istringstream(inputStr) >> numInputBits;
				} else if (line[1]=='o') {
					string outputStr = line.substr(3, line.find_last_not_of(" \t\f\v\n\r")-2);
					std::istringstream(outputStr) >> numOutputBits;
				}
			} else if (line[0]=='0' || line[0]=='1' || line[0]=='-') {
				unsigned int startIndex = 0;
				unsigned int nextSplit = line.find_first_not_of("01-");
				string inputStr = line.substr(startIndex, nextSplit);

				startIndex = line.find_first_not_of(" \t\f\v\n\r", nextSplit);
				nextSplit = line.find_first_of(" \t\f\v\n\r", startIndex);
				string fromState = line.substr(startIndex, nextSplit-startIndex);

				startIndex = line.find_first_not_of(" \t\f\v\n\r", nextSplit);
				nextSplit = line.find_first_of(" \t\f\v\n\r", startIndex);
				string toState = line.substr(startIndex, nextSplit-startIndex);

				startIndex = line.find_first_not_of(" \t\f\v\n\r", nextSplit);
				nextSplit = line.find_first_not_of("01-", startIndex);
				if (nextSplit==string::npos) nextSplit = line.length();
				string outputStr = line.substr(startIndex, nextSplit-startIndex);

				if (stringToIncSpecSeq.count(inputStr)==0) {
					stringToIncSpecSeq[inputStr] = new IncSpecSeq(inputStr);
				}

				if (stringToIncSpecSeq.count(outputStr)==0) {
					stringToIncSpecSeq[outputStr] = new IncSpecSeq(outputStr);
				}

				IncSpecSeq* input = stringToIncSpecSeq[inputStr];
				IncSpecSeq* output = stringToIncSpecSeq[outputStr];

				if (fromState!="*" && stateToInt.count(fromState)==0) {
					stateToInt[fromState] = states.size();
					states.resize(states.size()+1);
					if (resetState==-1 && firstStateReset) resetState=stateToInt[fromState];
				}

				if (stateToInt.count(toState)==0) {
					stateToInt[toState] = states.size();
					states.resize(states.size()+1);
					if (resetState==-1 && firstStateReset && toState!="*") resetState=stateToInt[toState];
				}


				if (fromState=="*") {
					undefinedFromStates.push_back(make_pair(input, make_pair(stateToInt[toState], output)));
					continue;
				}

				int fromStateI = stateToInt[fromState];
				int toStateI = stateToInt[toState];

				vector<pair<IncSpecSeq*,pair<int, IncSpecSeq*> > >& curMap = states[fromStateI];
				curMap.push_back(make_pair(input, make_pair(toStateI, output)));
			}
		}

		for (vector<pair<IncSpecSeq*,pair<int, IncSpecSeq*> > >::iterator it=undefinedFromStates.begin(); it!=undefinedFromStates.end(); it++) {
			pair<IncSpecSeq*,pair<int, IncSpecSeq*> >& p = *it;
			IncSpecSeq* input = p.first;
			for (unsigned int s=0; s<states.size(); s++) {
				states[s].push_back(make_pair(input, p.second));
			}
		}

		myfile.close();
	} else {
		std::cerr << "Unable to open file "<<filename << std::endl;
		exit(1);
	}
}
