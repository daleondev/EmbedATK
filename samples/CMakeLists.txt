add_executable(eatk_sample 
    ${CMAKE_CURRENT_SOURCE_DIR}/eatk_sample.cpp
)
target_link_libraries(eatk_sample
    PRIVATE
        EmbedATK::EmbedATK
)