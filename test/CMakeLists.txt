add_definitions(-UNDEBUG)
remove_definitions(-DNDEBUG)

add_executable(test-version test-version.cpp)
target_link_libraries(test-version qtcurve-utils)
add_test(NAME test-version COMMAND test-version)

add_executable(test-buff test-buff.cpp)
target_link_libraries(test-buff qtcurve-utils)
add_test(NAME test-buff COMMAND test-buff)

add_executable(test-strmap test-strmap.cpp)
target_link_libraries(test-strmap qtcurve-utils)
add_test(NAME test-strmap COMMAND test-strmap)

add_executable(test-printf test-printf.cpp)
target_link_libraries(test-printf qtcurve-utils)
add_test(NAME test-printf COMMAND test-printf)

add_executable(test-strlist test-strlist.cpp)
target_link_libraries(test-strlist qtcurve-utils)
add_test(NAME test-strlist COMMAND test-strlist)

add_executable(test-default-arg test-default-arg.cpp)
target_link_libraries(test-default-arg qtcurve-utils)
add_test(NAME test-default-arg COMMAND test-default-arg)

add_executable(test-color-str test-color-str.cpp)
target_link_libraries(test-color-str qtcurve-utils)
add_test(NAME test-color-str COMMAND test-color-str)

add_executable(test-default test-default.cpp)
target_link_libraries(test-default qtcurve-utils)
add_test(NAME test-default COMMAND test-default)

add_executable(test-call-cpp test-call.cpp)
target_link_libraries(test-call-cpp qtcurve-utils)
add_test(NAME test-call-cpp COMMAND test-call-cpp)

add_executable(test-process test-process.cpp)
target_link_libraries(test-process qtcurve-utils)
add_test(NAME test-process COMMAND test-process)

add_executable(test-popen test-popen.cpp)
target_link_libraries(test-popen qtcurve-utils)
add_test(NAME test-popen COMMAND test-popen)

add_executable(test-popen-buff test-popen-buff.cpp)
target_link_libraries(test-popen-buff qtcurve-utils)
add_test(NAME test-popen-buff COMMAND test-popen-buff)
