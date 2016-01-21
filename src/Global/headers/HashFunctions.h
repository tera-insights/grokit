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
#ifndef _HASH_FUNCTIONS_H_
#define _HASH_FUNCTIONS_H_

/** This header file contains various hash functions.

    NOTE: plese keep this file clean and add only hash functions not
    hash table stuff

*/
#ifdef __APPLE__
#include <sys/types.h>
#else
#include <bits/types.h>
#endif

/////////////////// Alin's congruent hash ///////////////////////
// if changing these constants, leave 0x0000 at the beginning
// if the constants are too large, they can produce overflows
// H_a has to be at most 56 bits. so we can avoid costly mod
#define H_a 0x0000b2334cff35abUL
// The H_a value used by drand48 and java.util.Random
//#define H_a 0x00000005DEECE66DUL
// H_a used by MMIX
//#define H_a 6364136223846793005UL

// H_b has to be at most 60 bits
// We select both to be 48bit to bt on the safe side
#define H_b 0x000034faccba783fUL
// the 2^61-1 constant
#define H_p 0x1fffffffffffffffUL

#ifndef UINT_MAX
#define UINT_MAX 0xffffffffffffffffUL
#endif

// the initial value for the  extra hash argument for chaining
#define HASH_INIT H_b



/** hash function.

	* Can be called like CongruentHash(x) or like
	* CongruentHash(x, chain)
	* the second form is used for hashing more than a long by
	* chaining calls of the hash

	Universal Hash = (ax+b)mod p
	Computing ax+b is straight forward. Computing mod needs to be well thought efficient operation.
	We select our constants a and b in such a way that we need less mod operations (only one). And
	of course we do not compute mod using division. There are several papers which talks about
	doing mod in a very simple way (just adding hi and lo bits). One such paper is by David G Carta.
	Though it talks about PRNG (pseudo number generator with full cycle period), but we can relate
	few things to our hashing problem too. hi+lo only works if we have mersenne prime.

	Value of p we choose is mersenne prime, 2^k - 1, and luckily 2^61-1 is the one we need.

	What David Carta say is, we can write ax+b = 2^61 hi + lo. And our modulo p would be
	hi + lo,               if (hi + lo < 2^61) OR
	hi + lo - 2^61 + 1     if (hi + lo >= 2^61) which essentially is another modulo to remove overflow

	SS: One website I would like to mention which consolidates several papers and comparisions b/w them
	    www.firstpr.com.au/dsp/rand31/#PRB69
			which explains the modulo nicely, here is explaination which is simpler than what David Carta explains.

			"Lets define some labels:

			lo = 61 bits 60 to 0.
			hi = rest of the high order bits

			multiplication product (ax + b) would be in the form:

				lo
			+ (hi * 0x2000000000000000)

			Observe that this is the same as:

			lo
		+ (hi * 0x1FFFFFFFFFFFFFFF)
		+ (hi * 0x0000000000000001)


			However, we don't need or want big result. We only want that result mod(0x1FFFFFFFFFFFFFFF). Therefore we can
			ignore the middle line and use for our intermediate result:

			lo
		+ hi

			This potentially 62 bit value still needs to be subjected to a mod(0x1FFFFFFFFFFFFFFF) operation to produce
			the final pseudo-random result.

			Because of the nature of the hi and lo bits, and the nature of the modulus constant, the modulus operation can
			be done with a simple bit 62 test and the only action which may need to be taken is to subtract a single
			instance of the modulus constant. It suffices to test bit 62, and if it is set, to zero it and increment the
			remaining bits, which are guaranteed not to overflow.  This is subtracting 0x2000000000000000 and adding 1."


	The constants a and b we have chosen makes sure that we do not exceed 122 bits in our multiplication
	Hence we choose 48 bits constants. The choice of constants do matter a lot, to acheive full cycle behavior
	if we talk about random number generation by using previous hash value. And we do have known
	constants for 2^31 - 1 case, but there is no research for Linear congruent generators for 64 bit case.
	Hence for now, we just choose any 48 bit H_a and H_b, since our problem is little different too. Just
	hope we don't find any collisions! Else, just find some good constants.

	Usually in research papers, they talk about minimal standard which means, constants which
	are small as possible are proven to show good behavior than the larger ones. Minimal standard
	would mean smallest possible constants which shows full cycle behavior when feed with just
	initial seed and then use previous results to compute next. For all possible initial seeds,
	it should be full cycle behavior to gurantee minimal standard.

	This hash shown to be 10 times faster than system modulo % operator, with below test

	int main(void){
  	Timer clock;
  	clock.Restart();
 	 	__uint64_t val=H_b;
 	 	for (int i = 0; i < 1000000000UL; i++)
 	  	 val = CongruentHash(i, val);
 	 	double time = clock.GetTime();
		cout << time;

  What we are doing below is writing y = hi*2^61 + lo since then
  y mod p = (hi + lo) mod p which is at most 1 p over

**/

