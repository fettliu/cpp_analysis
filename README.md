# cpp_analysis #
cpp grammar format analysis

### run ###
1. open with vs2015.
2. F5 to run.

### program ###
1. input analysis_target.h and enter.
2. input ll to list member.

### usage ###
To compile as library.
```c++
#include <cpp_analysis.h>
try
{
　auto result = cpp_analysis::analysis("[cpp_header_file]");
　// todo
}
catch (logic_error& e)
{
// todo:
}
```



[中文介绍](http://www.cnblogs.com/fyter/p/analysis-cpp-syntex-format.html)
