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
#ifndef _AGGREGATESTATES_H_
#define _AGGREGATESTATES_H_

#define LARGEPOSITIVEDOUBLE 1.0e+200
#define LARGENEGATIVEDOUBLE -1.0e+200

/** Helper classes to manipulate states of aggregates.
	* This is used in generated code to make it "sane".

	* All of the functions are inlined so this is as fast as access to
	* the actual data.

	* THESE CLASSES ARE NOT BROKEN INTO LOTS OF FILES TO AVOID
	* COMPLICATIONS WITH THE EFFICIENT CODE GENERATION. THE CLASSES
	* ARE NOT USED IN NORMAL(HANDWRITTEN) CODE.
**/


/** Abstract interface for all the state classes
	* No virtual functions allowed.
**/

class AggregateState {
public:
	// Need a constructor that creates the initial state by calling
	// Initialize in each of the derived classes
	AggregateState() {}

	// destructor
	virtual ~AggregateState() {}

	// initialization function (after this the state is ready to be updated)
	void Initialize(void);

	// iterator interface for a tuple/item
	// the state is updated with the value of $x$
	void AddItem(double x);

	// compute the value of the aggregate from the state
	double ComputeAggregate(void);

	// Add the state into another state object into this one
	void AddState(AggregateState& aux){ } // cannot be abstract since definition is changed
};

/* COUNT aggregate */

class CountState : public AggregateState{
private:
	// The state
	unsigned long long count; // just a counter
public:
	CountState(void){ Initialize(); }
	virtual ~CountState(void) {}

	void Initialize(void){ count=0; }
	void AddItem(double dummy){ count++;}
	void AddItem(void){ count++; }
	double ComputeAggregate(void){ return (double) count; }
	void AddState(CountState& aux){ count+=aux.count; }
};


/* SUM aggregate */

class SumState : public AggregateState{
private:
	// The state
	long double sum; // just the sum
public:
	SumState(void){ Initialize(); }
	virtual ~SumState(void) {}

	void Initialize(void){ sum=0.0; }
	void AddItem(double x){ sum+=x; }
	double ComputeAggregate(void){ return sum; }
	void AddState(SumState& aux){ sum+=aux.sum; }
};

/* AVERAGE aggregate */

class AverageState : public AggregateState{
private:
	// The state
	unsigned long long count; // the count
	long double sum; // just the sum

public:
	AverageState(void){ Initialize(); }
	virtual ~AverageState() {}

	void Initialize(void){ count=0; sum=0.0; }
	void AddItem(double x){ count++; sum+=x; }
	double ComputeAggregate(void){
		return (count>0) ? sum/(double)count : 0.0; }
	void AddState(AverageState& aux){ count+=aux.count; sum+=aux.sum; }
};

/* VARIANCE aggregate */

class VarianceState : public AggregateState{
private:
	// The state
	unsigned long long count; // the count
	long double sum; // just the sum
	long double sum2; // just the sum
public:
	VarianceState(void){ Initialize(); }
	virtual ~VarianceState() {}

	void Initialize(void){ count=0; sum=0.0; sum2=0.0; }
	void AddItem(double x){ count++; sum+=x; sum2+=x*x; }
	double ComputeAggregate(void){ return sum2/count-sum*sum/count/count; }
	void AddState(VarianceState& aux){ count+=aux.count; sum+=aux.sum; sum2+=aux.sum2; }
};


/* MIN aggregate */

class MinState : public AggregateState{
private:
	// The state
	double curr; // the current min
public:
	MinState(void){ Initialize(); }
	virtual ~MinState(void) {}

	void Initialize(void){ curr= LARGEPOSITIVEDOUBLE; }
	void AddItem(double x){ curr = (x<curr) ? x : curr; }
	double ComputeAggregate(void){ return curr; }
	void AddState(MinState& aux){ AddItem(aux.curr); }
};

/* MAX aggregate */

class MaxState : public AggregateState{
private:
	// The state
	double curr; // the current min
public:
	MaxState(void){ Initialize(); }
	virtual ~MaxState(void) {}

	void Initialize(void){ curr= LARGENEGATIVEDOUBLE; }
	void AddItem(double x){ curr = (x>curr) ? x : curr; }
	double ComputeAggregate(void){ return curr; }
	void AddState(MaxState& aux){ AddItem(aux.curr); }
};

#endif //_AGGREGATESTATES_H_
