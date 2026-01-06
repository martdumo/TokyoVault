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
    // Emitida cuando se hace Ctrl+Click en un wiki-link [[...]]
    void wikiLinkActivated(const QString &linkName);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};
