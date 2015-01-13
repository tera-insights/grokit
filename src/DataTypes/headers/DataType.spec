/** This is the specification for all data types in the system. 

		All the functionality is provided through functions instead of
		member functions. The reason for this is to ensure support of
		basic types without "class" wrappers. The worry is that the class
		wrappers obfuscate the code from the compiler and the generated
		code is not as efficient as it could be.
		
 */

class DataType; // generic datatype

/** Serialization/printing as plain text 

		The object "obj" needs to be transformed into a string and
		deposited at the location pointed out by "buffer".

		Return value: the number of characters writen.

		Assumptions:
		1. the buffer is large enough, no memory needs to be allocated
		
		Required behavior: 
		1. The returned value should reflect the \0 character at the end of string
		Example: return 1+sprintf(buffer, "%d", myInt); // serialize an int
		

		Usage scenario: Print waypoint
*/
int ToString(const DataType& obj, char* buffer);

/** Deserializatin from string/text of the object. The function will
		"populate" the already created where object with info from the
		text stream pointed by where.
		
		Assumptions:
		1. buffer is \0 terminated at the end of token

		Required behavior:
		1. The object obj is changed in place

		Usage scenario: bulk-loading from separator delimited text files

*/
void FromString(DataType& obj, char* buffer);


/** Function to compute the hash of an object. The implementation MUST
		make use of the system-wide hash form Hash.h 
		
		uint64_t hash(uint64_t x , uint64_t b)
		
		and chaining. 

		Example: To hash struct{ long x; long y} mySt;  we do:
		uint64_t Hash(mySt& obj, uint64_t b){
  		return hash( y, hash(x, b));
    }
		
		Requirements:
    ------------
		1. The argument b and the function hash() should be used to
		compute the hash. 


    Note: the function will be automatically generated for all
    datatypes that are simple, i.e. they contain no pointers.

*/
uint64_t Hash(const DataType& obj, uint64_t b);


/** The serialized size of an object. Function should use the object
		"obj" to determine its size.  For fixed size data types, this is a
		constant.

 */
int SerialzedSize(const DataType& bogus);

/** Function to serialize an object into a binary represenation. 

		Returned value:
		1. set the value of size with the amount writen (same in both cases)
		2. return the pointer to the data writen (either buffer or existing serialized object)

		Assumtions:

		1. The buffer leaves longer than the object (shoud not matter for serialize).

		Requirements:
 
		1. The function either "serializes" obj into the buffer or returns
		a pointer to where it is serialized already. In eiter case, the
		pointer to the serialized object is returned and size is set to
		the size in bytes writern. Note: the object has the option of not
		copying anything and simply returning a pointer to it's binary
		representation. This should always be done for "simple" datatypes

		2. Allocation of any memory to place an serialized object is NOT
		ALLOWED. It is either the case that the object already has the
		binary version available or it will serialize itself in "buffer".

		3. The serialized version should be self-sufficient in the sense
		that it completely represents the objec. Should the size bytes at
		location serPtr be copied in any place, the object should be
		constructable from this description using Deserialize.

		4. Should a value different than buffer be returned, it has to be
		the case that the memory pointed to is readable up to a 8 byte
		boundary. It is fine that junk is there but the system should not
		segfault. This is not a stringent requirement since the data will
		live in Storage objects that are a multiple of 8 bytes.
		

		Note: by allowing the user to specifiy a place where the object is
		already serialized, we can do certain optimiations for the
		situation when the serialized object is needed only
		temporarily. Hashing tuples falls in this category.

		Usage scenarios:
		1. Insert tuples in a hash table

		DECISION: shoud we get rid of the optimized version? Is it too
		much of a headache?
		
*/

void* OptimizedBinarySerialize(DataType& obj, void* buffer, int* size);


/** Version of above function in which the object has to serialize itself 
		into the buffer

		The optimized version can be implemented as:
		SerializedPair OptimizedBinarySerialize(DataType& obj, void* buffer){
		  return SerializedPair( buffer, BinarySerialize(obj,buffer) );
		}
		
		Returned value: number of bytes writen in buffer to serialize object

		Usage scenario: 
		1. Serialize data for the purpose of sending it through the network
		2. Storing data in columns

		Note: The object is allowed to recursively serialize itself and
		incrementaly write in buffer all its parts. The max-size of an
		object should be computed based on the parts that get serialized.


*/

int BinarySerialize(DataType& obj, void* buffer);


/** Reconstruct the object obj using binary information at location
		buffer.

		
		Note: one of the two versions of the Deserialize function can be
		implemented but not both. For fixed data types, returning a
		reference is preferable since there is no copying whatsoever. For
		complex types, the second version will ensure that a temporary
		object that gets automatically distroyed is built.

		Return value:
		1. reference or object DataType
		2. size is modified to reflect the amoutnt of input read.

		Usage scenario:
		const DataType& var = Deserialize(buffer, size);
		use var for something

		
 */

DataType& Deserialize(void* buffer, int* size);
DataType Deserialize(void* buffer, int* size);
