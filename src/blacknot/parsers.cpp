//======================================================================
// This is part of Project Blacknot:
//  https://github.com/yzt/blacknot
//======================================================================

#include <blacknot/parsers.hpp>

//======================================================================

namespace Blacknot {

//======================================================================

bool ConfigParser::IsWhiteSpace (char c)
{
	return ' ' == c || '\t' == c || '\n' == c || '\r' == c || '\f' == c || '\v' == c;
}

//----------------------------------------------------------------------

bool ConfigParser::IsNameStarter (char c)
{
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

//----------------------------------------------------------------------

bool ConfigParser::IsNameContinuer (char c)
{
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || '_' == c;
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------

ConfigParser::ConfigParser ()
{
}

//----------------------------------------------------------------------

ConfigParser::~ConfigParser ()
{
}

//----------------------------------------------------------------------

bool ConfigParser::parse ()
{
	if (!start())
		return false;

	m_line = 1;
	m_column = 0;
	m_byte = 0;
	m_curr_section_size = 0;
	m_curr_section[0] = '\0';

	popChar ();
	while (!eoi())
	{
		skipWS ();
		if (eoi())
			break;

		auto const c = curr();

		if (c == '[')
		{
			if (false == parseSectionHeader())
				return false;
		}
		else if (c == '#')
		{
			if (false == parseComment())
				return false;
		}
		else if (IsNameStarter(c))
		{
			if (false == parseNameValuePair())
				return false;
		}
		else
			if (false == error(Error::IllegalChar, m_line, m_column, m_byte))
				return false;

	}

	if (!end())
		return false;

	return true;
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------

bool ConfigParser::popChar ()
{
	if (!pop())
		return false;

	m_byte += 1;
	m_column += 1;

	if (curr() == '\n')
	{
		m_line += 1;
		m_column = 1;
	}

	return true;
}

//----------------------------------------------------------------------

void ConfigParser::skipWS ()
{
	while (IsWhiteSpace(curr()))
		if (!popChar())
			break;
}

//----------------------------------------------------------------------

bool ConfigParser::parseSectionHeader ()
{
	if (!popChar())
		return false;

	skipWS();

	if (!IsNameStarter(curr()))
		return false;

	m_curr_section_size = 0;
	if (false == pushCharChecked(curr(), m_curr_section, m_curr_section_size, msc_MaxSectionNameSize))
		return false;

	while (popChar())
		if (IsNameContinuer(curr()))
		{
			if (false == pushCharChecked(curr(), m_curr_section, m_curr_section_size, msc_MaxSectionNameSize))
				return false;
		}
		else
			break;

	m_curr_section[m_curr_section_size] = '\0';

	skipWS();

	if (curr() != ']')
		return false;

	if (false == newSection(m_curr_section, m_curr_section_size))
		return false;

	popChar();
	return true;
}

//----------------------------------------------------------------------

bool ConfigParser::parseComment ()
{
	do {
		if (!popChar())
			return false;
	} while (curr() != '\n');
	popChar ();
	return true;
}

//----------------------------------------------------------------------

bool ConfigParser::parseNameValuePair ()
{
	char name [msc_MaxNameSize + 1];
	char value [msc_MaxValueSize + 1];
	unsigned name_size = 0;
	unsigned value_size = 0;

	if (false == parseName(name, name_size))
		return false;

	skipWS ();
	if (curr() != '=')
	{
		error (Error::EqualSignExpected, m_line, m_column, m_byte);
		return false;
	}
	popChar ();
	skipWS ();

	if (curr() != '"')
	{
		error (Error::DoubleQuoteExpected, m_line, m_column, m_byte);
		return false;
	}

	if (false == parseValue(value, value_size))
		return false;

	if (false == newEntry(m_curr_section, m_curr_section_size, name, name_size, value, value_size))
		return false;

	return true;
}

//----------------------------------------------------------------------

bool ConfigParser::parseName (char * buffer, unsigned & size)
{
	size = 0;
	if (false == pushCharChecked(curr(), buffer, size, msc_MaxNameSize))
		return false;

	while (popChar())
	{
		if (IsNameContinuer(curr()))
		{
			if (false == pushCharChecked(curr(), buffer, size, msc_MaxNameSize))
				return false;
		}
		else
			break;
	}

	buffer[size] = '\0';
	return true;
}

//----------------------------------------------------------------------

bool ConfigParser::parseValue (char * buffer, unsigned & size)
{
	if (!popChar())
		return false;

	size = 0;
	bool escaped = false;

	if (curr() != '\\')
	{
		if (false == pushCharChecked(curr(), buffer, size, msc_MaxValueSize))
			return false;
	}
	else
		escaped = true;

	while (popChar())
	{
		auto const c = curr();

		if (escaped)
		{
			char ec = '\0';

			if (c == '\\') ec = '\\';
			else if (c == '"') ec = '"';
			else
			{
				error (Error::IllegalChar, m_line, m_column, m_byte);
				return false;
			}

			if (false == pushCharChecked(ec, buffer, size, msc_MaxValueSize))
				return false;
			escaped = false;
		}
		else if (c == '\\')
			escaped = true;
		else if (c == '"')
			break;
		else
		{
			if (false == pushCharChecked(c, buffer, size, msc_MaxValueSize))
				return false;
		}
	}
	
	buffer[size] = '\0';

	if ('"' != curr())
	{
		error (Error::DoubleQuoteExpected, m_line, m_column, m_byte);
		return false;
	}
	popChar();
	return true;
}

//----------------------------------------------------------------------
// Assumes there is always one more byte available in the buffer for the terminating NUL
bool ConfigParser::pushCharChecked (char ch, char * buffer, unsigned & in_out_size, unsigned max_size)
{
	if (in_out_size < max_size)
	{
		buffer[in_out_size++] = ch;
		return true;
	}
	else
		return error(Error::SizeViolation, m_line, m_column, m_byte);
}

//======================================================================

}	// namespace Blacknot

//======================================================================
