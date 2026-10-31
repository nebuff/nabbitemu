# Portions of this come from KiCad's CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

# Single source of truth for the release version: the VERSION file next to this
# script. nabbitemuBuilds reads the same file, so a release bump is one edit.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" NABBITEMU_VERSION)
string(STRIP "${NABBITEMU_VERSION}" NABBITEMU_VERSION)

# set the project name
project(nabbitemu VERSION ${NABBITEMU_VERSION} LANGUAGES C CXX)

# A plain `cmake -B build` used to produce an unoptimised binary; default to
# Release so local builds match the packaged ones.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

include(GNUInstallDirs)

# Sources are a mix of C and C++ that is built as C++; pin a standard that every
# current distribution compiler understands.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS ON)

# find wx + include it
# Turn on wxWidgets compatibility mode for some classes
add_definitions( -DWX_COMPATIBILITY )

# Don't link statically
set( wxWidgets_CONFIG_OPTIONS ${wxWidgets_CONFIG_OPTIONS} --static=no )

# now hunt for wxWidgets
# (adv is used for "advanced" widgets, such as the wizard)
find_package(wxWidgets REQUIRED adv net core base)
include(${wxWidgets_USE_FILE})

# mixing of C and C++ necessitates builds in c++
file(GLOB_RECURSE CFILES "${CMAKE_SOURCE_DIR}/*.c")
SET_SOURCE_FILES_PROPERTIES(${CFILES} PROPERTIES LANGUAGE CXX )

# add the executable
add_executable(nabbitemu
	#[[ Core ]]
	core/core.c
	core/alu.c
	core/control.c
	core/device.c
	core/indexcb.c
	
	#[[ Calc Interfacing ]]
	interface/calc.c
	interface/state.c
	
	#[[ Utilities/Libraries ]]
	utilities/exportvar.c
	utilities/fileutilities.c
	utilities/gif.c
	utilities/gifhandle.c
	utilities/label.c
	utilities/savestate.c
	utilities/sendfile.c
	utilities/var.c
	#[[ Hardware Emulation ]]
	hardware/81hw.c
	hardware/83hw.c
	hardware/83phw.c
	hardware/83psehw.c
	hardware/86hw.c
	hardware/keys.c
	hardware/lcd.c
	hardware/link.c
	#[[ wxWidgets GUI ]]
	gui/gui.cpp #[[ This requires gui/gui.h ]]
	gui/guiapp.cpp #[[ This requires gui/guiapp.h ]]
	gui/guiskinwindow.cpp #[[ This requires gui/guiskinwindow.h ]]
	gui/guilcd.cpp #[[ This requires gui/guilcd.h ]]
	gui/guivartree.cpp #[[ This requires gui/guivartree.h ]]
	gui/droptarget.cpp #[[ This requires gui/droptarget.h ]]
	gui/wizard/wizardstart.cpp #[[ This requires gui/wizard/wizardstart.h ]]
	gui/wizard/wizardcalctype.cpp #[[ This requires gui/wizard/wizardcalctype.h ]]
	gui/wizard/wizardos.cpp #[[ This requires gui/wizard/wizardos.h ]]
	gui/wizard/romwizard.cpp #[[ This requires gui/wizard/romwizard.h ]]
	debugger/guidebug.cpp #[[ This requires debugger/guidebug.h ]]
	#[[ Debugger and debugger GUI ]]
	debugger/disassemblyview.cpp #[[ This requires debugger/disassemblyview.h ]]
	debugger/disassemble.c
	debugger/panes/regpane.cpp #[[ This requires debugger/panes/regpane.h ]]
	debugger/panes/flagspane.cpp #[[ This requires debugger/panes/flagspane.h ]]
	debugger/panes/cpupane.cpp #[[ This requires debugger/panes/cpupane.h ]]
)

target_include_directories(nabbitemu PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           "${PROJECT_SOURCE_DIR}"
                           "${PROJECT_SOURCE_DIR}/core"
                           "${PROJECT_SOURCE_DIR}/debugger"
                           "${PROJECT_SOURCE_DIR}/gui"
                           "${PROJECT_SOURCE_DIR}/hardware"
                           "${PROJECT_SOURCE_DIR}/interface"
                           "${PROJECT_SOURCE_DIR}/utilities"
                           )

target_link_libraries(nabbitemu ${wxWidgets_LIBRARIES})

# Link-time optimisation trims the emulation hot loop; skipped silently on
# toolchains that do not support it.
option(NABBITEMU_LTO "Enable link-time optimisation for release builds" ON)
if(NABBITEMU_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT NABBITEMU_IPO_OK OUTPUT NABBITEMU_IPO_OUTPUT)
    if(NABBITEMU_IPO_OK)
        set_property(TARGET nabbitemu PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

add_definitions(-DHIGH_SHADE_GIF -DVERBOSE -D_LINUX -DWXVER)

# Enable Unicode
if( CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
	if( GCC_VERSION VERSION_EQUAL 4.8.0 OR GCC_VERSION VERSION_GREATER 4.8.0 )
		add_definitions(-D_UNICODE)
	endif()
endif()

install(TARGETS nabbitemu RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Desktop integration: menu entry + icon, so the packaged build has a launcher.
install(FILES res/nabbitemu.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES gui/wabbiticon.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/16x16/apps
        RENAME nabbitemu.png)

install(FILES README.md LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR})

# CPack for packaging
if( UNIX AND NOT APPLE )

    # Create a *.deb file:
    set( CPACK_GENERATOR "DEB" )
    set( CPACK_PACKAGE_NAME "nabbitemu" )
    set( CPACK_PACKAGE_VENDOR "Nebuff" )
    set( CPACK_PACKAGE_CONTACT "Nebuff <repo@nebuff.xyz>" )
    set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "TI-8x graphing calculator emulator" )
    set( CPACK_DEBIAN_PACKAGE_MAINTAINER "Nebuff <repo@nebuff.xyz>" )
    set( CPACK_DEBIAN_PACKAGE_SECTION "misc" )

    include( CPack )

endif()
