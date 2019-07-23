/**
 *************************************************************************
 *
 * @file file_util.cpp
 *
 * File utility implementation.
 *
 ************************************************************************/


#include "file_util.hpp"


#include "log.hpp"
#include "exception.hpp"
#include "integer_operations.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//	virtual_path
//
//////////////////////////////////////////////////////////////////////////


string virtual_path::extract_target(const string& path)
{
	int64 indexpath = path.seek_reverse('/');

	if (indexpath == string::invalid_index)
		return path;

	return
		path.substring(indexpath + 1);
}


string virtual_path::remove_extension(const string& path)
{
	int64 indexext = path.seek_reverse('.');

	if (indexext == string::invalid_index)
		return path;

	int64 indexpath = path.seek_reverse('/');

	if (indexpath != string::invalid_index &&
		indexext < indexpath)
	{
		// Fullstop found, but not in target element.
		return path;
	}

	return
		path.substring(0, indexext);
}


string virtual_path::extract_extension(const string& path)
{
	int64 indexext = path.seek_reverse('.');

	if (indexext == string::invalid_index ||
		indexext == path.size() - 1)
	{
		return string();
	}

	int64 indexpath = path.seek_reverse('/');

	if (indexpath != string::invalid_index &&
		indexext < indexpath)
	{
		// Fullstop found, but not in target element.
		return string();
	}

	return
		path.substring(indexext + 1);
}


string virtual_path::extract_parent(const string& path)
{
	int64 indexpath = path.seek_reverse('/');

	if (indexpath == string::invalid_index)
		return string();

	return
		path.substring(0, indexpath);
}


string virtual_path::extract_root(const string& path)
{
	int64 indexpath = path.seek('/');

	if (indexpath == string::invalid_index)
		return string();

	return
		path.substring(0, indexpath);
}


bool virtual_path::is_valid(const string& path)
{
	if (path.size() == 0)
		return false;

	for (int64 i = 0; i < path.size(); ++i)
	{
		char curchar = path[i];

		if (curchar >= 'a' && curchar <= 'z' ||
			curchar >= 'A' && curchar <= 'Z' ||
			curchar >= '0' && curchar <= '9' ||
			curchar == '_')
		{
			continue;
		}


		// Dot may not be first or last character of string,
		// or first char after or before slash.
		if (curchar == '.')
		{
			if (i == 0 || i == path.size() - 1)
				return false;

			if (path[i - 1] == '/' || path[i + 1] == '/')
				return false;

			continue;
		}


		// Slashes may not be first or last character
		// within string, or appear in sequence.
		if (curchar == '/')
		{
			if (i == 0 || i == path.size() - 1)
				return false;

			if (path[i + 1] == '/')
				return false;

			continue;
		}


		// Another - yet invalid - char found
		return false;
	}

	return true;
}




//////////////////////////////////////////////////////////////////////////
//
//	File
//
//////////////////////////////////////////////////////////////////////////


void file::read(char &value)
	{ read_bytes(&value, sizeof(char)); }
void file::write(char value)
	{ write_bytes(&value, sizeof(char)); }

void file::read(unsigned char &value)
	{ read_bytes(&value, sizeof(char)); }
void file::write(unsigned char value)
	{ write_bytes(&value, sizeof(char)); }

void file::read(int16 &value)
	{ read_bytes(&value, sizeof(int16)); }
void file::write(int16 value)
	{ write_bytes(&value, sizeof(int16)); }

void file::read(uint16 &value)
	{ read_bytes(&value, sizeof(uint16)); }
void file::write(uint16 value)
	{ write_bytes(&value, sizeof(uint16)); }

void file::read(int32 &value)
	{ read_bytes(&value, sizeof(int32)); }
void file::write(int32 value)
	{ write_bytes(&value, sizeof(int32)); }

void file::read(uint32 &value)
	{ read_bytes(&value, sizeof(uint32)); }
void file::write(uint32 value)
	{ write_bytes(&value, sizeof(uint32)); }

void file::read(float &value)
	{ read_bytes(&value, sizeof(float)); }
void file::write(float value)
	{ write_bytes(&value, sizeof(float)); }

void file::read(double &value)
	{ read_bytes(&value, sizeof(double)); }
void file::write(double value)
	{ write_bytes(&value, sizeof(double)); }


string file::read_nullterminated()
{
	// Search for terminating \\0.
	// This allows us to appropriately presize the string,
	// which is faster than perpetual resizing per character.
	int64 start = tell();
	int64 len = 0;
	char findnull;

	while (true)
	{
		read(findnull); // Throws on EoF.
		if (findnull == '\0') break;
		len = integer_operations::add_or_throw(len, 1);
	}

	seek(start, seek_begin);


	// The target string already has a terminating \0,
	// so opposed to writing, we need not fetch the \0
	// from the source file.
	string str;
	str.resize(len);
	read_bytes(str.data(), str.size());
	seek(sizeof(char), seek_current);

	return std::move(str);
}

