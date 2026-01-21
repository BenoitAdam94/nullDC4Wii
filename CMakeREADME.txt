
To use cmake:

mkdir build && cd build

and do 

cmake ..
ccmake ..	(recommended, small CLI UI/menu)
or use another cmake UI (there are a few)

then just build via whatever method was used:

	make for gcc toolchains,
	codeblocks / * project
	visual studio project	(recommend using existing VS SLN)
	*

TODO:

	Add more support for cross compiling, 
	right now it expects to use the default host compiler (and should find it automatically)

Other notes:

	For cmake you need to pass args to what type of project this is or it will default to linux/x86 prob?


