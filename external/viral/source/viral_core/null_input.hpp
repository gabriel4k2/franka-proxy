/**
 *************************************************************************
 *
 * @file null_input.hpp
 *
 * Null input handling.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__NULL_INPUT_HPP)
#define INCLUDED__VIRAL_CORE__NULL_INPUT_HPP


#include "build_config.hpp"


#include "input.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class null_input_context
 * @ingroup input_module
 *
 * An input_context that never provides input.
 *
 * Usage notes:
 * - A null_input_context always reports all input sources at zero value
 *		and as inactive. Conversely, the null_input_context never marks
 *		input sources as active, activated, or deactivated.
 * - There never are any text input events on null_input_context.
 * - The null_input_context ignores mouse freezing.
 *
 ************************************************************************/
class null_input_context :
	public input_context
{

	public:

		null_input_context(const string& context_name);
		~null_input_context() NOTHROW override;

		void update() override;

		float value_of(input_source_type source) const override;
		bool is_inactive(input_source_type source) const override;
		bool is_active(input_source_type source) const override;

		bool was_deactivated(input_source_type source) const override;
		bool was_activated(input_source_type source) const override;

		void get_text_input(text_input_buffer &text_input) const override;

		void freeze_mouse(bool freeze) override;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__NULL_INPUT_HPP) */