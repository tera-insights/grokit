#ifndef _HASH_FCT_GUS_H_
#define _HASH_FCT_GUS_H_

#include "SpookyV2_inlined.h"

inline __uint64_t CongruentHashModified( const __uint64_t value, const __uint64_t seed ) {
    SpookyHash hasher;

    return hasher.Hash64( (const void *) &value, sizeof( __uint64_t ), seed ); 
}

/*
/////////////////// Alin's congruent hash ///////////////////////
// if changing these constants, leave 0x0000 at the beginning
// if the constants are too large, they can produce overflows
// H_aMod has to be at most 56 bits. so we can avoid costly mod
#define H_aMod 0xa135b2334cff35abULL
// H_b has to be at most 60 bits
// We select both to be 48bit to bt on the safe side
#define H_bMod 0x970c34faccba783fULL
// the 2^61-1 constant
#define H_pMod 0x1fffffffffffffffULL

#ifndef UINT_MAX
#define UINT_MAX 0xffffffffffffffffULL
#endif

// the initial value for the  extra hash argument for chaining
#define HASH_INIT_MOD H_bMod



inline __uint64_t CongruentHashModified(const __uint64_t x, const __uint64_t b = H_bMod){
	__uint128_t y = (__uint128_t) x * H_aMod + b;
	__uint64_t yLo = y & H_pMod; // the lower 61 bits
	__uint64_t yHi= (y>>61) ; // no need to mask to get next 61 bits due to low constant H_aMod
	__uint64_t rez=yLo+yHi;

	// Below statement essentially means: __uint64_t rez2 = (rez<H_pMod ? rez : rez-H_pMod);
	// But only problem below is, we do not get zero result. May be we could say, it's
	// not the problem, but the solution. This nifty trick solves the branching statements,
	// which may or may not shown to be problematic
	__uint64_t rez2 = (rez & H_pMod) + (rez >> 61);
	return rez2;
}

*/

#endif // _HASH_FCT_GUS_H_
