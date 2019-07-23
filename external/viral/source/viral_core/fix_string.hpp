/**
 *************************************************************************
 *
 * @file fix_string.hpp
 *
 * Fixed-length strings.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__FIX_STRING_HPP)
#define INCLUDED__VIRAL_CORE__FIX_STRING_HPP


#include "build_config.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class fix_string
 * @ingroup string_module
 *
 * Immutable C-style character array with a fixed maximum length
 * of @bref{N} characters, stored without dynamical allocation.
 *
 * Notes:
 * - The fix_string is specialized for performance-critical storage
 *		and transfer of very many small strings, where the overhead
 *		of dynamical allocation is not acceptable.
 * - The fix_string is read-only by design. For editing operations,
 *		an approriately reserve()d string is suggested.
 * - On construction, any input characters past the first @bref{N}
 *		are discarded silently.
 * - Character counts and \\0-termination match those of the default
 *		@ref string class.
 *
 ************************************************************************/
template <int64 N>
	class fix_string
{
	public:

		fix_string() NOTHROW
		{
			// Never throws with these parameters.
			set("", 0);
		}

		fix_string(const fix_string& source) NOTHROW
		{
			// Never throws, any source fix_string is valid.
			set(source.data_, source.size_);
		}


		fix_string(const string& source)
			{ set(source.data(), source.size()); }
		fix_string(const char* nt_string)
			{ set(nt_string, string::size(nt_string)); }
		fix_string(const char* source, int64 count)
			{ set(source, count); }
	
	
		const char* data() const NOTHROW { return data_; }
		int64 size() const NOTHROW { return size_; }


		const char& operator [](int64 index) const NOTHROW
			{ return data_[index]; }

			
		bool operator ==(const fix_string& other) const NOTHROW
		{
			if (size_ != other.size_) return false;
			return memory::are_blocks_equal_typed
						(data_, other.data_, size_);
		}

		bool operator ==(const string& other) const NOTHROW
		{
			if (size_ != other.size()) return false;
			return memory::are_blocks_equal_typed
						(data_, other.data(), size_);
		}
		
		bool operator ==(const char* nt_other) const NOTHROW
		{
			int64 other_size = string::size(nt_other);
			if (size_ != other_size) return false;
			return memory::are_blocks_equal_typed
						(data_, nt_other, size_);
		}
		
		
		bool operator !=(const fix_string& other) const NOTHROW
			{ return !(*this == other); }
		bool operator !=(const string& other) const NOTHROW
			{ return !(*this == other); }
		bool operator !=(const char* nt_other) const NOTHROW
			{ return !(*this == nt_other); }

			
		bool operator <(const fix_string& other) const NOTHROW
		{
			memory::compare_result test =
				memory::compare_blocks_typed
					(data(), other.data(), min(size(), other.size()));
							 
			return (test == memory::compare_result_equal) ?
						(size() < other.size()) :
						(test == memory::compare_result_a_before_b);
		}
		
		bool operator <=(const fix_string& other) const NOTHROW
		{
			memory::compare_result test =
				memory::compare_blocks_typed
					(data(), other.data(), min(size(), other.size()));
							 
			return (test == memory::compare_result_equal) ?
						(size() <= other.size()) :
						(test == memory::compare_result_a_before_b);
		}
		
		bool operator >(const fix_string& other) const NOTHROW
			{ return !(*this <= other); }
		bool operator >=(const fix_string& other) const NOTHROW
			{ return !(*this < other); }

				
	private:

		void set(const char* array, int64 count)
		{
			if (count < 0 || (count > 0 && !array))
				throw invalid_parameter();
				
			if (count > N) count = N;
			memory::copy_block_typed(data_, array, count);
			data_[count] = '\0';
			size_ = count;
		}
	
	
		char data_[N + 1];
		int64 size_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__FIX_STRING_HPP) */
