#include "cpp_analysis.h"
#include <vector>
#include <set>
#include <fstream>
#include <sstream>
#include <iostream>

class eof : public exception {};

struct reader
{
	ifstream file;
	char current;
	bool end = false;
	reader(const string& filename) : file(filename)
	{
		if (!file)throw logic_error("file not found.");
		read();
	}
	bool read()
	{
		if (end)throw eof();
		char ret;
		file.read(&ret, 1);
		if (!file)return false;
		current = ret;
		return true;
	}
	char operator++()
	{
		if (!read())
		{
			end = true;
			current = ' ';
		}
		return current;
	}
	operator char()
	{
		return current;
	}
};

map<string, vector<string>> errors = {
	{"fmt", {"%s line %u:", "%s 第%u行 : "}},
	{"C1021", {"invalid preprocessor command", "无法识别的预处理指令"}},
	{"C2001", {"newline in constant", "常量中有换行符"}},
	{"C2015", {"too many characters in constant", "字符常量中太多字符"}},
	{"C2018", {"unknown character ", "未知字符"}},
	{"C2059", {"syntax error", "语法错误"}},
	{"C2143", {"syntax error : missing ']' before ')'", "语法错误 : 在“)”前缺少“]”"}},

	{"M1", {"don't support default value", "不支持默认值"}}
};

typedef shared_ptr<object> object_ptr;
typedef shared_ptr<comment> comment_ptr;
#define comment_ptr_new(v) comment_ptr(new comment(v))
typedef shared_ptr<variant> variant_ptr;
#define variant_ptr_new variant_ptr(new variant)
typedef shared_ptr<enumeration_value> enumeration_value_ptr;
#define enumeration_value_ptr_new enumeration_value_ptr(new enumeration_value)
typedef shared_ptr<enumeration> enumeration_ptr;
#define enumeration_ptr_new enumeration_ptr(new enumeration)
typedef shared_ptr<method> method_ptr;
#define method_ptr_new variant_ptr(new method)
typedef shared_ptr<type> type_ptr;
#define type_ptr_new type_ptr(new type)

struct st_state
{
	enum class e_domain { namespace_, global_, class_, method_, enum_, unin_, struct_, if_, while_, do_, for_, nothing_ }domain = e_domain::global_;
	enum class e_permission { public_, private_, protected_ }permission = e_permission::public_;
	enum class e_var_property { nothing_, static_ = 0b0001, const_ = 0b0010, extern_ = 0b0100 };
	int var_property = (int)e_var_property::nothing_;
};


set<string> keywords = { "int","double","float","short","long",
"const","char","unsigned","class","struct","using","public",
"private","protected","template","static","extern","sizeof",
"new","delete","void","namespace","return","if","while",
"do","for","case","switch","break","continue","goto","operator",
"auto" };


class document
{
	multimap<string, string> directives;
	map<string, string> macros;
	uint32_t line = 1;
	reader iter;
	int lang_index = 0;

