
#include <asr/defs>
#include <unordered_map>
#include <string>
#include <iostream>
#include <fstream>
#include <set>

using namespace asr;
using namespace std;

set<string> base_types = {
    "int", "uint"
};

int get_indentation (const string& line) {
    int size = 0;
    for (char c : line) {
        if (c == ' ' || c == '\t') size++;
        else break;
    }
    return size;
}

bool is_comment (const string& line) {
    return line[0] == ';';
}

string ltrim (const string& line) {
    auto pos = line.find_first_not_of(" \t");
    if (pos != string::npos)
        return line.substr(pos);
    return line;
}

string rtrim (const string& line) {
    auto pos = line.find_last_not_of(" \t");
    if (pos != string::npos)
        return line.substr(0, pos+1);
    return line;
}

string get_word (string& line) {
    line = ltrim(line);
    auto pos = line.find_first_of(" \t");
    string value = pos != string::npos ? line.substr(0, pos) : line;
    line = line.substr(value.size());
    return rtrim(value);
}

bool expect_word (string& line, const string& expected) {
    string word = get_word(line);
    if (word != expected) {
        cout << "\e[91merror:\e[0m expected: \e[97m" << expected << "\e[0m found: \e[97m" << word << "\e[0m\n";
        return false;
    }
    return true;
}

struct Type {
    string name;
    string base_type;
    unordered_map<string, Type> locals;
    string code;
    int indent_level;

    Type()
    { }

    Type(const string& name, const string& base_type, int indent_level) 
        : name(name), base_type(base_type), indent_level(indent_level)
    { }
};

/**
 */
int _main (int argc, const char *argv[])
{
    ifstream schema(argv[1]);
    if (!schema) {
        cout << "\e[91merror:\e[0m failed to open schema file: \e[97m" << argv[1] << "\e[0m\n";
        return 1;
    }

    ofstream output(argv[2]);
    if (!output) {
        cout << "\e[91merror:\e[0m failed to open output file: \e[97m" << argv[2] << "\e[0m\n";
        return 1;
    }

    unordered_map<string, Type> types;
    Type active_type;

    string line, cmd;
    int state = 0, target_indent_level = -1;

    while (getline(schema, line))
    {
        int indent_level = get_indentation(line);

        line = rtrim(line.substr(indent_level));
        if (is_comment(line) || line.empty())
            continue;

        if (target_indent_level != -1) {
            if (indent_level != target_indent_level)
                continue;
            target_indent_level = -1;
        }

        cmd = get_word(line);
        cout << ":::" << cmd << endl;

        switch (state)
        {
            case 0: // type, record

                // type <type_name> as <base_type>
                if (cmd == "type")
                {
                    string name = get_word(line);
                    if (types.find(name) != types.end()) {
                        cout << "\e[91merror:\e[0m duplicate type: \e[97m" << name << "\e[0m\n";
                        target_indent_level = indent_level;
                        break;
                    }

                    if (!expect_word(line, "as")) {
                        target_indent_level = indent_level;
                        break;
                    }

                    string type = get_word(line);
                    if (base_types.find(type) == base_types.end()) {
                        cout << "\e[91merror:\e[0m invalid base type: \e[97m" << type << "\e[0m\n";
                        target_indent_level = indent_level;
                        break;
                    }

                    active_type = Type(name, type, indent_level);
                    types[name] = active_type;

                    state = 1;
                    break;
                }

                if (cmd == "record")
                {
                    target_indent_level = indent_level;
                    break;
                }

                cout << "\e[91merror:\e[0m invalid keyword: \e[97m" << cmd << "\e[0m\n";
                target_indent_level = indent_level;
                break;

            case 1: // local, *
                if (cmd == "local")
                {
                    string name = get_word(line);
                    if (active_type.locals.find(name) != active_type.locals.end()) {
                        cout << "\e[91merror:\e[0m " << active_type.name << ": duplicate local variable: \e[97m" << name << "\e[0m\n";
                        break;
                    }

                    if (!expect_word(line, "as"))
                        break;

                    string type = get_word(line);
                    if (base_types.find(type) == base_types.end()) {
                        cout << "\e[91merror:\e[0m " << active_type.name << ": invalid base type: \e[97m" << type << "\e[0m\n";
                        break;
                    }

                    active_type.locals[name] = Type(name, type, 0);
                    break;
                }

                if (cmd == "peek")
                {
                }

                //peek uint8 into val
                //if !(val & 0x80)
                //    load uint8 into val
                //    ret val
                //load uint32be into val
                //ret val

                break;
        }
    }

    return 0;
}

/**
 */
int main (int argc, const char *argv[])
{
    auto n = asr::memblocks;

    if (argc < 3) {
        cout << "Usage: rssc <schema-file> <output-file>\n";
        return 0;
    }

    int exitcode = _main(argc, argv);

    asr::refs::shutdown();
    if (asr::memblocks != n) {
        cout << "\e[31mMemory leak detected: \e[91m" << asr::memsize << " bytes\e[0m\n";
        return 1;
    }

    return exitcode;
}
