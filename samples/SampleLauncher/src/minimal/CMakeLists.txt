# Copyright (c) 2017 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

if(OS_LINUX)
  message(FATAL_ERROR "Linux builds are not supported at this time.")
elseif(OS_MACOSX)
  message(FATAL_ERROR "MacOS builds are not supported at this time.")
endif()

#
# Source files.
#

# Main executable sources.
set(EXAMPLE_SRCS
  app_browser_minimal.cc
  client_minimal.cc
  client_minimal.h
  main_minimal.cc
  )

if(OS_LINUX OR OS_WINDOWS)
  # On Windows and Linux the same executable is used for all processes.
  set(EXAMPLE_SRCS
    ${EXAMPLE_SRCS}
    app_other_minimal.cc
    app_renderer_minimal.cc
    )
elseif(OS_MACOSX)
  # On macOS a separate helper executable is used for subprocesses.
  set(EXAMPLE_HELPER_SRCS
    app_other_minimal.cc
    app_renderer_minimal.cc
    process_helper_mac_minimal.cc
    )
endif()

if(OS_MACOSX OR OS_WINDOWS)
  # On macOS and Windows include the shared resources.
  set(EXAMPLE_RESOURCES_SRCS
    ${SHARED_RESOURCES_SRCS}
    )
endif()


#
# Shared configuration.
#

# Target executable names.
set(EXAMPLE_TARGET "minimal")
if(OS_MACOSX)
  set(EXAMPLE_HELPER_TARGET "minimal_Helper")
  set(EXAMPLE_HELPER_OUTPUT_NAME "minimal Helper")
endif()


#
# Linux configuration.
#

if(OS_LINUX)
  # Executable target.
  add_executable(${EXAMPLE_TARGET} ${EXAMPLE_SRCS})
  SET_EXAMPLE_EXECUTABLE_TARGET_PROPERTIES(${EXAMPLE_TARGET})
  add_dependencies(${EXAMPLE_TARGET} shared)
  target_link_libraries(${EXAMPLE_TARGET} shared)
endif()


#
# Mac OS X configuration.
#

if(OS_MACOSX)
  # Create source groups for Xcode.
  SET_EXAMPLE_SOURCE_GROUPS("${EXAMPLE_SRCS}")
  SET_EXAMPLE_SOURCE_GROUPS("${EXAMPLE_HELPER_SRCS}")

  # Output paths for the app bundles.
  set(EXAMPLE_APP "${EXAMPLE_TARGET_OUT_DIR}/${EXAMPLE_TARGET}.app")
  set(EXAMPLE_HELPER_APP "${EXAMPLE_TARGET_OUT_DIR}/${EXAMPLE_HELPER_OUTPUT_NAME}.app")

  # Variable referenced from Info.plist files.
  set(PRODUCT_NAME "${EXAMPLE_TARGET}")

  # Helper executable target.
  add_executable(${EXAMPLE_HELPER_TARGET} MACOSX_BUNDLE ${EXAMPLE_HELPER_SRCS})
  SET_EXAMPLE_EXECUTABLE_TARGET_PROPERTIES(${EXAMPLE_HELPER_TARGET})
  add_dependencies(${EXAMPLE_HELPER_TARGET} shared_helper)
  target_link_libraries(${EXAMPLE_HELPER_TARGET} shared_helper)
  set_target_properties(${EXAMPLE_HELPER_TARGET} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/${SHARED_HELPER_INFO_PLIST}"
    OUTPUT_NAME ${EXAMPLE_HELPER_OUTPUT_NAME}
    )

  # Main executable target.
  add_executable(${EXAMPLE_TARGET} MACOSX_BUNDLE ${EXAMPLE_SRCS} ${EXAMPLE_RESOURCES_SRCS})
  SET_EXAMPLE_EXECUTABLE_TARGET_PROPERTIES(${EXAMPLE_TARGET})
  add_dependencies(${EXAMPLE_TARGET} shared "${EXAMPLE_HELPER_TARGET}")
  target_link_libraries(${EXAMPLE_TARGET} shared)
  set_target_properties(${EXAMPLE_TARGET} PROPERTIES
    RESOURCE "${EXAMPLE_RESOURCES_SRCS}"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/${SHARED_INFO_PLIST}"
    )

  # Copy files into the main app bundle.
  add_custom_command(
    TARGET ${EXAMPLE_TARGET}
    POST_BUILD
    # Copy the helper app bundle into the Frameworks directory.
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${EXAMPLE_HELPER_APP}"
            "${EXAMPLE_APP}/Contents/Frameworks/${EXAMPLE_HELPER_OUTPUT_NAME}.app"
    # Copy the CEF framework into the Frameworks directory.
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CEF_BINARY_DIR}/Chromium Embedded Framework.framework"
            "${EXAMPLE_APP}/Contents/Frameworks/Chromium Embedded Framework.framework"
    VERBATIM
    )

  if(NOT ${CMAKE_GENERATOR} STREQUAL "Xcode")
    # Manually process and copy over resource files.
    # The Xcode generator handles this via the set_target_properties RESOURCE directive.
    # Remove these prefixes from input file paths.
    set(PREFIXES
      "../shared/resources/mac/"
      )
    COPY_MACOSX_RESOURCES("${EXAMPLE_RESOURCES_SRCS}" "${PREFIXES}" "${EXAMPLE_TARGET}" "${CMAKE_CURRENT_SOURCE_DIR}" "${EXAMPLE_APP}")
  endif()
endif()


#
# Windows configuration.
#

if(OS_WINDOWS)
    # Add resources to the sources variable for convenience.
  set(EXAMPLE_SRCS
    ${EXAMPLE_SRCS}
    ${EXAMPLE_RESOURCES_SRCS}
    )

  # Create source groups for Visual Studio.
  SET_EXAMPLE_SOURCE_GROUPS("${EXAMPLE_SRCS}")

  # Executable target.
  add_executable(${EXAMPLE_TARGET} WIN32 ${EXAMPLE_SRCS})
  SET_EXAMPLE_EXECUTABLE_TARGET_PROPERTIES(${EXAMPLE_TARGET})
  add_dependencies(${EXAMPLE_TARGET} shared)
  target_link_libraries(${EXAMPLE_TARGET} shared)
endif()
