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
#ifndef _VECTOR_H_
#define _VECTOR_H_
#include<assert.h>

template <class T, int size>
class Vector {

	T mVec[size];

public:

	Vector(); // do nothing

	Vector(const T& initVal); // initialize all values

	void insert(const T& val, int index);

	// Provide const and non-const versions since return type differs
	T& operator[](int index);
	const T& operator[](int index) const;

	Vector& operator=(const Vector<T, size>& vec);

	bool operator==(const Vector<T, size>& vec) const;
	//bool operator==(const Vector<T, size>& vec) {return ((static_cast<const Vector<T, size>& >(*this)) == vec);}

	bool operator!=(const Vector<T, size>& vec) const;
	//bool operator!=(const Vector<T, size>& vec) {return ((static_cast<const Vector<T, size>& >(*this)) != vec);}

	// for STL ordering
	bool operator<(const Vector<T, size>& vec) const;
	//bool operator<(const Vector<T, size>& vec) {return ((static_cast<const Vector<T, size>& >(*this)) < vec);}

};

template <class T, int size>
inline Vector<T, size>::Vector() {
	// do nothing
}

template <class T, int size>
inline Vector<T, size>::Vector(const T& initVal) {

	for (int i = 0; i < size; i++)
		mVec[i] = initVal;
}

template <class T, int size>
inline void Vector<T, size>::insert(const T& val, int index) {

	mVec[index] = val;
}

template <class T, int size>
inline T& Vector<T, size>::operator[](int index) {

	return mVec[index];
}

template <class T, int size>
inline const T& Vector<T, size>::operator[](int index) const {

	return mVec[index];
}

template <class T, int size>
inline Vector<T, size>& Vector<T, size>::operator=(const Vector<T, size>& vec) {

	for (int i = 0; i < size; i++)
		mVec[i] = vec.mVec[i];

	return *this;
}

template <class T, int size>
inline bool Vector<T, size>::operator==(const Vector<T, size>& vec) const {

	for (int i = 0; i < size; i++) {
		if (mVec[i] != vec.mVec[i])
			return false;
	}
	return true;
}

template <class T, int size>
inline bool Vector<T, size>::operator!=(const Vector<T, size>& vec) const {

	return !(*this == vec);
}

template <class T, int size>
inline bool Vector<T, size>::operator<(const Vector<T, size>& vec) const {

	for (int i = 0; i < size; i++) {
		if (!(mVec[i] < vec.mVec[i]))
			return false;
	}
	return true;
}

// -----------------------------------------------------------------------------------------

template <class T, int size>
class SquareMatrix {

	Vector<Vector<T, size>, size> mMatrix;

public:

	SquareMatrix(); // do nothing

	SquareMatrix(const T& initVal); // initialize all values

	void insert(const T& val, int x, int y);

	void insert(const Vector<T, size>& val, int index);

	// both versions since return type differs
	Vector<T, size>& operator[](int index);
	const Vector<T, size>& operator[](int index) const;

	SquareMatrix& operator=(const SquareMatrix<T, size>& vec);

	bool operator==(const SquareMatrix<T, size>& val) const;
	//bool operator==(const SquareMatrix<T, size>& val) {return ((static_cast<const SquareMatrix<T, size>& >(*this)) == val);}

	bool operator!=(const SquareMatrix<T, size>& val) const;
	//bool operator!=(const SquareMatrix<T, size>& val) {return ((static_cast<const SquareMatrix<T, size>& >(*this)) != val);}

	// for STL ordering
	bool operator<(const SquareMatrix<T, size>& val) const;
	//bool operator<(const SquareMatrix<T, size>& val) {return ((static_cast<const SquareMatrix<T, size>& >(*this)) < val);}
};


template <class T, int size>
inline SquareMatrix<T, size>::SquareMatrix() {
	// do nothing
}

template <class T, int size>
inline SquareMatrix<T, size>::SquareMatrix(const T& initVal) {

	for (int i = 0; i < size; i++)
		for (int j = 0; j < size; j++)
			mMatrix[i][j] = initVal;
}

template <class T, int size>
inline void SquareMatrix<T, size>::insert(const T& val, int x, int y) {

	mMatrix[x][y] = val;
}

