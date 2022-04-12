// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <cstdint>
#include <cstddef>

unsigned constexpr hash(char const *input)
{
    return *input ? static_cast<unsigned int>(*input) + 33 * hash(input + 1) : 5381;
}

extern "C"
{
size_t strlen(const char *str);

char *strcpy(char *destination, const char *source);
char *strncpy(char *destination, const char *source, size_t n);

char *strcat(char *destination, const char *source);
char *strchr(const char *str, char ch);

int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);

char *strdup(const char *src);

long strtol(const char *nPtr, char **endPtr, int base);
long long int strtoll(const char *nPtr, char **endPtr, int base);
unsigned long strtoul(const char *nPtr, char **endPtr, int base);
unsigned long long int strtoull(const char *nPtr, char **endPtr, int base);

char** strsplit(const char* s, const char* delim);
char** strsplit_count(const char* s, const char* delim, size_t &nb);

char *strstr(const char *str, const char *substr);
}

char *getline(const char *str, const char *substr, char *buffer, int skip = 0);
int lstrstr(const char *str, const char *substr, int skip = 0);
char *strrm(char *str, const char *substr);

char *reverse(char *str);

bool isspace(char c);
bool isempty(char *str);
static inline bool isempty(const char *str)
{
    return isempty(const_cast<char*>(str));
}

int isdigit(int c);
int isalpha(int c);
int islower(int c);

char tolower(char c);
char toupper(char c);
int tonum(char c);
char *tostr(char c);

char *int2string(int num);
int string2int(const char *str);

/*
*** String implementation by https://github.com/cocoz1/cpp-string
*** Modified for use in this kernel by ilobilo
*/

#define STR_USE_CLEANUP_FUNCTION
#define STR_USE_ASSERTIONS

/*
*******************************************
*** C++ string class
*** written by cocoz1 in 06, 2018
*** features a simple (header-only) C++ string class which implements most of std::basic_string's functionalities
*** If you find any bugs in my code and want to contribute, please leave an issue on github.
*** ----> You can also suggest new features or better implementations of single functions, anything is appreciated.
*** ----> I'm generally open for any kinds of criticism, as long as it's contructive and not offensive in any way.
*** ----> Almost all implementations in this class are personally written by me, if not, I pasted an URL to the source above the function.
***
***
*** L I C E N S E   T E X T
*** This software is provided 'as-is', without any express or implied warranty!!!
*** In no event will me be held liable for any damages arising from the use of this software.
***
*** RULES:
***			1. The origin of this software must not be misrepresented;
***				you must not claim that you wrote the original software.
***				If you use this software in a product, an acknowledgment
***				in the product documentation is required.
***			2. Altered source versions must be marked as such, and must not be misrepresented
				as being the original software.
***			3. This notice may not be removed or altered from any source distribution.
***
***
*** NOTE:
***			This software is not supposed to replace the std::basic_string library,
***			it should show people how such a class could work;
***			Might be also used as a reference or an inspiration for future implementations of such a class
***			Besides that, it can be well used as an alternative to std::basic_string;
***
*** (C) 2018 cocoz1, All rights reserved.
*******************************************
*/

#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1900)
	#define STR_CPP11_OR_HIGHER
#endif

/*
*** include this header for several functions
*** such as memcpy(), memset() and memmove()
*/
#include <lib/memory.hpp> /* for memmove(), memcpy() */
#include <lib/alloc.hpp> /* include stdlib.h for realloc() function */
/* define STR_USE_ASSERTIONS somewhere in your code to enable assertions */
#ifdef STR_USE_ASSERTIONS
	#include <lib/panic.hpp> /* for assert() */
#endif

/*
*** default string capacity;
*** (for standard constructor and so on) [def: 32]
*** Version 1.1: def(8) -> def(32)
*/
#define STR_DEFSTRCAP 32

/*
*** DEF_LARGESTRING is a setting to optimize
capacity allocation for large or for smaller strings
*** allocate DEF_ALLOC * sizeof(T) more space if required
(It's a high number to minimize reallocations in program) [def: 8192 or 16384] */
#ifdef STR_LARGESTRING
	#define STR_ALLOC 16384
#else
	#define STR_ALLOC 8192
#endif

/*
*** string version;
*** can be used to check compatibility or for an outdated version
*** Macro added with version 1.1
*** Is changed on new release
*/
#define STR_VERSION "1.8"

/* define NULL macro if it's not defined by default */
#ifndef NULL
	#define NULL ((void *)0)
#endif

/*
*** some tiny helper-macros for string class
----------------
*** ABS => return absolute value
*** MIN => return least value
*** MAX => return highest value
*/
#define ABS(x) (x >= 0 ? x : -x) /* return absolute value of x */
#define MIN(x, y) (x > y ? y : x) /* return lower value of x and y */
#define MAX(x, y) (x > y ? x : y) /* return higher value of x and y */

/* some typedefs for shorter code */
typedef unsigned _u_;					/* unsigned (int) */
typedef unsigned long _ul_;				/* unsigned long */
typedef unsigned long long _ull_;		/* unsigned long long */
typedef long long _ll_;					/* long long */
typedef long double _ld_;				/* long double */

/* TEMPLATE CLASS string_base<T> */
template <typename T>
class string_base {
public:
	typedef T *iterator;						/* normal iterator type */
	typedef const T *const_iterator;			/* const iterator type */
	typedef T &reference;						/* normal reference type */
	typedef const T &const_reference;			/* const reference type */

	/*
	*** string_base<T>()
	*** standard contructor
	*** allocates DEF_STRCAP (def: 32) spaces for
	chars and sets the first one to 0x00
	*** is pretty much the standard allocation state
	*** use reset() to restore these settings
	*/
	explicit string_base<T>()
		: len(0), cap(STR_DEFSTRCAP) {
		raw_data = new T[STR_DEFSTRCAP];
		raw_data[0] = 0x00;
	}
	/*
	*** string_base<T>(unsigned)
	*** constructor to allocate "capacity" spaces by default
	*** sets first character to 0x00 (null-terminator)
	*/
	explicit string_base<T>(unsigned capacity)
		: len(0), cap(capacity) {
		raw_data = new T[cap];
		raw_data[0] = 0x00;
	}
	/*
	*** string_base<T>(const T &, unsigned)
	*** fill constructor
	*** fill the freshly allocated string
	with specified char "ch" ("count" times)
	*** allocates ("count" + 1) places
	*** sets last char to 0x00 to finish string
	*/
	string_base<T>(const T &ch, unsigned count)
		: len(count), cap(count + 1) {
		raw_data = new T[cap];
		for (unsigned i = 0; i < count; i++)
			raw_data[i] = ch;
		raw_data[len] = 0x00;
	}
	/*
	*** string_base<T>(const T *)
	*** constructor to assign a C-String
	*** copies "str" in current string
	*** automatically inserts null-terminator at the end
	*/
	string_base<T>(const T *c_str)
		: len(strlength<T>(c_str)), cap(len + 1) {
		raw_data = new T[cap];
		memcpy(raw_data, c_str, len * sizeof(T));
		raw_data[len] = 0x00;
	}
	/*
	*** string_base<T>(const T *, unsigned)
	*** constructor to copy first "len" chars of c_str's value in current string
	*** automatically inserts null-terminator at the end
	*** Version: 1.3.5: Added this contructor
	*/
	string_base<T>(const T *c_str, unsigned len) {
		this->len = MIN(len, strlength<T>(c_str));
		cap = this->len + 1;
		raw_data = new T[cap];
		memcpy(raw_data, c_str, this->len * sizeof(T));
		raw_data[this->len] = 0x00;
	}
	/*
	*** string_base<T>(const string_base<T> &)
	*** copy constructor
	*** just copy "obj" in current value
	*** also copy allocated state (allocate same amount of space as in "str")
	*** automatically inserts null-terminator at the end
	*/
	string_base<T>(const string_base<T> &obj)
		: len(obj.len), cap(obj.cap) {
		raw_data = new T[cap];
		memcpy(raw_data, obj.raw_data, len * sizeof(T));
		raw_data[len] = 0x00;
	}

