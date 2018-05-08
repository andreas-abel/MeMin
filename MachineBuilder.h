/*
 * MachineBuilder.h
 *
 *  Created on: 06.03.2015
 *      Author: Andreas Abel
 */

#ifndef MACHINEBUILDER_H_
#define MACHINEBUILDER_H_

#include <stdlib.h>
#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

#include "IncSpecSeq.h"

using std::cout;
using std::endl;
using std::vector;
using std::map;
using std::set;
using std::pair;
using std::stringstream;

void buildMachine(vector<vector<int> >& newMachineNextState, vector<vector<IncSpecSeq*> >& newMachineOutput, int& newResetState, int nClasses, vector<int>& dimacsOutput, vector<pair<int, int> >& literalToStateClass,  vector<vector<int> >& origMachineNextState, vector<vector<IncSpecSeq*> >& origMachineOutput, int origResetState, int maxInput);


string inputToBinary(int input, int inputLength);

void writeKISSFile(vector<vector<int> >& machineNextState, vector<vector<IncSpecSeq*> >& machineOutput, int resetState, int inputLength, int outputLength, vector<IncSpecSeq>& inputIDToIncSpecSeq, string filename);


#endif /* MACHINEBUILDER_H_ */
