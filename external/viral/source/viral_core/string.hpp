/**
 *************************************************************************
 *
 * @file string.hpp
 *
 * String utilities header file.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__STRING_HPP)
#define INCLUDED__VIRAL_CORE__STRING_HPP


#include "build_config.hpp"


#include "container_forward.hpp"
#include "string_forward.hpp"


#include "memory.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup string_module String Module
 *
 * Provides basic string classes and associated utility functions,
 * such as string decoration for HTML output.
 *
 ************************************************************************/


/**
 *************************************************************************
 *
 * @class string_buffer
 * @ingroup string_module
 *
 * Manages @ref string memory.
 *
 * Implementation notes:
 * - A string_buffer bypasses the heap in favour of literal constants
 *		for empty strings to enable @ref NOTHROW default constructors
 *		and @ref NOTHROW move operations.
 * - To ensure constant data() pointers when shrinking a string_buffer,
 *		string buffers do not revert to bypassing after using the heap
 *		unless clients explicity free() all heap memory.
 * - String buffers internally handle the terminating \\0 for @ref string
 *		(e.g. by adding \\0 on resize() and by ignoring \\0 in size()).
 *
 ************************************************************************/
class string_buffer
{

	public:

		string_buffer() NOTHROW;
		string_buffer(string_buffer&& other) NOTHROW;

		~string_buffer() NOTHROW;

		void reserve(int64 request_capacity);
		void resize(int64 new_size);

		int64 size() const NOTHROW;
		int64 capacity() const NOTHROW;

		const char* data() const NOTHROW;
		char* data() NOTHROW;

		const char& operator [](int64 index) const NOTHROW;
		char& operator [](int64 index) NOTHROW;

		const char& at(int64 index) const;
		char& at(int64 index);

		void swap_with(string_buffer& other) NOTHROW;

		void clear() NOTHROW;
		void free() NOTHROW;


	private:

		static char* default_empty_literal_;

		char* data_;
		int64 size_;
		int64 capacity_;
};




/**
 *************************************************************************
 *
 * @class string
 * @ingroup string_module
 *
 * High-level wrapper for C-style character arrays,
 * similar in functionality to std::string.
 *
 * Usage notes:
 * - All strings are implicitly \\0-terminated and are safe for use
 *		with all functions that accept conventional const char*.
 * - The implicit terminating \\0 is not considered to be a character
 *		of any (stored or input C-style) string and is not counted
 *		in any character count or string length. However, reading
 *		the terminating \\0 (e.g. with at() or data()) is valid.
 * - A string may contain additional \\0 characters that behave
 *		like regular characters.
 *
 ************************************************************************/
class string
{

	public:

		/** Create empty string for the C character literal "". */
		string() NOTHROW;


		/**
		 * Copy or move @bref{source} string.
		 * Moving leaves an empty() @bref{source}.
		 */
		//@{
		string(const string& source);
		string(string&& source) NOTHROW;
		//@}


		/** Create copy from fixed-length @bref{source} string. */
		template <int64 N>
			string(const fix_string<N>& source)
				{ set(source.data(), source.size()); }


		/**
		 * Create copy of the C-style \\0-terminated @bref{nt_string}.
		 * A @NULL @bref{nt_string} is interpreted as "".
		 */
		string(const char* nt_string);


		/**
		 * Copy @bref{count} characters from @bref{source} string.
		 * A @NULL @bref{source} is interpreted as "" and accepted
		 * only if @bref{count} is 0.
		 */
		string(const char* source, int64 count);


		/** Create single-character string. */
		string(char ascii);


		/** Create string from some integral @bref{value}. */
		//@{
		string(int32 value);
		string(uint32 value);
		string(int64 value);
		string(uint64 value);
		//@}


