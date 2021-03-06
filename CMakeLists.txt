cmake_minimum_required(VERSION 2.8.12)
find_package(ECM 5.40.0 REQUIRED NO_MODULE)
set(CMAKE_CXX_STANDARD 14)
set(QT_MIN_VERSION "5.8.0")

if(POLICY CMP0071)
	cmake_policy(SET CMP0071 NEW)
endif()

project(kaidan)

set(APPLICATION_ID "im.kaidan.kaidan")
set(APPLICATION_NAME "kaidan")
set(APPLICATION_DISPLAY_NAME "Kaidan")
if(UBUNTU_TOUCH)
	set(APPLICATION_NAME "${APPLICATION_ID}")
endif()
set(VERSION_STRING "0.4.0~dev")
set(DEVELOPMENT_BUILD TRUE)
set(ANDROID_VERSION_CODE 3)

# CMake options
option(I18N "Enable i18n support" FALSE)
option(STATIC_BUILD "Build Kaidan statically")
option(UBUNTU_TOUCH "Building a Ubuntu Touch click (internal use only!)" FALSE)
option(CLICK_ARCH "Architecture that will be used in the click's manifest")
option(CLICK_DATE "Date used in the version number in the click's manifest")
option(QUICK_COMPILER "Use QtQuick compiler to improve performance" TRUE)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc, uic and rcc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(AUTOMOC_MOC_OPTIONS -Muri=${APPLICATION_ID})

#
# Dependecies
#

# CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

# CMake modules include
include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(ECMInstallIcons)
include(FeatureSummary)
kde_enable_exceptions()

# Find packages
find_package(PkgConfig REQUIRED)
find_package(Qt5 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Core Qml Quick Svg Sql QuickControls2 Xml)
find_package(KF5Kirigami2 REQUIRED)
pkg_search_module(QXmpp REQUIRED qxmpp>=0.9)

# Optional QtQuickCompiler
if(QUICK_COMPILER)
	find_package(Qt5QuickCompiler)
	set_package_properties(Qt5QuickCompiler PROPERTIES
		DESCRIPTION "Compile QML at build time"
		TYPE OPTIONAL
	)
endif()

# Optional QWidget style integration (not on mobile)
if(NOT UBUNTU_TOUCH AND NOT ANDROID AND NOT IOS)
	find_package(Qt5Widgets)
	set_package_properties(Qt5Widgets PROPERTIES
		DESCRIPTION "Integration with QWidget based desktop styles"
		TYPE OPTIONAL
	)
endif()

# Platform-specific
if(Qt5Widgets_FOUND)
	add_definitions(-DHAVE_QWIDGETS -DQAPPLICATION_CLASS=QApplication)
	set(__Qt5Widgets_LIBRARIES Qt5::Widgets)
else()
	add_definitions(-DQAPPLICATION_CLASS=QGuiApplication)
endif()

if(ANDROID)
	find_package(Qt5 REQUIRED COMPONENTS AndroidExtras)
endif()

if(ANDROID OR WIN32)
	pkg_search_module(OPENSSL REQUIRED openssl IMPORTED_TARGET)
	message(STATUS "Using OpenSSL ${OPENSSL_VERSION}")
endif()


#
# Load submodules
#

# Main kaidan sources
include("${CMAKE_SOURCE_DIR}/src/CMakeLists.txt")

# I18n support
if(I18N)
	include("${CMAKE_SOURCE_DIR}/i18n/CMakeLists.txt")
endif()


#
# Sources / Resources
#

# Include bundled icons on Ubuntu Touch, Android, Windows, macOS and iOS
if(UBUNTU_TOUCH OR ANDROID OR WIN32 OR APPLE)
	set(KAIDAN_ICONS_QRC kirigami-icons.qrc)
endif()

# Bundle images on Android, Windows, macOS and iOS
if(ANDROID OR WIN32 OR APPLE)
	set(KAIDAN_IMAGES_QRC "data/images/images.qrc")
endif()

