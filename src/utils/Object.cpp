
#include <asr/defs>
#include <asr/utils/Object>

#include <cstdlib>
#include <cstdio>
#include <cmath>

namespace asr {
namespace utils {

	/**
	 * @brief Temporal string used when parsing an object from JSON data.
	 */
	String* Object::buffer = nullptr;

	/**
	 * @brief Constructs an object of the specified type.
	 * @param type One of the Object's constants.
	 */
	Object::Object (int type)
	{
		this->type = type;

		switch (type)
		{
			case ARRAY:
				this->value._array = new List<Object*> ();
				break;

			case OBJECT:
				this->value._object = new Map<const String*,Object*> ();
				break;

			case BOOL:
				this->value._bool = false;
				break;

			case NUMERIC:
				this->value._numeric = 0;
				break;

			case STRING:
				this->value._string = new String();
				break;
		}
	}

	/**
	**	Constructs an object with NULL value.
	*/
	Object::Object (std::nullptr_t value)
	{
		this->type = NUL;
		this->value._numeric = 0;
	}

	/**
	**	Constructs a boolean object.
	*/
	Object::Object (bool value)
	{
		this->type = BOOL;
		this->value._bool = value;
	}

	/**
	**	Constructs a numeric object.
	*/
	Object::Object (double value)
	{
		this->type = NUMERIC;
		this->value._numeric = value;
	}

	/**
	**	Constructs a string object.
	*/
	Object::Object (String* value)
	{
		this->type = STRING;
		this->value._string = value;
	}

	/**
	**	Destroys the object and its contents.
	*/
	Object::~Object ()
	{
		switch (type)
		{
			case ARRAY:
				delete this->value._array;
				break;

			case OBJECT:
				delete this->value._object;
				break;

			case STRING:
				delete this->value._string;
				break;
		}
	}

	/**
	**	Returns the type of the object.
	*/
	int Object::getType()
	{
		return this->type;
	}

	/**
	**	Returns the number of objects contained. Requires Object::ARRAY or Object::OBJECT.
	*/
	int Object::length()
	{
		return type == OBJECT ? value._object->length() : (type == ARRAY ? value._array->length() : 0);
	}

	/**
	**	Returns a Map list with the contents of the OBJECT.
	*/
	LList<Pair<const String*,Object*>*> *Object::getObjectData (const char* name)
	{
		Object *value = get(name);
		return value == nullptr || value->type != OBJECT ? nullptr : value->value._object->get_data();
	}

	LList<Pair<const String*,Object*>*> *Object::getObjectData (int index)
	{
		Object *value = get(index);
		return value == nullptr || value->type != OBJECT ? nullptr : value->value._object->get_data();
	}

	LList<Pair<const String*,Object*>*> *Object::getObjectData()
	{
		return type == OBJECT ? value._object->get_data() : nullptr;
	}

	/**
	**	Returns the named/indexed OBJECT or nullptr if not found (or if not an OBJECT).
	*/
	Object *Object::getObject (const char* name)
	{
		Object *value = get(name);
		return value == nullptr || value->type != OBJECT ? nullptr : value;
	}

	Object *Object::getObject (int index)
	{
		Object *value = get(index);
		return value == nullptr || value->type != OBJECT ? nullptr : value;
	}

		/**
		**	Returns boolean indicating if the property exists in the OBJECT.
		*/
		bool Object::has (const char* name)
		{
			if (type != OBJECT || name == nullptr) return false;

			return this->value._object->get(name) != nullptr;
		}

		/**
		**	Returns a property given its name, requires object to be Object::OBJECT.
		*/
		Object *Object::get (const char* name)
		{
			if (type != OBJECT || name == nullptr) return nullptr;

			return this->value._object->get(name);
		}

		/**
		**	Sets the value of a property. Requires object to be of type Object::OBJECT.
		*/
		Object *Object::set (const char* name, Object *value, bool _delete)
		{
			if (type != OBJECT || name == nullptr || value == nullptr)
				return this;

			this->value._object->set(name, value, _delete);
			return this;
		}

		/**
		**	Removes a property from an OBJECT given its name and returns it, unless the _delete parameter is set to true
		**	in which case nullptr will be returned and the property will be removed and deleted.
		*/
		Object *Object::remove (const char* name, bool _delete)
		{
			if (type != OBJECT) return nullptr;

			return this->value._object->remove(name, _delete);
		}