	context_ptr context = shared_ptr<struct context>(new struct context());
	void insert(string name, object_ptr obj, int ppp)
	{
		obj->ppp = ppp;
		obj->line = line;
		validate_varname(name);
		context->objects.insert(make_pair(name, obj));
	}
	operator char() { return iter; }
	void error(const char* str, const char* arg = nullptr)
	{
		string out(100, 0);
		sprintf(&out[0], errors["fmt"][lang_index].c_str(), str, line);
		out = out.c_str();
		if (errors.find(str) == errors.end())
			throw logic_error(str);
		throw logic_error(out + errors[str][lang_index] + (arg ? arg : ""));
	}
	char operator++()
	{
		++iter;
		return iter;
	}
	void push(context_ptr value)
	{
		value->parent = context;
		context = value;
	}
	void pop()
	{
		context = context->parent;
	}
	string readto_nextline()
	{
		stringstream ss;
		do
		{
			if (iter == '\n')
				break;
			ss << iter;
			++iter;
		} while (1);
		next();
		return ss.str();
	}
	void backslash()
	{
		if (iter == '/')
		{
			++iter;
			if (iter == '/')
			{
				++iter;
				insert("//", comment_ptr_new(readto_nextline()), 0);
			}
			else if (iter == '*')
			{
				++iter;
				readto_commentend();
			}
			else
				next();
		}
	}
	int next()
	{
		int count = 0;
		while (1)
		{
			if (iter.end)return 0;
			if (iter < 0)break;
			if (iter == '/')backslash();
			if (iter == '\n') { ++line; ++iter; continue; }
			if (!isblank(iter) && isprint(iter))break;
			++iter;
			++count;
		}
	}
	string get_word()
	{
		next();
		stringstream ss;
		while (1) {
			if ('_' == iter || '~' == iter || iter < 0) { ss << iter; ++iter; continue; }
			//if (':' == iter) { ++iter; if (iter != ':')error("syntax error，:: is namespace usage"); ss << "::"; ++iter; continue; }
			if (isblank(iter))break;
			if (ispunct(iter))break;
			if (!isprint(iter))break;
			ss << iter;
			++iter;
		}
		auto word = ss.str();
		next();
		return word;
	}
	string get_char_literal()
	{
		stringstream ss;
		for (;;)
		{
			if (iter == '\\') { ++iter; if (iter == '\r') { ++iter; } if (iter == '\n') ++line; ++iter; continue; }
			else if (iter == '\'') { ++iter; break; }
			else if (iter == '\n') { line++; }
			ss << iter;
			++iter;
		}
		return ss.str();
	}
	string get_string_literal()
	{
		stringstream ss;
		for (;;)
		{
			if (iter == '\n') { error("C2001"); }
			if (iter == '\\') { ++iter; if (iter == '\r') { ++iter; } if (iter == '\n') ++line; ++iter; continue; }
			if (iter == '"') { ++iter; break; }
			ss << iter;
			++iter;
		}
		return ss.str();
	}
	string get_range(char ch)
	{
		if (!(ch == '}' || ch == ']' || ch == '>' || ch == ')'))throw;
		char beg = ch == ']' ? '[' : (ch == '}' ? '{' : (ch == ')' ? '(' : (ch == '>' ? '<' : '?')));
		stringstream ss;
		for (;;)
		{
			if (iter == beg) { ss << iter; ++iter; ss << get_range(ch); ss << ch; continue; }
			else if (iter == '\'') { ++iter; ss << get_char_literal(); continue; }
			else if (iter == '"') { ++iter; ss << get_string_literal(); continue; }
			else if (iter == '\n') { ++line; }
			else if (iter == '/') { backslash(); continue; }
			else if (iter == ch)
			{
				++iter; next();	break;
			}
			ss << iter;
			++iter;
		}
		return ss.str();
	}
	void readto_commentend()
	{
		stringstream ss;
		do
		{
			if (iter == '*')
			{
				++iter;
				if (iter == '/')
				{
					++iter;
					break;
				}
				else
				{
					ss << '*';
				}
			}
			if (iter == '\n') { ++line; ++iter; continue; }
			ss << iter;
			++iter;
		} while (1);
		next();
		auto temp = comment_ptr_new(ss.str());
		insert("//", temp, 0);
	}
	string readto(char ch)
	{
		stringstream ss;
		do
		{
			if (iter == ch)
			{
				++iter;
				next();
				break;
			}
			if (iter == '(') { ++iter; get_range(')'); continue; }
			if (iter == '{') { ++iter; get_range('}'); continue; }
			if (iter == '[') { ++iter; get_range(']'); continue; }
			if (iter == '<') { ++iter; get_range('>'); continue; }
			if (iter == '\n') { ++line; }
			ss << iter;
			++iter;
		} while (1);
		return ss.str();
	}
	string readto(const string& match)
	{
		stringstream ss;
		do
		{
			if (match.find_first_of(iter) != string::npos)
			{
				break;
			}
			if (iter == '(') { ++iter; get_range(')'); continue; }
			if (iter == '{') { ++iter; get_range('}'); continue; }
			if (iter == '[') { ++iter; get_range(']'); continue; }
			if (iter == '<') { ++iter; get_range('>'); continue; }
			if (iter == '\n') { ++line;}
			ss << iter;
			++iter;
		} while (1);
		return ss.str();
	}
	void precmd()
	{
		++iter;
		auto word = get_word();
		if (word == "include" || word == "pragma" || word == "define" || word == "ifdef" || word == "else" || word == "if" || word == "endif")
		{
			readto_nextline();
		}
		else
			error("C1021");
	}
	void method_argument(method_ptr& mtd)
	{
		next();
		if (iter == ')') { ++iter; next(); return; }
		for (;;)
		{
			list<string> all;
			for (;;)
			{
				auto seg = get_word();
				if (seg.empty() && all.empty())
					error("C2059");
				if (iter >= 0 && ispunct(iter))
				{
					if (iter == '&' || iter == '*') { all.push_back(seg); all.push_back(string(1, iter)); ++iter; continue; }
					else if (iter == '[') { ++iter; next(); if (iter != ']')error("C2143"); all.push_back("[]"); ++iter; }
					else if (iter == '=')
						readto(",)");//error("M1");
					else if (iter == '<') { ++iter; all.push_back("<" + get_range('>') + ">"); continue; }
					else if (iter != ','&&iter != ')')
						error("C2018");
					variant_ptr temp(new variant);
					temp->name = all.empty() ? "" : seg;
					if (!temp->name.empty())validate_varname(temp->name);
					temp->type = all.empty() ? seg : list2str(all);
					mtd->argument.push_back(temp);
					if (iter == ')') { ++iter; next(); return; }
					++iter;
					break;
				}
				all.push_back(seg);
			}
		}
	}
	bool in(const string& char_list)
	{
		auto ch = (char)iter;
		for (auto item : char_list)
			if (item == ch)
				return true;
		return false;
	}
	void validate_varname(const string& word)
	{
		if (word.empty())
			error("C2059");
		if (keywords.find(word) != keywords.end())
			error("C2059");
		//if (word.find_first_of(':') != string::npos)
		//	error("C2059", ":");
	}
	string list2str(list<string> l) { stringstream ss; for (auto& s : l)ss << s << " "; return ss.str(); }
	// domain, 0=global,1=class,2=struct,3=globalfunction,4=memberfunction,5=lambda,6=namespace,7=noname
	void block(int dm)
	{
		auto ppp = 0;// 0=public,1=private, 2=protected
		while (1)
		{
			if(next() == 0)return;
			if (iter == '#') { precmd(); continue; }
			if (iter == ';') { ++iter; continue; }
			if (dm != 0 && iter == '}') { ++iter; return; }
			list<string> types;
			while (1)
			{
				auto word = get_word();
				if (word.empty())if (dm < 3)error("C2059"); else error("未实现执行代码的解析");
				if (dm == 1 || dm == 2)
				{
					if (word == "public") { ppp = 0; readto(':'); break; }
					else if (word == "private") { ppp = 1; readto(':'); break; }
					else if (word == "protected") { ppp = 2; readto(':'); break; }
				}
				if (word == "using" || word == "typedef")
				{
					readto(';');
					break;
				}
				else if (word == "class" || word == "struct")
				{
					auto tname = get_word();
					if (iter == ':')
					{
						++iter;
						auto temp = type_ptr(new type());
						temp->bases = readto('{');
						temp->name = tname;
						insert(tname, object_ptr(temp), ppp);
						temp->pre_declare = true;
						push(temp->context);
						block(word == "class" ? 1 : 2);
						temp->pre_declare = false;
						pop();
					}
					else if (iter == ';') { ++iter; break; }
					else if (iter == '{')
					{
						++iter;
						auto temp = type_ptr(new type());
						temp->name = tname;
						insert(tname, object_ptr(temp), ppp);
						temp->pre_declare = true;
						push(temp->context);
						block(word == "class" ? 1 : 2);
						temp->pre_declare = false;
						pop();
					}
					word = "class " + tname;
					if (iter == ';') { ++iter; break; }
				}
				else if (word == "template")
				{
					if (iter != '<')error("C2059", string(1, iter).c_str());
					++iter;
					get_range('>');
					types.push_back("template<>");
					continue;
				}
				else if (word == "operator")
				{
					word = get_word();
					if (word.empty())
					{
						if (iter == '(')
						{
							++iter;
							next();
							if (iter != ')')error("C2059", string(1, iter).c_str());
							++iter;
							next();
							if (iter != '(')error("C2059", string(1, iter).c_str());
							word = "operator()";
						}
						else
						{
							word = "operator" + readto("(");
						}
					}
					else
						word = "operator " + word;
				}
				else if (word == "enum")
				{
					auto enum_ = enumeration_ptr_new;
					auto name = get_word();
					if (name == "class")
					{
						enum_->isclass = true;
						name = get_word();
					}
					if (name.empty())
						enum_->name = "<unnamed_enum>";
					else
						enum_->name = name;
					if (iter == ':') { ++iter; enum_->base = readto('{'); }
					else if (iter != '{')error("C2059", string(1, iter).c_str());
					else ++iter;
					long long index = 0;
					while (1)
					{
						auto mname = get_word();
						auto var = enumeration_value_ptr_new;
						if (iter == '=')
						{
							++iter;
							auto value = get_word();
							if (value.empty())error("C2059", string(1, iter).c_str());
							index = atoi(value.c_str()) + 1;
							var->name = mname;
							var->value = value;
							if (!enum_->isclass)insert(mname, var, ppp);
							enum_->members[mname] = var;
							if (iter != ','&&iter != '}')error("C2059", string(1, iter).c_str());
							if (iter == '}') { ++iter; break; }
							++iter;
						}
						else if (iter == ',' || iter == '}')
						{
							var->name = mname;
							var->value = to_string(index++);
							if (!enum_->isclass)insert(mname, var, ppp);
							enum_->members[mname] = var;
							if (iter == '}') { ++iter; break; }
							++iter;
						}
						else
							error("C2059", string(1, iter).c_str());
					}
					insert(enum_->name, enum_, ppp);
					next();
					if (iter == ';') { ++iter; break; }
					continue;
				}

				if (iter >= 0 && ispunct(iter))
				{
					if (iter == ',')
					{
						++iter;
						variant_ptr var(new variant);
						var->reftype = list2str(types);
						var->name = word;
						insert(word, var, ppp);
						while (1) {
							auto vname = get_word();
							if (vname.empty())error("C2059");
							var->name = vname;
							insert(vname, var, ppp);
							if (iter == ';')break;
							if (iter != ',')error("C2059", string(1, iter).c_str());
							++iter;
						}
						break;
					}
					if (iter == ':')
					{
						do {
							++iter; if (iter != ':')error("C2059", ":"); ++iter;
							auto next = get_word(); if (next.empty())error("C2059", string(1, (char)iter).c_str());
							word += "::" + next;
						} while (iter == ':');
					}
					if (iter == '(')// function
					{
						++iter;
						method_ptr var(new method);
						method_argument(var);
						if (iter != ';')readto("{");
						if (iter == '{')
						{
							++iter; get_range('}');
						}
						else if (iter != ';')
							error("C2059", string(1, iter).c_str());
						else
							var->decl = true;
						var->return_type = list2str(types);
						insert(word, var, ppp);
						break;
					}
					else if (iter == ';')
					{
						++iter;
						variant_ptr var(new variant);
						var->name = word;
						var->reftype = list2str(types);
						insert(word, var, ppp);
						types.clear();
						break;
					}
					else if (iter == '=')
					{
						++iter;
						variant_ptr var(new variant);
						var->reftype = list2str(types);
						var->name = word;
						var->default_value = readto(",;");
						insert(word, var, ppp);
						if (iter == ';') { ++iter; break; }
						continue;
					}
					else if (iter == '<')
					{
						++iter;
						types.push_back(word+"<"+get_range('>')+">");
						continue;
					}
					else if (iter == '*' || iter == '&')
					{
						if (types.empty())
							error("C2059", string(1, iter).c_str());
						types.push_back(word);
						types.push_back(string(1, iter));
						++iter;
						continue;
					}
					else
					{
						//readto(';');
						//cout << "don't supported, ignore this line." << endl;
						//continue;
						error("C2059", string(1, iter).c_str());
					}
				}

				types.push_back(word);
			}
		}
	}
public:
	document(const string& filename) :iter(filename)
	{
		//string loc = setlocale(LC_ALL, "");
		//string forchs = "Chinese (Simplified)_China.936";
		//if (loc == forchs)
		//	lang_index = 1;
		if(iter==(char)0xEF && ++iter == (char)0xBB && ++iter == (char)0xBF)++iter;// UTF-8
		else if(iter==(char)0xFF && ++iter == (char)0xFE && ++iter == (char)0x00 && ++iter == (char)0x00)++iter;// UTF-32, little endian
		else if (iter == (char)0x00 && ++iter == (char)0x00 && ++iter == (char)0xFE && ++iter == (char)0xFF)++iter;// UTF-32, bit endian
		else if(iter==(char)0xFF && ++iter == (char)0xFE)++iter;// UTF-16, little endian
		else if(iter==(char)0xFE && ++iter == (char)0xFF)++iter;// UTF-16, bit endian
		try { block(0); }
		catch (eof& e) {}
	}
	auto get_root()
	{
		return context;
	}
};


