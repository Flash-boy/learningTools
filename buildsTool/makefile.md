## makefile文件的使用和编写
### make和makefile
make是解释makefile文件的工具，makefile是一系列gcc类似的编译指令，将许多源文件编译生成目标文件，最后组合链接变成一个可执行文件，makefile有点类似于脚本，make是执行这个脚本的工具。
首先我们用一个实例来说明makefile文件的编写。以GNU的make使用手册的8个C文件和3个头文件来看看整个过程    
1. 如果这个工程吗，没有被编译过，那么整个C文件都要被编译并被链接
2. 如果这个工程某个C文件被修改，那么只需要编译这些被修改过的C文件，并链接程序
3. 如果某些头文件被修改，那么会编译所有引用了这些头文件的C文件，并链接程序
### makefile的规则
最简单的makefile规则是由    
target:prerequisites...    
command     
许多类似的语句构成    
例如：   
main:main.c   
gcc main.c -o main   
该例就是最简单的makefile,编译生成一个可执行文件main   
**target**:也就是一个目标文件(包括.i,.s,.o和可执行文件等)，也可以是一个标签
**prerequisites**:也就是生成target所需要的文件
**command**:利用prerequisites中的文件如何生成target文件   
整个makefile就是利用make不断解析类似的指令，直至完成   
### 一个示例
假设一共有main.c kbd.c command.c display.c insert.c search.c files.c utils.c这8个C文件和defs.h command.h buffer.h这3个头文件   
```
edit:main.o kbd.o command.o display.o insert.o search.o files.o utils.o
    gcc -o edit main.o kbd.o command.o display.o insert.o search.o files.o utils.o

main.o:main.c defs.h
    gcc -c main.c -o main.o

kbd.o:kbd.c defs.h command.h
    gcc -c kbd.c -o kbd.o

command.o:command.c defs.h command.h
    gcc -c command.c -o command.o

display.o:display.c defs.h buffer.h 
    gcc -c display.c -o display.o

insert.o:insert.c defs.h buffer.h
    gcc -c insert.c -o insert.o

search.o:search.c defs.h buffer.h
    gcc -c search.c -o search.o

files.o:files.c defs.h buffer.h command.h
    gcc -c files.c -o files.o

utils.o:utils.c defs.h
    gcc -c utils.c -o utils.o

clean:
    rm edit main.o kbd.o command.o display.o insert.o search.o files.o utils.o

```
1. 可以看到生成的可执行文件edit由8个.o文件组成，每个.o文件又由对于的.c文件和头文件生成。将makefile文件和源文件放在一个目录下使用make命令，某人会生成第一个目标文件也就是最上面的可执行文件edit，然后会查看8个目标文件.o是否存在，不存在就查找是否有生成.o文件的命令有就生成，没有就依赖报错，其实就是一棵依赖树。   
2. 可以看到每个如何生成目标文件的command前面都有空白，一定要以Tab键开头，make只管执行所定义的命令，make会检查target和prerequisites的修改日期如果prerequisites文件的日期比target新，或者target不存在那么后续的命令会被make执行  
3. 这里的clean并不是一个文件，其实他是一个标签，他并没有依赖任何文件，说明其后面定义的命令可以执行，但make默认是执行第一个target，所以要显示的执行该clean命令就需要在make后面显示的体现出来make clean,这也是我们看到网上许多开源的软件生成的makefile都提供的make clean命令。这是为了方便makefile定义不用编译的命令
### make工作方式
 在默认的方式下，也就是我们只输入make命令。那么，

make会在当前目录下找名字叫“Makefile”或“makefile”的文件。

如果找到，它会找文件中的第一个目标文件（target），在上面的例子中，他会找到“edit”这个文件，并把这个文件作为最终的目标文件。

如果edit文件不存在，或是edit所依赖的后面的.o文件的文件修改时间要比edit这个文件新，那么，他就会执行后面所定义的命令来生成edit这个文件。

如果edit所依赖的.o文件也存在，那么make会在当前文件中找目标为.o文件的依赖性，如果找到则再根据那一个规则生成.o文件。（这有点像一个堆栈的过程）

当然，你的C文件和H文件是存在的啦，于是make会生成.o文件，然后再用.o文件声明make的终极任务，也就是执行文件edit了。

这就是整个make的依赖性，make会一层又一层地去找文件的依赖关系，直到最终编译出第一个目标文件。在找寻的过程中，如果出现错误，比如最后被依赖的文件找不到，那么make就会直接退出，并报错，而对于所定义的命令的错误，或是编译不成功，make根本不理。make只管文件的依赖性，即，如果在我找了依赖关系之后，冒号后面的文件还是不在，那么对不起，我就不工作啦。

