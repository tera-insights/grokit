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
#ifndef _PAGECONVERSTIONS_H_
#define _PAGECONVERSTIONS_H_

/** conversion functions from bytes to pages and back */

//convert size from bytes to number of pages.
inline off_t bytesToPages(off_t pageSize, off_t size){
	off_t rez = size / pageSize;
	if (size>pageSize*rez){
		rez++;
	}
	return rez;
}

// add to the size until it is a multiple of the page size
inline off_t bytesToPageAlligned(off_t pageSize, off_t size){
	return pageSize*bytesToPages(pageSize,size);
}

// transform pages back into bytes
inline off_t pagesToBytes(off_t pageSize, off_t sizeInPages){
	return pageSize*sizeInPages;
}

#endif // _PAGECONVERSTIONS_H_