	/*
	*** string_base<T>(const string_base<T> &, unsigned, unsigned)
	*** constructor to assign substring of "str" to current value
	(starting at position "start2 with a length of "count")
	*** allocate ("count" + 1) spaces for substring
	*** automatically inserts null-terminator at the end
	*** if "start" is greater than str's length, then start = 0
	*** Version 1.2: Optimizing, using memcpy() now
	*** Version 1.2: fix crash when "start" is greater than str's length
	*/
	explicit string_base<T>(const string_base<T> &str, unsigned start, unsigned count) {
		unsigned l = str.len;
		if (start > l) start = 0;
		if ((start + count) > l)
			count = (l - start);
		len = count; cap = len + 1;
		raw_data = new T[cap];
		memcpy(raw_data, str.raw_data + start, count * sizeof(T));
		raw_data[len] = 0x00;
	}

	/*
	*** string_base<T>(const T*, unsigned, unsigned)
	*** constructor to assign substring of a C-String
	(starting at position "start2 with a length of "count")
	*** allocate enough space ("count" + 1)
	*** automatically inserts null-terminator at the end
	*** if "start" is greater than str's length, then start = 0
	*** Version 1.2: Optimizing, using memcpy() now
	*** Version 1.2: fix crash when "start" is greater than str's length
	*/
	explicit string_base<T>(const T *c_str, unsigned start, unsigned count) {
		unsigned l = strlength<T>(c_str);
		if (start > l) start = 0;
		if ((start + count) > l)
			count = (l - start);
		len = count; cap = len + 1;
		raw_data = new T[cap];
		memcpy(raw_data, c_str + start, count * sizeof(T));
		raw_data[len] = 0x00;
	}
	/*
	*** ~string_base<T>()
	*** default destructor to deallocate used memory
	*** You might consider using cleanup() instead
	(define DEF_USE_CLEANUP_FUNCTION to make it available)
	*** Readded with Version 1.4
	*/
	~string_base<T>() {
		delete[] raw_data;
#ifdef STR_CPP11_OR_HIGHER
		/* nullptr is a typesafe alternative to NULL */
		raw_data = nullptr;
#else
		/* C++98 support by avoiding nullptr */
		raw_data = NULL;
#endif
	}

	T *data() const { return raw_data; } /* returns C-String value */
	const T *c_str() const { return (const T *)raw_data; } /* returns C-String (const) value */
	unsigned length() const { return len; } /* returns length of current string instance */
	unsigned size() const { return len * sizeof(T); } /* returns current size in bytes the actual string with its chars uses */
	unsigned capacity() const { return cap; } /* returns maximum number of chars this value can hold in currently allocated state */
	/*
	*** T &at(unsigned)
	*** returns reference to a character at position "pos"
	*** if "pos" is out of range, it returns the last char of string (len - 1)
	*** Version 1.2: Optimizing, remove "ac_len" variable
	*/
	reference at(unsigned pos) { return raw_data[(pos > (len - 1) ? (len - 1) : pos)]; }
	/*
	*** const T &at(unsigned) const
	*** returns reference to a character at position "pos"
	*** if "pos" is out of range, it returns the last char of string (len - 1)
	*** if current string value is const-qualified, it returns this function when
	"at" is called outside of the class
	*** Version 1.2: Optimizing, remove "ac_len" variable
	*/
	const_reference at(unsigned pos) const { return raw_data[(pos > (len - 1) ? (len - 1) : pos)]; }
	/*
	*** bool at(const T, unsigned)
	*** checks if given character "ch" is at given position "pos"
	*** if "pos" is out of range, it compares last char of string (len - 1)
	*** returns true, if char's value is at position "pos", false if not
	*** Version 1.2: Optimizing, remove "ac_len" variable
	*/
	bool at(const T ch, unsigned pos) { return (raw_data[(pos > (len - 1) ? (len - 1) : pos)] == ch); }
	/*
	*** T &first()
	*** returns reference to first char of string
	*** can be also used to change value of first char
	*/
	reference first() { return raw_data[0]; }
	/*
	*** const T &first() const
	*** returns reference to first char of string
	*** if current string value is const-qualified,
	it returns this function when "first" is called
	outside of the class
	*/
	const_reference first() const { return raw_data[0]; }
	/*
	*** T &last()
	*** returns reference to last char of string
	*** can be also used to change value of last char
	*/
	reference last() {
		if (!len) return raw_data[0];
		return raw_data[len - 1];
	}
	/*
	*** const T &last() const
	*** returns reference to last char of string
	*** if current string value is const-qualified,
	it returns this function when "last" is called
	outside of the class
	*/
	const_reference last() const {
		if (!len) return raw_data[0];
		return raw_data[len - 1];
	}
	bool empty() const { return (len == 0); } /* checks whether string is empty */
	iterator begin() const { return &raw_data[0]; } /* returns iterator to the beginning of string */
	iterator end() const { return &raw_data[len]; } /* returns iterator to the ending of string */
	const_iterator cbegin() const { return &raw_data[0]; } /* returns constant iterator to the beginning of string */
	const_iterator cend() const { return &raw_data[len]; } /* returns constant iterator to the ending of string */

	/*
	*** unsigned copy(T *, unsigned, unsigned = 0U)
	*** copy "size" chars of current string object starting from
	position "size" to position "pos" in "buffer"
	*** doesn't modify current string value
	*** returns number of chars copied
	*** does nothing if
		-> buffer is NULL
		-> current string data is NULL
		-> size == 0
	*** if buffer is too small, behavior is undefined
	*** Added with Version 1.7
	*/
	unsigned copy(T *buffer, unsigned size, unsigned pos = 0U) {
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
		assert(buffer != NULL);
#else
		if (!buffer || !raw_data || !size)
			return 0;
#endif
		if (pos > len) pos = 0;
		if ((pos + size) > len) size = (len - pos);
		memcpy(buffer, raw_data + pos, size * sizeof(T));
		buffer[size] = 0x00;
		return size;
	}

	/*
	*** string_base<T> &splice(string_base<T> &, unsigned, unsigned, unsigned)
	*** transfer substring of current string value (starting on position "start"
	with a length of "size") and insert it in "buffer" at position "pos"
	*** transfer means, it deletes the substring portion from current string value
	*** does nothing if
		-> current string data == NULL
		-> size == 0
		-> current string equals to buffer
	*** returns (eventually) modified string object
	*** Added with Version 1.7
	*/
	string_base<T> &splice(string_base<T> &buffer, unsigned start, unsigned size, unsigned pos) {
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
		assert(this != &buffer);
#else
		if (!raw_data || !size || this == &buffer) return (*this);
#endif
		if (start > len) start = 0;
		if (pos > buffer.len) pos = 0;
		if ((pos + size) > len) size = (len - pos);
		buffer.insert(*this, pos, start, size);
		erase(start, size);
		return (*this);
	}

