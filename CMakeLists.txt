# Versión mínima de CMake.
cmake_minimum_required(VERSION 3.20)

# Definición del proyecto.
project(MarkdownEditor LANGUAGES CXX)

# Configuración del estándar C++20.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Habilita las herramientas automáticas de Qt 6.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Configuración Específica del Entorno ---
if(WIN32)
    set(CMAKE_TOOLCHAIN_FILE "F:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
    set(CMAKE_PREFIX_PATH "F:/Qt/6.10.1/msvc2022_64" CACHE PATH "Qt installation directory")
else()
    # En otros sistemas, CMake buscará Qt en las rutas estándar.
endif()

# Busca el paquete de Qt6.
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core Network)

# Incluye los directorios de cabeceras.
include_directories(include)

# Lista de Archivos Fuente
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MarkdownHighlighter.cpp
    src/MarkdownTextEdit.cpp
    src/FindReplaceDialog.cpp
    src/SearchWorker.cpp
    src/EditorStyler.cpp
    include/MainWindow.h
    include/MarkdownHighlighter.h
    include/MarkdownTextEdit.h
    include/FindReplaceDialog.h
    include/SearchWorker.h
    include/EditorStyler.h
    resources.qrc
)

# Añade el ejecutable.
add_executable(MarkdownEditor ${SOURCES})

# Vincula las bibliotecas de Qt6.
target_link_libraries(MarkdownEditor PRIVATE
    Qt6::Widgets
    Qt6::Gui
    Qt6::Core
    Qt6::Network
)