	/**
	**	Returns an Object list with the contents of the ARRAY.
	*/
	List<Object*> *Object::getArrayData(const char* name)
	{
		Object *value = get(name);
		return value == nullptr || value->type != ARRAY ? nullptr : value->value._array;
	}

	List<Object*> *Object::getArrayData(int index)
	{
		Object *value = get(index);
		return value == nullptr || value->type != ARRAY ? nullptr : value->value._array;
	}

	List<Object*> *Object::getArrayData()
	{
		return type == ARRAY ? value._array : nullptr;
	}

	/**
	**	Returns the named/indexed ARRAY or nullptr if not found (or if not an ARRAY).
	*/
	Object *Object::getArray (const char* name)
	{
		Object *value = get(name);
		return value == nullptr || value->type != ARRAY ? nullptr : value;
	}

	Object *Object::getArray (int index)
	{
		Object *value = get(index);
		return value == nullptr || value->type != ARRAY ? nullptr : value;
	}

		/**
		**	Returns the object at the given index of the ARRAY.
		*/
		Object *Object::get (int index)
		{
			if (type != ARRAY) return nullptr;

			return this->value._array->getAt (index);
		}

		/**
		**	Adds an object to the ARRAY.
		*/
		Object *Object::add (Object *value)
		{
			if (type != ARRAY) return this;

			this->value._array->push (value);
			return this;
		}

		/**
		**	Removes an object from the ARRAY given its index and returns it, unless the _delete parameter is set to true
		**	in which case nullptr will be returned and the object will be removed and deleted.
		*/
		Object *Object::remove (int index, bool _delete)
		{
			if (type != ARRAY) return nullptr;

			Linkable<Object*> *item = value._array->getNodeAt(index);
			if (item == nullptr) return nullptr;

			Object *value = this->value._array->remove(item);

			if (_delete && value)
			{
				delete value;
				return nullptr;
			}

			return value;
		}

	/**
	**	Returns the string value of the object (requires type Object::STRING). If the nullify flag is set the object will be converted into
	**	a NUL object, such that when the object is deleted the string will be preserved. Useful when you want to discard the object but keep the string.
	*/
	String* Object::getString (bool nullify)
	{
		if (type != STRING) return nullptr;

		String* value = this->value._string;
		if (nullify)
		{
			this->type = NUL;
			this->value._numeric = 0;
		}

		return value;
	}

	String* Object::getString (const char* name, bool nullify)
	{
		Object *value = get(name);
		return value != nullptr ? value->getString(nullify) : nullptr;
	}

	String* Object::getString (int index, bool nullify)
	{
		Object *value = get(index);
		return value != nullptr ? value->getString(nullify) : nullptr;
	}

	/**
	**	Returns the numeric value of the object (requires type Object::NUMERIC, Object::BOOL, or Object::STRING - if the later, it will be converter to double value).
	*/
	double Object::getNumeric()
	{
		return type == BOOL ? (this->value._bool ? 1 : 0) : (type == NUMERIC ? this->value._numeric : (type == STRING ? std::atof(this->value._string->c_str()) : 0));
	}

	double Object::getNumeric(const char* name)
	{
		Object *value = get(name);
		return value != nullptr ? value->getNumeric() : 0;
	}

	double Object::getNumeric(int index)
	{
		Object *value = get(index);
		return value != nullptr ? value->getNumeric() : 0;
	}

	/**
	**	Returns the bool value of the object (requires type Object::BOOL, Object::NUMERIC or Object::STRING - if the later, will be compared against true/false).
	*/
	bool Object::getBool()
	{
		return type == NUMERIC ? (this->value._numeric != 0 ? true : false) : (type == BOOL ? this->value._bool : (type == STRING ? this->value._string->equals("true") : false));
	}

	bool Object::getBool(const char* name)
	{
		Object *value = get(name);
		return value != nullptr ? value->getBool() : false;
	}

	bool Object::getBool(int index)
	{
		Object *value = get(index);
		return value != nullptr ? value->getBool() : false;
	}

	/**
	**	Returns the integer numeric value of the object (same as calling (int)_numeric()).
	*/
	int Object::getInt() {
		return getNumeric();
	}

	int Object::getInt(const char* name) {
		return getNumeric(name);
	}

	int Object::getInt(int index) {
		return getNumeric(index);
	}

