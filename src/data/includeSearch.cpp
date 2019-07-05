#include <tao/pegtl.hpp>
#include <sirEdit/data/skillInclude.hpp>
#include <fstream>
#include <stdio.h>

using namespace tao::pegtl;

// Language
struct TrueComment : public seq<one<'#'>, until<eolf>> {};

struct SingelLineComment : public seq<string<'/', '/'>, until<eolf>, eolf> {};
struct MultiLineComment : public seq<string<'/', '*'>, until<string<'*', '/'>>, string<'*', '/'>> {};
struct WhiteSpace : public sor<one<' ', '\t'>, eol, SingelLineComment, MultiLineComment> {};

struct CString : public seq<one<'"'>, star<sor<seq<one<'\\'>, any>, not_one<'\\', '"'>>> , one<'"'>> {};
struct Import : public seq<sor<TAO_PEGTL_STRING("import"), TAO_PEGTL_STRING("with")>, plus<WhiteSpace, CString>> {};

struct Document : public seq<star<TrueComment>, star<Import>, until<eof>> {};

// Parse
template<class RULE>
struct Accepter : nothing<RULE> {};
template<>
struct Accepter<CString> {
	template<class INPUT>
	static void apply(const INPUT& in, std::list<std::string>& out) {
		out.push_back(in.string());
	}
};

std::list<std::string> sirEdit::data::SKilL_Include::__addIncludes(const std::string& source) {
	// Get includes
	std::list<std::string> tmp;
	{
		auto repres = string_input(source, "");
		tao::pegtl::parse<Document, Accepter>(repres, tmp);
	}

	// Remove back ticks
	std::list<std::string> result;
	for(auto& i : tmp) {
		std::string tmp2 = "";
		for(size_t j = 1; j < i.size() - 1; j++) {
			if(i[j] == '\\')
				j++;
			tmp2 += i[j];
		}
		result.push_back(tmp2);
	}
	return result;
}

// Source: https://stackoverflow.com/questions/3213037/determine-if-linux-or-windows-in-c
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
static const std::string slash="\\";
#else
static const std::string slash="/";
#endif

void sirEdit::data::SKilL_Include::__addFile(const std::string& file, const std::string& content) {
	std::ofstream out(this->__path + slash + file, std::ios::out | std::ios::binary);
	out << content;
	out.close();
}
void sirEdit::data::SKilL_Include::__deleteFile(const std::string& file) {
	remove((this->__path + slash + file).c_str());
}
