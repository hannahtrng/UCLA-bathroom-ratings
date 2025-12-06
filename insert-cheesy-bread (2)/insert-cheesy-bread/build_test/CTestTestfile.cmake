# CMake generated Testfile for 
# Source directory: /usr/src/projects/insert-cheesy-bread
# Build directory: /usr/src/projects/insert-cheesy-bread/build_test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/usr/src/projects/insert-cheesy-bread/build_test/config_parser_test[1]_include.cmake")
include("/usr/src/projects/insert-cheesy-bread/build_test/session_test[1]_include.cmake")
include("/usr/src/projects/insert-cheesy-bread/build_test/server_test[1]_include.cmake")
include("/usr/src/projects/insert-cheesy-bread/build_test/server_main_test[1]_include.cmake")
include("/usr/src/projects/insert-cheesy-bread/build_test/request_parser_test[1]_include.cmake")
include("/usr/src/projects/insert-cheesy-bread/build_test/response_builder_test[1]_include.cmake")
include("/usr/src/projects/insert-cheesy-bread/build_test/request_dispatcher_test[1]_include.cmake")
add_test(integration_test "/usr/bin/python3" "/usr/src/projects/insert-cheesy-bread/tests/integration_test.py")
set_tests_properties(integration_test PROPERTIES  ENVIRONMENT "SERVER_BINARY=/usr/src/projects/insert-cheesy-bread/build_test/bin/server_main" TIMEOUT "20" WORKING_DIRECTORY "/usr/src/projects/insert-cheesy-bread" _BACKTRACE_TRIPLES "/usr/src/projects/insert-cheesy-bread/CMakeLists.txt;126;add_test;/usr/src/projects/insert-cheesy-bread/CMakeLists.txt;0;")
subdirs("googletest")
