#include "MarkdownTextEdit.h"

#include <QTextCursor>
#include <QTextDocument>
#include <QDebug> // Para depuración
#include <QWheelEvent>
#include <QShortcut>

MarkdownTextEdit::MarkdownTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
    // Atajos para hacer zoom con el teclado
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus), this, SLOT(zoomIn()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus), this, SLOT(zoomOut()));
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
    // Llama a la implementación base para mantener el comportamiento normal de QTextEdit
    QTextEdit::mousePressEvent(event);

    // Solo procesamos si es un clic izquierdo y la tecla Ctrl está presionada
    if (event->button() == Qt::LeftButton && event->modifiers().testFlag(Qt::ControlModifier)) {
        // Obtenemos la posición del cursor de texto en la que se hizo clic
        QTextCursor cursor = cursorForPosition(event->pos());
        int clickedPos = cursor.position();

        // Creamos una expresión regular para buscar el patrón [[...]]
        QRegularExpression wikiLinkRegex(R"(\[\[([^\]]+)\]\])");

        // Buscamos hacia atrás desde la posición del clic
        QString text = document()->toPlainText();
        QRegularExpressionMatchIterator it = wikiLinkRegex.globalMatch(text);

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