通过上述分析，我们知道，像clean这种，没有被第一个目标文件直接或间接关联，那么它后面所定义的命令将不会被自动执行，不过，我们可以显示要make执行。即命令——“make clean”，以此来清除所有的目标文件，以便重编译。

于是在我们编程中，如果这个工程已被编译过了，当我们修改了其中一个源文件，比如file.c，那么根据我们的依赖性，我们的目标file.o会被重编译（也就是在这个依性关系后面所定义的命令），于是file.o的文件也是最新的啦，于是file.o的文件修改时间要比edit要新，所以edit也会被重新链接了（详见edit目标文件后定义的命令）。

而如果我们改变了“command.h”，那么，kdb.o、command.o和files.o都会被重编译，并且，edit会被重链接。
### makefile高级使用
#### 变量的使用
变量的声明是为了方便makefile的书写，其实变量就是左边等价于右边   
makefile中变量声明用=，变量名=...,例如：   
Object=kbd.o command.o display.o insert.o search.o files.o utils.o   
使用变量就只需要$(Object)，例如就可以更改为   
```
Object=kbd.o command.o display.o insert.o search.o files.o utils.o 
edit:$(Object)
    gcc -o edit $(Object)
```
其实就是类似于各种脚本语言中变量的使用类似shell中   
#### make自动进行类型推导
make另外的强大的功能是根据目标文件推导所需要的文件例如   
```
main.o:main.c defs.h
    gcc -c main.c -o main.o
上面的显示指出来生成main.o需要的所以源文件和头文件，但我们知道
一般源文件和目标文件名字是一致的所以可以简写
main.o:defs.h
    gcc -c main.c -o main.o
这就是make的隐晦规则，当然如果你随便起的名字，让make猜不出来那到这一步就编译失败了
```
还有一个规则叫伪目标  
```
.PHONY:clean
clean:
    rm edit $(Object)
.PHONY表示clean是一个伪目标，不是我们传统认为的目标文件这只是标识出来clean是伪目标
不显示指出来向最开始那样写clean也知道clean是一个伪目标，因为不需要依赖任何文件
当然你也可以让伪目标有依赖，一般是为了生成多个目标文件，伪目标总是被执行的，不管target和prerequisites的日期关系
```
#### 另类的makefile
我们看到目标文件可以用变量表示从而简写，那么好多个头文件呢？其实也可以处理
```
objects = main.okbd.o command.o display.o \
insert.osearch.o files.o utils.o
edit : $(objects)
cc -o edit $(objects)
$(objects) : defs.h
kbd.o command.ofiles.o : command.h
display.o insert.osearch.o files.o : buffer.h

.PHONY : clean
clean :
rm edit $(objects)
```
但这种不清晰，容易混乱不推荐
### makefile总述
1. **makefile包含什么**
   makefile主要包括5个显式规则，隐晦规则，变量定义，文件指示，注释    
   1.显式规则：有makefile规则书写的如何生成目标文件，生成的命令，makefile主体内容    
   2.隐晦规则：由make提供的一些推导功能    
   3.变量的定义：makefile中定义的变量主要是字符串，类似于宏定义    
   4.文件指示：包括3部分，一个是makefile引用另一个makefile，类似于include，另一个是指定依据指示表示makefile某些部分有效，最后一个是定义一个多行命令    
   5.注释：makefile的注释用#号类似于shell脚本    
2. **makefile文件名**
   默认的make会在当前目录下查找makefile,Makefile,GNUmakefile文件，然后执行该文件，也可以显示指定文件名只要在make命令加上-f或者--file选项。推荐使用Makefile文件名
3. **include和环境变量MAKEFILES**
   include 可以包含其他makefile文件，可以加载其他makefile文件类似于#include预处理加载头文件   
   例如有a.mk,b.mk,c.mk,foo.mk,还有一个变量\$(bar)包含e.mk,f.mk   
   include foo.mk *.mk \$(bar)等价于
   include foo.mk a.mk b.mk c.mk e.mk f.mk   
   默认情况下会去当前文件夹下寻找对应文件也可以make -I dirname显示指出去那个目录下找  
   MAKEFILES环境变量类似于shell下的变量一旦被定义使用就会影响所以makefile,主要的影响是类似于include的动作  
4. **make执行规则**      
   1.读入所有makefiel    
   2.读入被include的makefile    
   3.初始化所以变量    
   4.推导满足条件的隐晦规则    
   5.为所以目标文件建立依赖关系    
   6.根据依赖关系决定哪些目标文件生成    
   7.执行生成命令    
   记住makefile的第一个目标文件为最终的目标  
   make支持三种通配符"*","?".[...]这和shell脚本一样   
   变量中通配符不起作用，因为变量类似宏定义，除非显示的用wildcard关键字指示是通配符   
   例如：objects:=\$(wildcard*.o)   
