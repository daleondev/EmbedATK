add_executable(mempp_sample 
    ${CMAKE_CURRENT_SOURCE_DIR}/mempp_sample.cpp
)
target_link_libraries(mempp_sample
    PRIVATE
        Mempp::Mempp
)

# add_executable(statemachine_sample 
#     ${CMAKE_CURRENT_SOURCE_DIR}/statemachine_sample.cpp
# )
# target_link_libraries(statemachine_sample
#     PRIVATE
#         Mempp::Mempp
# )