	/*
	*** string_base<T> &fill(const T &)
	*** fill whole string buffer with T's value
	*** inserts null-terminator at the end
	*** Does nothing if
		-> len == 0 (std::string is empty)
		-> raw_data == NULL (allocation error)
	*** returns (eventually modified) *this object
	*** Added with Version 1.5
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &fill(const T &ch) {
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		if (!len) return (*this);
		for (unsigned i = 0; i < len; i++)
			raw_data[i] = ch;
		raw_data[len] = 0x00;
		return (*this);
	}
	/*
	*** string_base<T> &fill(const T &)
	*** fill first "count" places in
	string buffer with T's value
	*** inserts null-terminator at the end
	*** Does nothing if
		-> len == 0 (std::string is empty)
		-> raw_data == NULL (allocation error)
		-> count == 0 (no chars to copy)
	*** returns (eventually modified) *this object
	*** Added with Version 1.5
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &fill(const T &ch, unsigned count) {
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		if (!len || !count) return (*this);
		unsigned ac_len = MIN(count, len);
		for (unsigned i = 0; i < ac_len; i++)
			raw_data[i] = ch;
		raw_data[len] = 0x00;
		return (*this);
	}
	/*
	*** string_base<T> &fill(const T &)
	*** fill substring of string buffer with T's value
	*** inserts null-terminator at the end
	*** Does nothing if
		-> len == 0 (std::string is empty)
		-> raw_data == NULL (allocation error)
		-> count == 0 (no chars to copy)
	*** If start is greater than current string length, then start = 0
	*** returns (eventually modified) *this object
	*** Added with Version 1.5
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &fill(const T &ch, unsigned start, unsigned count) {
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		if (!len || !count) return (*this);
		if (start > len) start = 0;
		if ((start + count) > len) count = (len - start);
		for (unsigned i = start; i < (start + count); i++)
			raw_data[i] = ch;
		raw_data[len] = 0x00;
		return (*this);
	}

	/*
	*** string_base<T> &assign(const T *)
	*** assign c_str's value to this string
	*** old data is lost once you call this function
	*** performs a reallocation with adjusted size and capacity
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimization, using memcpy() now
	*/
	string_base<T> &assign(const T *c_str) {
		if (!*c_str) return (*this);
		delete[] raw_data;
		len = strlength<T>(c_str);
		cap = len + 1;
		raw_data = new T[cap];
		memcpy(raw_data, c_str, len * sizeof(T));
		raw_data[len] = 0x00;
		return (*this);
	}
	/*
	*** string_base<T> &assign(const T *, unsigned)
	*** assigns the first "len" characters of "c_str" to this string
	*** old data is lost once you call this function
	*** performs a reallocation with adjusted size and capacity
	*** returns (modified) *this object
	*** Version 1.2: Optimization, using memcpy() now
	*** Version 1.3.5:
			raw_data[len] = 0x00;
		has been changed to
			raw_data[this->len] = 0x00;
	*/
	string_base<T> &assign(const T *c_str, unsigned len) {
		if (!*c_str || !len) return (*this);
		unsigned ac_len = MIN(len, strlength<T>(c_str));
		delete[] raw_data;
		this->len = ac_len;
		cap = this->len + 1;
		raw_data = new T[cap];
		memcpy(raw_data, c_str, this->len * sizeof(T));
		raw_data[this->len] = 0x00;
		return (*this);
	}
	/*
	*** string_base<T> &assign(const string_base<T> &)
	*** assign str's value to this string
	*** old data is lost once you call this function
	*** performs a reallocation with adjusted size and capacity
	*** does nothing if
		-> you try to assign *this to this string
		-> str's length is 0
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimization, using memcpy() now
	*/
	string_base<T> &assign(const string_base<T> &str) {
		if (this == &str) return (*this);
		delete[] raw_data;
		len = str.len;
		cap = len + 1;
		raw_data = new T[cap];
		memcpy(raw_data, str.raw_data, len * sizeof(T));
		raw_data[len] = 0x00;
		return (*this);
	}

	/*
	*** string_base<T> &assign(const T &, unsigned len)
	*** assigns ch's value to this string (actually, "len" times)
	*** old data is lost once you call this function
	*** performs a reallocation with adjusted size and capacity
	*** returns (modified) *this object
	*/
	string_base<T> &assign(const T &ch, unsigned len) {
		if (!len) return (*this);
		delete[] raw_data;
		this->len = len;
		cap = len + 1;
		raw_data = new T[cap];
		for (unsigned i = 0; i < cap; i++)
			raw_data[i] = ch;
		raw_data[len] = 0x00;
		return (*this);
	}
	/*
	*** string_base<T> &assign(string_base<T> &, unsigned, unsigned)
	*** assigns a substring of "str" (starting at "start" with a length
	of "count") to this string
	*** old data is lost once you call this function
	*** performs a reallocation with adjusted size and capacity
	*** does nothing if
		-> you try to assign *this with same length to this string
		-> substring's length ("count") == 0
	*** if "start" is greater than str's length, then start = 0
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimization, using memcpy() now
	*** based on this article: https://www.includehelp.com/c-programs/substring-implementation.aspx
	*/
	string_base<T> &assign(const string_base<T> &str, unsigned start, unsigned count) {
		if (!count || (this == &str && start == 0 && count == len))
			return (*this);
		unsigned l = str.len;
		if (start > l) start = 0;
		if ((start + count) > l)
			count = (l - start);
		len = count; cap = len + 1;
		delete[] raw_data;
		raw_data = new T[cap];
		memcpy(raw_data, str.raw_data + start, len * sizeof(T));
		raw_data[len] = 0x00;
		return (*this);
	}

	/*
	*** string_base<T> &assign(const T *, unsigned, unsigned)
	*** assigns a substring of "c_str" (starting at "start" with a length
	of "count") to this string
	*** old data is lost once you call this function
	*** performs a reallocation with adjusted size and capacity
	*** does nothing if
	-> c_str's value == ""
	-> substring's length ("count") == 0
	*** if "start" is greater than c_str's length, then start = 0
	*** returns (eventually modified) *this object
	*** Version 1.2: Added this assign overload
	*/
	string_base<T> &assign(const T *c_str, unsigned start, unsigned count) {
		if (!*c_str || !count) return (*this);
		unsigned l = strlength<T>(c_str);
		if (start > l) start = 0;
		if ((start + count) > l)
			count = (l - start);
		len = count; cap = len + 1;
		delete[] raw_data;
		raw_data = new T[cap];
		memcpy(raw_data, c_str + start, len * sizeof(T));
		raw_data[len] = 0x00;
		return (*this);
	}

