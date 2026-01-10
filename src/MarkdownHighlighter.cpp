#include "MarkdownHighlighter.h"

#include <QTextDocument>
#include <QBrush>
#include <QFont>
#include <QColor>

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // Hardcoded Tokyo Night colors
    m_headerFormat.setFontWeight(QFont::Bold);
    m_headerFormat.setForeground(QColor("#bb9af7")); // Tokyo Night heading color

    m_linkFormat.setForeground(QColor("#7aa2f7")); // Tokyo Night accent color

    m_codeFormat.setFontFamilies({"JetBrains Mono", "Consolas", "monospace"});
    m_codeFormat.setForeground(QColor("#9ece6a")); // Tokyo Night code color

    m_boldFormat.setFontWeight(QFont::Bold);
    m_boldFormat.setForeground(QColor("#ff9e64")); // Tokyo Night bold color

    m_italicFormat.setFontItalic(true);
    m_italicFormat.setForeground(QColor("#c0caf5")); // Tokyo Night text color

    m_quoteFormat.setBackground(QColor("#2a2e42")); // Tokyo Night quote background
    m_quoteFormat.setForeground(QColor("#565f89")); // Tokyo Night muted foreground

    m_multiLineCodeFormat.setFontFamilies({"JetBrains Mono", "Consolas", "monospace"});
    m_multiLineCodeFormat.setForeground(QColor("#9ece6a")); // Tokyo Night code color
    m_multiLineCodeFormat.setBackground(QColor("#24283b").darker(110)); // Tokyo Night editor background darkened

    m_codeBlockStartExpression = QRegularExpression(R"(^```)");
    m_codeBlockEndExpression = QRegularExpression(R"(^```$)");
}

void MarkdownHighlighter::highlightBlock(const QString &text)
{
    QTextCharFormat defaultFormat;
    setFormat(0, text.length(), defaultFormat);

    QRegularExpression headerRegex(R"(^(#{1,6})\s+(.+))");
    QRegularExpressionMatch headerMatch = headerRegex.match(text);
    if (headerMatch.hasMatch()) {
        int headerLevel = headerMatch.captured(1).length();
        int headerStart = headerMatch.capturedStart(1);
        int headerEnd = headerMatch.capturedEnd(2);
        setFormat(headerStart, headerEnd - headerStart, m_headerFormat);
    }

    QRegularExpression quoteRegex(R"(^>\s+(.+))");
    QRegularExpressionMatch quoteMatch = quoteRegex.match(text);
    if (quoteMatch.hasMatch()) {
        setFormat(0, text.length(), m_quoteFormat);
    }

    QVector<QPair<int, int>> codeSpans;
    QRegularExpression codeRegex(R"(`[^`]*`)");
    QRegularExpressionMatchIterator codeIter = codeRegex.globalMatch(text);
    while (codeIter.hasNext()) {
        QRegularExpressionMatch match = codeIter.next();
        codeSpans.append(qMakePair(match.capturedStart(), match.capturedEnd()));
        setFormat(match.capturedStart(), match.capturedLength(), m_codeFormat);
    }

    QRegularExpression boldRegex(R"(\*\*[^\*]+\*\*)");
    QRegularExpressionMatchIterator boldIter = boldRegex.globalMatch(text);
    while (boldIter.hasNext()) {
        QRegularExpressionMatch match = boldIter.next();
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

    QRegularExpression italicRegex(R"((?<!\*)\*([^\*]+)\*(?!\*))");
    QRegularExpressionMatchIterator italicIter = italicRegex.globalMatch(text);
    while (italicIter.hasNext()) {
        QRegularExpressionMatch match = italicIter.next();
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

    QRegularExpression linkRegex(R"(\[([^\]]+)\]\([^)]+\)|\[\[([^\]]+)\]\])");
    QRegularExpressionMatchIterator linkIter = linkRegex.globalMatch(text);
    while (linkIter.hasNext()) {
        QRegularExpressionMatch match = linkIter.next();
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