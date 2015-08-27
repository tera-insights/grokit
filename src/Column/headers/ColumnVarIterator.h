//
//  Copyright 2012 Christopher Dudley
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

#ifndef _COLUMN_VAR_ITERATOR_H_
#define _COLUMN_VAR_ITERATOR_H_

#include "Constants.h"
#include "Column.h"
#include "Iterator.h"

#ifndef _dp_max
#define _dp_max(x,y) ((x) > (y)? (x) : (y))
#endif

template <class DataType, uint64_t headerSize = DataType::HeaderLength, uint64_t dtSize = DataType::MaxObjectLength>
class ColumnVarIterator {

    DataType myValue;

    Iterator it;

public:
    
    // Creates a column iterator for the given column. The requests for data
    // that are sent to the column are of stepSize. iterateMe is consumed.
    ColumnVarIterator (Column &iterateMe, uint64_t stepSize = COLUMN_ITERATOR_STEP);

    ColumnVarIterator (Column &iterateMe, uint64_t fragmentStart, uint64_t fragmentEnd, uint64_t stepSize = COLUMN_ITERATOR_STEP);

    ColumnVarIterator();

    ~ColumnVarIterator();

    void Insert( const DataType& addMe );

    void Advance();

    const DataType& GetCurrent(); 

	void Done (Column &iterateMe);

    void Restart(void);

	uint64_t AtUnwrittenByte ();

	void CreateDeepCopy (ColumnVarIterator& fromMe);

	void CreateShallowCopy (ColumnVarIterator& fromMe);

	void CheckpointSave ();

	void CheckpointRestore ();

	// swap
	void swap (ColumnVarIterator& swapMe);

	void MarkFragment ();
};

template <class DataType, uint64_t headerSize, uint64_t dtSize>
inline
ColumnVarIterator<DataType, headerSize, dtSize> :: ColumnVarIterator( Column &iterateMe, uint64_t stepSize )
    : it( iterateMe, headerSize, _dp_max(stepSize, dtSize) )
{
    if( it.IsInvalid() )
        return;

    if( !it.IsWriteOnly() ) {
        it.EnsureHeaderSpace();
        size_t serializedSize = SizeFromBuffer<DataType>(it.GetData());
        it.EnsureSpace(serializedSize, serializedSize);
        Deserialize(it.GetData(), myValue);
    } 
}

template <class DataType, uint64_t headerSize, uint64_t dtSize>
inline
ColumnVarIterator<DataType, headerSize, dtSize> :: ColumnVarIterator( Column &iterateMe, uint64_t fragmentStart, uint64_t fragmentEnd, uint64_t stepSize )
    : it( iterateMe, fragmentStart, fragmentEnd, headerSize, _dp_max(stepSize, dtSize) )
{
    if( it.IsInvalid() )
        return;

    if( !it.IsWriteOnly() ) {
        it.EnsureHeaderSpace();
        size_t serializedSize = SizeFromBuffer<DataType>(it.GetData());
        it.EnsureSpace(serializedSize, serializedSize);
        Deserialize(it.GetData(), myValue);
    } 
}

template <class DataType, uint64_t headerSize, uint64_t dtSize>
inline
ColumnVarIterator<DataType, headerSize, dtSize> :: ColumnVarIterator()
    : it()
{
}

template <class DataType, uint64_t headerSize, uint64_t dtSize>
inline
ColumnVarIterator<DataType, headerSize, dtSize> :: ~ColumnVarIterator() {
}

template <class DataType, uint64_t headerSize, uint64_t dtSize>
inline
void ColumnVarIterator<DataType, headerSize, dtSize> :: Insert( const DataType& addMe ) {
    if( it.IsInvalid() )
        return;

    assert( it.IsWriteOnly() == true );

    it.SetObjLen( SerializedSize(addMe) );
    it.EnsureWriteSpace();
    
    char* myData = it.GetData(); 

    myValue = addMe;
    Serialize(myData, myValue);
}

template <class DataType, uint64_t headerSize, uint64_t dtSize>
inline
void ColumnVarIterator<DataType, headerSize, dtSize> :: Advance() {
    if( it.IsInvalid() )
        return;

    uint64_t oLen = SerializedSize(myValue);
    it.SetObjLen( oLen );
    it.AdvanceBy( oLen );

    if( !it.IsWriteOnly() ) {
        it.EnsureHeaderSpace();
        size_t serializedSize = SizeFromBuffer<DataType>(it.GetData());
        it.EnsureSpace(serializedSize, serializedSize);
        Deserialize(it.GetData(), myValue);
    }
}

template <class DataType, uint64_t headerSize, uint64_t dtSize>
inline
const DataType& ColumnVarIterator<DataType, headerSize, dtSize> :: GetCurrent() {
    return myValue;
}

template <class DataType, uint64_t headerSize, uint64_t dtSize>
void ColumnVarIterator <DataType, headerSize, dtSize> :: Done (Column &iterateMe) {
	it.Done(iterateMe);
}

template <class DataType, uint64_t headerSize, uint64_t dtSize>
void ColumnVarIterator <DataType, headerSize, dtSize> :: CreateDeepCopy (ColumnVarIterator<DataType, headerSize, dtSize>& fromMe) {
	it.CreateDeepCopy(fromMe.it);
}

template <class DataType, uint64_t headerSize, uint64_t dtSize>
void ColumnVarIterator <DataType, headerSize, dtSize> :: swap (ColumnVarIterator& swapMe) {
	it.swap (swapMe.it);
}

template <class DataType, uint64_t headerSize, uint64_t dtSize>
void ColumnVarIterator <DataType, headerSize, dtSize> :: CreateShallowCopy (ColumnVarIterator& copyMe) {
	it.CreateShallowCopy (copyMe.it);
}

template <class DataType, uint64_t headerSize, uint64_t dtSize>
void ColumnVarIterator <DataType, headerSize, dtSize> :: CheckpointSave () {
	it.CheckpointSave ();
}

template <class DataType, uint64_t headerSize, uint64_t dtSize>
void ColumnVarIterator <DataType, headerSize, dtSize> :: CheckpointRestore () {
	it.CheckpointRestore ();
}

template <class DataType, uint64_t headerSize, uint64_t dtSize >
inline uint64_t ColumnVarIterator <DataType, headerSize, dtSize > :: AtUnwrittenByte () {
  return it.AtUnwrittenByte ();
}

template <class DataType, uint64_t headerSize, uint64_t dtSize >
inline void ColumnVarIterator <DataType, headerSize, dtSize > :: Restart () {
  it.Restart();
}


template <class DataType, uint64_t headerSize, uint64_t dtSize >
inline void ColumnVarIterator <DataType, headerSize, dtSize > :: MarkFragment () {
  if (it.IsInvalid())
	return;
  it.MarkFragment ();
}

#endif // _COLUMN_VAR_ITERATOR_H_