	/*
	*** string_base<T> &append(const T *)
	*** append c_str's value to this string
	*** allocates much more memory if needed
	*** does nothing if
		-> first character of c_str's value == 0x00
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimization, using memcpy() now
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &append(const T *c_str) {
		if (!*c_str) return (*this);
		unsigned old = len, l = strlength<T>(c_str);
		if (cap <= (len += l)) {
			cap += (l + STR_ALLOC);
			raw_data = (T *)realloc(raw_data, cap * sizeof(T));
		}
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		memcpy(raw_data + old, c_str, (len - old) * sizeof(T));
		raw_data[len] = 0x00;
		return (*this);
	}
	/*
	*** string_base<T> &append(const T &)
	*** appends ch's value to this string
	*** allocates much more memory if needed
	*** returns (modified) *this object
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &append(const T &ch) {
		if (cap <= (len += 1)) {
			cap += (1 + STR_ALLOC);
			raw_data = (T *)realloc(raw_data, cap * sizeof(T));
		}
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		raw_data[len - 1] = ch;
		raw_data[len] = 0x00;
		return (*this);
	}
	/*
	*** string_base<T> &append(const T &, unsigned)
	*** appends ch's value "count" times to this string
	*** allocates much more memory if needed
	*** does nothing if
		-> count == 0
	*** returns (eventually modified) *this object
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &append(const T &ch, unsigned count) {
		if (!count) return (*this);
		unsigned o = len;
		if (cap <= (len += count)) {
			cap += (count + STR_ALLOC);
			raw_data = (T *)realloc(raw_data, cap * sizeof(T));
		}
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		for (unsigned i = o; i < (o + count); i++)
			raw_data[i] = ch;
		raw_data[len] = 0x00;
		return (*this);
	}
	/*
	*** string_base<T> &append(const string_base<T> &)
	*** appends str's value to this string
	*** allocates much more memory if needed
	*** does nothing if
		-> str's length == 0
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimization, using memcpy() now
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &append(const string_base<T> &str) {
		if (!str.len) return (*this);
		if (this == &str) {
			append(string_base<T>(str));
			return (*this);
		}
		unsigned o = len, l = str.len;
		if (cap <= (len += l)) {
			cap += (l + STR_ALLOC);
			raw_data = (T *)realloc(raw_data, cap * sizeof(T));
		}
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		memcpy(raw_data + o, str.raw_data, (len - o) * sizeof(T));
		raw_data[len] = 0x00;
		return (*this);
	}

	/*
	*** string_base<T> &append(const T *, unsigned)
	*** appends the first "count" characters of c_str's value to this string
	*** allocates much more memory if needed
	*** does nothing if
		-> first character of "c_str" == 0x00
		-> count == 0
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimization, using memcpy() now
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &append(const T *c_str, unsigned count) {
		if (!*c_str || !count) return (*this);
		unsigned ac_len = MIN(count, strlength<T>(c_str));
		unsigned o = len;
		if (cap <= (len += ac_len)) {
			cap += (ac_len + STR_ALLOC);
			raw_data = (T *)realloc(raw_data, cap * sizeof(T));
		}
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		memcpy(raw_data + o, c_str, ac_len * sizeof(T));
		raw_data[len] = 0x00;
		return (*this);
	}
	/*
	*** string_base<T> &append(const T *, unsigned, unsigned)
	*** appends a substring of "c_str" (starting at position
	"start" with a length of "count") to this string
	*** allocates much more memory if needed
	*** does nothing if
		-> substring's length ("count") == 0
		-> first character of c_str's value == 0x00
	*** if "start" is greater than c_str's length, then start = 0
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimization, using memcpy() now
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &append(const T *c_str, unsigned start, unsigned count) {
		if (!count || !*c_str) return (*this);
		unsigned l = strlength<T>(c_str);
		if (start > l) start = 0;
		if ((start + count) > l)
			count = (l - start);
		if (cap <= (len += count)) {
			cap += (count + STR_ALLOC);
			raw_data = (T *)realloc(raw_data, cap * sizeof(T));
		}
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		memcpy(raw_data, c_str + start, count * sizeof(T));
		raw_data[len] = 0x00;
		return (*this);
	}
	/*
	*** string_base<T> &append(const string_base<T> &, unsigned, unsigned)
	*** appends a substring of "str" (starting at position "start" with a
	length of "count") to this string
	*** allocates much more memory if needed
	*** does nothing if
		-> substring's length ("count") == 0
		-> str's length == 0
	*** if "start" is greater than str's length, then start = 0
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimization, using memcpy() now
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &append(const string_base<T> &str, unsigned start, unsigned count) {
		if (!count || !str.len) return (*this);
		if (this == &str) {
			append(string_base<T>(str), start, count);
			return (*this);
		}
		unsigned l = str.len;
		if (start > l) start = 0;
		if ((start + count) > l)
			count = (l - start);
		if (cap <= (len += count)) {
			cap += (count + STR_ALLOC);
			raw_data = (T *)realloc(raw_data, cap * sizeof(T));
		}
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		memcpy(raw_data, str.raw_data + start, count * sizeof(T));
		raw_data[len] = 0x00;
		return (*this);
	}

	/*
	*** string_base<T> &insert(const string_base<T> &, unsigned)
	*** insert str's value at position "pos" in this string
	*** allocates much more memory if needed
	*** does nothing if
		-> Given "pos" is greater than current value's length (this->length())
		-> str's length == 0
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimizing, using memmove() and memcpy() now
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &insert(const string_base<T> &str, unsigned pos) {
		if (pos > len || !str.len) return (*this);
		if (this == &str) {
			insert(string_base<T>(str), pos);
			return (*this);
		}
		if (cap <= (len += str.len)) {
			cap += (str.len + STR_ALLOC);
			raw_data = (T *)realloc(raw_data, cap * sizeof(T));
		}
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		memmove(raw_data + (pos + str.len), raw_data + pos, (len - str.len - pos) * sizeof(T));
		memcpy(raw_data + pos, str.raw_data, str.len * sizeof(T));
		raw_data[len] = 0x00;
		return (*this);
	}
	/*
	*** string_base<T> &insert(const string_base<T> &, unsigned, unsigned, unsigned)
	*** inserts a substring of "str" (starting at position "start" with a length of
	"count") at position "pos" in this string
	*** allocates much more memory if needed
	*** does nothing if
		-> Given "pos" is greater than current value's length (this->length())
		-> substring's length ("count") == 0
	*** if "start" is greater than str's length, then start = 0
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimizing, using memmove() and memcpy() now
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &insert(const string_base<T> &str, unsigned pos, unsigned start, unsigned count) {
		if (pos > len || !count) return (*this);
		if (this == &str) {
			insert(string_base<T>(str), pos, start, count);
			return (*this);
		}
		unsigned l = str.len;
		if (start > l) start = 0;
		if ((start + count) > l)
			count = (l - start);
		if (cap <= (len += count)) {
			cap += (count + STR_ALLOC);
			raw_data = (T *)realloc(raw_data, cap * sizeof(T));
		}
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		memmove(raw_data + (pos + count), raw_data + pos, (len - count - pos) * sizeof(T));
		memcpy(raw_data + pos, str.raw_data + start, count * sizeof(T));
		raw_data[len] = 0x00;
		return (*this);
	}

	/*
	*** string_base<T> &insert(const string_base<T> &, unsigned, unsigned)
	*** inserts the first "count" characters of str's value at position "pos" in this string
	*** allocates much more memory if needed
	*** does nothing if
		-> Given "pos" is greater than current value's length (this->length())
		-> str's length == 0
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimizing, using memmove() and memcpy() now
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &insert(const string_base<T> &str, unsigned pos, unsigned count) {
		if (pos > len || !str.len)
			return (*this);
		if (this == &str) {
			insert(string_base<T>(str), pos, count);
			return (*this);
		}
		if (count > str.len)
			count -= (count - str.len);
		if (cap <= (len += count)) {
			cap += (count + STR_ALLOC);
			raw_data = (T *)realloc(raw_data, cap * sizeof(T));
		}
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		memmove(raw_data + (pos + count), raw_data + pos, (len - count - pos) * sizeof(T));
		memcpy(raw_data + pos, str.raw_data, count * sizeof(T));
		raw_data[len] = 0x00;
		return (*this);
	}

	/*
	*** string_base<T> &insert(const T *, unsigned)
	*** inserts c_str's value at position "pos" in this string
	*** allocates much more memory if needed
	*** does nothing if
		-> Given "pos" is greater than current value's length (this->length())
		-> first character of c_str's value == 0x00
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimizing, using memmove() and memcpy() now
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &insert(const T *c_str, unsigned pos) {
		if (pos > len || !*c_str) return (*this);
		unsigned l = strlength<T>(c_str);
		if (cap <= (len += l)) {
			cap += (l + STR_ALLOC);
			raw_data = (T *)realloc(raw_data, cap * sizeof(T));
		}
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		memmove(raw_data + (pos + l), raw_data + pos, (len - l - pos) * sizeof(T));
		memcpy(raw_data + pos, c_str, l * sizeof(T));
		raw_data[len] = 0x00;
		return (*this);
	}
	/*
	*** string_base<T> &insert(const T *, unsigned, unsigned, unsigned)
	*** inserts a substring of "c_str" (starting at position "start" with a
	length of "count") at position "pos" in this string
	*** allocates much more memory if needed
	*** does nothing if
		-> Given "pos" is greater than current value's length (this->length())
		-> substring's length ("count") == 0
		-> first character of c_str's value == 0x00
	*** if "start" is greater than str's length, then start = 0
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimizing, using memmove() and memcpy() now
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &insert(const T *c_str, unsigned pos, unsigned start, unsigned count) {
		if (pos > len || !count || !*c_str) return (*this);
		unsigned l = strlength<T>(c_str);
		if (start > count) start = 0;
		if ((start + count) > l)
			count = (l - start);
		if (cap <= (len += count)) {
			cap += (count + STR_ALLOC);
			raw_data = (T *)realloc(raw_data, cap * sizeof(T));
		}
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		memmove(raw_data + (pos + count), raw_data + pos, (len - count - pos) * sizeof(T));
		memcpy(raw_data + pos, c_str + start, count * sizeof(T));
		raw_data[len] = 0x00;
		return (*this);
	}
	/*
	*** string_base<T> &insert(const T *, unsigned, unsigned)
	*** inserts the first "count" chars of c_str's value at position "pos" in this string
	*** allocates much more memory if needed
	*** does nothing if
		-> Given "pos" is greater than current value's length (this->length())
		-> substring's length ("count") == 0
		-> first character of c_str's value == 0x00
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimizing, using memmove() and memcpy() now
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &insert(const T *c_str, unsigned pos, unsigned count) {
		if (pos > len || !count || !*c_str)
			return (*this);
		unsigned l = strlength<T>(c_str);
		if (count > l) count -= (count - l);
		if (cap <= (len += count)) {
			cap += (count + STR_ALLOC);
			raw_data = (T *)realloc(raw_data, cap * sizeof(T));
		}
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		memmove(raw_data + (pos + count), raw_data + pos, (len - count - pos) * sizeof(T));
		memcpy(raw_data + pos, c_str, count * sizeof(T));
		raw_data[len] = 0x00;
		return (*this);
	}
	/*
	*** string_base<T> &insert(const T &, unsigned, count)
	*** inserts ch's value "count" times at position "pos" in this string
	*** allocates much more memory if needed
	*** does nothing if
		-> Given "pos" is greater than current value's length (this->length())
		-> count == 0
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimizing, using memmove() now
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &insert(const T &ch, unsigned pos, unsigned count) {
		if (pos > len || !count)
			return (*this);
		if (cap <= (len += count)) {
			cap += (count + STR_ALLOC);
			raw_data = (T *)realloc(raw_data, cap * sizeof(T));
		}
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		memmove(raw_data + (pos + count), raw_data + pos, (len - count - pos) * sizeof(T));
		for (unsigned i = pos; i < (pos + count); i++)
			raw_data[i] = ch;
		raw_data[len] = 0x00;
		return (*this);
	}
	/*
	*** string_base<T> &insert(const T &, unsigned)
	*** inserts ch's value at position "pos" in this string
	*** allocates much more memory if needed
	*** does nothing if
		-> Given "pos" is greater than current value's length (this->length())
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimization, using memmove() now
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	string_base<T> &insert(const T &ch, unsigned pos) {
		if (pos > len) return (*this);
		if (cap <= (len += 1)) {
			cap += (1 + STR_ALLOC);
			raw_data = (T *)realloc(raw_data, (cap * sizeof(T)));
		}
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		memmove(raw_data + (pos + 1), raw_data + pos, (len - 1 - pos) * sizeof(T));
		raw_data[pos] = ch;
		raw_data[len] = 0x00;
		return (*this);
	}

	/*
	*** string_base<T> &erase(unsigned)
	*** erases character at position "pos" from this string
	*** performs no reallocation, doesn't touch capacity but decrements size by 1
	*** does nothing if
		-> Given "pos" is greater than highest index of current string value
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimizing, using memmove() now
	*/
	string_base<T> &erase(unsigned pos) {
		if (pos > (len - 1) || !len) return (*this);
		memmove(raw_data + pos, raw_data + pos + 1, (len - pos) * sizeof(T));
		--this->len;
		raw_data[len] = 0x00;
		return (*this);
	}
	/*
	*** string_base<T> &erase(unsigned, unsigned)
	*** erases a sequence of characters (starting at position "pos" with a length of "count") from this string
	*** performs no reallocation, doesn't touch capacity but decrements size by "count"
	*** does nothing if
		-> Given "pos" is greater than highest index of current string value
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimizing, using memmove() now
	*/
	string_base<T> &erase(unsigned pos, unsigned count) {
		if (pos > (len - 1) || !len) return (*this);
		if ((pos + count) > len) count = (len - pos);
		memmove(raw_data + pos, raw_data + pos + count, (len - count - pos) * sizeof(T));
		this->len -= count;
		raw_data[len] = 0x00;
		return (*this);
	}

