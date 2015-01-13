//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
#ifndef _BITSTRING128_H
#define _BITSTRING128_H

/**
	This header specifies the implementation of the Bitstring128

	The code is written in terms of types bitmapword, signedbitmapword
	and the representaion of 1 in the desired format, BITSTRING_ONE.
	These are defined in Bitsring.h

	The implementation of Bistring128 uses two words to keep the bits.
*/


class Bitstring128
{
private:
	bitmapword	w; // hi bits
	bitmapword	v; // low bits
	// the whole representation is [wv]

public:
	/* Default constructor */
	Bitstring128 ();

	/* Constructor from int*/
	/* x is an index from 0 to MaxSize () - 1*/
	Bitstring128 (unsigned int x);

	/* x is an index from 0 to MaxSize () - 1,
		the return value tells whether the operation is successful */
	bool AddMember (unsigned int x); 	/* set bit x */
	bool DeleteMember (unsigned int x);	/* reset bit x */
	bool IsMember (unsigned int x);

	/* test functions */
	bool IsSubsetOf (const Bitstring128 &b);
	bool Overlaps (const Bitstring128 &b); // do b and ourselves intersect?
	bool IsEqual (const Bitstring128 &b);
	bool operator == (const Bitstring128 &b) const;
	bool IsEmpty ();

	/* Set operations */
	void Union (const Bitstring128 &b);
	void Intersect (const Bitstring128 &b);
	void Difference (const Bitstring128 &b);

	/* manipulation function */
	void Empty ();
	Bitstring128 Clone ();
	void copy (Bitstring128 &b);
	void swap (Bitstring128 &b);

	/* Utility functions */
	long Size ();	/* current size of set */
	static long MaxSize (); /* max capcity of this set */
	void Print (); /* print set as query name or binary */
	void PrintBinary(); // print set in binary

	/* Auxiliary functions to use Bitstirngs in a map */
	bool operator<(const Bitstring128& other) const;
	bool LessThan(const Bitstring128& other) const;

	/*----------
		* GetFirst - find and remove first member of a set.
		* This is intended as support for iterating through the members of a set.
		* Returns a singleton bitstring with the removed member.
		* Returns 0 if set is empty.
		* NB: set is destructively modified!
		*
		* The typical pattern is
		*		Bitstring128 input;
		*		Bitstring128 tmp = input.Clone ();
		*		while (!tmp.IsEmpty())
		*			Bitstring128 x = tmp.GetFirst ();
		*			process member x;
		*----------
		*/
	Bitstring128 GetFirst ();

	/** find a 0 in the bitstring and return a Bitstring128 with the 1 in
			that position. If one cannot be found, return empty bitstring */
	Bitstring128 GetNew();

	/* GetInt - returns integer representation of the bitmapword.
		The return value is meaningless for anything but a singleton set */
	unsigned int GetInt ();
};

/* inline functions */

inline
Bitstring128 Bitstring128::GetNew(){
	Bitstring128 rez;
	rez.v=RIGHTMOST_ONE(~v);
	if (rez.v==0)
		rez.w=RIGHTMOST_ONE(~w);
	// rez.w will be 0, hence the query empty if no position left
	return rez;
}

inline
bool Bitstring128::operator<(const Bitstring128& other) const{
	return (w<other.w || (w==other.w && v<other.v));
}


inline
bool Bitstring128::LessThan(const Bitstring128& other) const{
	return (w<other.w || (w==other.w && v<other.v));
}


inline
Bitstring128::Bitstring128 () {
	w = 0;
	v = 0;
}

inline
Bitstring128::Bitstring128 (unsigned int x) {
	w = 0; v=0;
	AddMember (x);
}

inline
bool Bitstring128::AddMember (unsigned int index) {
	if (index >= 2*BITS_PER_WORD)
		return false;
	unsigned int bitnum = BITNUM(index);
	if (bitnum < BITS_PER_WORD)
		v |= (BITSTRING_ONE << bitnum);
	else
		w |= (BITSTRING_ONE << (bitnum-BITS_PER_WORD));
	return true;
}

