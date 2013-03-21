project(PLDLiveInstaller)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)
find_package(KDE4 REQUIRED)
include (KDE4Defaults)

set(CMAKE_CXX_FLAGS "-O3 -g -fexceptions")

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES})

set(PLDLiveInstaller_SRCS PLDLiveInstaller.cpp DiskListWidget.cpp main.cpp)
qt4_automoc(${PLDLiveInstaller_SRCS})
add_executable(pldliveinstaller ${PLDLiveInstaller_SRCS})
target_link_libraries(pldliveinstaller ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} 
${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KUTILS_LIBS} kworkspace crypt)

install(TARGETS pldliveinstaller ${INSTALL_TARGETS_DEFAULT_ARGS})
