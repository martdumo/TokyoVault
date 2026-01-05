# Versión mínima de CMake.
cmake_minimum_required(VERSION 3.20)

# Definición del proyecto.
project(MarkdownEditor LANGUAGES CXX)

# Configuración del estándar C++20.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Habilita las herramientas automáticas de Qt 6 (Meta-Object Compiler, Resource Compiler, UI Compiler).
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Configuración Específica del Entorno ---
# Establece la ruta al toolchain de vcpkg. CMake lo usará para encontrar paquetes.
# La integración global de vcpkg debería hacer esto innecesario, pero es una buena práctica ser explícito.
set(CMAKE_TOOLCHAIN_FILE "F:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Ayuda a CMake a encontrar la instalación de Qt 6.
# Modifica esta ruta si tu versión o ubicación de Qt es diferente.
set(CMAKE_PREFIX_PATH "F:/Qt/6.10.1/msvc2022_64" CACHE PATH "Qt installation directory")
# --- Fin Configuración Específica ---

# Busca el paquete de Qt6 con los componentes necesarios (Widgets, Gui, Core).
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core)

# Incluye los directorios de cabeceras.
include_directories(include)

# Añade el ejecutable y sus archivos fuente.
# CMAKE_AUTORCC se encargará de procesar 'resources.qrc'.
add_executable(MarkdownEditor
    src/main.cpp
    src/MainWindow.cpp
    src/MarkdownHighlighter.cpp
    src/MarkdownTextEdit.cpp
    src/FindReplaceDialog.cpp
    include/MainWindow.h
    include/MarkdownHighlighter.h
    include/MarkdownTextEdit.h
    include/FindReplaceDialog.h
    resources.qrc
)

# Vincula las bibliotecas de Qt6 a nuestro ejecutable.
target_link_libraries(MarkdownEditor PRIVATE
    Qt6::Widgets
    Qt6::Gui
    Qt6::Core
)
