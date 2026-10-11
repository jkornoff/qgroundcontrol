# ============================================================================
# BSD Custom Build Overlay
# Keeps Big Sky Dynamics changes out of src/ so upstream rebases stay clean.
# See custom/README.md and BSD_NOTES.md.
# ============================================================================

message(STATUS "QGC: Adding BSD custom plugin")

# ----------------------------------------------------------------------------
# QML resource overrides
# Alias must exactly match the upstream resource path under /qml; the URL
# interceptor in CustomPlugin.cc redirects loads of :/qml/<alias> to
# :/Custom/qml/<alias> when the override exists.
# ----------------------------------------------------------------------------

set(CUSTOM_QML_OVERRIDES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PlanView/PlanToolBarIndicators.qml"
)
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/PlanView/PlanToolBarIndicators.qml"
    PROPERTIES QT_RESOURCE_ALIAS "QGroundControl/PlanView/PlanToolBarIndicators.qml"
)
qt_add_resources(${CMAKE_PROJECT_NAME} custom_qml_overrides
    PREFIX "/Custom/qml"
    FILES ${CUSTOM_QML_OVERRIDES}
)

# ----------------------------------------------------------------------------
# Custom plugin sources
# ----------------------------------------------------------------------------

set(CUSTOM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CustomPlugin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CustomPlugin.h
    PARENT_SCOPE
)

set(CUSTOM_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    PARENT_SCOPE
)

set(CUSTOM_DEFINITIONS
    QGC_CUSTOM_BUILD
    CUSTOMHEADER="CustomPlugin.h"
    CUSTOMCLASS=CustomPlugin
    PARENT_SCOPE
)

set(CUSTOM_QT_COMPONENTS
    Core
    PARENT_SCOPE
)
