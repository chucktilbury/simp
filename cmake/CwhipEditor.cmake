set(CWHIP_EDITOR_EXECUTABLE "${CWHIP_RUNTIME_OUTPUT_DIRECTORY}/cwhip-editor${CMAKE_EXECUTABLE_SUFFIX}")
if(NOT CWHIP_GTK_SOURCEVIEW)
    add_custom_target(cwhip_editor
        COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_LIST_DIR}/CwhipEditorUnavailable.cmake"
        VERBATIM)
    message(STATUS "Cwhip Editor unavailable: requires CWHIP_GTK=ON and CWHIP_GTK_SOURCEVIEW=ON with GTK 4 and GtkSourceView 5 development files")
    return()
endif()

set(cwhip_editor_source "${CMAKE_CURRENT_SOURCE_DIR}/../examples/editor")
set(cwhip_editor_project "${CMAKE_CURRENT_BINARY_DIR}/cwhip-editor-project")
file(GLOB_RECURSE cwhip_editor_inputs CONFIGURE_DEPENDS "${cwhip_editor_source}/*")
file(MAKE_DIRECTORY "${cwhip_editor_project}/modules"
    "${cwhip_editor_project}/home" "${cwhip_editor_project}/xdg")
# Use only this build's resources and package roots, never user overrides.
set(cwhip_editor_environment
    "${CMAKE_COMMAND}" -E env
    --unset=CWHIP_PACKAGE_PATH --unset=CWHIP_MODULE_REGISTRY --unset=CWHIP_MODULE_DIR
    --unset=DESTDIR
    "HOME=${cwhip_editor_project}/home" "XDG_CONFIG_HOME=${cwhip_editor_project}/xdg"
    "CWHIP_HOME=${CWHIP_STAGE_PREFIX}"
    "CWHIP_RUNTIME_DIR=${CWHIP_STAGE_RUNTIME_DIRECTORY}"
    "CWHIP_INCLUDE_DIR=${CWHIP_STAGE_PREFIX}/${CWHIP_RELATIVE_INCLUDEDIR}"
    "CWHIP_BUILTIN_DIR=${CWHIP_STAGE_BUILTIN_DIRECTORY}"
    "CWHIP_STDLIB_MODULE_DIR=${CWHIP_STAGE_MODULE_DIRECTORY}"
    "CC=${CWHIP_CLANG_EXECUTABLE}")
add_custom_command(
    OUTPUT "${cwhip_editor_project}/cwhip-pkg.toml" "${cwhip_editor_project}/cwhip-pkg.lock"
    COMMAND "${CMAKE_COMMAND}" -E remove -f
        "${cwhip_editor_project}/cwhip-pkg.toml" "${cwhip_editor_project}/cwhip-pkg.lock"
    COMMAND ${cwhip_editor_environment} "${Python3_EXECUTABLE}"
        "${CWHIP_RUNTIME_OUTPUT_DIRECTORY}/cwhip-pkg" init "${cwhip_editor_project}"
    DEPENDS cwhip_resources "${CWHIP_RESOURCE_STAMP}"
        "${CMAKE_CURRENT_LIST_FILE}"
        "${CWHIP_RUNTIME_OUTPUT_DIRECTORY}/cwhip-pkg"
        "$<TARGET_FILE:cwhip_sqlite_native>" "$<TARGET_FILE:cwhip_gtk_native>"
    COMMENT "Locking Cwhip Editor's staged standard packages"
    VERBATIM)
add_custom_command(
    OUTPUT "${CWHIP_EDITOR_EXECUTABLE}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${cwhip_editor_source}" "${cwhip_editor_project}/editor"
    COMMAND ${cwhip_editor_environment} "$<TARGET_FILE:cwhip>"
        "${cwhip_editor_project}/editor/cwhip_editor.cw"
        -M "${CWHIP_STAGE_MODULE_DIRECTORY}" -o "${CWHIP_EDITOR_EXECUTABLE}"
    DEPENDS cwhip cwhip_runtime cwhip_resources
        "${CMAKE_CURRENT_LIST_FILE}"
        "$<TARGET_FILE:cwhip_runtime>" "$<TARGET_FILE:cwhip_gtk_native>"
        "$<TARGET_FILE:cwhip_sqlite_native>"
        "${CWHIP_RESOURCE_STAMP}" ${cwhip_editor_inputs}
        "${cwhip_editor_project}/cwhip-pkg.toml" "${cwhip_editor_project}/cwhip-pkg.lock"
    COMMENT "Building Cwhip Editor"
    VERBATIM)
add_custom_target(cwhip_editor DEPENDS "${CWHIP_EDITOR_EXECUTABLE}")
set_property(TARGET cwhip_editor PROPERTY CWHIP_EDITOR_EXECUTABLE "${CWHIP_EDITOR_EXECUTABLE}")
