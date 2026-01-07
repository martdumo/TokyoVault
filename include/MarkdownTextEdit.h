#pragma once

#include <QTextEdit>
#include <QMouseEvent>
#include <QRegularExpression>

class MarkdownTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit MarkdownTextEdit(QWidget *parent = nullptr);

signals:
    // Emitida al hacer Ctrl+Click en un wiki-link [[...]] o al hacer clic normal en un link en modo preview.
    void wikiLinkActivated(const QString &linkName);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
};