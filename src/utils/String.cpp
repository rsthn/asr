
#include <asr/utils/String>
#include <asr/crypto/CRC32>
#include <asr/defs>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

namespace asr {
namespace utils {


	/**
	**	Constructor from constant character buffer (if no length provided strlen will be used).
	*/
	String::String (const char *value, int length)
	{
		this->invalidate()->set(value, length);
	}

	/**
	**	Constructs the string from another string object.
	*/
	String::String (const String *string)
	{
		this->invalidate()->set(string);
	}

	/**
	**	Allocates a string with the specified buffer length but stores just a zero-length string.
	*/
	String::String (int length)
	{
		this->invalidate()->resize(length)->chop(0);
	}

	/**
	**	Copy constructor.
	*/
	String::String (const String& string)
	{
		this->invalidate()->set(&string);
	}

	/**
	**	Constructs a new string using the provided buffer without allocating any new memory. Make sure the buffer's
	**	length is actually n+1 in order to use c_str() without errors.
	*/
	String *String::fromBuffer (char *buffer, int length)
	{
		return (new String())->setBuffer(buffer, length);
	}

	/**
	**	Deletes all allocated buffers.
	*/
	String::~String()
	{
		if (this->value != nullptr)
		{
			asr::dealloc (this->value);
			this->value = nullptr;
		}
	}

	/**
	**	Resets the string object to its initial conditions but does NOT deallocate the string buffer.
	*/
	String *String::invalidate()
	{
		this->value = nullptr;
		this->length = this->bufferLength = 0;

		return this;
	}

	/**
	**	Forcefully changes the internal buffer of the string. Make sure the provided buffer's length actually is n+1 to use c_str() without errors.
	*/
	String *String::setBuffer (char *buffer, int length)
	{
		if (this->value != nullptr)
			asr::dealloc (this->value);

		if (length == -1)
			length = strlen(buffer);

		this->value = buffer;
		this->bufferLength = length+1;

		this->length = length;
		this->value[this->length] = '\0';

		return this;
	}

	/**
	**	Resizes the length of the buffer to the given length. Note that if the underlying buffer is already of a size such that
	**	is capable of holding length+1 bytes, then no physical resize will occur (but logically will).
	*/
	String *String::resize (int length)
	{
		if (this->value == nullptr)
		{
			this->value = (char *)asr::alloc (bufferLength = length + 1);
		}
		else
		{
			if (length+1 > bufferLength)
			{
				char *temp = this->value;

				this->value = (char *)asr::alloc (bufferLength = length + 1);
				memcpy (this->value, temp, this->length);

				asr::dealloc (temp);
			}
		}

		this->length = length;
		this->value[this->length] = '\0';

		return this;
	}

	/**
	**	Returns the C ASCIIZ representation of the string.
	*/
	const char *String::c_str() const
	{
		return value;
	}


	/**
	**	Returns the integer representation of the string.
	*/
	int String::c_int() const
	{
		return atoi(c_str());
	}

	/**
	**	Returns the double representation of the string.
	*/
	double String::c_double() const
	{
		return atof(c_str());
	}

	/**
	**	Sets the string contents using the specified buffer.
	*/
	String *String::set (const char *buffer, int length)
	{
		if (buffer == nullptr)
			return this;

		if (length == -1)
			length = strlen (buffer);

		this->resize(length);
		memcpy (value, buffer, length);

		return this;
	}

	String *String::set (const String *str)
	{
		return set (str->value, str->length);
	}

	/**
	**	Returns a clone of the string.
	*/
	String *String::clone() const
	{
		return new String (this);
	}

	/**
	**	Chops the last N bytes of the string. The actual buffer is not modified at all, only the internal length indicator is reduced.
	*/
	String *String::chop (int numBytes)
	{
		this->length -= numBytes ? numBytes : this->length;
		if (this->length < 0) this->length = 0;

		this->value[this->length] = '\0';
		return this;
	}

	/**
	**	Removes all white-space from the beginning and end of the string.
	*/
	String *String::trim()
	{
		char *s = this->value;
		char *p = this->value + this->length;

		while (s < p && *s <= 32) s++;

		if (s != this->value)
			memcpy (this->value, s, this->length -= (int)(s - this->value));

		p = this->value + this->length;

		while (*--p <= 32) this->length--;

		if (this->length < 0) this->length = 0;
		this->value[this->length] = '\0';

		return this;
	}

	/**
	**	Returns a new string made from the substring in the given range.
	*/
	String *String::substr (int from, int numBytes) const
	{
		// Return empty string if initial offset is beyond string's size.
		if (from > length) return new String ("");

		// Process relative offset (negative).
		if (numBytes < 1) numBytes += length;
		if (from < 0) from += length;

		// Calculate correct numBytes to avoid memory access errors.
		if (from + numBytes > length) numBytes = length - from;

		// Create string and copy the bytes to it.
		return new String (value + from, numBytes);
	}

