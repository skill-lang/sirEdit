#include <fstream>
#include <sirEdit/main.hpp>
#include <stdlib.h>
#include <sstream>
#include <stdio.h>

using namespace std;


extern std::string sirEdit_codegen;

// Source: https://stackoverflow.com/questions/3213037/determine-if-linux-or-windows-in-c
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
static const std::string slash="\\";
#else
static const std::string slash="/";
#endif

/**
 * Escapes a string
 * @param toEscape the string that should be escaped
 * @param stream the target stream to add
 */
inline void escapeString(const string& toEscape, stringstream& stream) {
	for(auto& i : toEscape) {
		if(i == '$'  ||
		   i == '\\' ||
		   i == ' '  ||
		   i == '"'  ||
		   i == '\'' ||
		   i == ';'  ||
		   i == '|'  ||
		   i == '&'  ||
		   i == '('  ||
		   i == ')'  ||
		   i == '{'  ||
		   i == '}'  ||
		   i == '['  ||
		   i == ']'  ||
		   i == '*')
			stream << "\\" << i;
		else if(i == '\t')
			stream << "\\t";
		else if(i == '\n')
			stream << "\\n";
		else if(i == '\r')
			stream << "\\r";
		else
			stream << i;
	}
}


namespace sirEdit {
	int runCommand(const std::vector<std::string>& arguments, const std::string& path) {
		// parse args
		std::stringstream args;
		args << "cd ";
		escapeString(path, args);
		args << ";";
		for(auto& i : arguments) {
			args << " ";
			escapeString(i, args);
		}

		// Run code
		return system(args.str().c_str());
	}

	int runCodegen(const std::vector<std::string>& arguments, const std::string& path) {
		// Export codegen
		std::string codegenPath = path + slash + "codegen.jar";
		bool exportedCodegen = !(ifstream(codegenPath, ios::in).good());
		if(exportedCodegen) {
			ofstream output(codegenPath, ios::out | ios::binary);
			output << sirEdit_codegen;
			output.close();
		}

		// Run command
		{
			vector<string> command = {"java", "-jar", "codegen.jar"};
			command.resize(command.size() + arguments.size());
			for(size_t i = 0; i < arguments.size(); i++)
				command[i + 3] = arguments[i];
			return runCommand(command, path);
		}

		// Clean up
		if(exportedCodegen)
			remove(codegenPath.c_str());
	}
}
