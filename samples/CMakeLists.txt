add_executable(mempp_sample 
    ${CMAKE_CURRENT_SOURCE_DIR}/mempp_sample.cpp
)
target_link_libraries(mempp_sample
    PRIVATE
        Mempp::Mempp
)