		/**
		 * Create string from some floating point @bref{value}.
		 *
		 * An explicit @bref{precision} gives the maximum number
		 * of significant digits in the result string, by default 
		 * @bref{float_to_string_precision_default}. A precision
		 * of at least @bref{float_to_string_precision_roundtrip}
		 * enables exact roundtrip conversions.
		 *
		 * NAN and INF values map to "(+/-)NAN" and "(+/-)INF".
		 * Thus, roundtrip conversions discard NAN payload bits.
		 */
		//@{
		string(float value, int64 precision);
		string(float value);
		//@}


		/** Create on-heap clone of input string. */
		auto_pointer<string> clone() const;


		/**
		 * Ensure that at least @bref{capacity} characters fit into
		 * the string without intermediate dynamical allocation.
		 */
		void reserve(int64 capacity);


		/**
		 * Reallocate string to given @bref{size}, in characters.
		 * The string grows or shrinks at the end, any new contents
		 * are undefined (except for the non-counted terminating \\0).
		 * The call does not throw if at least @bref{size} characters
		 * have already been reserve()d.
		 */
		void resize(int64 size);


		/** Retrieve string length, in characters. */
		int64 size() const NOTHROW;


		/**
		 * Find length in characters of any \\0-terminated @bref{nt_string}.
		 * A @NULL @bref{nt_string} has a length of 0.
		 *
		 * @attent Excessively long strings may return clamped results.
		 * Clients should not rely on a \\0 after the counted characters.
		 */
		static int64 size(const char *nt_string) NOTHROW;


		/**
		 * Fetch reserve()d capacity.
		 * A valid resize() up to capacity() does not throw.
		 */
		int64 capacity() const NOTHROW;


		/**
		 * Test if string contains at least a single character
		 * except for the terminating \\0.
		 */
		bool empty() const NOTHROW;


		/**
		 * Replace string with the first @bref{count} characters
		 * from @bref{array}. If @bref{array} is @NULL, @bref{count}
		 * must be 0, and an empty string "" is applied.
		 */
		void set(const char *array, int64 count);


		/**
		 * Return the contained character array.
		 * @attent In the non-const version, only existing,
		 * valid characters in the string must be modified.
		 */
		//@{
		const char* data() const NOTHROW;
		char* data() NOTHROW;
		//@}


		/** Unchecked character read/write access. */
		//@{
		const char& operator [](int64 index) const NOTHROW;
		char& operator [](int64 index) NOTHROW;
		//@}


		/**
		 * Checked character access, throws on invalid @bref{index}.
		 * Does not throw when accessing the terminating \0.
		 */
		//@{
		const char& at(int64 index) const;
		char& at(int64 index);
		//@}


		/** Append a @bref{chr} to the string one or @bref{count} times. */
		//@{
		void append(char chr);
		void append(char chr, int64 count);
		//@}

		/** Append the first @bref{count} characters from @bref{str}ing. */
		void append(const char* str, int64 count);


		/**
		 * Insert or remove a character at a valid character @bref{index}.
		 * For back-insertion, the string length also is a valid index.
		 * Index removal silently ignores invalid indices for @ref NOTHROW.
		 */
		//@{
		void insert(char chr, int64 index);
		void remove(int64 index) NOTHROW;
		//@}


		/**
		 * Remove all occurrences of given character from string.
		 * Return self-reference after removing.
		 */
		string& remove_all(char chr) NOTHROW;


		/**
		 * Insert or remove characters at the beginning or end
		 * of the string. Both pop_front() and pop_back() do not
		 * have any effect on empty strings.
		 */
		//@{
		void push_front(char chr);
		void push_back(char chr);
		void pop_front() NOTHROW;
		void pop_back() NOTHROW;
		//@}


		/**
		 * Test if this string contains an@bref{other} string,
		 * an unterminated @bref{array} with @bref{count} chars,
		 * or a null-terminated @bref{nt_string}.
		 *
		 * Returns @false if either string is empty(),
		 * if @bref{nt_string} or @bref{array} is @NULL,
		 * or if @bref{count} is zero or less.
		 *
		 * See seek_array() for runtime considerations.		 
		 */
		//@{
		bool contains(const string& other) const NOTHROW;
		bool contains(const char* nt_string) const NOTHROW;