	/**
	**	Parses an object from the specified stream (JSON format) and returns it.
	*/
	Object *Object::loadFrom (FILE *input)
	{
		int state = 0, temp, sign, sign2, exponent;
		Object *elem, *self;
		Pair<const String*,Object*> *pair;

		if (buffer == nullptr) buffer = new String (1024);

		if (input == nullptr)
			return nullptr;

		while (true)
		{
			int _ch = getc(input);
			if (_ch == EOF) break;

			char ch = _ch;

			switch (state)
			{
				// -----------------------------------------------
				// Initial element character.

				case 0:
					if (ch <= 32) break;

					buffer->resize(0);

					if (ch == '"')
					{
						state = 1;
						break;
					}

					if (ch == '\'')
					{
						state = 2;
						break;
					}

					if (ch == '+')
					{
						state = 3;
						sign = 1; exponent = 0;
						break;
					}

					if (ch == '-')
					{
						state = 3;
						sign = -1; exponent = 0;
						break;
					}

					if (ch >= '0' && ch <= '9')
					{
						state = 3;
						sign = 1; exponent = 0;
						buffer->append(&ch, 1);
						break;
					}

					if (ch == 't')
					{
						state = 4;
						break;
					}

					if (ch == 'f')
					{
						state = 5;
						break;
					}

					if (ch == 'n')
					{
						state = 6;
						break;
					}

					if (ch == '{')
					{
						self = new Object(OBJECT);

						state = 7;
						break;
					}

					if (ch == '[')
					{
						self = new Object(ARRAY);

						ungetc(ch, input);
						state = 8;
						break;
					}

					ungetc(ch, input);
					return nullptr;

				// -----------------------------------------------
				// Double-Quoted String

				case 1:

					if (ch == '\\')
					{
						state = 100;
						break;
					}

					if (ch == '"')
						return new Object (buffer->clone());

					buffer->append(&ch, 1);
					break;

					// ---------------
					case 100:

						switch (ch)
						{
							case '0': ch = '\0'; break;
							case 'b': ch = '\b'; break;
							case 't': ch = '\t'; break;
							case 'n': ch = '\n'; break;
							case 'f': ch = '\f'; break;
							case 'r': ch = '\r'; break;
							case '\\': ch = '\\'; break;
							case '\"': ch = '\"'; break;
							case '\'': ch = '\''; break;
							case 'x':
								temp = 0;
								state = 101;
								break;
						}

						if (state == 101) break;

						buffer->append(&ch, 1);

						state = 1;
						break;

					// ---------------
					case 101:
						temp = digit(ch);
						state = 102;
						break;

					// ---------------
					case 102:
						temp = (temp<<4) + digit(ch);
						buffer->append((char*)&temp, 1);

						state = 1;
						break;

				// -----------------------------------------------
				// Single-Quoted String

				case 2:
					if (ch == '\\')
					{
						state = 200;
						break;
					}

					if (ch == '\'')
						return new Object (buffer->clone());

					buffer->append(&ch, 1);
					break;

					// ---------------
					case 200:

						switch (ch)
						{
							case '0': ch = '\0'; break;
							case 'b': ch = '\b'; break;
							case 't': ch = '\t'; break;
							case 'n': ch = '\n'; break;
							case 'f': ch = '\f'; break;
							case 'r': ch = '\r'; break;
							case '\\': ch = '\\'; break;
							case '\"': ch = '\"'; break;
							case '\'': ch = '\''; break;
							case 'x':
								temp = 0;
								state = 201;
								break;
						}

						if (state == 201) break;

						buffer->append(&ch, 1);

						state = 2;
						break;

					// ---------------
					case 201:
						temp = digit(ch);
						state = 202;
						break;

					// ---------------
					case 202:
						temp = (temp<<4) + digit(ch);
						buffer->append((char*)&temp, 1);

						state = 2;
						break;

				// -----------------------------------------------
				// Numeric

				case 3:
					if (ch >= '0' && ch <= '9')
					{
						buffer->append(&ch, 1);
						break;
					}

					if (ch == '.')
					{
						buffer->append(&ch, 1);
						state = 300;
						break;
					}

					if (ch == 'e' || ch == 'E')
					{
						temp = 0; sign2 = 1;
						state = 301;
						break;
					}

					ungetc(ch, input);
					return new Object (sign*std::atof(buffer->c_str()));

					// After the decimal point.
					case 300:
						if (ch >= '0' && ch <= '9')
						{
							buffer->append(&ch, 1);
							break;
						}

						if (ch == 'e' || ch == 'E')
						{
							temp = 0; sign2 = 1;
							state = 301;
							break;
						}

						ungetc(ch, input);
						return new Object (sign*std::atof(buffer->c_str()));

					// After the E indicator (sign).
					case 301:
						if (ch == '+')
						{
							sign2 = 1;
							state = 302;
							break;
						}

						if (ch == '-')
						{
							sign2 = -1;
							state = 302;
							break;
						}

						state = 302;

					// After the E indicator (digits).
					case 302:

						if (ch >= '0' && ch <= '9')
						{
							temp = (temp*10) + digit(ch);
							break;
						}

						ungetc(ch, input);
						return new Object (sign*std::atof(buffer->c_str())*std::pow(10.0f, sign2*temp));

				// -----------------------------------------------
				// t[r]ue

				case 4:
					if (ch != 'r') return nullptr;
					state = 400;
					break;

					// tr[u]e
					case 400:
						if (ch != 'u') return nullptr;
						state++;
						break;

					// tru[e]
					case 401:
						if (ch != 'e') return nullptr;
						state++;
						break;

					// true
					case 402:
						if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) return nullptr;

						ungetc(ch, input);
						return new Object(true);

				// -----------------------------------------------
				// f[a]lse

				case 5:
					if (ch != 'a') return nullptr;
					state = 500;
					break;

					// fa[l]se
					case 500:
						if (ch != 'l') return nullptr;
						state++;
						break;

					// fal[s]e
					case 501:
						if (ch != 's') return nullptr;
						state++;
						break;

					// fals[e]
					case 502:
						if (ch != 'e') return nullptr;
						state++;
						break;

					// false
					case 503:
						if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) return nullptr;

						ungetc(ch, input);
						return new Object(false);

				// -----------------------------------------------
				// n[u]ll
				case 6:
					if (ch != 'u') return nullptr;
					state = 600;
					break;

					// nu[l]l
					case 600:
						if (ch != 'l') return nullptr;
						state++;
						break;

					// nul[l]
					case 601:
						if (ch != 'l') return nullptr;
						state++;
						break;

					// null
					case 602:
						if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
							return nullptr;

						ungetc(ch, input);
						return new Object(NUL);

				// -----------------------------------------------
				// object (identifier first letter)
				case 7:
					if (ch <= 32) break;

					if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_')
					{
						buffer->append(&ch, 1);
						state = 700;
						break;
					}

					if (ch == '"')
					{
						state = 701;
						break;
					}

					if (ch == '}')
						return self;

					throw "Object: Error: Expected object member identifier.";

					// -----------------------------------------------
					// object (identifier rest)
					case 700:
						if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_')
						{
							buffer->append(&ch, 1);
							break;
						}

						ungetc(ch, input);
						state = 702;
						break;

					// -----------------------------------------------
					// object (identifier rest, ends with quote)
					case 701:
						if (ch == '\\')
						{
							state = 710;
							break;
						}

						if (ch == '"')
						{
							state = 702;
							break;
						}

						buffer->append(&ch, 1);
						break;

					// -----------------------------------------------
					// object (colon)
					case 702:
						if (ch <= 32) break;

						if (ch == ':')
						{
							pair = self->value._object->set(buffer->c_str(), nullptr)->getNode(buffer->c_str());

							elem = Object::loadFrom (input);
							if (elem == nullptr) return nullptr;

							pair->value = elem;

							state = 703;
							break;
						}

						throw "Object: Error: Expected ':' after object member identifier.";

					// -----------------------------------------------
					// object (comma or object-end)
					case 703:
						if (ch <= 32) break;

						if (ch == ',')
						{
							buffer->resize(0);
							state = 7;
							break;
						}

						if (ch == '}') return self;

						throw "Object: Error: Expected ',' or '}' after object member value.";

					// ---------------
					case 710:

						switch (ch)
						{
							case '0': ch = '\0'; break;
							case 'b': ch = '\b'; break;
							case 't': ch = '\t'; break;
							case 'n': ch = '\n'; break;
							case 'f': ch = '\f'; break;
							case 'r': ch = '\r'; break;
							case '\\': ch = '\\'; break;
							case '\"': ch = '\"'; break;
							case '\'': ch = '\''; break;
							case 'x':
								temp = 0;
								state = 711;
								break;
						}

						if (state == 711) break;

						buffer->append(&ch, 1);

						state = 701;
						break;

					// ---------------
					case 711:
						temp = digit(ch);
						state = 712;
						break;

					// ---------------
					case 712:
						temp = (temp<<4) + digit(ch);
						buffer->append((char*)&temp, 1);

						state = 701;
						break;

				// -----------------------------------------------
				// array
				case 8:

					elem = Object::loadFrom (input);
					if (elem == nullptr)
					{
						state = 800;
						break;
					}

					self->value._array->push(elem);

					state = 800;
					break;
					
					// -----------------------------------------------
					// array (comma or array-end)
					case 800:
						if (ch <= 32) break;

						if (ch == ',')
						{
							ungetc(ch, input);
							state = 8;
							break;
						}

						if (ch == ']') return self;

						throw "Object: Error: Expected ',' or ']' after array value.";
			}
		}

