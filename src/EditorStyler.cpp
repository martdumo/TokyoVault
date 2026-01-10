#include "EditorStyler.h"
#include <QRegularExpression>
#include <QFont>

EditorStyler::EditorStyler()
{
    // Constructor is now empty since we don't have themes
}


QString EditorStyler::renderMarkdown(const QString& markdownContent, const QFont& editorFont) const
{
    QString content = markdownContent;

    // 1. Escape basic HTML characters
    content.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;");

    // 2. Process headers (H1 to H6)
    // Process headers from most specific (H6) to least specific (H1) to avoid conflicts
    content.replace(QRegularExpression(R"(^######\s+(.*)$)", QRegularExpression::MultilineOption), R"(<h6>\1</h6>)");
    content.replace(QRegularExpression(R"(^#####\s+(.*)$)", QRegularExpression::MultilineOption), R"(<h5>\1</h5>)");
    content.replace(QRegularExpression(R"(^####\s+(.*)$)", QRegularExpression::MultilineOption), R"(<h4>\1</h4>)");
    content.replace(QRegularExpression(R"(^###\s+(.*)$)", QRegularExpression::MultilineOption), R"(<h3>\1</h3>)");
    content.replace(QRegularExpression(R"(^##\s+(.*)$)", QRegularExpression::MultilineOption), R"(<h2>\1</h2>)");
    content.replace(QRegularExpression(R"(^#\s+(.*)$)", QRegularExpression::MultilineOption), R"(<h1>\1</h1>)");

    // 3. Refined line spacing
    content.replace(QRegularExpression(R"(^\s*$)"), "&nbsp;");

    // 4. Process block elements
    content.replace(QRegularExpression(R"(^>\s+(.*)$)", QRegularExpression::MultilineOption), R"(<blockquote>\1</blockquote>)");
    content.replace(QRegularExpression(R"(^(\s*-\s*){3,}$|^\s*(\*\s*){3,}$)", QRegularExpression::MultilineOption), "<hr>");

    // 5. Process tables
    // Split content into lines to process tables
    QStringList lines = content.split("\n");
    QStringList processedLines;
    bool inTable = false;

    for (const QString& line : lines) {
        if (line.trimmed().startsWith("|") && line.contains("|")) {
            // This is a table row
            if (!inTable) {
                // Start of table
                inTable = true;
                processedLines << QString("<table style=\"border-collapse: collapse; width: 100%; margin: 10px 0; border: 1px solid #565f89;\">"); // Tokyo Night mutedFg
            }

            // Process table row - skip separator rows (contain ---)
            if (line.contains("---|") || line.contains("|---") || line.contains(":---")) {
                continue; // Skip the separator row
            } else {
                // Regular table row
                QStringList cells = line.split("|");
                QString tableRow = "<tr>";
                for (int i = 1; i < cells.size() - 1; ++i) { // Skip first and last empty elements
                    QString cellContent = cells[i].trimmed();
                    tableRow += QString("<td style=\"border: 1px solid #565f89; padding: 8px; text-align: left; background-color: #16161e; color: #c0caf5;\">%1</td>").arg(cellContent); // Tokyo Night colors
                }
                tableRow += "</tr>";
                processedLines << tableRow;
            }
        } else {
            // Not a table line
            if (inTable) {
                // End of table
                inTable = false;
                processedLines << "</table>";
            }
            processedLines << line;
        }
    }

    if (inTable) {
        // Close table if it wasn't closed
        processedLines << "</table>";
    }

    content = processedLines.join("\n");

    // 6. Process inline elements
    // Ensure all anchor tags are properly closed and prevent potential issues with special characters
    // Use non-greedy quantifiers to avoid capturing too much text
    content.replace(QRegularExpression(R"(\[\[([^\]]+?)\]\])"), R"(<a href="\1.md" style="color: #7aa2f7; text-decoration: underline;">\1</a>)"); // Tokyo Night accent
    content.replace(QRegularExpression(R"(\[([^\]]+?)\]\(([^\)]+?)\))"), R"(<a href="\2" style="color: #7aa2f7; text-decoration: underline;">\1</a>)"); // Tokyo Night accent

    // Add support for images: ![alt text](image_url)
    content.replace(QRegularExpression(R"(!\[([^\]]+?)\]\(([^\)]+?)\))"), R"(<img src="\2" alt="\1" style="max-width:100%; height:auto; display:block; margin:10px 0; border-radius:4px;" />)");

    content.replace(QRegularExpression(R"(\*\*(.*?)\*\*)"), R"(<b style="color: #c0caf5;">\1</b>)"); // Tokyo Night text color
    content.replace(QRegularExpression(R"(\*(.*?)\*)"), R"(<i style="color: #c0caf5;">\1</i>)"); // Tokyo Night text color
    content.replace(QRegularExpression(R"(`(.*?)`)"), R"(<code style="background-color: #2a2e42; color: #9ece6a; padding: 2px 4px; border-radius: 3px; font-family: 'JetBrains Mono', Consolas, monospace;">\1</code>)"); // Tokyo Night code colors

    // 6. Build CSS string in chunks to avoid C2026
    QString css;
    css += "body { white-space: pre-wrap; font-family: '" + editorFont.family() + "'; background-color: #1a1b26; color: #c0caf5; margin: 10px; }"; // Tokyo Night windowBg and textFg
    css += "h1, h2, h3, h4, h5, h6 { color: #bb9af7; margin-top: 12px; margin-bottom: 8px; }"; // Tokyo Night heading
    css += "a { color: #7aa2f7; text-decoration: underline; }"; // Tokyo Night accent
    css += "blockquote { border-left: 4px solid #7aa2f7; padding-left: 10px; margin-left: 0; font-style: italic; color: #c0caf5; background-color: #16161e; padding: 5px 10px; border-radius: 0 4px 4px 0; }"; // Tokyo Night accent and textFg
    css += "code { background-color: #2a2e42; color: #9ece6a; padding: 2px 4px; border-radius: 3px; font-family: 'JetBrains Mono', Consolas, monospace; }"; // Tokyo Night quoteBg and code
    css += "pre { background-color: #2a2e42; color: #9ece6a; padding: 10px; border-radius: 5px; overflow-x: auto; white-space: pre; font-family: 'JetBrains Mono', Consolas, monospace; }"; // Tokyo Night quoteBg and code
    css += "hr { border: 0; border-top: 1px solid #bb9af7; margin: 1em 0; }"; // Tokyo Night heading
    css += "img { max-width: 100%; border-radius: 4px; }";
    css += "table { border-collapse: collapse; width: 100%; margin: 10px 0; border: 1px solid #565f89; }"; // Tokyo Night mutedFg
    css += "th, td { border: 1px solid #565f89; padding: 8px; text-align: left; }"; // Tokyo Night mutedFg
    css += "th { background-color: #24283b; color: #c0caf5; }"; // Tokyo Night editorBg and textFg
    css += "tr:nth-child(even) { background-color: #16161e; }"; // Tokyo Night editorBg darker

    // 7. Wrap in HTML with dynamic, centralized CSS
    QString htmlTemplate = QString("<!DOCTYPE html><html><head><style>%1</style></head><body>%2</body></html>");
    QString finalHtml = htmlTemplate.arg(css).arg(content);

    return finalHtml;
}