#include "MarkdownHighlighter.h"
#include "MainWindow.h" // Para la estructura Theme

#include <QTextDocument>
#include <QBrush>
#include <QFont>
#include <QColor>

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // Los formatos se inicializan con colores base, pero setTheme los sobrescribirá
    m_headerFormat.setFontWeight(QFont::Bold);
    m_boldFormat.setFontWeight(QFont::Bold);
    m_italicFormat.setFontItalic(true);
    m_multiLineCodeFormat.setFontFixedPitch(true);
    
    // Delimitadores para bloques de código multilínea (```)
    m_codeBlockStartExpression = QRegularExpression(R"(^```)");
    m_codeBlockEndExpression = QRegularExpression(R"(^```$)");
}

void MarkdownHighlighter::setTheme(const Theme &theme)
{
    m_headerFormat.setForeground(theme.heading);
    m_linkFormat.setForeground(theme.link);
    m_codeFormat.setForeground(theme.code);
    m_boldFormat.setForeground(theme.bold);
    m_italicFormat.setForeground(theme.italic);
    
    m_quoteFormat.setBackground(theme.quoteBg);
    m_quoteFormat.setForeground(theme.mutedFg);
    
    m_multiLineCodeFormat.setForeground(theme.code);
    m_multiLineCodeFormat.setBackground(theme.editorBg.darker(110));

    // Refresca todo el documento
    rehighlight();
}


void MarkdownHighlighter::highlightBlock(const QString &text)
{
    // --- Reglas de una línea ---
    // Títulos (#, ##, ...)
    QRegularExpressionMatchIterator matchIterator = QRegularExpression(R"(^#{1,6}\s.*)").globalMatch(text);
    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_headerFormat);
    }
    
    // Links ([texto](url) y [[wikilink]])
    matchIterator = QRegularExpression(R"(\[\[[^\]]+\]\]|\[[^\]]+\]\([^\)]+\))").globalMatch(text);
    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_linkFormat);
    }

    // Código en línea (`)
    matchIterator = QRegularExpression(R"(`[^`]+`)").globalMatch(text);
    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_codeFormat);
    }

    // Negrita (**)
    matchIterator = QRegularExpression(R"(\*\*[^\*]+\*\*)").globalMatch(text);
    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_boldFormat);
    }

    // Itálica (*)
    matchIterator = QRegularExpression(R"(\*[^\*]+\*)").globalMatch(text);
     while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_italicFormat);
    }

    // Citas (>)
    matchIterator = QRegularExpression(R"(^>\s.*)").globalMatch(text);
    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_quoteFormat);
    }


    // --- Bloques de Código Multilínea ---
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1) {
        startIndex = m_codeBlockStartExpression.match(text).capturedStart();
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch match = m_codeBlockEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int length = 0;

        if (endIndex == -1) {
            setCurrentBlockState(1);
            length = text.length() - startIndex;
        } else {
            length = endIndex - startIndex + match.capturedLength();
        }
        setFormat(startIndex, length, m_multiLineCodeFormat);
        startIndex = m_codeBlockStartExpression.match(text, startIndex + length).capturedStart();
    }
}