shared_ptr<context> cpp_analysis::analysis(const string & filename) throw()
{
	document doc(filename);
	return doc.get_root();
}

#ifdef _DEBUG
int main(int argc, char* argv[])
{
	while (1) try
	{
		string in;
		cout << "pls input cpp header file name (enter or q for exit):" << endl;
		getline(cin, in);
		if (in.empty() || in == "q")return 0;
		auto result = cpp_analysis::analysis(in);
		cout << "analysis done." << result->objects.size() << " objects." << endl;
		cout << "q for next, ls/ll for list, cd classname for enter class, cd .. for parent class or range:" << endl;
		string note = ">";
		while (1)
		{
			cout << note;
			getline(cin, in);
			if (in == "q")break;
			else if (in == "ls")
			{
				for (auto& i : result->objects)
					cout << i.first << " ";
				cout << endl;
			}
			else if (in == "ll")
			{
				cout << "total " << result->objects.size() << " objects." << endl;
				for (auto& i : result->objects)
				{
					cout << i.first << " line:" << i.second->line << " type:" << i.second->to_string() << endl;
				}
			}
			else if (in.substr(0, 3) == "cd ")
			{
				auto t = in.substr(3);
				if (t == "..")
				{
					if (result->parent != nullptr)
					{
						result = result->parent;
						note = note.substr(0, note.find_last_of(':') - 1) + ">";
					}
				}
				else
				{
					auto iter = result->objects.find(t);
					if (result->objects.find(t) == result->objects.end()) { cout << "not found" << endl; continue; }
					else if (iter->second->type != object::e_type::_type) { cout << "is not type" << endl; continue; }
					result = ((type*)iter->second.get())->context;
					note.resize(note.size() - 1);
					note += "::" + t + ">";
				}
			}
		}
	}
	catch (logic_error& e)
	{
		auto what = e.what();
		cout << what << endl;
	}
	catch (...)
	{
		cout << "other error" << endl;
	}

	return 0;
}
#endif