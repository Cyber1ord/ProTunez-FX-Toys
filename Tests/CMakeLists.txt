include(FetchContent)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(Catch2)

add_executable(DSPTests DSPTests.cpp)
target_compile_features(DSPTests PRIVATE cxx_std_17)
target_link_libraries(DSPTests PRIVATE Catch2::Catch2WithMain)

include(CTest)
add_test(NAME DSPTests COMMAND DSPTests)
