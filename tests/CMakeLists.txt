include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
  rallytest
  stackallocator.test.cc
  threadpool.test.cc
  vec.test.cc
)
target_link_libraries(
  rallytest
  gtest_main
  rally
)

include(GoogleTest)
gtest_discover_tests(rallytest)