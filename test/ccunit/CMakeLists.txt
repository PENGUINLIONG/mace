include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB MACE_CC_TEST_SRCS
  mace/utils/*.cc
  mace/port/*.cc
  mace/ops/*.cc
)

if(MACE_ENABLE_HTA)
  set(MACE_CC_TEST_SRCS ${MACE_CC_TEST_SRCS})
endif(MACE_ENABLE_HTA)

add_executable(mace_cc_test ${MACE_CC_TEST_SRCS})
target_link_libraries(mace_cc_test PUBLIC
  mace_cc_test_utils
  mace_static
  extra_link_libs_target
  gtest
  gtest_main
)

install(TARGETS mace_cc_test RUNTIME DESTINATION bin)
