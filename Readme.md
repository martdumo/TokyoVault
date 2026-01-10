# 🧳 Markdown Editor "Maletín" (Native C++ Edition)

![C++](https://img.shields.io/badge/C++-20-blue?style=for-the-badge&logo=c%2B%2B) ![Qt](https://img.shields.io/badge/Qt-6.10-41CD52?style=for-the-badge&logo=qt) ![CMake](https://img.shields.io/badge/CMake-Build-064F8C?style=for-the-badge&logo=cmake) ![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Arch%20Linux-lightgrey?style=for-the-badge)

**"Maletín"** es un gestor de notas y bóveda personal (Personal Vault) diseñado bajo la premisa de **Alto Rendimiento** y **Fidelidad Visual**. A diferencia de las soluciones basadas en Electron (como Obsidian), este editor ha sido desarrollado nativamente en C++ para ofrecer una latencia cercana a cero y un consumo de recursos mínimo.

## 🌟 Características Principales

- **Modo Dual Inteligente:** Conmutación instantánea entre edición de texto plano y previsualización enriquecida (Preview) con paridad visual 1:1.
- **Gestión de Bóvedas (Vaults):** Navegación recursiva de directorios locales integrada con un explorador de archivos reactivo.
- **Wiki-Links Core:** Soporte nativo para enlaces `[[Nota]]` con capacidad de creación automática de archivos inexistentes.
- **Estética "Tokyo Night":** Interfaz optimizada para el modo oscuro, reduciendo la fatiga visual durante sesiones prolongadas de escritura.
- **Buscador Híbrido:** Motor de búsqueda asíncrona que indexa tanto títulos como contenido mediante hilos de trabajo secundarios (`QThread`).
- **Seguridad de Datos:** Sistema de "File Guard" que previene el desbordamiento de memoria al bloquear archivos mayores a 5MB.


## 🏗️ Arquitectura del Sistema

El proyecto sigue una arquitectura **Modular y Orientada a Objetos**, priorizando el desacoplamiento de la lógica de negocio frente a la interfaz de usuario.

### 🧩 Desglose de Componentes

1.  **MainWindow (Orquestador):** Actúa como el controlador central del sistema. Gestiona el ciclo de vida de los widgets, la persistencia de configuraciones vía `QSettings` y la coordinación de los flujos de archivos.
2.  **EditorStyler (Motor de Estilo):** Clase desacoplada encargada de la "fábrica de temas" y el procesado de Markdown. Implementa un sistema de inyección dinámica de **QSS (Qt Style Sheets)** que asegura la coherencia visual en todos los niveles de la jerarquía de widgets.
3.  **MarkdownTextEdit (Motor de Eventos):** Una subclase avanzada de `QTextEdit` que gestiona eventos de bajo nivel (`wheelEvent`, `mousePressEvent`). Implementa un **Cargador de Recursos** personalizado (`loadResource`) para la gestión eficiente de imágenes locales y remotas.
4.  **SearchWorker (Procesamiento Asíncrono):** Para evitar bloqueos en el hilo principal de la interfaz (GUI Thread), la búsqueda global se delega a un trabajador que opera en un `QThread` independiente, utilizando señales y slots para la comunicación inter-hilos.

### 🛡️ Seguridad y Robustez de Datos

-   **Sistema de Buffer Dual:** Implementé una variable de control `m_rawMarkdownBuffer` que actúa como la "Única Fuente de la Verdad". Esto garantiza que el proceso de renderizado HTML nunca corrompa el archivo original de texto plano.
-   **Manejo de Memoria:** Uso extensivo de punteros inteligentes (`std::unique_ptr`) y la jerarquía de propiedad de Qt para prevenir fugas de memoria (memory leaks).
-   **File Guard:** Filtro preventivo de E/S que valida el tamaño de los metadatos del archivo antes de la carga, protegiendo la estabilidad del proceso ante archivos malformados o excesivamente pesados.


    
## ⚡ Rendimiento y Eficiencia (Benchmark)

Una de las principales motivaciones de este proyecto fue superar las limitaciones de consumo de los editores basados en la pila Web (Electron).

| Métrica | Obsidian (Electron) | **Maletín (C++)** | Mejora |
| :--- | :--- | :--- | :--- |
| **Uso de RAM (Idle)** | ~400 MB - 1 GB | **40 MB - 60 MB** | ~10x menos |
| **Peso del Binario** | ~300 MB | **87 MB** (Portable) | ~3.5x menos |
| **Latencia de Interfaz** | Moderada (JS) | **Ultra-baja (Nativa)** | Instantánea |

## 🛠️ Instalación y Compilación

El proyecto utiliza **CMake** como sistema de construcción, lo que garantiza una portabilidad fluida entre Windows y Linux.

### 🪟 Windows 11 (MSVC 2022)
1. Instalar dependencias mediante **vcpkg**: `vcpkg integrate install`.
2. Abrir la carpeta raíz en **Visual Studio 2022**.
3. Configurar el perfil de CMake como `x64-Release`.
4. Compilar (`Ctrl + Shift + B`).
5. Generar paquete portable usando `windeployqt`:
   ```powershell
   .\windeployqt.exe --no-translations --compiler-runtime MarkdownEditor.exe
```
  

🐧 Arch Linux

    Instalar dependencias base:
    ```code Bash

    
sudo pacman -S base-devel cmake qt6-base qt6-5compat

  ```

Clonar y compilar:
```code Bash

    
mkdir build && cd build
cmake ..
make -j$(nproc)
```
  ## 🤖 Metodología de Desarrollo: "Vibecoding" Estructurado

Este proyecto ha sido desarrollado utilizando técnicas avanzadas de **IA-Assisted Development**. Para mantener la coherencia técnica y evitar la alucinación de los modelos de lenguaje en una base de código C++ extensa, se implementó un sistema de gestión de contexto basado en:

-   **AI Blueprints:** Documentación técnica viva que describe el ADN del proyecto, permitiendo a diferentes IAs retomar el desarrollo con una comprensión profunda de la arquitectura.
-   **Atomic Iterations:** Flujo de trabajo basado en micro-objetivos con validación de compilación continua, minimizando la deuda técnica y asegurando la estabilidad del Linker.
-   **Multi-model Orchestration:** Uso estratégico de diferentes LLMs (Gemini, ChatGPT, Grok, Qwen) para realizar auditorías cruzadas de código y optimizar la lógica de renderizado.

## 🚀 Roadmap de Futuras Implementaciones

- [ ] **Soporte CommonMark Completo:** Integración de la librería `md4c` para soporte total de tablas anidadas y listas complejas.
- [ ] **Cloud Sync:** Sincronización automática de bóvedas mediante protocolos cifrados.
- [ ] **Plugin System:** Arquitectura de micro-kernel para permitir extensiones desarrolladas por la comunidad.

## ✍️ Autor

Desarrollado con pasión por **Quicksilver** (martdumo). 

Si eres un reclutador o un desarrollador interesado en sistemas nativos de alto rendimiento, no dudes en contactarme o revisar mi historial de commits para ver la evolución técnica de este editor.

---
*Este proyecto es una prueba de que la simbiosis entre un desarrollador creativo y la Inteligencia Artificial puede producir software nativo de alta calidad en tiempo récord.*