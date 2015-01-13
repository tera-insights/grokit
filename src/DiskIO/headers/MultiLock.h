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
#ifndef _MULTI_LOCK_
#define _MULTI_LOCK_

/** Lock that is granted a number of times to the same caller then
		other caller can compete for it
*/


#include <pthread.h>
#include <assert.h>
#include <sys/types.h> // for off_t


class MultiLock {
private:
	int isLocked; // how may times the lock is acquired
								// if 0, the lock is up for grabs

	int counter;// the current value of the counter for the current
							// owner. When it reaches cntMax, it will not be
							// allowed to increase numLocks

	int cntMax; // the reset value of the counter

	pthread_mutex_t mutex;
	pthread_cond_t cond;// conditional variable to wait for the lock to
											// become available

	pthread_t owner;// the owner thread of the lock
									// if counter=cntMax, nobody owns it

public:
	/** Constructor with parameter the number of successes */
	MultiLock(int _numSuccesses);
	virtual ~MultiLock();

	// locking function
	// succeeds if the nobody has the lock or if the calee has the lock
	// and counter did not reach cntMax
	void Lock();

	// pair unlocking function. counter not changed
	void Unlock();
};

inline MultiLock::MultiLock(int _numSuccesses):
	cntMax(_numSuccesses){
	counter=0;
	isLocked = 0;

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
}

inline MultiLock::~MultiLock(){
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
}

inline void MultiLock::Lock(void){

	pthread_mutex_lock(&mutex);

	// if we own this one already, then just check to make sure we are OK to get the lock
	if (isLocked && pthread_equal( owner, pthread_self() )){
		counter++;
		if (counter == cntMax) {
			cerr << "Got to cntMax\n";
			isLocked = 0;
			pthread_cond_broadcast(&cond);
			pthread_mutex_unlock(&mutex);
		} else {
			cerr << "not to cntMax... still OK\n";
			// got the lock again!
			pthread_mutex_unlock(&mutex);
			return;
		}
	} else {
		pthread_mutex_unlock(&mutex);
	}

	// now, wait until it is not locked
	cerr << "Trying to get lock.\n";
	pthread_mutex_lock(&mutex);
	while (isLocked)
		pthread_cond_wait(&cond, &mutex);
	cerr << "Got it!\n";

	// we got it!
	isLocked = 1;
	owner=pthread_self();
	counter = 0;
	pthread_mutex_unlock(&mutex);
}

inline void MultiLock::Unlock(void){
	pthread_mutex_lock(&mutex);

	cerr << "Got an unlock!$%^&\n";
	isLocked = 0;

	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&mutex);
}

#endif // _MULTI_LOCK_
