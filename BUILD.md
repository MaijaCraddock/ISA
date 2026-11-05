Put all .cpp and .hpp files in a /src folder in the same directory as CMakeLists.txt

To use the cmake on Windows with C++23, run the following commands:

cmake -B build -S . || exit /b 1
cmake --build build --config Debug --clean-first > build.log 2>&1 || (type build.log & exit /b 1)
type build.log
build\Debug\machine.exe

Follow the last command with flags:
ex. build\Debug\machine.exe -harts 1