		throw "Object: Error: Unexpected end of source stream.";
		return nullptr;
	}

	/**
	**	Parses an object from the specified file (JSON format) and returns it.
	*/
	Object *Object::loadFrom (const char* filename)
	{
		FILE *input = fopen (filename, "rb");
		if (input == nullptr) throw "Object: Unable to open input file.";

		Object *elem = nullptr;

		try {
			elem = loadFrom (input);
			if (elem == nullptr) throw "Object: Unable to parse input file.";
		}
		catch (...) {
			fclose (input);
			throw;
		}

		fclose (input);
		return elem;
	}

	/**
	**	Saves the object to the specified Buffer (JSON format).
	*/
	void Object::saveTo (Buffer *output, bool pretty, bool strict, int level)
	{
		auto writeEscaped = [](Buffer *output, const char* str)
		{
			int state = 0;

			while (*str != '\0')
			{
				int ch = *str++;

				switch (ch)
				{
					case '\0': output->write_uint8('\\'); output->write_uint8('0'); break;
					case '\b': output->write_uint8('\\'); output->write_uint8('b'); break;
					case '\t': output->write_uint8('\\'); output->write_uint8('t'); break;
					case '\n': output->write_uint8('\\'); output->write_uint8('n'); break;
					case '\f': output->write_uint8('\\'); output->write_uint8('f'); break;
					case '\r': output->write_uint8('\\'); output->write_uint8('r'); break;
					case '\\': output->write_uint8('\\'); output->write_uint8('\\'); break;
					case '\"': output->write_uint8('\\'); output->write_uint8('"'); break;
					case '\'': output->write_uint8('\\'); output->write_uint8('\''); break;
					default: output->write_uint8(ch); break;
				}
			}
		};

		switch (type)
		{
			case ARRAY:
				output->write("[\n");
				level++;

				for (Linkable<Object*> *i = this->value._array->head(); i; i = i->next())
				{
					if (pretty) output->write(String::sprintf("%*s", 4*level, ""));
					i->value->saveTo(output, pretty, strict, level);
					if (i->next()) {
						output->write_uint8(',');
						if (pretty) output->write_uint8('\n');
					}
				}

				level--;
				if (pretty) output->write(String::sprintf("\n%*s", 4*level, ""));
				output->write("]");
				break;

			case OBJECT:
				output->write("{\n");
				level++;

				for (Linkable<Pair<const String*,Object*>*> *i = this->value._object->get_data()->head(); i; i = i->next())
				{
					if (pretty) output->write(String::sprintf("%*s", 4*level, ""));
					if (strict) output->write_uint8('"');
					writeEscaped(output, i->value->key->c_str());
					if (strict) output->write_uint8('"');
					output->write_uint8(':');
					if (pretty) output->write_uint8(' ');

					i->value->value->saveTo(output, pretty, strict, level);
					if (i->next()) {
						output->write_uint8(',');
						if (pretty) output->write_uint8('\n');
					}
				}

				level--;
				if (pretty) output->write(String::sprintf("\n%*s", 4*level, ""));
				output->write_uint8('}');
				break;

			case BOOL:
				output->write(this->value._bool ? "true" : "false");
				break;

			case NUMERIC:
				output->write(String::sprintf("%g", this->value._numeric));
				break;

			case STRING:
				output->write_uint8('"');
				writeEscaped(output, this->value._string->c_str());
				output->write_uint8('"');
				break;

			case NUL:
				output->write("null");
				break;
		}
	}

	/**
	**	Saves the object to the specified FILE (JSON format).
	*/
	bool Object::saveTo (FILE *output, bool pretty, bool strict)
	{
		if (output == nullptr) return false;

		OFileBuffer fso (output);
		saveTo(&fso, pretty, strict);

		return true;
	}

	/**
	**	Saves the object to the specified file (JSON format).
	*/
	bool Object::saveTo (const char* filename, bool pretty, bool strict)
	{
		if (filename == nullptr) return false;

		OFileBuffer fso (filename, OFileBuffer::O_BINARY);
		saveTo (&fso, pretty, strict);

		return true;
	}

}};