	/**
	**	Concatenates the current and the given string, returns new string.
	*/
	String *String::concat (const char *buffer, int length)
	{
		if (buffer == nullptr) return this;
		if (length == -1) length = strlen (buffer);

		String *ns = new String ();

		ns->resize (length + this->length);

		memcpy (ns->value, this->value, this->length);
		memcpy (ns->value + this->length, buffer, length);

		return ns;
	}

	String *String::concat (String *s)
	{
		return this->concat (s->value, s->length);
	}

	/**
	**	Appends a string to the current one. Doesn't return new string.
	*/
	String *String::append (const char *buffer, int length)
	{
		if (buffer == nullptr) return this;
		if (length == -1) length = strlen (buffer);

		int p_length = this->length;

		this->resize (p_length + length);
		memcpy (this->value + p_length, buffer, length);

		return this;
	}

	String *String::append (String *s)
	{
		return this->append (s->value, s->length);
	}

	String *String::append (char ch)
	{
		return this->append (&ch, 1);
	}

	/**
	**	Converts the string to lower case.
	*/
	String *String::toLowerCase()
	{
		for (int i = 0; i < length; i++) value[i] = tolower(value[i]);
		return this;
	}

	/**
	**	Converts the string to upper case.
	*/
	String *String::toUpperCase()
	{
		for (int i = 0; i < length; i++) value[i] = toupper(value[i]);
		return this;
	}

	/**
	**	Replaces one character by another.
	*/
	String *String::replace (char a, char b)
	{
		for (int i = 0; i < length; i++) if (value[i] == a) value[i] = b;
		return this;
	}

	/**
	**	Returns a character from the specified index. Returns zero if index is out of bounds.
	*/
	char String::charAt (int index) const
	{
		if (index < 0) index += length;

		return index >= 0 && index < length ? value[index] : 0;
	}

	/**
	**	Returns true if the provided string equals the current string.
	*/
	bool String::equals (const String *value) const
	{
		if (value == nullptr || value->length != length)
			return false;

		return !memcmp (value->value, this->value, length);
	}

	bool String::equals (const char *str) const
	{
		if (str == nullptr || (int)strlen(str) != length)
			return false;

		return !memcmp (str, value, length);
	}

	/**
	**	Compares the strings and returns -1, 0 or 1 as strcmp would.
	*/
	int String::compare (const String *value) const
	{
		if (value == nullptr || value->length != length)
			return -1;

		return memcmp (value->value, this->value, length);
	}

	int String::compare (const char *str) const
	{
		if (str == nullptr || (int)strlen(str) != length)
			return -1;

		return memcmp (str, value, length);
	}

	/**
	**	Returns a boolean indicating if the string ends with the same characters as the one provided.
	*/
	bool String::endsWith (const char *value, int length) const
	{
		if (value == nullptr) return false;
		if (length == -1) length = strlen (value);

		if (this->length < length)
			return false;

		return !memcmp (value, this->value+(this->length-length), length);
	}

	/**
	**	Returns a boolean indicating if the string starts with the same characters as the one provided.
	*/
	bool String::startsWith (const char *value, int length) const
	{
		if (value == nullptr) return false;
		if (length == -1) length = strlen (value);

		if (this->length < length)
			return false;

		return !memcmp (value, this->value, length);
	}

	/**
	**	Returns a CRC32 hash of the given string. If length is not provided strlen will be used.
	*/
	uint32_t String::getHash (const char *value, int length)
	{
		return crypto::crc32(value, length);
	}

	/**
	**	Returns the hash of the string.
	*/
	uint32_t String::getHash() const
	{
		return crypto::crc32(value, length);
	}

	/**
	**	Formats a message and returns it as a new string.
	*/
	String *String::printf (const char *msg, ...)
	{
		va_list args;
		va_start (args, msg);

		char *buffer = tempbuff();

		vsprintf (buffer, msg, args);
		return new String (buffer);
	}

	/**
	 * Formats a message and returns a static internal buffer.
	 */
	const char *String::sprintf (const char *msg, ...)
	{
		char *buffer = tempbuff();

		va_list args;
		va_start (args, msg);

		vsnprintf (buffer, TEMPBUFF_SIZE-1, msg, args);
		buffer[TEMPBUFF_SIZE-1] = '\0';

		va_end (args);

		return buffer;
	}

	/**
	 * Formats a message and returns a new buffer. Uses `vsnprintf` underneath to prevent buffer overflow.
	 */
	const char *String::ssprintf (const char *msg, ...)
	{
		char *buffer = (char *)asr::alloc(TEMPBUFF_SIZE);

		va_list args;
		va_start (args, msg);

		vsnprintf (buffer, TEMPBUFF_SIZE-1, msg, args);
		buffer[TEMPBUFF_SIZE-1] = '\0';

		va_end (args);

		return buffer;
	}

}};
