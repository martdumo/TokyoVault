#include "MainWindow.h"
#include <QApplication>
#include <QFontDatabase>
#include <QFile>
#include <QTextStream>
#include <QDebug>

int main(int argc, char *argv[])
{
    // Punto de entrada principal de la aplicación Qt.
    QApplication app(argc, argv);

    // --- Carga de la fuente personalizada ---
    // Registra la fuente desde el archivo de recursos (previamente definido en resources.qrc).
    int fontId = QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono-Regular.ttf");
    if (fontId == -1) {
        qWarning() << "Could not load font.";
    } else {
        // Obtiene el nombre de la familia de la fuente cargada.
        QString family = QFontDatabase::applicationFontFamilies(fontId).at(0);
        // Establece la fuente por defecto para toda la aplicación.
        QFont defaultFont(family);
        app.setFont(defaultFont);
    }

    // --- Carga de la hoja de estilos QSS ---
    // Abre el archivo de estilos desde los recursos.
    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&styleFile);
        // Aplica la hoja de estilos a toda la aplicación.
        app.setStyleSheet(stream.readAll());
        styleFile.close();
    } else {
        qWarning() << "Could not open stylesheet.";
    }

    // Crea la instancia de la ventana principal.
    MainWindow w;
    w.show();

    // Inicia el bucle de eventos de la aplicación.
    return app.exec();
}
