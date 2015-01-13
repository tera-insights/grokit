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
#ifndef BITMAP_H
#define BITMAP_H

#include <cstring>
#include <stdlib.h>
#include <pthread.h>

#include "Bitstring.h"
#include "Swap.h"


class Bitmap {

#include "BitmapPrivate.h"

    public:
        // This allows you to access the bits in the query membership bitmap
        // for reading and/or writing.  numSlots returns the number of slots
        // or rows in the bitmap.  After the call, the bits are still "owned"
        // by the bitmap in the sense that the bitmap will be responsible for
        // killing the memory used to store the bits...
        Bitstring *GetBits (int &_numSlots);
        void SetBits (int _numSlots, Bitstring *_bits);

        // This creates a bitmap with the specified number of slots or rows
        Bitmap (int numSlots);
        Bitmap (int numSlots, char ); // constructor with all bits set
        Bitmap (int numSlots, Bitstring &_mask);

        // This creates an empty bitmap
        Bitmap ();

        // This swaps two bitmaps
        void swap (Bitmap &withMe);

        // This kills a bitmap
        virtual ~Bitmap ();

        // What is the size of this bitmap?
        int GetSize(void);

        // This resizes a bitmap so that it is numSlots in length.  If the number
        // of slots is reduced, then the values at the end of the bitmap are cut
        // out of it.  If the number of slots is increased, then the bitmap is
        // padded with a bunch of zero values.
        void Resize (int numSlots);

        // This copies the bitmap; the copy is a deep one (we never do a shallow
        // copy of a bitmap, since it is assumed that the bitmap will always be
        // written to)
        void copy (Bitmap &copyMe);

        //operations between each Bitstring in the bitmap and Bitstring(S) given as parameter(s)
        void ORAll(Bitstring &_mask);
        void OROne(int index, Bitstring &_mask);
        void Clear(int index);
        bool AnySet();

        // return -1 if we cannot find any
        int FindFirstSet(int _start);

        // print the contents
        void Print ();
};

///////////////////////
// INLINE FUNCTIONS

inline Bitmap :: Bitmap () {
    numSlots = 0;
    bits = NULL;
    refCount = new int;
    *refCount = 1;
    mutex = new pthread_mutex_t;
    pthread_mutex_init(mutex, NULL);
}

inline Bitmap :: Bitmap (int len, char c) {
    numSlots = len;
    bits = (Bitstring *) malloc (sizeof (Bitstring) * numSlots);

    // big hack to put all bits to 1
    memset((char*) bits, 0xff, sizeof (Bitstring) * numSlots);

    refCount = new int;
    *refCount = 1;
    mutex = new pthread_mutex_t;
    pthread_mutex_init (mutex, NULL);

}

inline Bitmap :: Bitmap (int len) {
    numSlots = len;
    bits = (Bitstring *) malloc (sizeof (Bitstring) * numSlots);

    for (int i = 0; i < numSlots; i++) {
        bits[i].Empty();
    }

    refCount = new int;
    *refCount = 1;
    mutex = new pthread_mutex_t;
    pthread_mutex_init (mutex, NULL);
}

inline Bitmap :: Bitmap (int len, Bitstring &_mask) {
    numSlots = len;
    bits = (Bitstring *) malloc (sizeof (Bitstring) * numSlots);

    for (int i = 0; i < numSlots; i++) {
        bits[i]=_mask;
    }

    refCount = new int;
    *refCount = 1;
    mutex = new pthread_mutex_t;
    pthread_mutex_init (mutex, NULL);
}

inline Bitmap :: ~Bitmap () {
    pthread_mutex_lock (mutex);
    if (*refCount == 1) {
        free(bits);
        delete refCount;

        pthread_mutex_unlock (mutex);
        pthread_mutex_destroy(mutex);
        delete mutex;
    } else {
        (*refCount)--;
        pthread_mutex_unlock (mutex);
    }
}

