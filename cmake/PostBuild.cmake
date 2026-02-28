# --- Post build steps ---
set(CONFIG_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/config/config.toml")
set(CONFIG_OUTPUT_DIR "$<TARGET_FILE_DIR:maze_cli>/config")

add_custom_command(
    TARGET maze_cli POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CONFIG_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CONFIG_SOURCE}
            ${CONFIG_OUTPUT_DIR}
    COMMENT "Copying config.toml to output directory"
)
