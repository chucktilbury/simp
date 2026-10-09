set(SIMP_CWHIP_EDITOR_EXECUTABLE "${SIMP_RUNTIME_OUTPUT_DIRECTORY}/cwhip-editor${CMAKE_EXECUTABLE_SUFFIX}")
if(NOT SIMP_GTK_SOURCEVIEW)
    add_custom_target(tweed
        COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_LIST_DIR}/CwhipEditorUnavailable.cmake"
        VERBATIM)
    add_custom_target(cwhip_editor DEPENDS tweed)
    message(STATUS "Cwhip Editor unavailable: requires SIMP_GTK=ON and SIMP_GTK_SOURCEVIEW=ON with GTK 4 and GtkSourceView 5 development files")
    return()
endif()

set(simp_editor_source "${CMAKE_CURRENT_SOURCE_DIR}/../examples/editor")
set(simp_editor_project "${CMAKE_CURRENT_BINARY_DIR}/cwhip-editor-project")
file(GLOB_RECURSE simp_editor_inputs CONFIGURE_DEPENDS "${simp_editor_source}/*")
file(MAKE_DIRECTORY "${simp_editor_project}/modules"
    "${simp_editor_project}/home" "${simp_editor_project}/xdg")
# Use only this build's resources and package roots, never user overrides.
set(simp_editor_environment
    "${CMAKE_COMMAND}" -E env
    --unset=SIMP_PACKAGE_PATH --unset=SIMP_MODULE_REGISTRY --unset=SIMP_MODULE_DIR
    --unset=DESTDIR
    "HOME=${simp_editor_project}/home" "XDG_CONFIG_HOME=${simp_editor_project}/xdg"
    "SIMP_HOME=${SIMP_STAGE_PREFIX}"
    "SIMP_RUNTIME_DIR=${SIMP_STAGE_RUNTIME_DIRECTORY}"
    "SIMP_INCLUDE_DIR=${SIMP_STAGE_PREFIX}/${SIMP_RELATIVE_INCLUDEDIR}"
    "SIMP_BUILTIN_DIR=${SIMP_STAGE_BUILTIN_DIRECTORY}"
    "SIMP_STDLIB_MODULE_DIR=${SIMP_STAGE_MODULE_DIRECTORY}"
    "CC=${SIMP_CLANG_EXECUTABLE}")
add_custom_command(
    OUTPUT "${simp_editor_project}/simpkg.toml" "${simp_editor_project}/simpkg.lock"
    COMMAND "${CMAKE_COMMAND}" -E remove -f
        "${simp_editor_project}/simpkg.toml" "${simp_editor_project}/simpkg.lock"
    COMMAND ${simp_editor_environment} "${Python3_EXECUTABLE}"
        "${SIMP_RUNTIME_OUTPUT_DIRECTORY}/simpkg" init "${simp_editor_project}"
    DEPENDS simp_resources "${SIMP_RESOURCE_STAMP}"
        "${CMAKE_CURRENT_LIST_FILE}"
        "${SIMP_RUNTIME_OUTPUT_DIRECTORY}/simpkg"
        "$<TARGET_FILE:simp_sqlite_native>" "$<TARGET_FILE:simp_gtk_native>"
    COMMENT "Locking Cwhip Editor's staged standard packages"
    VERBATIM)
add_custom_command(
    OUTPUT "${SIMP_CWHIP_EDITOR_EXECUTABLE}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${simp_editor_source}" "${simp_editor_project}/editor"
    COMMAND ${simp_editor_environment} "$<TARGET_FILE:cwhip>"
        "${simp_editor_project}/editor/cwhip_editor.cw"
        -M "${SIMP_STAGE_MODULE_DIRECTORY}" -o "${SIMP_CWHIP_EDITOR_EXECUTABLE}"
    DEPENDS cwhip simp_runtime simp_resources
        "${CMAKE_CURRENT_LIST_FILE}"
        "$<TARGET_FILE:simp_runtime>" "$<TARGET_FILE:simp_gtk_native>"
        "$<TARGET_FILE:simp_sqlite_native>"
        "${SIMP_RESOURCE_STAMP}" ${simp_editor_inputs}
        "${simp_editor_project}/simpkg.toml" "${simp_editor_project}/simpkg.lock"
    COMMENT "Building Cwhip Editor"
    VERBATIM)
add_custom_target(cwhip_editor DEPENDS "${SIMP_CWHIP_EDITOR_EXECUTABLE}")
add_custom_target(tweed DEPENDS cwhip_editor)
set_property(TARGET cwhip_editor PROPERTY SIMP_EXECUTABLE "${SIMP_CWHIP_EDITOR_EXECUTABLE}")
set_property(TARGET tweed PROPERTY SIMP_EXECUTABLE "${SIMP_CWHIP_EDITOR_EXECUTABLE}")