		bool contains_array
			(const char* array, int64 count) const NOTHROW;
		//@}


		/**
		 * Test if this string starts or ends with another string.
		 * See contains(), but with an O(size(other)) worst-case.
		 */
		//@{
		bool begins_with(const string& other) const NOTHROW;
		bool begins_with(const char* nt_string) const NOTHROW;

		bool ends_with(const string& other) const NOTHROW;
		bool ends_with(const char* nt_string) const NOTHROW;

		bool begins_with_array
			(const char* array, int64 count) const NOTHROW;
		bool ends_with_array
			(const char* array, int64 count) const NOTHROW;
		//@}


		/**
		 * Find the index of @bref{chr} within the string, starting at
		 * and including the given @bref{start_index}, searching either
		 * in order or in reverse. On a return of @bref{invalid_index},
		 * the char was not found within the string.
		 *
		 * If no @bref{start_index} is given, in-order seek() starts
		 * at the first character of the string, while seek_reverse()
		 * starts at the last character.
		 *
		 * The search interval automatically clamps to valid range:
		 * seek(c, -5) == seek(c, 0), seek(0, length()) == invalid_index,
		 * seek_reverse(c, length()) == seek_reverse(c, length() - 1).
		 */
		//@{
		int64 seek(char chr, int64 start_index) const NOTHROW;
		int64 seek(char chr) const NOTHROW;
		int64 seek_reverse(char chr, int64 start_index) const NOTHROW;
		int64 seek_reverse(char chr) const NOTHROW;
		//@}


		/**
		 * Returns the starting index of the first occurrence
		 * of an@bref{other} string, a null-terminated @bref{nt_string},
		 * or an unterminated @bref{array} with @bref{count} chars.
		 *
		 * The call returns @bref{invalid_index} if there is no match,
		 * if either string is empty(), if @bref{count} is zero or less,
		 * or if @bref{nt_string} or @bref{array} is @NULL.
		 *
		 * Optionally, searching starts at @bref{start_index}.
		 * An invalid @bref{start_index} is automatically and
		 * silently clamped to valid range, possibly past-end.
		 *
		 * All variants have a O(size(this) * size(other))
		 * worst-case runtime, but usually run in O(size(this)).		 
		 */
		//@{
		int64 seek(const string& other) const NOTHROW;
		int64 seek(const string& other, int64 start_index) const NOTHROW;

		int64 seek(const char* nt_string) const NOTHROW;
		int64 seek(const char* nt_string, int64 start_index) const NOTHROW;

		int64 seek_array
			(const char* array, int64 count) const NOTHROW;
		int64 seek_array
			(const char* array, int64 count, int64 start_index) const NOTHROW;
		//@}


		/**
		 * Replace all occurrences of @bref{oldchr} with @bref{newchr}.
		 * Return self-reference after replacing.
		 */
		string& replace_all(char oldchr, char newchr) NOTHROW;


		/**
		 * Append given @bref{c}har to left or right of string
		 * until the string has at least @bref{min_size} chars.
		 * Throws, amongst others, on invalid @bref{min_size}.
		 */
		//@{
		string pad_left(char c, int64 min_size) const;
		string pad_right(char c, int64 min_size) const;
		//@}


		/**
		 * Remove leading, trailing, or redundant whitespace
		 * (tabs '\\t', spaces '\\ ', linefeeds '\\r' and '\\n')
		 * in this string.
		 *
		 * While trim() only removes leading and trailing whitespace,
		 * compact() additionally replaces any subsequent whitespace
		 * in the string by a single space character.
		 */
		//@{
		string trim() const;
		string compact() const;
		//@}


