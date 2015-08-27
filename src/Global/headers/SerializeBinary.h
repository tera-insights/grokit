#ifndef SERIALIZE_BINARY_H
#define SERIALIZE_BINARY_H

#include <cstddef>

// Default functions for Serialize/Deserialize

template <class DataType>
size_t SizeFromBuffer(const char * buffer) {
	return sizeof(DataType);
}

template <class DataType>
size_t SerializedSize(const DataType& src) {
    return sizeof(DataType);
}

template <class DataType>
size_t Serialize(char * buffer, const DataType& src) {
    DataType* asTypePtr = reinterpret_cast<DataType*>(buffer);
    *asTypePtr = src;
    return sizeof(DataType);
}

template <class DataType>
size_t Deserialize(const char * buffer, DataType& dest) {
    const DataType* asTypePtr = reinterpret_cast<const DataType*>(buffer);
    dest = *asTypePtr;
    return sizeof(DataType);
}

#endif // SERIALIZE_BINARY_H