# Set app icon
if(APPLE)
	set(KAIDAN_ICNS "${CMAKE_SOURCE_DIR}/misc/macos/kaidan.icns")
	set_source_files_properties(${KAIDAN_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
elseif(WIN32)
	include(ECMAddAppIcon)
	file(GLOB WIN_ICONS_SRCS "${CMAKE_SOURCE_DIR}/misc/windows/*kaidan.png")
	message(STATUS ${WIN_ICONS_SRCS})
	ecm_add_app_icon(KAIDAN_ICNS ICONS ${WIN_ICONS_SRCS})
	message(STATUS ${KAIDAN_ICNS})
endif()

link_directories(${QXmpp_LIBDIR})

if(QUICK_COMPILER)
	qtquick_compiler_add_resources(KAIDAN_QML_QRC kaidan_qml.qrc)
else()
	qt5_add_resources(KAIDAN_QML_QRC kaidan_qml.qrc)
endif()

add_executable(${PROJECT_NAME}
	MACOSX_BUNDLE
	WIN32
	${KAIDAN_ICNS}
	${KAIDAN_SOURCES}
	${KAIDAN_QML_QRC}
	${KAIDAN_ICONS_QRC}  # only set if enabled
	${KAIDAN_IMAGES_QRC} #
	${I18N_QRC_CPP}      #
)

target_link_libraries(${PROJECT_NAME}
	Qt5::Core
	Qt5::Sql
	Qt5::Qml
	Qt5::Quick
	Qt5::Svg
	Qt5::Network
	Qt5::Xml
	${__Qt5Widgets_LIBRARIES}
	# currently needs to be hardcoded for windows builds
	qxmpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
	${QXmpp_INCLUDE_DIRS}
)

target_compile_options(${PROJECT_NAME} PUBLIC
	${QXmpp_CFLAGS_OTHER}
)

if(ANDROID OR WIN32 OR IOS)
	target_link_libraries(${PROJECT_NAME} Qt5::QuickControls2 KF5::Kirigami2)
endif()

if(ANDROID OR WIN32)
	target_link_libraries(${PROJECT_NAME} PkgConfig::OPENSSL)
endif()

if(ANDROID)
	target_link_libraries(${PROJECT_NAME} Qt5::AndroidExtras)
endif()

if(STATIC_BUILD)
	add_definitions(-DQXMPP_BUILD)

	find_package(Perl REQUIRED)

	set(STATIC_DEPENDENCIES_CMAKE_FILE "${CMAKE_BINARY_DIR}/QtStaticDependencies.cmake")
	if(EXISTS ${STATIC_DEPENDENCIES_CMAKE_FILE})
		file(REMOVE ${STATIC_DEPENDENCIES_CMAKE_FILE})
	endif()

	get_target_property(QT_LIBDIR Qt5::Core LOCATION)
	get_filename_component(QT_LIBDIR ${QT_LIBDIR} DIRECTORY)

	macro(CONVERT_PRL_LIBS_TO_CMAKE _qt_component)
		if(TARGET Qt5::${_qt_component})
			get_target_property(_lib_location Qt5::${_qt_component} LOCATION)
			execute_process(COMMAND ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/utils/convert-prl-libs-to-cmake.pl
				--lib ${_lib_location}
				--libdir ${QT_LIBDIR}
				--out ${STATIC_DEPENDENCIES_CMAKE_FILE}
				--component ${_qt_component}
				--compiler ${CMAKE_CXX_COMPILER_ID}
			)
		endif()
	endmacro()

	link_directories(${_qt5_install_prefix}/../)

	foreach(qt_module Gui Quick QuickControls2 Network Qml Svg Sql QSQLiteDriverPlugin
             QJpegPlugin QGifPlugin QSvgPlugin QSvgIconPlugin QICOPlugin QGenericEnginePlugin
	         QLocalClientConnectionFactory QTcpServerConnectionFactory)
		CONVERT_PRL_LIBS_TO_CMAKE(${qt_module})
	endforeach()

	if(WIN32)
		CONVERT_PRL_LIBS_TO_CMAKE(QWindowsIntegrationPlugin)
	elseif(IOS)
		foreach(qt_module QIOSIntegrationPlugin QMacHeifPlugin QMacJp2Plugin
			QICNSPlugin QTgaPlugin QTiffPlugin QWbmpPlugin QWebpPlugin)
			CONVERT_PRL_LIBS_TO_CMAKE(${qt_module})
		endforeach()
	endif()

	if(NOT EXISTS ${STATIC_DEPENDENCIES_CMAKE_FILE})
		message(FATAL_ERROR "Unable to find ${STATIC_DEPENDENCIES_CMAKE_FILE}")
	endif()
	include(${STATIC_DEPENDENCIES_CMAKE_FILE})

	set(QT_QML_PATH ${_qt5Quick_install_prefix})

	find_library(KIRIGAMI_PLUGIN kirigamiplugin PATHS ${Kirigami2_INSTALL_PREFIX}/${KDE_INSTALL_QMLDIR}/org/kde/kirigami.2)
	find_library(QUICK_PLUGIN qtquick2plugin PATHS ${QT_QML_PATH}/qml/QtQuick.2)
	find_library(LABS_PLATFORM_PLUGIN qtlabsplatformplugin PATHS ${QT_QML_PATH}/qml/Qt/labs/platform)
	find_library(GRAPHEFFECTS_PLUGIN qtgraphicaleffectsplugin PATHS ${QT_QML_PATH}/qml/QtGraphicalEffects)
	find_library(GRAPHEFFECTS_PRIVATE_PLUGIN qtgraphicaleffectsprivate PATHS ${QT_QML_PATH}/qml/QtGraphicalEffects/private)
	find_library(QQC2_PLUGIN qtquickcontrols2plugin PATHS ${QT_QML_PATH}/qml/QtQuick/Controls.2)
	find_library(QQC2_MATERIAL_PLUGIN qtquickcontrols2materialstyleplugin PATHS ${QT_QML_PATH}/qml/QtQuick/Controls.2/Material)
	find_library(QQC2_UNIVERSAL_PLUGIN qtquickcontrols2universalstyleplugin PATHS ${QT_QML_PATH}/qml/QtQuick/Controls.2/Universal)
	find_library(QLAYOUTS_PLUGIN qquicklayoutsplugin PATHS ${QT_QML_PATH}/qml/QtQuick/Layouts)
	find_library(QWINDOW_PLUGIN windowplugin PATHS ${QT_QML_PATH}/qml/QtQuick/Window.2)
	find_library(QSHAPES_PLUGIN qmlshapesplugin PATHS ${QT_QML_PATH}/qml/QtQuick/Shapes)
	find_library(QUICKSHAPES Qt5QuickShapes PATHS ${QT_LIBDIR})
	find_library(QTEMPLATES_PLUGIN qtquicktemplates2plugin PATHS ${QT_QML_PATH}/qml/QtQuick/Templates.2)
	find_library(QMODELS_PLUGIN modelsplugin PATHS ${QT_QML_PATH}/qml/QtQml/Models.2)
	target_link_libraries(${PROJECT_NAME}
		${plugin_libs}
		Qt5::QSQLiteDriverPlugin
		Qt5::QJpegPlugin
		Qt5::QGifPlugin
		Qt5::QSvgPlugin
		Qt5::QSvgIconPlugin
		Qt5::QICOPlugin
		Qt5::QGenericEnginePlugin
		Qt5::QLocalClientConnectionFactory
		Qt5::QTcpServerConnectionFactory
		${KIRIGAMI_PLUGIN}
		${QUICK_PLUGIN}
		${LABS_PLATFORM_PLUGIN}
		${GRAPHEFFECTS_PLUGIN}
		${GRAPHEFFECTS_PRIVATE_PLUGIN}
		${QQC2_PLUGIN}
		${QQC2_MATERIAL_PLUGIN}
		${QQC2_UNIVERSAL_PLUGIN}
		${QLAYOUTS_PLUGIN}
		${QWINDOW_PLUGIN}
		${QSHAPES_PLUGIN}
		${QUICKSHAPES}
		${QTEMPLATES_PLUGIN}
		${QMODELS_PLUGIN}
		${__Qt5Widgets_LIBRARIES}
	)

	if(WIN32)
		target_link_libraries(${PROJECT_NAME}
			Qt5::QWindowsIntegrationPlugin
		)
	elseif(IOS)
		target_link_libraries(${PROJECT_NAME}
			Qt5::QIOSIntegrationPlugin
			Qt5::QMacHeifPlugin
			Qt5::QMacJp2Plugin
			Qt5::QICNSPlugin
			Qt5::QTgaPlugin
			Qt5::QTiffPlugin
			Qt5::QWbmpPlugin
			Qt5::QWebpPlugin
		)
	endif()
endif()

# Set a custom plist file for the app bundle
if(APPLE)
	if(IOS)
		set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/misc/ios/Info.plist)
	else()
		set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/misc/macos/Info.plist)
	endif()