		/**
		 * Remove all leading or trailing @bref{c}haracters.
		 */
		//@{
		string trim_left(char c) const;
		string trim_right(char c) const;
		//@}


		/**
		 * Add or remove double quotes '"' at the start and the end
		 * of this string. Unenquote does nothing if either end is
		 * not a quote.
		 */
		//@{
 		string enquote() const;
		string unenquote() const;
		//@}


		/**
		 * Starting with @bref{offset}, extract at most @bref{count}
		 * characters, or all characters up to the end of the string.
		 * The character @bref{count} must be positive, but the
		 * extraction interval automatically clamps to valid range.
		 */
		//@{
		string substring(int64 offset) const;
		string substring(int64 offset, int64 count) const;
		//@}


		/**
		 * Return @bref{count} characters from the left or right
		 * of the string, as by respective substring() calls.
		 */
		//@{
		string left(int64 count) const;
		string right(int64 count) const;
		//@}


		/**
		 * Remove each @bref{chr} from string, split string into
		 * substrings at these points, and replace @bref{result}
		 * with the generated substrings. If @bref{chr} does not
		 * occur in the string, @bref{result} contains a single
		 * copy of the string. This also means that @bref{result} 
		 * is never empty, but at least contains an empty string.
		 */
		void split(char chr, list<string>& result) const;


		/** Transform 'a'-'z' to 'A'-'Z' and vice versa. */
		//@{
		string to_upper() const;
		string to_lower() const;
		//@}


		/**
		 * Lax conversion of string to numerical type.
		 * Returns 0 if the string contains no nonzero result.
		 * For example, to_int32() of "a123b4" is 1234,
		 * and to_bool() of "falsetruefalse" is true.
		 */
		//@{
		bool to_bool() const NOTHROW;

		int32 to_int32() const NOTHROW;
		uint32 to_uint32() const NOTHROW;

		int64 to_int64() const NOTHROW;
		uint64 to_uint64() const NOTHROW;

		float to_float() const NOTHROW;
		//@}


		void swap_with(string& other) NOTHROW;


		/**
		 * String concattenation and assignment.
		 * Move assignment leaves an empty() other string.
		 */
		//@{
		string operator +(const string& other) const;
		string& operator +=(const string& other);

		string& operator =(const string& other);
		string& operator =(string&& other) NOTHROW;
		//@}


		/** String comparison. */
		//@{
		bool operator ==(const string& other) const NOTHROW;
		bool operator ==(const char* nt_other) const NOTHROW;
		bool operator !=(const string& other) const NOTHROW;
		bool operator !=(const char* nt_other) const NOTHROW;
		//@}

		/** Lexicographical compare. */
		//@{
		bool operator <(const string& other) const NOTHROW;
		bool operator <=(const string& other) const NOTHROW;
		bool operator >(const string& other) const NOTHROW;
		bool operator >=(const string& other) const NOTHROW;
		//@}


		/** Build string hash code, see viral_core::hash(). */
		uint64 hash() const NOTHROW;


		/**
		 * Reset to "", but with (explicit) NOTHROW guarantee.
		 * Keeps any existing capacity().
		 */
		void clear() NOTHROW;


		/** Reset to "" and release any allocated memory. */
		void free() NOTHROW;


		/**
		 * Invalid or non-existing index as a return tag
		 * for several query methods.
		 */		
		static const int64 invalid_index = -1;


		/**
		 * Significant digits for float-to-string conversions.
		 * Notably, @bref{float_to_string_precision_roundtrip}
		 * is the minimum precision needed so that a string
		 * exactly converts back to the original float.
		 */
		//@{
		static const int64 float_to_string_precision_min = 4;
		static const int64 float_to_string_precision_default = 6;		
		static const int64 float_to_string_precision_roundtrip = 10;
		static const int64 float_to_string_precision_max = 16;
		//@}


	private:

		string_buffer buffer_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__STRING_HPP) */