inline void Bitmap::SetBits (int _numSlots, Bitstring *_bits) {
    pthread_mutex_lock (mutex);
    if (*refCount == 1) {
        free(bits);
        pthread_mutex_unlock (mutex);
    } else {
        (*refCount)--;
        pthread_mutex_unlock (mutex);
        refCount = new int;
        *refCount = 1;
        mutex = new pthread_mutex_t;
        pthread_mutex_init (mutex, NULL);
    }

    numSlots = _numSlots;
    bits = _bits;
}

inline void Bitmap :: swap (Bitmap &withMe) {
    char temp[sizeof (Bitmap)];
    memmove (temp, &withMe, sizeof (Bitmap));
    memmove (&withMe, this, sizeof (Bitmap));
    memmove (this, temp, sizeof (Bitmap));
}

inline int Bitmap:: GetSize(void) {
    return numSlots;
}

inline void Bitmap :: Resize (int len) {
    // make sure we own the bits
    bits = GetBits (numSlots);

    // get the new memory
    Bitstring *temp = (Bitstring *) malloc (sizeof (Bitstring) * len);

    // don't copy over more data than is there!
    if (len < numSlots)
        numSlots = len;

    // see if we need to copy the data over
    if (numSlots > 0) {
        memcpy (temp, bits, sizeof (Bitstring) * numSlots);
    }

    // and clean it up
    free(bits);
    bits = temp;
    numSlots = len;
}

inline void Bitmap :: copy (Bitmap &copyMe) {
    // kill the old data
    pthread_mutex_lock (mutex);
    if (*refCount == 1) {
        free(bits);
        delete refCount;

        pthread_mutex_unlock (mutex);
        pthread_mutex_destroy(mutex);
        delete mutex;
    } else {
        (*refCount)--;
        pthread_mutex_unlock (mutex);
    }

    // copy over the new data's ref counter and mutex
    refCount = copyMe.refCount;
    mutex = copyMe.mutex;

    // indcrement the ref count and copy the data
    pthread_mutex_lock (mutex);
    (*refCount)++;
    pthread_mutex_unlock (mutex);
    bits = copyMe.bits;
    numSlots = copyMe.numSlots;
}

inline Bitstring *Bitmap :: GetBits (int &len) {
    len = numSlots;

    // if the reference count exceeds one, then do a deep copy
    pthread_mutex_lock (mutex);
    if (*refCount > 1) {
        // make a copy of the bits
        Bitstring *bitsCopy =
            (Bitstring *) malloc (sizeof (Bitstring) * numSlots);
        memcpy (bitsCopy, bits, sizeof (Bitstring) * numSlots);
        bits = bitsCopy;

        // dec the ref counter
        (*refCount)--;
        pthread_mutex_unlock (mutex);

        // and create a new ref counter and mutex
        refCount = new int;
        *refCount = 1;
        mutex = new pthread_mutex_t;
        pthread_mutex_init (mutex, NULL);
    } else {
        pthread_mutex_unlock (mutex);
    }

    return bits;
}

inline void Bitmap::ORAll(Bitstring &_mask) {
    bits = GetBits (numSlots);
    for (int i = 0; i< numSlots; i++) {
        bits[i].Union(_mask);
    }
}

inline void Bitmap::OROne(int index, Bitstring &_mask) {
    bits = GetBits (numSlots);
    bits[index].Union(_mask);
}

inline void Bitmap::Clear(int index) {
    bits = GetBits (numSlots);
    bits[index].Empty();
}

inline bool Bitmap::AnySet() {
    Bitstring mark = bits[0];
    for (int i = 1; i < numSlots; i++) {
        mark.Union(bits[i]);
    }

    return !mark.IsEmpty();
}


inline int Bitmap::FindFirstSet(int _start) {
    for (int i = _start; i < numSlots; i++) {
        if (!bits[i].IsEmpty()) {
            return i;
        }
    }

    for (int i = 0; i < _start; i++) {
        if (!bits[i].IsEmpty()) {
            return i;
        }
    }

    return -1;
}



inline void Bitmap::Print() {
    for (int i = 0; i < numSlots; i++) {
        printf("Tuple %5d: ", i);

        //PRINT for Bitstring being a bitmap
        //string toPrint = bits[i].to_string<char, char_traits<char>, allocator<char> >();
        //printf("%s\n", toPrint.c_str());
    }
}

#endif