	/*
	*** string_base<T> &substr(unsigned, unsigned)
	*** generates a substring of current string value (starting at position "pos" with a length of "count")
	*** performs a reallocation, changes size and capacity
	*** does nothing if
		-> Given "pos" is greater than highest index of current string value
		-> substring's length ("count") == 0
	*** if "start" is greater than str's length, then start = 0
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimizing, using memcpy() now
	*/
	string_base<T> &substr(unsigned start, unsigned count) {
		if (start > (len - 1) || !count) return (*this);
		if ((start + count) > len)
			count = (len - start);
		T *new_data = new T[cap];
		memcpy(new_data, raw_data + start, count * sizeof(T));
		this->len = count;
		new_data[len] = 0x00;
		delete[] raw_data;
		raw_data = new_data;
		return (*this);
	}

	/*
	*** string_base<T> &remove(const T &)
	*** removes a specified character value represented by "ch" from whole string
	*** performs a reallocation, changes size and capacity
	*** does nothing if
		-> current string's length == 0
		-> ch's value == 0x00 (null-terminator)
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimizing, using another remove() overload now
	*/
	string_base<T> &remove(const T &ch) { remove((T)ch, 0, len); return (*this); }
	/*
	*** string_base<T> &remove(const T &, unsigned, unsigned)
	*** removes a specified character represented by ch's value
	from a specified range within this string value
	(starting with position "start" with a length of "count")
	*** performs a reallocation, changes size but not capacity
	*** does nothing if
		-> current string's length == 0
		-> substring's length ("count") == 0
		-> ch's value == 0x00 (null-terminator)
		-> Given "pos" is greater than highest index of current string value
	*** returns (eventually modified) *this object
	*** Version 1.2: Optimizing, using memmove() now
	*/
	string_base<T> &remove(const T &ch, unsigned start, unsigned count) {
		if (!len || !count || !ch || start > (len - 1))
			return (*this);
		unsigned rem_c = 0;
		if ((start + count) > len)
			count = (len - start);
		for (unsigned i = start; i < (start + count); i++)
			if (raw_data[i] == ch) {
				memmove(raw_data + i, raw_data + i + 1, (len - i) * sizeof(T));
				++rem_c;
			}
		this->len -= rem_c;
		raw_data[len] = 0x00;
		return (*this);
	}

	string_base<T> &remove_leading() { while (*cbegin() == ' ') pop_front(); return (*this); }
	string_base<T> &remove_trailing() { while (*(cend() - 1) == ' ') pop_back(); return (*this); }
	string_base<T> &remove_middle() { while (find("  ") != length()) erase(find("  "), 1); return (*this); }
	string_base<T> &whitespaces() { remove_leading(); remove_middle(); remove_trailing(); return (*this); }
	string_base<T> &backspaces()
	{
		unsigned int i = 0;
		while ((i = find('\b', 0)) != length())
		{
			erase(i - 1, 2);
		}
		return (*this);
	}

	/*
	*********************************************************************
	*** string comparison table										  ***
	*********************************************************************

	compared string = *this

	* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
	*     * Either the value of the first character that does not		*
	* <0  * match is lower in the compared string, or all compared		*
	*     * characters match but the compared string is shorter.		*
	* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
	*     *																*
	* =0  * They compare equal (so str1 == str2).						*
	*     *																*
	* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
	*     * Either the value of the first character that does not		*
	* >0  * match is greater in the compared string, or all compared	*
	*     * characters match but the compared string is longer.			*
	* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
	* from: http://www.cplusplus.com/reference/string/string/compare/	*
	* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
	*/

	/* from: https://stackoverflow.com/questions/34873209/implementation-of-strcmp/34873406 */
	/*
	*** int compare(const string_base<T> &) const
	*** compares current string value with str's value
	*** for return value, please look at the "string comparison table" table above
	*/
	int compare(const string_base<T> &str) const {
		if (this == &str) return 0;
		unsigned i = 0;
		while (raw_data[i] && raw_data[i] == str.raw_data[i]) ++i;
		return raw_data[i] - str.raw_data[i];
	}
	/*
	*** int compare(const string_base<T> &, unsigned, unsigned) const
	*** compares current string value with a substring of str's value
	starting at position "start" with a length of "count"
	*** for return value, please look at the "string comparison table" table above
	*/
	int compare(const string_base<T> &str, unsigned start, unsigned count) const {
		if (this == &str && !start &&
			count == str.len) return 0;
		unsigned i = 0;
		while (raw_data[i] && raw_data[i] == str.raw_data[start + i] && count)
			--count, ++i;
		return raw_data[i] - str.raw_data[start + i];
	}
	/*
	*** int compare(const string_base<T> &, unsigned, unsigned, unsigned, unsigned) const
	*** compares a substring of current string with a substring of str's value
	*** parameters:
		-> str			- other string value which substring is compared with a substring of current value
		-> s1pos		- starting position of current value's substring
		-> s1count		- length of current value's substring
		-> s2pos		- starting position of str's substring
		-> s2count		- length of str's substring
	*** for return value, please look at the "string comparison table" table above
	*/
	int compare(const string_base<T> &str, unsigned s1pos, unsigned s1count, unsigned s2pos, unsigned s2count) const {
		if (this == &str && !s1pos && !s2pos &&
			s1count == len && s2count == str.len)
			return 0;
		unsigned i = 0;
		while (raw_data[s1pos + i] && raw_data[s1pos + i] == str.raw_data[s2pos + i] && s1count && s2count)
			++i, --s1count, --s2count;
		return raw_data[s1pos + i] - str.raw_data[s2pos + i];
	}

