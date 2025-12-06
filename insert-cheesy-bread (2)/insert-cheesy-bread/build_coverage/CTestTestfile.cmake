# CMake generated Testfile for 
# Source directory: /usr/src/projects/insert-cheesy-bread
# Build directory: /usr/src/projects/insert-cheesy-bread/build_coverage
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/usr/src/projects/insert-cheesy-bread/build_coverage/config_parser_test[1]_include.cmake")
include("/usr/src/projects/insert-cheesy-bread/build_coverage/session_test[1]_include.cmake")
include("/usr/src/projects/insert-cheesy-bread/build_coverage/server_test[1]_include.cmake")
include("/usr/src/projects/insert-cheesy-bread/build_coverage/server_main_test[1]_include.cmake")
include("/usr/src/projects/insert-cheesy-bread/build_coverage/request_parser_test[1]_include.cmake")
include("/usr/src/projects/insert-cheesy-bread/build_coverage/response_builder_test[1]_include.cmake")
include("/usr/src/projects/insert-cheesy-bread/build_coverage/request_dispatcher_test[1]_include.cmake")
add_test(integration_test "/usr/bin/python3" "/usr/src/projects/insert-cheesy-bread/tests/integration_test.py")
set_tests_properties(integration_test PROPERTIES  ENVIRONMENT "SERVER_BINARY=/usr/src/projects/insert-cheesy-bread/build_coverage/bin/server_main" TIMEOUT "20" WORKING_DIRECTORY "/usr/src/projects/insert-cheesy-bread" _BACKTRACE_TRIPLES "/usr/src/projects/insert-cheesy-bread/CMakeLists.txt;117;add_test;/usr/src/projects/insert-cheesy-bread/CMakeLists.txt;0;")
subdirs("googletest")
