add_definitions(-Wno-unused-result )
add_executable(test_app test.cpp ../include/machine.h ../include/states.h ../include/mach_mem.h ../include/tokenizer.h)
target_link_libraries (test_app ${Boost_LIBRARIES}  )
add_test (test_app test_app)