	/*
	*** int compare(const T *) const
	*** compares current string value with c_str's value
	*** for return value, please look at the "string comparison table" table above
	*/
	int compare(const T *c_str) const {
		unsigned i = 0;
		while (raw_data[i] && raw_data[i] == c_str[i]) ++i;
		return raw_data[i] - c_str[i];
	}
	/*
	*** int compare(const T *, unsigned, unsigned) const
	*** compares current string value with a substring of c_str's value
	starting at position "start" with a length of "count"
	*** for return value, please look at the "string comparison table" table above
	*/
	int compare(const T *c_str, unsigned start, unsigned count) const {
		unsigned i = 0;
		while (raw_data[i] && raw_data[i] == c_str[start + i] && count)
			--count, ++i;
		return raw_data[i] - c_str[start + i];
	}
	/*
	*** int compare(const T *, unsigned) const
	*** compares the first "count" characters of current string value
	with the first "count" characters of c_str's value
	*** for return value, please look at the "string comparison table" table above
	*/
	int compare(const T *c_str, unsigned count) const {
		unsigned i = 0;
		while (raw_data[i] && raw_data[i] == c_str[i] && count)
			--count, ++i;
		return raw_data[i] - c_str[i];
	}
	/*
	*** int compare(const T *, unsigned, unsigned, unsigned, unsigned) const
	*** compares a substring of current string value with a substring of c_str's value
	*** parameters:
		-> c_str		- C-String which substring is compared with a substring of current value
		-> s1pos		- starting position of current value's substring
		-> s1count		- length of current value's substring
		-> s2pos		- starting position of c_str's substring
		-> s2count		- length of c_str's substring
	*** for return value, please look at the "string comparison table" table above
	*/
	int compare(const T *c_str, unsigned s1pos, unsigned s1count, unsigned s2pos, unsigned s2count) const {
		unsigned i = 0;
		while (raw_data[s1pos + i] && raw_data[s1pos + i] == c_str[s2pos + i] && s1count && s2count)
			++i, --s1count, --s2count;
		return raw_data[s1pos + i] - c_str[s2pos + i];
	}

	/*
	*** unsigned find(const string_base<T> &, unsigned = 0U)
	*** returns position of the first occurrence of needle's value in current string value
	*** "pos" is the first position which should be considered as the beginning of the seeked string
	*** does nothing if
		-> current string's length == 0
		-> current string's length is less than needle's length
		-> needle's length == 0
	*** returns position of first occurence of needle's value or string's length if it has not been found
	*** implementation is based on this article: https://stackoverflow.com/questions/12784766/check-substring-exists-in-a-string-in-c
	*/
	unsigned find(const string_base<T> &needle, unsigned pos = 0U) {
		unsigned counter = 0, ctr = pos, l = needle.len;
		if (len < 1 || len < l || l < 1) return len;
		while (ctr <= (len - l) && l > 0) {
			if ((raw_data[ctr]) == needle.raw_data[0]) {
				counter = 0;
				for (unsigned count = ctr; count < (ctr + l); count++) {
					if (raw_data[count] == needle.raw_data[counter])
						++counter;
					else { counter = 0; break; }
				}
				if (counter == l)
					return ctr;
			} ++ctr;
		}
		return len;
	}

	/*
	*** unsigned find(const T *, unsigned = 0U)
	*** returns position of the first occurrence of needle's value in current string value
	*** "pos" is the first position which should be considered as the beginning of the seeked string
	*** does nothing if
		-> current string's length == 0
		-> current string's length is less than needle's length
		-> needle's length == 0
	*** returns position of first occurrence of needle's value or string's length if it has not been found
	*/
	unsigned find(const T *needle, unsigned pos = 0U) {
		unsigned counter = 0, ctr = pos, l = strlength<T>(needle);
		if (len < 1 || len < l || l < 1) return len;
		while (ctr <= (len - l) && l > 0) {
			if ((raw_data[ctr]) == needle[0]) {
				counter = 0;
				for (unsigned count = ctr; count < (ctr + l); count++) {
					if (raw_data[count] == needle[counter])
						++counter;
					else { counter = 0; break; }
				}
				if (counter == l)
					return ctr;
			} ++ctr;
		}
		return len;
	}
	/*
	*** unsigned find(const T *, unsigned, unsigned)
	*** returns position of the first occurrence of needle's
	value in a substring of current string value starting with
	position "start" with a length of "count"
	*** does nothing if
		-> current string's length == 0
		-> current string's length is less than needle's length
		-> needle's length == 0
	*** returns position of needle's start within that substring,
	or string's length if it has not been found within the substring
	*/
	unsigned find(const T *needle, unsigned pos, unsigned count) {
		unsigned counter = 0, ctr = pos, l = strlength<T>(needle);
		if (len < 1 || len < l || l < 1) return len;
		while (ctr <= (len - l) && l > 0 && count) {
			if ((raw_data[ctr]) == needle[0]) {
				counter = 0;
				for (unsigned count = ctr; count < (ctr + l); count++) {
					if (raw_data[count] == needle[counter])
						++counter;
					else { counter = 0; break; }
				}
				if (counter == l)
					return ctr;
			} ++ctr, --count;
		}
		return len;
	}
	/*
	*** unsigned find(const T &, unsigned = 0U)
	*** returns position of the first occurrence of needle's value in current string value
	*** "pos" is the first position which should be considered as the seeked character
	*** returns position of first occurrence, or string's length if it has not been found
	*/
	unsigned find(const T &needle, unsigned pos = 0U) {
		for (unsigned i = pos; i < len; i++)
			if (raw_data[i] == needle)
				return i;
		return len;
	}
	/*
	*** unsigned find(const T &, unsigned, unsigned)
	*** returns position of the first occurrence of needle's
	value in a substring of current string value starting with
	position "start" with a length of "count"
	*** returns string's length if
		-> needle's value has not been found within that substring
		-> string's length is less than ("pos" + "count")
	*** if it has been found, it returns position of its first occurrence
	*/
	unsigned find(const T &needle, unsigned pos, unsigned count) {
		if (len < (pos + count)) return len;
		for (unsigned i = pos; i < (pos + count); i++)
			if (raw_data[i] == needle)
				return i;
		return len;
	}

	/*
	*** string_base<T> &replace(unsigned, const T &)
	*** replaces character at position "pos" with replace's value
	*** does nothing if
		-> Given "pos" is greater than highest index of current string value
	*** returns (eventually modified) *this object
	*/
	string_base<T> &replace(unsigned pos, const T &replace) {
		if (pos > (len - 1)) return (*this);
		raw_data[pos] = replace;
		return *this;
	}
	/*
	*** string_base<T> &replace(const string_base<T> &, unsigned, unsigned)
	*** replaces a substring of current string value (starting at position
	"start" with a length of "count") with replace's value
	*** returns (modified) *this object
	*/
	string_base<T> &replace(const string_base<T> &replace, unsigned start, unsigned count) {
		if (this == &replace) {
			this->replace(string_base<T>(replace), start, count);
			return (*this);
		}
		if (start >= len || len < (start + count)) return (*this);
		erase(start, count);
		insert(replace, start);
		return (*this);
	}

