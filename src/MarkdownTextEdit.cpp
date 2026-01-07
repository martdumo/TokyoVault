#include "MarkdownTextEdit.h"

#include <QTextCursor>
#include <QTextDocument>
#include <QDebug>
#include <QWheelEvent>
#include <QShortcut>
#include <QCursor>

MarkdownTextEdit::MarkdownTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
    // Atajos para hacer zoom con el teclado
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus), this, SLOT(zoomIn()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus), this, SLOT(zoomOut()));

    // Habilitar el seguimiento del mouse para el cursor de "manito"
    setMouseTracking(true);
}

void MarkdownTextEdit::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    } else {
        QTextEdit::wheelEvent(event);
    }
}

void MarkdownTextEdit::mousePressEvent(QMouseEvent *event)
{
    // En modo preview (solo lectura), manejamos los clics en anclas (links)
    if (isReadOnly()) {
        QString url = anchorAt(event->pos());
        if (!url.isEmpty()) {
            emit wikiLinkActivated(url);
            return; // El evento ha sido manejado, no hacer más nada.
        }
    }

    // Llama a la implementación base para mantener el comportamiento normal (ej. mover el cursor)
    QTextEdit::mousePressEvent(event);

    // Solo procesamos si es un clic izquierdo y la tecla Ctrl está presionada (modo edición)
    if (event->button() == Qt::LeftButton && event->modifiers().testFlag(Qt::ControlModifier)) {
        // Obtenemos la posición del cursor de texto en la que se hizo clic
        QTextCursor cursor = cursorForPosition(event->pos());
        int clickedPos = cursor.position();

        // Creamos una expresión regular para buscar el patrón [[...]]
        QRegularExpression wikiLinkRegex(R"(\[\[([^\]]+)\]\])");

        // Buscamos en todo el documento
        QRegularExpressionMatchIterator it = wikiLinkRegex.globalMatch(document()->toPlainText());

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            if (clickedPos >= match.capturedStart() && clickedPos <= match.capturedEnd()) {
                // El clic ocurrió dentro de un wiki-link
                QString linkName = match.captured(1); // Captura el texto dentro de los corchetes
                emit wikiLinkActivated(linkName);
                return; // Ya hemos encontrado el link, salimos
            }
        }
    }
}

void MarkdownTextEdit::mouseMoveEvent(QMouseEvent *event)
{
    // Solo cambiamos el cursor si el editor está en modo preview (isReadOnly)
    if (isReadOnly()) {
        if (!anchorAt(event->pos()).isEmpty()) {
            viewport()->setCursor(Qt::PointingHandCursor);
        } else {
            viewport()->setCursor(Qt::IBeamCursor);
        }
    } else {
        // En modo edición, siempre es el cursor de texto
        viewport()->setCursor(Qt::IBeamCursor);
    }
    QTextEdit::mouseMoveEvent(event);
}