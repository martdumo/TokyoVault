#pragma once

#include <QString>
#include <QColor>
#include <QMap>
#include <QFont>

class QApplication;
class MarkdownHighlighter;

// Moved from MainWindow.h
struct Theme {
    QString name;
    QColor windowBg, editorBg, textFg, mutedFg, accent;
    QColor heading, link, code, bold, italic, quoteBg;
};

class EditorStyler
{
public:
    EditorStyler();

    void setupThemes();
    void applyTheme(const QString& themeName, QApplication* app, MarkdownHighlighter* highlighter);
    QString renderMarkdown(const QString& markdownContent, const QFont& editorFont, const QString& currentThemeName) const;

    const QMap<QString, Theme>& getThemes() const;

private:
    QMap<QString, Theme> m_themes;
};
