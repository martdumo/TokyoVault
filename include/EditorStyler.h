#pragma once

#include <QString>
#include <QColor>
#include <QFont>

class QApplication;
class MarkdownHighlighter;

class EditorStyler
{
public:
    EditorStyler();

    QString renderMarkdown(const QString& markdownContent, const QFont& editorFont) const;
};