endif()

#
# Global C++ variables
#

# iOS-specific linker flags
if(IOS)
	set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-Wl,-e,_qt_main_wrapper -u _qt_registerPlatformPlugin")
endif()

if(UBUNTU_TOUCH)
	set(KAIDAN_COMPILE_DEFINITIONS UBUNTU_TOUCH=True)
elseif(NOT ANDROID AND NOT IOS AND NOT WIN32)
	set(KAIDAN_COMPILE_DEFINITIONS NOTIFICATIONS_LIBNOTIFY=True)
endif()

if(STATIC_BUILD)
	set(KAIDAN_COMPILE_DEFINITIONS
		STATIC_BUILD=True
		${KAIDAN_COMPILE_DEFINITIONS}
	)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
	DEBUG_SOURCE_PATH="${CMAKE_SOURCE_DIR}"
	VERSION_STRING="${VERSION_STRING}"
	APPLICATION_ID="${APPLICATION_ID}"
	APPLICATION_NAME="${APPLICATION_NAME}"
	APPLICATION_DISPLAY_NAME="${APPLICATION_DISPLAY_NAME}"
	${KAIDAN_COMPILE_DEFINITIONS}
)

#
# Install Kaidan
#

if(ANDROID)
	configure_file(${CMAKE_SOURCE_DIR}/misc/android/AndroidManifest.xml.in
	               ${CMAKE_SOURCE_DIR}/misc/android/AndroidManifest.xml)