	/*
	*** string_base<T> &replace(const T *, unsigned, unsigned)
	*** replaces a substring of current string value (starting at position
	"start" with a length of "count") with replace's value
	*** does nothing if
		-> given "start" is greater or equal to current string's length
		-> current string's length is less than ("start" + "count")
	*** return (eventually modified) *this object
	*/
	string_base<T> &replace(const T *replace, unsigned start, unsigned count) {
		if (start >= len || len < (start + count)) return (*this);
		erase(start, count);
		insert(replace, start);
		return (*this);
	}
	/*
	*** string_base<T> &replace(const string_base<T> &, const string_base<T> &)
	*** replaces element's value in current string (if found in current string value) with replace's value
	*** does nothing if
		-> current string value equals to replace's value
		-> element's value has not been found in current string value
	*** returns (eventually modified) *this object
	*/
	string_base<T> &replace(const string_base<T> &element, const string_base<T> &replace) {
		if (this == &replace) return (*this);
		if (this == &element) {
			this->replace(string_base<T>(element), replace);
			return (*this);
		}
		unsigned pos = find(element, 0);
		if (pos == len) return (*this);
		erase(pos, element.len);
		insert(replace, pos);
		return (*this);
	}

	/*
	*** string_base<T> &replace(const T *, const T *)
	*** replaces element's value in current string (if found in current string value) with replace's value
	*** does nothing if
		-> element's value has not been found in current string value
	*** returns (eventually modified) *this object
	*/
	string_base<T> &replace(const T *element, const T *replace) {
		unsigned pos = find(string_base<T>(element), 0);
		if (pos == len) return (*this);
		erase(pos, strlength<T>(element));
		insert(replace, pos);
		return (*this);
	}
	/*
	*** string_base<T> &replace(const T &, const T &, unsigned)
	*** replaces element's value in current string with replace's value beginning at index "pos" in current string
	*** does nothing if
		-> Given index ("pos") is greater or equals to current string's length
	*** returns (eventually modified) *this object
	*/
	string_base<T> &replace(const T &element, const T &replace, unsigned pos = 0U) {
		if (pos >= len) return (*this);
		for (unsigned i = pos; i < len; i++)
			if (raw_data[i] == element)
				raw_data[i] = replace;
		return (*this);
	}
	/*
	*** string_base<T> &replace(const T &, const T &, unsigned, unsigned)
	*** replaces element's value in current string with replace's value
	in substring of current string starting at position "pos" with a length of "count"
	*** does nothing if
		-> Given index ("pos") is greater or equals to current string's length
		-> current string's length is less than ("start" + "count")
	*** returns (eventually modified) *this object
	*/
	string_base<T> &replace(const T &element, const T &replace, unsigned pos, unsigned count) {
		if (pos >= len || len < (pos + count)) return (*this);
		for (unsigned i = pos; i < (pos + count); i++)
			if (raw_data[i] == element)
				raw_data[i] = replace;
		return (*this);
	}

	void push_back(const T &ch) { append(ch); }									/* append ch's value to current value */
	void push_back(const T *c_str) { append(c_str); }							/* append c_str's value to current value */
	void push_back(const string_base<T> &str) { append(str); }					/* append str's value to current value */

	void push_front(const T &ch) { insert(ch, 0); }								/* insert ch's value at string's front */
	void push_front(const T *c_str) { insert(c_str, 0); }						/* insert c_str's value at string's front */
	void push_front(const string_base<T> &str) { insert(str, 0); }				/* insert str's value at string's front */

	void pop_back(unsigned count = 1) { erase(len - count, count); }			/* delete last "count" characters from string's back */
	void pop_front(unsigned count = 1) { erase(0, count); }						/* delete first "count" characters from string's front */
	/*
	*** void reserve(unsigned)
	*** reallocate string with new size of (cap += count)
	*** doesn't touch data at all
	*** Version 1.2: Add null-terminator at the end of array
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	void reserve(unsigned count = 1) {
		if (!count) return;
		cap += count;
		raw_data = (T *)realloc(raw_data, cap * sizeof(T));
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		raw_data[cap] = 0x00;
	}
	/* resize string to "count", fill possible new spaces with '\0' */
	void resize(unsigned count) { resize(count, (T)'\0'); }
	/*
	*** void resize(unsigned, T)
	*** resize string to a length of "count"
	*** changes capacity to ("count" + 1), performs reallocation
	*** if "count" > str.length(), fill new spaces with "ch"
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	void resize(unsigned count, T ch) {
		if (count == len) return;
		if (count < len) {
			len = count; cap = len + 1;
			raw_data = (T *)realloc(raw_data, cap * sizeof(T));
#ifdef STR_USE_ASSERTIONS
			assert(raw_data != NULL);
#endif
			raw_data[len] = 0x00;
		}
		else if (count > len) {
			unsigned o = len; len = count; cap = len + 1;
			raw_data = (T *)realloc(raw_data, cap * sizeof(T));
#ifdef STR_USE_ASSERTIONS
			assert(raw_data != NULL);
#endif
			for (unsigned i = o; i < count; i++)
				raw_data[i] = ch;
			raw_data[len] = 0x00;
		}
	}
	/*
	*** void swap(string_base<T> &)
	*** exchange contents of *this and "value"
	*** Example:
		string a = "Pear", b = "Apple";
		a.swap(b);
		// a is now "Apple" and b is now "Pear"
	*** performs an assign operation internally
	*** Version 1.2: fix function
	*** Version 1.6: replace cleanup() with delete call
	*/
	void swap(string_base<T> &value) {
		if (this == &value) return;
		string_base<T> *tmp =
			new string_base<T>(value);
		value.assign(*this);
		assign(*tmp);
		delete tmp;
	}
	/*
	*** void reverse()
	*** reverse string from pos 0 to end
	*** from: https://www.geeksforgeeks.org/write-a-program-to-reverse-an-array-or-string/
	*/
	void reverse() { reverse(0, len - 1); }
	/*
	*** void reverse(unsigned, unsigned)
	*** reverses string's data from "start" to "end"
	*** Example:
		string a = "Hello world!";
		a.reverse(0, a.length() - 1);
		// a is now "!dlrow olleH"
	*** doesn't touch size or capacity
	*/
	void reverse(unsigned start, unsigned end) {
		if (start == end || len < 2 ||
			end > (len - 1) ||
			start > (len - 1)) return;
		while (start < end) {
			T tmp = raw_data[start];
			raw_data[start] = raw_data[end];
			raw_data[end] = tmp;
			++start, --end;
		}
	}
	/*
	*** void shrink()
	*** shrink capacity to fit with len
	*** performs a reallocation
	*** doesn't touch string's data
	*** Version 1.6: Added assertions to make debugging the program easier
	*/
	void shrink() {
		if (cap == (len + 1)) return;
		cap = len + 1;
		raw_data = (T *)realloc(raw_data, cap * sizeof(T));
#ifdef STR_USE_ASSERTIONS
		assert(raw_data != NULL);
#endif
		raw_data[len] = 0x00;
	}
	/*
	*** void clear()
	*** clear string (destroy all chars)
	*** doesn't touch capacity, performs a reallocation
	*** sets first char to 0x00
	*/
	void clear() {
		delete[] raw_data;
		raw_data = new T[cap];
		raw_data[0] = 0x00;
		len = 0;
	}
	/*
	*** void reset()
	*** set string's allocation state tback to default
	*** most likely deallocates array and makes it ready for a reuse
	*** All data is lost, so only do it when you really want it
	*/
	void reset() {
		delete[] raw_data;
		raw_data = new T[STR_DEFSTRCAP];
		raw_data[0] = 0x00;
		len = 0; cap = STR_DEFSTRCAP;
	}

#ifdef STR_USE_CLEANUP_FUNCTION
	/*
	*** void cleanup()
	*** custom destructor you can call if you
	want to deallocate the memory used by this string object
	*** this object and all of its members will be invalided after calling this function
	*** All data is lost when calling this
	*** Version 1.1: add this function and remove default destructor
	*** Version 1.5: define STR_USE_CLEANUP_FUNCTION to enable this function
	*/
	void cleanup() {
		delete[] raw_data;
#ifdef STR_CPP11_OR_HIGHER
		/* nullptr is a typesafe alternative to NULL */
		raw_data = nullptr;
#else
		/* C++98 support by avoiding nullptr */
		raw_data = NULL;
#endif
		// delete this;
	}
#endif

