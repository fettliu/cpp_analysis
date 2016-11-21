#pragma once
#include <string>
#include <set>
using namespace std;
static int first//注释暂时，这里有各种中文字符
, second123, _klksd123;

template<class T>class templateclass {
	int i;
	templateclass();
	//templateclass(int):i(0){
	//}
	~templateclass();
};
template<class T>void templatefunction();
;;;;
static double 中文变量名测试 = 100;
// Lambda Variant
static auto lambdavar = []() {"\}\[\}\)\("; };
;;;
enum namedenum
{
	ldfslkd,
	sk,
	lk23
};
static enum : long long {
	kladf,
	klsdjf = 67,
	sldf
}eee, eeee2;

//void (*lamfunc)() = []() {};
static const int* intptr = nullptr;

static const class name {} name1, name2;
static class name *name3, name4;

static set<int> funct(int, char)/*C Style Comment*/
{
	char ch = 'a\\
		\n\'';// VS has a bug for ch = '\t'
	auto literal = "lsjdklj\"klj//kjsdf\
sk>  )d]jfk}";
	[]() {}();

	{
		// TODO
	}
	return set<int>();
}

// 注释
class test
{
	template<class T>class tc {
		T vvv;
	};
public:int vvvv;
	   // 注释
	   class vvvv {
		   double v;
	   };
	   int vv;
	   double 中文变量名;
	   // 注释
	   int func();
	   void 中文函数名(int i);

private:
	void function(const int const v);
};

class testb : public test
{
public:
	int func2(void* v, int *);
};

int testb::func2(void* v, int*) throw()
{
	return 0;
}