endif()

if(UBUNTU_TOUCH AND CLICK_ARCH)
	set(CLICK_VERSION ${VERSION_STRING})
	if(DEVELOPMENT_BUILD)
		set(CLICK_VERSION "${CLICK_VERSION}.${CLICK_DATE}")
	endif()

	# will replace ${CLICK_ARCH} with its value
	configure_file(${CMAKE_SOURCE_DIR}/misc/ubuntu-touch/manifest.json.in
	               ${CMAKE_SOURCE_DIR}/misc/ubuntu-touch/manifest.json)

	# install kaidan binary
	install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/bin/${PROJECT_NAME}
	        DESTINATION ${CMAKE_INSTALL_PREFIX}
	)
	# install kaidan media
	install(DIRECTORY "data/images"
	        DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/${APPLICATION_ID}"
	)
	# install icon
	install(FILES "misc/kaidan.svg" "misc/ubuntu-touch/apparmor.json"
	              "misc/ubuntu-touch/kaidan.desktop" "misc/ubuntu-touch/manifest.json"
	        DESTINATION "${CMAKE_INSTALL_PREFIX}"
	)
elseif(UNIX AND NOT APPLE)
	# install kaidan binary
	install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/bin/${PROJECT_NAME}
	        DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
	)

	# install kaidan media
	install(DIRECTORY "data/images"
	        DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}"
	)

	# install icon (scalable + 128x)
	install(FILES "misc/kaidan.svg"
	        DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor/scalable/apps"
	)
	install(FILES "misc/kaidan-128x128.png"
	        DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor/128x128/apps"
	        RENAME "kaidan.png"
	)
	# install desktop file
	install(FILES "misc/kaidan.desktop"
	        DESTINATION "${KDE_INSTALL_APPDIR}"
	)
	# install metainfo
	install(FILES "misc/metadata.xml"
		DESTINATION "${KDE_INSTALL_METAINFODIR}"
		RENAME "im.kaidan.kaidan.appdata.xml"
	)
endif()


feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