void file::write_nullterminated(const string& str)
{
	// Additionally store terminating \0 of string.
	write_bytes(str.data(), str.size() + 1);
}


string file::read_text()
{
	int64 cur = tell();
	int64 size = find_size();

	if (cur > size)
	{
		LOG_ERROR("File position exceeds file size.");
		throw assert_failed();
	}


	// Copy remaining file content,
	// which need not include a terminating \0.
	string str;
	str.resize(size - cur);
	read_bytes(str.data(), str.size());

	return std::move(str);
}

void file::write_text(const string& str)
{
	// Variant without terminating \0 of string.
	write_bytes(str.data(), str.size());
}




//////////////////////////////////////////////////////////////////////////
//
//	disk_file
//
//////////////////////////////////////////////////////////////////////////


disk_file::disk_file
	(const string& path, file_operation operation)
	:
	path_(path),
	operation_(operation),
	handle_(NULL)
{
	bool success = false;

	if (operation == read_only)
		success = (fopen_s(&handle_, path.data(), "rb") == 0);
	else if (operation == read_write_truncate)
		success = (fopen_s(&handle_, path.data(), "w+b") == 0);
	else if (operation == read_write_update)
		success = (fopen_s(&handle_, path.data(), "r+b") == 0);
	else
	{
		LOG_ERROR("Unknown file opening mode on file: " + path_);
		throw invalid_parameter();
	}

	if (!success || !handle_)
	{
		if (handle_)
			fclose(handle_); // No destructor on exception.

		LOG_ERROR("Failed to open: " + path_);
		throw file_operation_failed();
	}
}


disk_file::~disk_file() NOTHROW
{
	if (handle_)
	{
		fclose(handle_);
		handle_ = NULL;
	}
}


void disk_file::seek(int64 offset, seek_mode source)
{
	int64 result = -1;
	
	if (source == seek_begin)
		result = _fseeki64(handle_, offset, SEEK_SET); 
	else if (source == seek_end)
		result = _fseeki64(handle_, offset, SEEK_END);
	else if (source == seek_current)
		result = _fseeki64(handle_, offset, SEEK_CUR);
	else
	{
		LOG_ERROR("Unknown seeking mode on file: " + path_);
		throw file_operation_failed();
	}

	if (result != 0)
	{
		LOG_ERROR("Seeking failed on file: " + path_);
		throw file_operation_failed();
	}
}


int64 disk_file::tell()
{
	int64 result = _ftelli64(handle_);

	if (result < 0)
	{
		LOG_ERROR("Telling failed on file: " + path_);
		throw file_operation_failed();
	}

	return result;
}


int64 disk_file::find_size()
{
	int64 current_pos = tell();
	seek(0, seek_end);
	int64 length = tell();
	seek(current_pos, seek_begin);

	if (length < 0) // Redundant to tell() checks.
	{
		LOG_ERROR("File has negative length: " + path_);
		throw file_operation_failed();
	}

	return length;
}


bool disk_file::eof()
{
	// We must enforce a read on each eof() test,
	// because some calls (e.g. fwrite(), fseek())
	// ignore or even reset the feof() bit.
	if (fgetc(handle_) == EOF)
	{
		if (ferror(handle_))
		{
			LOG_ERROR("File error while checking for EOF.");
			throw file_operation_failed();
		}

		return true;
	}

	seek(-1, file::seek_current);
	return false;
}


void disk_file::read_bytes(void* data, int64 count)
{
	if (count < 0 || (count > 0 && !data))
	{
		LOG_ERROR("Invalid file reading parameters.");
		throw invalid_parameter();
	}

	size_t fread_count =
		integer_operations::cast_or_throw<size_t>(count);
	
	if (fread(data, 1, fread_count, handle_) != fread_count)
	{
		LOG_CRITICAL("Failed to read from: " + path_);
		throw file_operation_failed();
	}
}


void disk_file::write_bytes(const void* data, int64 count)
{
	if (operation_ != read_write_truncate &&
		operation_ != read_write_update)
	{
		LOG_ERROR("Cannot write to read-only file: " + path_);
		throw invalid_state();
	}

	if (count < 0 || (count > 0 && !data))
	{
		LOG_ERROR("Invalid file writing parameters.");
		throw invalid_parameter();
	}

	size_t fwrite_count =
		integer_operations::cast_or_throw<size_t>(count);
	
	if (fwrite(data, 1, fwrite_count, handle_) != fwrite_count) 
	{
		LOG_CRITICAL("Failed to write to: " + path_);
		throw file_operation_failed();
	}
}




} /* namespace viral_core */
