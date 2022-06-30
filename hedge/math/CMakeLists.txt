
file(GLOB hedge_MATH_SRC_FILES "*.c")

target_sources(hedge
    PUBLIC
    ${hedge_MATH_SRC_FILES}
)

install(
    FILES
    #        ${CMAKE_CURRENT_LIST_DIR}/batchencoder.h
    DESTINATION
        ${hedge_INCLUDES_INSTALL_DIR}/hedge
)
