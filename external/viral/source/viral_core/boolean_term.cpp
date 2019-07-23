/**
 *************************************************************************
 *
 * @file boolean_term.cpp
 *
 * Evaluation of boolean terms, implementation.
 *
 ************************************************************************/


#include "boolean_term.hpp"


#include "string.hpp"
#include "tree_set.hpp"
#include "log.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// boolean_term
//
//////////////////////////////////////////////////////////////////////////


bool boolean_term::evaluate
	(const string& term, const option_set& options)
{
	int64 parsing_position = 0;
	return evaluate_or(term, parsing_position, options, true);
}


bool boolean_term::evaluate_variable
	(const string& term, int64& parsing_position,
	 const option_set& options)
{
	// Subordinate term.
	int64& c = parsing_position;
	if (term[c] == '(')
	{
		++c;
		return evaluate_or(term, parsing_position, options, false);
	}


	// Variable identifier.
	int64 variable_start = parsing_position;
	while (term[c] >= 'a' && term[c] <= 'z' || // find end of variable
		   term[c] >= 'A' && term[c] <= 'Z' ||
		   term[c] >= '0' && term[c] <= '9' ||
		   term[c] == '_') { ++c; }
	
	if (c == variable_start)
	{
		LOG_ERROR("Expected valid variable identifier "
				  " at position: " + variable_start +
				  " in term: " + term);
		throw syntax_error();
	}

	string variable
		(term.substring(variable_start, c - variable_start));

	return options.contains(variable);
}


bool boolean_term::evaluate_not
	(const string& term, int64& parsing_position,
	 const option_set& options)
{
	// Sequence of multiple, whitespace-separated negates.
	bool negate = false;

	int64& c = parsing_position;
	while (term[c] == '!')
	{
		++c;
		negate = !negate;

		while (term[c] == ' ' || term[c] == '\t' ||
			   term[c] == '\r' || term[c] == '\n') { ++c; }
	}


	// Operand, mandatory
	return
		negate ^ evaluate_variable(term, parsing_position, options);
}


bool boolean_term::evaluate_and
	(const string& term, int64& parsing_position,
	 const option_set& options)
{
	bool result = true;

	int64& c = parsing_position;
	while (true)
	{
		// Leading whitespace.
		while (term[c] == ' ' || term[c] == '\t' ||
			   term[c] == '\r' || term[c] == '\n') { ++c; }


		// Operand, mandatory.
		// Can't short-circuit evaluation since we need
		// to parse remaining term.
		result &=
			evaluate_not(term, parsing_position, options);
		

		// Intermediate whitespace.
		while (term[c] == ' ' || term[c] == '\t' ||
			   term[c] == '\r' || term[c] == '\n') { ++c; }


		// Another operator or entire term finished.
		// Any syntax errors are handled in evaluate_or().
		if (c + 1 < term.size() && term[c] == '&' && term[c + 1] == '&')
		{
			c += 2;
			continue;
		}
		else
			return result;
	}
}


bool boolean_term::evaluate_or
	(const string& term, int64& parsing_position,
	 const option_set& options, bool is_root_term)
{
	bool result = false;

	int64& c = parsing_position;
	while (true)
	{
		// Leading whitespace.
		while (term[c] == ' ' || term[c] == '\t' ||
			   term[c] == '\r' || term[c] == '\n') { ++c; }


		// Operand, mandatory.
		// Can't short-circuit evaluation since we need
		// to parse remaining term.
		result |=
			evaluate_and(term, parsing_position, options);
		

		// Intermediate whitespace.
		while (term[c] == ' ' || term[c] == '\t' ||
			   term[c] == '\r' || term[c] == '\n') { ++c; }


		// Another operator or entire term finished.
		// Any other case means syntax error.
		if (c + 1 < term.size() && term[c] == '|' && term[c + 1] == '|')
		{
			c += 2;
			continue;
		}
		else if (is_root_term && c == term.size())
		{
			return result;
		}
		else if (!is_root_term && term[c] == ')')
		{
			++c;
			return result;
		}
		else
		{
			LOG_ERROR("Expected operator or term end at position: " + c +
					  " in term: " + term);
			throw syntax_error();
		}
	}
}




} /* namespace viral_core */
