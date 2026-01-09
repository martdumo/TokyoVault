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

    // Nueva fuente para el código
    m_codeFormat.setFontFamilies({"JetBrains Mono", "Consolas", "monospace"});
    m_multiLineCodeFormat.setFontFamilies({"JetBrains Mono", "Consolas", "monospace"});

    // Delimitadores para bloques de código multilínea (```)
    m_codeBlockStartExpression = QRegularExpression(R"(^```)");
    m_codeBlockEndExpression = QRegularExpression(R"(^```$)");
}

void MarkdownHighlighter::setTheme(const Theme &theme)
{
    m_headerFormat.setForeground(theme.heading);
    // Usamos el color de acento para los links para mayor visibilidad
    m_linkFormat.setForeground(theme.accent);
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
    // Reset the format for this block
    QTextCharFormat defaultFormat;
    setFormat(0, text.length(), defaultFormat);

    // Process the text in order of complexity to avoid conflicts
    // 1. Headers first (they start at the beginning of the line)
    QRegularExpression headerRegex(R"(^(#{1,6})\s+(.+))");
    QRegularExpressionMatch headerMatch = headerRegex.match(text);
    if (headerMatch.hasMatch()) {
        int headerLevel = headerMatch.captured(1).length(); // Number of #
        int headerStart = headerMatch.capturedStart(1);
        int headerEnd = headerMatch.capturedEnd(2);
        
        // Format the header text
        setFormat(headerStart, headerEnd - headerStart, m_headerFormat);
    }

    // 2. Blockquotes (start with > at the beginning of the line)
    QRegularExpression quoteRegex(R"(^>\s+(.+))");
    QRegularExpressionMatch quoteMatch = quoteRegex.match(text);
    if (quoteMatch.hasMatch()) {
        setFormat(0, text.length(), m_quoteFormat);
    }

    // 3. Process inline elements with proper precedence
    // First, find all code spans to avoid processing markdown inside them
    QVector<QPair<int, int>> codeSpans; // Store start and end positions of code spans
    QRegularExpression codeRegex(R"(`[^`]*`)");
    QRegularExpressionMatchIterator codeIter = codeRegex.globalMatch(text);
    while (codeIter.hasNext()) {
        QRegularExpressionMatch match = codeIter.next();
        codeSpans.append(qMakePair(match.capturedStart(), match.capturedEnd()));
        setFormat(match.capturedStart(), match.capturedLength(), m_codeFormat);
    }

    // 4. Process bold and italic, avoiding code spans
    // Bold: **text**
    QRegularExpression boldRegex(R"(\*\*[^\*]+\*\*)");
    QRegularExpressionMatchIterator boldIter = boldRegex.globalMatch(text);
    while (boldIter.hasNext()) {
        QRegularExpressionMatch match = boldIter.next();
        // Check if this overlaps with any code span
        bool overlaps = false;
        for (const auto& span : codeSpans) {
            if (!(match.capturedEnd() <= span.first || match.capturedStart() >= span.second)) {
                overlaps = true;
                break;
            }
        }
        if (!overlaps) {
            setFormat(match.capturedStart(), match.capturedLength(), m_boldFormat);
        }
    }

    // 5. Italic: *text* (but not if it's part of bold **text** or inside words)
    QRegularExpression italicRegex(R"((?<!\*)\*([^\*]+)\*(?!\*))");
    QRegularExpressionMatchIterator italicIter = italicRegex.globalMatch(text);
    while (italicIter.hasNext()) {
        QRegularExpressionMatch match = italicIter.next();
        // Check if this overlaps with any code span
        bool overlaps = false;
        for (const auto& span : codeSpans) {
            if (!(match.capturedEnd() <= span.first || match.capturedStart() >= span.second)) {
                overlaps = true;
                break;
            }
        }
        if (!overlaps) {
            setFormat(match.capturedStart(), match.capturedLength(), m_italicFormat);
        }
    }

    // 6. Links: [text](url) and [[wikilinks]]
    QRegularExpression linkRegex(R"(\[([^\]]+)\]\([^)]+\)|\[\[([^\]]+)\]\])");
    QRegularExpressionMatchIterator linkIter = linkRegex.globalMatch(text);
    while (linkIter.hasNext()) {
        QRegularExpressionMatch match = linkIter.next();
        // Check if this overlaps with any code span
        bool overlaps = false;
        for (const auto& span : codeSpans) {
            if (!(match.capturedEnd() <= span.first || match.capturedStart() >= span.second)) {
                overlaps = true;
                break;
            }
        }
        if (!overlaps) {
            setFormat(match.capturedStart(), match.capturedLength(), m_linkFormat);
        }
    }

    // 7. Process multiline code blocks
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