#pragma once

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>

// Forward-declare para evitar dependencia circular con MainWindow.h
struct Theme;

class MarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit MarkdownHighlighter(QTextDocument *parent = nullptr);
    void setTheme(const Theme &theme);

protected:
    void highlightBlock(const QString &text) override;

private:
    QTextCharFormat m_headerFormat;
    QTextCharFormat m_linkFormat;
    QTextCharFormat m_codeFormat;
    QTextCharFormat m_boldFormat;
    QTextCharFormat m_italicFormat;
    QTextCharFormat m_quoteFormat;
    QTextCharFormat m_multiLineCodeFormat;

    QRegularExpression m_codeBlockStartExpression;
    QRegularExpression m_codeBlockEndExpression;
};