/*
inline __uint64_t CongruentHash(const __uint64_t x, const __uint64_t b = H_b){
	__uint128_t y = (__uint128_t) x * H_a + b;
	__uint64_t yLo = y & H_p; // the lower 61 bits
	__uint64_t yHi= (y>>61) ; // no need to mask to get next 61 bits due to low constant H_a
	__uint64_t rez=yLo+yHi;

	// Below statement essentially means: __uint64_t rez2 = (rez<H_p ? rez : rez-H_p);
	// But only problem below is, we do not get zero result. May be we could say, it's
	// not the problem, but the solution. This nifty trick solves the branching statements,
	// which may or may not shown to be problematic
	__uint64_t rez2 = (rez & H_p) + (rez >> 61);
	return rez2;
}
*/

// Modified MurmurHash2
inline __uint64_t CongruentHash ( __uint64_t data, __uint64_t seed = H_b )
{
    const __uint64_t m = 0xc6a4a7935bd1e995;
    const int r = 47;

    __uint64_t h = seed ^ (sizeof(__uint64_t) * m);

    data *= m;
    data ^= data >> r;
    data *= m;

    h ^= data;
    h *= m;

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

/** All bytes after the first byte that is 0 are turned to 0
*/

/** funtion to hash a \0 terminated string */

// MurmurHash2, 64-bit versions, by Austin Appleby
inline __uint64_t HashString ( const void * key, int len, __uin+t64_t seed = H_b )
{
	const __uint64_t m = 0xc6a4a7935bd1e995;
	const int r = 47;

	__uint64_t h = seed ^ (len * m);

	const __uint64_t * data = (const __uint64_t *)key;
	const __uint64_t * end = data + (len/8);

	while(data != end)
	{
		__uint64_t k = *data++;

		k *= m;
		k ^= k >> r;
		k *= m;

		h ^= k;
		h *= m;
	}

	const unsigned char * data2 = (const unsigned char*)data;

	switch(len & 7)
	{
	case 7: h ^= __uint64_t(data2[6]) << 48;
	case 6: h ^= __uint64_t(data2[5]) << 40;
	case 5: h ^= __uint64_t(data2[4]) << 32;
	case 4: h ^= __uint64_t(data2[3]) << 24;
	case 3: h ^= __uint64_t(data2[2]) << 16;
	case 2: h ^= __uint64_t(data2[1]) << 8;
	case 1: h ^= __uint64_t(data2[0]);
	        h *= m;
	};

	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
}

// Global hash function for user-defined types before template specialization.
template <typename T>
inline uint64_t Hash(const T& data) {
  return HashString(&data, sizeof(T))
}

/*
inline __uint64_t HashString(const char* str, __uint64_t b = H_b){

	__uint64_t hash = b;

	// if there is a \0 inside need to pad with \0 and end loop
	__uint64_t* p = (__uint64_t*)str;

	while (true) {

		__uint64_t v = *p;

		// SS. Found somewhere on web, fastest known way to know 0 byte in a word, it sets
		//     all bits zeros in the output except those which actually is zero in the input.
		//     For 0 byte in a word, it sets one bit high in that byte in the output.
		//     For eg, if input = abc\0de\0fgh, output = 0x0001001000.
		//     When char is represented in int form, some big-indian, little-indian issues
		//     come up, hence 01 may mean 80 in the output, hence the comparisions below
		__uint64_t hasZeroByte = (v - 0x0101010101010101UL) & ~(v) & 0x8080808080808080UL;

		// If 64 bit word has some 0 byte, turn all trailing bits zeros starting the MSB
		// Find some faster way, avoid else branch etc
		if (hasZeroByte) {
      //if (hasZeroByte &  0x8000000000000000UL)
      //        v &= 0xffffffffffffffffUL;
      if (hasZeroByte &  0x0080000000000000UL)
              v &= 0x00ffffffffffffffUL;
      if (hasZeroByte &  0x0000800000000000UL)
              v &= 0x0000ffffffffffffUL;
      if (hasZeroByte &  0x0000008000000000UL)
              v &= 0x000000ffffffffffUL;
      if (hasZeroByte &  0x0000000080000000UL)
              v &= 0x00000000ffffffffUL;
      if (hasZeroByte &  0x0000000000800000UL)
              v &= 0x0000000000ffffffUL;
      if (hasZeroByte &  0x0000000000008000UL)
              v &= 0x000000000000ffffUL;
      if (hasZeroByte &  0x0000000000000080UL)
              v &= 0x00000000000000ffUL;
		}

		hash = CongruentHash(v, hash);

		++p;

		if (hasZeroByte)
			break;
	}

	return hash;
}
*/

#endif // _HASH_FUNCTIONS_H_