5. **文件搜寻**    
   1.一般情况依赖的文件makefile会去当前目录下寻找，但可以用VPATH特殊变量指示需要去寻找依赖文件的目录   
   VPATH=src:../headers   
   表明去当前目录下的src目录和上一个目录headers下去寻找依赖项      
   2.vpath(小写)是一个关键字可以指定某个依赖文件去某个目录下寻找          
    vpath \<pattern> \<dirname> 为符合模式\<pattern>的文件指定搜索目录\<dirname>     
    vpath \<pattern> 清除符合模式\<pattern>的文件搜索目录    
    vpath 清除所有已经被设置号的文件搜索目录      
    vpath %.h ../headers 为所有.h文件指定搜索目录为../headers      
    如果有重复的pattern则会按先后顺序去搜索   
6.  **静态模式**      
    \<target...>:\<target pattern>:\<prerequisites pattern>
    ```
    objects=foo.o bar.o
    all:$(object)
    $(object):%.o:%.c
        gcc -c $< -o $@
    其中最后两句就是静态模式，表示要生成foo.o,bar.o "%.o"表明要所有以.o结尾的目标，也就是foo.o bar.o,
    也就是target pattern的模式，而"%.c"表明依赖模式取模式"%.o"的"%",也就是foo,bar，并加上后缀.c
    也即是prerequisites pattern为foo.c bar.c,$<,$@是自动化变量表示上面选取的模式的输出
    $<这里表示foo.c bar.c,$@这里表示foo.o bar.o
    ```
7. **make的变量**     
   1.变量可以嵌套定义例如：        
   ```
   foo=$(bar)
   bar=realbar
   这里$(foo)的值就是realbar,但这种前面可以用后面变量的定义容易造成循环引用，容易死循环
   可以利用:=这种变量赋值只能后面的变量用前面的值
   foo:=$(x)bar
   x:=bar1
   这里foo的值就只是bar，而不是bar1
   ```
   2.定义有空格的变量   
   ```
   nullstring:=
   space:=$(nullstring) #变量搭配"#"号使用
   一个空的字符加一个空格然后以一个注释符号结束表示多了一个空格，主要#号的作用暗示了到#号前都是有效的字符
   FOO?=bar
   如果FOO没有被定义就为bar的值如果定义了就为原来的值什么也不做
   ```
   3.追加变量的值    
   ```
   objects=main.c foo.c
   objects+=bar.c 
   这样$(objects)就为main.c foo.c bar.c  
   ```
8. **判断语句**
   ```
   第一个关键字ifeq
   ifeq($(CC),gcc)
        .....
   else
        .....
   endif
   第二个关键字ifneq
   第三个关键字iddef 
   foo=bar
   ifdef foo
    ans=yes
   else
    ans=no
   endif
   如果foo不为空则是为真否则为假
   第四个关键字ifndef
   ```
9.  **函数**           
    \$(funName arg1,arg2,...) 
    函数的使用和变量类似，用$(函数名 参数1，参数2，...)用()抱起来，参数之间用,分隔    
    
    字符串处理函数       
    1.\$(subst \<from>,\<to>,\<text>)    
    功能：把字符串text中的from替换为to       
    例如:\$(subst ee,EE,feet)      
    把字符串feet中的ee替换为EE最后变为fEEt    
    2.\$(patsubst \<pattern>,\<replacement>,\<text>)     
    3.\$(strip \<string>)    
    4.\$(findstring\<find>,\<in>)   
    5.\$(filter \<pattern...>,\<text>)   
    6.\$(filter-out \<pattern...>,\<text>)   
    7.\$(sort \<list>)    
    8.\$(word \<n>,\<text>)    
    9.\$(wordlist \<s>,\<e>,\<text>)    
    10.\$(words \<text>)    
    11.\$(firstword \<text>)    
    文件名操作函数    
    1.\$(dir \<names...>)    
    2.\$(notdir \<names...>)    
    3.\$(suffix\<names...>)    
    4.\$(basename \<names...>)    
    5.\$(addsuffix \<suffix>,\<names...>)    
    6.\$(addprefix \<prefix>,\<names...>)    
    7.\$(join\<list1>,\<list2>)    
    其他函数   
    1.\$(if\<condition>,\<then-part>)    
    2.\$(call \<expression>,\<parm1>,\<parm2>,\<parm3>...)    
    3.\$(origin <variable>)    
    4.files:=$(shell echo *.c) shell执行的结果作为变量    
    5.\$(error \<text...>)    
### 总结
makefile是在gcc的基础上定义了如何组织多个文件的编译，是一个重要的工具，乍一看好像就是这么点东西，其实大工程还是不适合手写makefile，更好的方式是写CMakeLists.txt这个是对makefile的封装其对应的工具是cmake，cmake是处理CMakeLists.txt文件的，而cmake处理的结果是生成对应的makefile文件，然后利用make完成整个编译过程。

    


