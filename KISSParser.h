/*
 * KISSParser.h
 *
 *  Created on: Feb 28, 2015
 *      Author: Andreas Abel
 */

#ifndef KISSPARSER_H_
#define KISSPARSER_H_

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <utility>

#include "IncSpecSeq.h"

void parseKISSFile(std::string filename, std::vector<std::vector<std::pair<IncSpecSeq*,std::pair<int, IncSpecSeq*> > > >& states, int& resetState, int& numInputBits, int& numOutputBits);

#endif /* KISSPARSER_H_ */
