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
// for Emacs -*- c++ -*-

#ifndef _PRIORITY_LIST_H_
#define _PRIORITY_LIST_H_

#include "EfficientMap.h"
#include "Tokens.h"
#include "TwoWayList.h"

// Priority starts from 1

template <class T>
class PriorityList {
public:
	// type for functions to decide what is the next token to be processed
	// map from priority to list of tokens
	typedef Keyify<int> CInt;
	EfficientMap<CInt, TwoWayList<T> > priQueue;

	typedef int (*DecisionFunction)(EfficientMap<CInt, TwoWayList<T> >& pQ);

	DecisionFunction decFct; // user provided decision function

	// the default Decision Function
	static int defaultDecisionFunction(EfficientMap<CInt, TwoWayList<T> >& pQ);

	int size;

public:

	PriorityList ();
	virtual ~PriorityList ();

	// Add a token to the queue
	void Insert(T& Payload, int priority = 2); // default priority for everyone is 2

	// the internal payload is placed in Payload
	// the messages of the same type are returned in order
	// messages of different types are retreived in the increasing priority
	// order (default behavior) or the order controlled by the user specified function
	// return true if we found token
	bool Remove (T& Payload);

	// Function to register another decision policy for what message should be
	// extracted from the message queue
	void RegisterDecisionPolicy(DecisionFunction fct);

	// return the number of pending members
	int Length() {return size;}

	// If higher priority guys are waiting, return true
	bool HigherPriorityWaiting (int priority);
};

#endif
