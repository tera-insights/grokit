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
#include "PriorityList.h"
#include <assert.h>

template <class T>
int PriorityList<T>::defaultDecisionFunction (EfficientMap<CInt, TwoWayList<T> >& pQ) {

	// the default decision function just finds the highest priority message
	int minP = 0xffff;
	for (pQ.MoveToStart(); !(pQ).AtEnd(); (pQ).Advance()) {
		if (pQ.CurrentData().Length() && pQ.CurrentKey() < minP)
			minP = pQ.CurrentKey();
	}
	return minP;
}

template <class T>
PriorityList<T>::PriorityList() {
	decFct = defaultDecisionFunction;
	size = 0;
}

template <class T>
PriorityList<T>::~PriorityList() {
}

template <class T>
void PriorityList<T>::Insert (T& _Payload, int _priority) {

	assert(_priority); // should always be greater than zero, highest priority is 1

	CInt ci = _priority;

	if (priQueue.IsThere(ci)) {
		TwoWayList<T>& list = priQueue.Find(ci);
		list.MoveToFinish(); // append at the end
		list.Insert (_Payload);
	} else {
		TwoWayList<T> list;
		list.Insert (_Payload);
		priQueue.Insert(ci, list);
	}

	size++;
}

template <class T>
bool PriorityList<T>::Remove (T& _Payload) {

	// call the decision function to figure out what token to pop
	int priRet = decFct(priQueue);

	CInt ci = priRet;

	if (priQueue.IsThere(ci)) {
		TwoWayList<T>& list = priQueue.Find(ci);
		if (list.Length()) {
			list.MoveToStart();
			list.Remove (_Payload);
			size--;
			return true;
		}
	}
	return false;
}

template <class T>
bool PriorityList<T>::HigherPriorityWaiting (int priority) {

	for (priQueue.MoveToStart(); !(priQueue).AtEnd(); (priQueue).Advance()) {
		if (priQueue.CurrentData().Length() && priQueue.CurrentKey() < priority)
			return true;
	}
	return false;
}