template <class T, int size>
inline void SquareMatrix<T, size>::insert(const Vector<T, size>& val, int index) {

	mMatrix[index] = val;
}

template <class T, int size>
inline Vector<T, size>& SquareMatrix<T, size>::operator[](int index) {

	return mMatrix[index];
}

template <class T, int size>
inline const Vector<T, size>& SquareMatrix<T, size>::operator[](int index) const {

	return mMatrix[index];
}

template <class T, int size>
inline SquareMatrix<T, size>& SquareMatrix<T, size>::operator=(const SquareMatrix<T, size>& val) {

	for (int i = 0; i < size; i++)
		for (int j = 0; j < size; j++)
			mMatrix[i][j] = val.mMatrix[i][j];

	return *this;
}

template <class T, int size>
inline bool SquareMatrix<T, size>::operator==(const SquareMatrix<T, size>& val) const {

	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			if (mMatrix[i][j] != val.mMatrix[i][j])
				return false;
		}
	}
	return true;
}

template <class T, int size>
inline bool SquareMatrix<T, size>::operator!=(const SquareMatrix<T, size>& val) const {

	return !(*this == val);
}

template <class T, int size>
inline bool SquareMatrix<T, size>::operator<(const SquareMatrix<T, size>& val) const {

	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			if (!(mMatrix[i][j] < val.mMatrix[i][j]))
				return false;
		}
	}
	return true;
}

//-------------------------------------------------------------------------------

// type must be above fixed size Vector type, and size could be anything
// m x n matrix class
// For eg, T = Vector<int, 100>
template <class T, int size>
class Matrix {

	Vector<T, size> mMatrix;

public:

	Matrix(); // do nothing

	Matrix(const T& initVal); // initialize all values

	void insert(const T& val, int x, int y);

	void insert(const Vector<T, size>& val, int index);

	// both versions since return type differs
	T& operator[](int index);
	const T& operator[](int index) const;

	Matrix& operator=(const Matrix<T, size>& vec);

	bool operator==(const Matrix<T, size>& val) const;
	//bool operator==(const Matrix<T, size>& val) {return ((static_cast<const Matrix<T, size>& >(*this)) == val);}

	bool operator!=(const Matrix<T, size>& val) const;
	//bool operator!=(const Matrix<T, size>& val) {return ((static_cast<const Matrix<T, size>& >(*this)) != val);}

	// for STL ordering
	bool operator<(const Matrix<T, size>& val) const;
	//bool operator<(const Matrix<T, size>& val) {return ((static_cast<const Matrix<T, size>& >(*this)) < val);}
};


template <class T, int size>
inline Matrix<T, size>::Matrix() {
	// do nothing
}

template <class T, int size>
inline Matrix<T, size>::Matrix(const T& initVal) {

	for (int i = 0; i < size; i++)
		for (int j = 0; j < size; j++)
			mMatrix[i][j] = initVal;
}

template <class T, int size>
inline void Matrix<T, size>::insert(const T& val, int x, int y) {

	mMatrix[x][y] = val;
}

template <class T, int size>
inline void Matrix<T, size>::insert(const Vector<T, size>& val, int index) {

	mMatrix[index] = val;
}

template <class T, int size>
inline T& Matrix<T, size>::operator[](int index) {

	return mMatrix[index];
}

template <class T, int size>
inline const T& Matrix<T, size>::operator[](int index) const {

	return mMatrix[index];
}

template <class T, int size>
inline Matrix<T, size>& Matrix<T, size>::operator=(const Matrix<T, size>& val) {

	// 2D assignments should be automatic
	for (int i = 0; i < size; i++)
			mMatrix[i] = val.mMatrix[i];

	return *this;
}

template <class T, int size>
inline bool Matrix<T, size>::operator==(const Matrix<T, size>& val) const {

	for (int i = 0; i < size; i++) {
		if (mMatrix[i] != val.mMatrix[i])
			return false;
	}
	return true;
}

template <class T, int size>
inline bool Matrix<T, size>::operator!=(const Matrix<T, size>& val) const {

	return !(*this == val);
}

template <class T, int size>
inline bool Matrix<T, size>::operator<(const Matrix<T, size>& val) const {

	for (int i = 0; i < size; i++) {
		if (!(mMatrix[i] < val.mMatrix[i]))
			return false;
	}
	return true;
}

#endif
