#pragma once
#include <string>
#include <map>
#include <list>
#include <memory>
using namespace std;

struct object
{
	enum class e_type
	{
		_null,
		_type,
		_method,
		_variant,
		_enum,
		_enum_value,
		_comment,
	};
	e_type type;
	int ppp = 0;// 0=public,1=private, 2=protected
	unsigned line = 0;
	object(e_type type = e_type::_null) :type(type) {}
	virtual ~object() {}
	operator bool()
	{
		return type != e_type::_null;
	}
	string to_string()
	{
		if (type == e_type::_type)return "class";
		else if (type == e_type::_enum)return "enum";
		else if (type == e_type::_enum_value)return "enum value";
		else if (type == e_type::_variant)return "variant";
		else if (type == e_type::_comment)return "comment";
		else if (type == e_type::_method)return "function";
		else return "nothing";
	}
};

struct comment : public object
{
	string content;
	comment(const string& content) :object(e_type::_comment), content(content) {}
};

struct variant : public object
{
	bool isconst = false;
	bool isextern = false;
	bool isstatic = false;
	string reftype;// && & &*
	string type;
	string name;
	string default_value;
	variant() :object(e_type::_variant) {}
};

struct enumeration_value : public object
{
	string name;
	string value;
	enumeration_value() :object(e_type::_enum_value) {}
};

struct enumeration : public object
{
	string name;
	string base;
	bool isclass = false;
	map<string, shared_ptr<enumeration_value>> members;
	enumeration() :object(e_type::_enum) {}
};

struct method : public object
{
	string return_type;
	bool isconst = false;
	bool isextern = false;
	bool isstatic = false;
	bool decl = false;
	using object::object;
	list<shared_ptr<variant>> argument;
	method() :object(e_type::_method) {}
	~method() {}
};

struct context;
typedef shared_ptr<context> context_ptr;
struct context
{
	context_ptr parent;
	multimap<string, shared_ptr<object>> objects;
	context(context_ptr parent = nullptr) :parent(parent) {}
};

struct type : public object
{
	using object::object;
	string name;
	bool pre_declare = false;
	bool is_template = false;
	string bases;
	list<string> friends;
	context_ptr context;
	multimap<string, shared_ptr<object>> objects;
	type() :object(e_type::_type), context(new ::context()) {}
};

class cpp_analysis
{
public:
	static shared_ptr<context> analysis(const string& filename) throw();
};