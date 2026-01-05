#include "MarkdownHighlighter.h"

#include <QTextDocument>
#include <QBrush>
#include <QFont>
#include <QColor>

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // Formato para Títulos (Cyan y Negrita)
    HighlightingRule rule;
    QTextCharFormat headerFormat;
    headerFormat.setForeground(QColor("#7dcfff")); // Cyan
    headerFormat.setFontWeight(QFont::Bold);
    // Expresión regular para títulos (Markdown: #, ##, ###...)
    rule.pattern = QRegularExpression(R"(^#{1,6}\s.*)");
    rule.format = headerFormat;
    m_highlightingRules.append(rule);

    // Formato para Links y WikiLinks (Púrpura)
    QTextCharFormat linkFormat;
    linkFormat.setForeground(QColor("#bb9af7")); // Púrpura
    // Expresión regular para links [texto](url) y wiki-links [[texto]]
    rule.pattern = QRegularExpression(R"(\[\[[^\]]+\]\]|\[[^\]]+\]\([^\)]+\))");
    rule.format = linkFormat;
    m_highlightingRules.append(rule);

    // Formato para Código (Naranja)
    QTextCharFormat codeFormat;
    codeFormat.setForeground(QColor("#ff9e64")); // Naranja
    // Expresión regular para bloques de código en línea (`)
    rule.pattern = QRegularExpression(R"(`[^`]+`)");
    rule.format = codeFormat;
    m_highlightingRules.append(rule);

    // Formato para Bloques de Código Multilínea
    m_multiLineCodeFormat.setForeground(QColor("#ff9e64")); // Naranja
    // Opcional: fondo ligeramente más oscuro para bloques de código
    m_multiLineCodeFormat.setBackground(QColor("#2c3044")); // Un poco más oscuro que #24283b
    m_multiLineCodeFormat.setFontFixedPitch(true); // Fuente monoespaciada para código

    // Delimitadores para bloques de código multilínea (```)
    m_codeBlockStartExpression = QRegularExpression(R"(^```)");
    m_codeBlockEndExpression = QRegularExpression(R"(^```$)");
}

void MarkdownHighlighter::highlightBlock(const QString &text)
{
    // Primero, aplica las reglas de una sola línea
    for (const HighlightingRule &rule : qAsConst(m_highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // Manejo de bloques de código multilínea
    setCurrentBlockState(0); // Reinicia el estado del bloque

    int startIndex = 0;
    if (previousBlockState() != 1) { // Si el bloque anterior no terminó un bloque de código
        startIndex = m_codeBlockStartExpression.match(text).capturedStart();
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch match = m_codeBlockEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int length = 0;

        if (endIndex == -1) { // Si no se encuentra el fin del bloque de código en esta línea
            setCurrentBlockState(1); // Marca el bloque como "dentro de un bloque de código"
            length = text.length() - startIndex;
        } else { // Se encontró el fin del bloque de código
            length = endIndex - startIndex + match.capturedLength();
        }
        setFormat(startIndex, length, m_multiLineCodeFormat);
        startIndex = m_codeBlockStartExpression.match(text, startIndex + length).capturedStart();
    }
}
