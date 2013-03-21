project(PLDLiveInstaller)
cmake_minimum_required(VERSION 2.6)
set (QT_MIN_VERSION "4.7.0")
find_package(Qt4 REQUIRED)
find_package(KDE4 REQUIRED)
find_package(KDE4Workspace REQUIRED)
include (KDE4Defaults)

#set(CMAKE_CXX_FLAGS "-O2 -g -fexceptions")

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} ${KDE4WORKSPACE_INCLUDE_DIR})

set(PLDLiveInstaller_SRCS
    PLDLiveInstaller.cpp
    DiskListWidget.cpp 
    Updater.cpp
    main.cpp
)
qt4_automoc(${PLDLiveInstaller_SRCS})
add_executable(pldliveinstaller ${PLDLiveInstaller_SRCS})
target_link_libraries(pldliveinstaller ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} 
${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KUTILS_LIBS} ${KDE4WORKSPACE_KWORKSPACE_LIBRARY} crypt)

install(TARGETS pldliveinstaller ${INSTALL_TARGETS_DEFAULT_ARGS})