	/*
	*********************************************************************************
	*** OPERATOR OVERLOADS														  ***
	*********************************************************************************
	*** The code below defines some functions to convert string to several arithmetic types
	*** Added in Version 1.4
	*** supported types:
		-> long								[stol]
		-> long long (_ll_)					[stoll]
		-> unsigned long (_ul_)				[stoul]
		-> unsigned long long (_ull_)		[stoull]
		-> float							[stof]
		-> double							[stod]
		-> long double (_ld_)				[stold]
	*/

	long stol() const { char *x = new char[len]; for (_u_ i = 0; i < len; i++) x[i] = raw_data[i] & 0x7F; long r = strtol(x, NULL, 10); delete[] x; return r; }			/* -> long */
	_ll_ stoll() const { char *x = new char[len]; for (_u_ i = 0; i < len; i++) x[i] = raw_data[i] & 0x7F; _ll_ r = strtoll(x, NULL, 10); delete[] x; return r; }			/* -> long long */
	_ul_ stoul() const { char *x = new char[len]; for (_u_ i = 0; i < len; i++) x[i] = raw_data[i] & 0x7F; _ul_ r = strtoul(x, NULL, 10); delete[] x; return r; }			/* -> unsigned long */
	_ull_ stoull() const { char *x = new char[len]; for (_u_ i = 0; i < len; i++) x[i] = raw_data[i] & 0x7F; _ull_ r = strtoull(x, NULL, 10); delete[] x; return r; }		/* -> unsigned long long */
	// float stof() const { char *x = new char[len]; for (_u_ i = 0; i < len; i++) x[i] = raw_data[i] & 0x7F; float r = strtof(x, NULL); delete[] x; return r; }				/* -> float */
	// double stod() const { char *x = new char[len]; for (_u_ i = 0; i < len; i++) x[i] = raw_data[i] & 0x7F; double r = strtod(x, NULL); delete[] x; return r; }			/* -> double */
	// _ld_ stold() const { char *x = new char[len]; for (_u_ i = 0; i < len; i++) x[i] = raw_data[i] & 0x7F; _ld_ r = strtold(x, NULL); delete[] x; return r; }				/* -> long double */

	/*
	*********************************************************************************
	*** OPERATOR OVERLOADS														  ***
	*********************************************************************************
	*** Code below overloads some operators to create some pretty shortcuts of some
	function calls
	*/

	/*
	*** T &operator [](unsigned)
	*** return a reference to the char which is at position "pos"
	*** usage:
	***		string a = "Hello world!";
	***		a[0] = 'Y';						-> change first char of "a" to 'Y'
	*** can be used to output the character at position "pos" as well
	*/
	T &operator [](unsigned pos) { return raw_data[(pos > (len - 1) ? (len - 1) : pos)]; }
	/*
	*** const T &operator [](unsigned) const
	*** return a reference to the char which is at position "pos"
	*** can be used to output the character at position "pos" as well
	*** if current string value is const-qualified, it returns this function when
	"operator[]" is called outside of the class
	*/
	const T &operator [](unsigned pos) const { return raw_data[(pos > (len - 1) ? (len - 1) : pos)]; }

	string_base<T> &operator =(const T &ch) { return assign(ch, 1); }					/* assign ch's value to current string value */
	string_base<T> &operator =(const T *c_str) { return assign(c_str); }				/* assign c_str's value to current string value */
	string_base<T> &operator =(const string_base<T> &str) { return assign(str); }		/* assign a str's value to current string value */

	friend string_base<T> operator +(const string_base<T> &str, const char *str2) { return string_base<T>(str) += str2; } /* create copy of current string and append second string to it */
	friend string_base<T> operator +(const string_base<T> &str, const string_base<T> &str2) { return string_base<T>(str) += str2; } /* create copy of current string and append second string to it */
	string_base<T> &operator +=(const T &ch) { return append(ch); }						/* append (concatenate) ch's value to current string value */
	string_base<T> &operator +=(const T *c_str) { return append(c_str); }				/* append c_str's value to current string value */
	string_base<T> &operator +=(const string_base<T> &str) { return append(str); }		/* append str's value to current string value */

	string_base<T> &operator -=(const T &ch) { return remove(ch); }						/* remove all characters which equal to ch's value from current string */

	string_base<T> &operator ~() { reverse(); return *this; }							/* operator overload to reverse string */

	bool operator ==(const T *c_str) const { return compare(c_str) == 0; }					/* check whether *this == c_str */
	bool operator ==(const string_base<T> &right) const { return compare(right) == 0; }		/* check whether *this == right */

	bool operator !=(const T *c_str) { return compare(c_str) != 0; }					/* check whether *this != c_str */
	bool operator !=(const string_base<T> &right) { return compare(right) != 0; }		/* check whether *this != right */

	/*
	*********************************************************************
	*** string comparison table										  ***
	*********************************************************************

	compared string = *this

	* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
	*     * Either the value of the first character that does not		*
	* <0  * match is lower in the compared string, or all compared		*
	*     * characters match but the compared string is shorter.		*
	* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
	*     *																*
	* =0  * They compare equal (so str1 == str2).						*
	*     *																*
	* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
	*     * Either the value of the first character that does not		*
	* >0  * match is greater in the compared string, or all compared	*
	*     * characters match but the compared string is longer.			*
	* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
	* from: http://www.cplusplus.com/reference/string/string/compare/	*
	* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
	*/

	inline bool operator >(const T *c_str) { return compare(c_str) > 0; }				/* check whether *this > c_str */
	inline bool operator <(const T *c_str) { return compare(c_str) < 0; }				/* check whether *this < c_str */
	inline bool operator >=(const T *c_str) { return compare(c_str) >= 0; }				/* check whether *this >= c_str */
	inline bool operator <=(const T *c_str) { return compare(c_str) <= 0; }				/* check whether *this <= c_str */

	inline bool operator >(const string_base<T> &str) { return compare(str) > 0; }		/* check whether *this > str */
	inline bool operator <(const string_base<T> &str) { return compare(str) < 0; }		/* check whether *this < str */
	inline bool operator >=(const string_base<T> &str) { return compare(str) >= 0; }	/* check whether *this >= str */
	inline bool operator <=(const string_base<T> &str) { return compare(str) <= 0; }	/* check whether *this <= str */

	/* end of public segment */
private:
	T *raw_data;		/* actual character (std::string) storage */
	unsigned len, cap;	/* len = length of string / cap = capacity of string */

	/*
	*** private function which is used to get the
	length of str's C-String value with value_type "T_"
	*** only used within the class, not outside
	(and also not reachable from outside the class)
	*/
	template <typename T_>
	static unsigned strlength(const T_ *c_str) {
		/* return 0 if first char is null-terminator */
		if (!*c_str) return 0;
		unsigned res = 0;
		for (; c_str[res] != 0x00; res++);
		return res;
	}

	/*
	*** private function to fill string buffer
	*** (uses a memset-like algorithm)
	*** works with every template type
	(memset has problems with char32_t,
	therefore I made a similar function. It
	is currently unused but may be used for other
	functions in the future)
	*** returns dest (parameter 1)
	*/
	template <typename T__>
	static T__ *fill_str(T__ *dest, const T__ &val, _u_ len) {
		const T__ *p = dest;
		while (len-- > 0) *p++ = val;
		return dest;
	}
};

using string = string_base<char>;
using wstring = string_base<wchar_t>;
using string16 = string_base<char16_t>;
using string32 = string_base<char32_t>;

namespace std
{
    using string = ::string;
    using wstring = ::wstring;
    using string16 = ::string16;
    using string32 = ::string32;
}

class printer
{
	private:
	lock_t lock;
	int (*func)(const char *, ...);

	public:
	printer(int (*func)(const char *, ...));

	printer &operator<<(std::string str);
	printer &operator<<(std::string *str);
	printer &operator<<(size_t i);
	printer &operator<<(int i);
	printer &operator<<(unsigned int i);
};

extern printer cout;
extern printer coutl;
extern printer coutw;
extern printer coute;

static inline char *operator"" _c(const char *str, unsigned long length)
{
    return const_cast<char*>(str);
}