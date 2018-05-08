/*
 * Output.h
 *
 *  Created on: Feb 28, 2015
 *      Author: Andreas Abel
 */

#ifndef INCSPECSEQ_H_
#define INCSPECSEQ_H_

#include <iostream>
#include <vector>
#include <set>

using std::string;
using std::vector;
using std::set;


class IncSpecSeq {
public:
	//the i-th bit is 1 iff the i-th element is specified and 1, otherwise the bit is 0
	vector<unsigned int> seq;

	//the i-th bit is 0 iff the i-th element is not specified
	vector<unsigned int> specifiedBits;

	//in bits
	unsigned int seqLength;

	//the first seqLength%(sizeof(int)*8) bits are 1
	unsigned int lastMask;

	bool fullySpecified;

	IncSpecSeq() {
		seqLength = 0;
		lastMask = 0;
		fullySpecified = true;
	}

	IncSpecSeq(string& s) {
		seqLength=s.length();
		unsigned int vecLength = (seqLength-1)/(sizeof(int)*8) +1;

		seq.clear();
		seq.resize(vecLength, 0);
		specifiedBits.clear();
		specifiedBits.resize(vecLength, 0);

		fullySpecified = true;

		for (unsigned int i=0; i<s.length(); i++) {
			unsigned int& curOutputInt = seq[i/(sizeof(int)*8)];
			unsigned int& curSpecBitsInt = specifiedBits[i/(sizeof(int)*8)];
			int curPos = i%(sizeof(int)*8);

			char c = s[i];
			if (c=='0' || c=='1') {
				curSpecBitsInt |= (1<<curPos);
				if (c=='1') {
					curOutputInt |= (1<<curPos);
				}
			} else {
				fullySpecified = false;
			}
			if (i/(sizeof(int)*8) == vecLength-1) {
				lastMask = (lastMask<<1)|1;
			}
		}
	}

	//bits that are specified in both outputs must be the same
	bool isCompatible(const IncSpecSeq& other) const {
		for (unsigned int i=0; i<specifiedBits.size(); i++) {
			unsigned int mask = specifiedBits[i] & other.specifiedBits[i];
			unsigned int spec1 = (seq[i] & mask);
			unsigned int spec2 = (other.seq[i] & mask);
			if (spec1!=spec2) return false;
		}
		return true;
	}

	//bits are specified if they are specified in at least one of the sequences
	//assumes that sequences have the same length and that they are compatible
	IncSpecSeq intersect(const IncSpecSeq& other) const {
		IncSpecSeq ret;

		ret.seqLength = seqLength;
		ret.lastMask = lastMask;
		ret.fullySpecified = fullySpecified || other.fullySpecified;

		ret.seq.clear();
		ret.seq.resize(seq.size(), 0);
		ret.specifiedBits.clear();
		ret.specifiedBits.resize(specifiedBits.size(), 0);

		for (unsigned int i=0; i<seq.size(); i++) {
			ret.specifiedBits[i] = specifiedBits[i] | other.specifiedBits[i];
			ret.seq[i] = (seq[i]&specifiedBits[i]) | (other.seq[i] & other.specifiedBits[i]);
		}

		return ret;
	}

	//two sequences are disjoint if their intersection is empty
	bool isDisjoint(const IncSpecSeq& other) const {
		for (unsigned int i=0; i<seq.size(); i++) {
			unsigned int mask = specifiedBits[i] & other.specifiedBits[i];
			if ((seq[i]&mask) != (other.seq[i]&mask)) return true;
		}
		return false;
	}

	//all words that are in this sequence, but not in the other
	//assumes that there is at least one such word
	vector<IncSpecSeq> diff(const IncSpecSeq& other) const {
		vector<IncSpecSeq> ret;

		IncSpecSeq last = *this;
		for (unsigned int i=0; i<seq.size(); i++) {
			for (unsigned int b=0; b<(sizeof(int)*8); b++) {
				if (i*(sizeof(int)*8)+b>=seqLength) break;

				if (!((specifiedBits[i]>>b)&1) && ((other.specifiedBits[i]>>b)&1)) {
					IncSpecSeq newSeq=last;
					unsigned int otherBitNew = ((~other.seq[i])>>b)&1;
					unsigned int otherBitLast = (other.seq[i]>>b)&1;

					newSeq.specifiedBits[i] |= (1<<b);
					last.specifiedBits[i] |= (1<<b);

					newSeq.seq[i] |= (otherBitNew<<b);
					last.seq[i] |= (otherBitLast<<b);

					ret.push_back(newSeq);
				}
			}
		}

		return ret;
	}

	//if a bit of this seq is unspecified, the corresponding bit of the other seq must also be unspecified
	//all specified bits must be equal
	bool isSubset(const IncSpecSeq& other) const {
		for (unsigned int i=0; i<seq.size(); i++) {
			if ((other.specifiedBits[i]&specifiedBits[i])!=other.specifiedBits[i]) {
				return false;
			}
		}

		return isCompatible(other);
	}

	bool isFullySpecified() const {
		return fullySpecified;
	}

	bool operator<(const IncSpecSeq& other) const {
		if (seq<other.seq) return true;
		if (seq>other.seq) return false;
		return specifiedBits<other.specifiedBits;
	}

	bool equals(const IncSpecSeq& other) const {
		if (seq!=other.seq) return false;
		if (specifiedBits!=other.specifiedBits) return false;
		return true;
	}

	bool operator==(const IncSpecSeq& other) const {
		if (seq!=other.seq) return false;
		if (specifiedBits!=other.specifiedBits) return false;
		return true;
	}

	string toString() const {
		string s;
		s.reserve(seqLength);
		for (unsigned int i=0; i<seqLength; i++) {
			int curPos = i%(sizeof(int)*8);
			if (((specifiedBits[i/(sizeof(int)*8)]>>curPos)&1)==0) {
				s.push_back('-');
			} else {
				s.push_back(((seq[i/(sizeof(int)*8)]>>curPos)&1)+'0');
			}
		}
		return s;
	}

	char* CString = NULL;
	char* toCString() {
		if (CString != NULL) {
			return CString;
		}
		CString = new char[seqLength+1];
		CString[seqLength] = '\0';
		for (unsigned int i=0; i<seqLength; i++) {
			int curPos = i%(sizeof(int)*8);
			if (((specifiedBits[i/(sizeof(int)*8)]>>curPos)&1)==0) {
				CString[i] = '-';
			} else {
				CString[i] = (((seq[i/(sizeof(int)*8)]>>curPos)&1)+'0');
			}
		}
		return CString;
	}

	friend std::ostream& operator<< (std::ostream &out, IncSpecSeq& output);

};

namespace std {
template<> struct hash<IncSpecSeq> {
	std::size_t operator()(const IncSpecSeq& k) const {
		std::size_t seed = 0;
		for (auto& i : k.seq) {
			seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		return seed;
	}
};
}

#endif /* INCSPECSEQ_H_ */