inline
bool Bitstring128::DeleteMember (unsigned int index) {
	w &= ~(BITSTRING_ONE << BITNUM(index));
	return true;
}

inline
bool Bitstring128::IsMember (unsigned int index) {
	if ((v & (BITSTRING_ONE << BITNUM(index)))!=0 ||
			(w & (BITSTRING_ONE << (BITNUM(index)-BITS_PER_WORD))) != 0)
		return true;
	return false;
}

inline
bool Bitstring128::IsSubsetOf (const Bitstring128 &b) {
	if ((v & ~(b.v)) != 0 || (w & ~(b.w)) != 0  )
		return false;
	return true;
}

inline
bool Bitstring128::Overlaps (const Bitstring128 &b) {
	if ((v & (b.v)) != 0 || (w & (b.w)) != 0)
		return true;
	return false;
}


inline
bool Bitstring128::IsEqual (const Bitstring128 &b) {
	if (w != b.w || v!=b.v)
		return false;
	return true;
}

inline
bool Bitstring128::operator == (const Bitstring128 &b) const{
	return (w == b.w && v == b.v);
}

inline
void Bitstring128::Union (const Bitstring128 &b) {
	w |= (b.w);
	v |= (b.v);
}

inline
void Bitstring128::Intersect (const Bitstring128 &b) {
	w &= (b.w);
	v &= (b.v);
}

inline
void Bitstring128::Difference (const Bitstring128 &b) {
	w &= ~b.w;
	v &= ~b.v;
}

inline
Bitstring128 Bitstring128::Clone () {
	Bitstring128 res;
	res.w = w;
	res.v = v;
	return res;
}

inline
void Bitstring128::copy (Bitstring128 &b) {
	w = b.w;
	v = b.v;
}

inline
void Bitstring128::swap (Bitstring128 &b) {
	SWAP (w, b.w);
	SWAP (v, b.v);
}

inline
bool Bitstring128::IsEmpty () {
	if ((w|v) == 0)
		return true;
	return false;
}

inline
long Bitstring128::Size () {
	int res = 0;
	bitmapword x = w;
	while (x != 0) {
		res += NumberOfOnes[x & 255UL];
		x >>= 8;
	}

	x = v;
	while (x != 0) {
		res += NumberOfOnes[x & 255UL];
		x >>= 8;
	}
	return res;
}

inline
long Bitstring128::MaxSize () {
	return 2*BITS_PER_WORD;
}

// the definition of Print() is moved to QueryManager.cc since it mostly uses that
// to print the name

inline
void Bitstring128::PrintBinary() {
	bitmapword mask = BITSTRING_ONE;
	mask = mask << (BITS_PER_WORD - 1);
	for (unsigned int i = 1; i <= BITS_PER_WORD; i++) {
		if ((w & mask) == 0)
			cout << 0;
		else
			cout << 1;
		mask = mask >> 1;
		if (i%8 == 0)
			cout << " ";
	}

	// now v
	mask = BITSTRING_ONE;
	mask = mask << (BITS_PER_WORD - 1);
	for (unsigned int i = 1; i <= BITS_PER_WORD; i++) {
		if ((v & mask) == 0)
			cout << 0;
		else
			cout << 1;
		mask = mask >> 1;
		if (i%8 == 0)
			cout << " ";
	}

	cout << endl;
}

inline
void Bitstring128::Empty(){
	w=0; v=0;
}

inline
Bitstring128 Bitstring128::GetFirst () {
	Bitstring128 rez;
	rez.v=RIGHTMOST_ONE (v);
	if (rez.v == 0){
		rez.w=RIGHTMOST_ONE (w);
		w=w&(~rez.w);
	} else {
		v=v&(~rez.v);
	}
	return rez;
}

inline
unsigned int Bitstring128::GetInt () {
	// TODO: What is this. Is this funcion even used?
	return BITNUM(w >> 1);
}

#endif
