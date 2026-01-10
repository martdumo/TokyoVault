#include "EditorStyler.h"
#include "MarkdownHighlighter.h"
#include <QApplication>
#include <QRegularExpression>
#include <QFont>

EditorStyler::EditorStyler()
{
    setupThemes();
}

const QMap<QString, Theme>& EditorStyler::getThemes() const
{
    return m_themes;
}

void EditorStyler::setupThemes()
{
    m_themes.clear();
    m_themes["Tokyo Night"] = {"Tokyo Night", "#1a1b26", "#24283b", "#a9b1d6", "#565f89", "#7aa2f7", "#7dcfff", "#bb9af7", "#ff9e64", "#c0caf5", "#c0caf5", "#2a2e42"};
    m_themes["Nord"] = {"Nord", "#2E3440", "#3B4252", "#D8DEE9", "#4C566A", "#88C0D0", "#81A1C1", "#B48EAD", "#EBCB8B", "#ECEFF4", "#ECEFF4", "#434C5E"};
    m_themes["Catppuccin"] = {"Catppuccin", "#1e1e2e", "#181825", "#cdd6f4", "#585b70", "#89b4fa", "#94e2d5", "#cba6f7", "#fab387", "#bac2de", "#bac2de", "#313244"};
    m_themes["Gruvbox"] = {"Gruvbox", "#282828", "#3c3836", "#ebdbb2", "#7c6f64", "#83a598", "#8ec07c", "#d3869b", "#fe8019", "#d5c4a1", "#d5c4a1", "#504945"};
    m_themes["One Dark"] = {"One Dark", "#282c34", "#21252b", "#abb2bf", "#5c6370", "#61afef", "#98c379", "#c678dd", "#e5c07b", "#e06c75", "#e06c75", "#3f4451"};
    m_themes["Light Mode"] = {"Light Mode", "#ffffff", "#f8f9fa", "#212529", "#adb5bd", "#0d6efd", "#198754", "#6f42c1", "#fd7e14", "#dc3545", "#dc3545", "#e9ecef"};
    m_themes["Cyberpunk"] = {"Cyberpunk", "#0d0d0d", "#1a1a1a", "#00ffcc", "#4d4d4d", "#ff00ff", "#00ffff", "#ff00ff", "#ffcc00", "#ff00cc", "#ff00cc", "#262626"};
    m_themes["Synthwave"] = {"Synthwave", "#1a1a2a", "#26263a", "#ff00ff", "#5d5d7d", "#00ffff", "#00ffcc", "#ff00ff", "#ffff00", "#ff66cc", "#ff66cc", "#33334a"};
    m_themes["Commodore"] = {"Commodore", "#3434ac", "#4d4dcc", "#ffffff", "#7b7bcb", "#ffff00", "#00ff00", "#ffff00", "#ff0000", "#00ffff", "#00ffff", "#5d5dcd"};
    m_themes["Vaporwave"] = {"Vaporwave", "#82aaff", "#c792ea", "#f78c6c", "#7fdbca", "#ff5370", "#c3e88d", "#ff5370", "#ffcb6b", "#f07178", "#f07178", "#a0a0c0"};
    m_themes["Mac Tonight"] = {"Mac Tonight", "#000080", "#0000a0", "#ffffff", "#0000ff", "#ffff00", "#ff8c00", "#ffff00", "#ff0000", "#00ff00", "#00ff00", "#0000c0"};
}

void EditorStyler::applyTheme(const QString& themeName, QApplication* app, MarkdownHighlighter* highlighter)
{
    if (!m_themes.contains(themeName) || !app || !highlighter) return;
    
    Theme theme = m_themes.value(themeName);
    QString styleSheet = QString(
        "QMainWindow,QDialog{background-color:%1;color:%3;}"
        "QMenuBar{background-color:%1;color:%3;}"
        "QMenuBar::item:selected{background-color:%5;color:%2;}"
        "QMenu{background-color:%2;color:%3;border:1px solid %4;}"
        "QToolBar{background-color:%1;border:none;}"
        "QToolButton{color:%3;background-color:transparent;border:1px solid %4;padding:4px;border-radius:4px;}"
        "QStatusBar{color:%3;background-color:%1;}"
        "QTreeView{background-color:%1;color:%3;border:none;}"
        "QLineEdit{background-color:%2;color:%3;border:1px solid %4;border-radius:4px;padding:4px;}"
        "QTextEdit{background-color:%2;color:%3;border:none;margin-left:25px;padding-top:10px;}"
        "QTextEdit a{color:%5;text-decoration:underline;}"
        "QScrollBar:vertical{background:%1;width:10px;margin:0;}"
        "QScrollBar::handle:vertical{background:%4;min-height:20px;border-radius:5px;}"
    ).arg(theme.windowBg.name(), theme.editorBg.name(), theme.textFg.name(), theme.mutedFg.name(), theme.accent.name());
    
    app->setStyleSheet(styleSheet);
    highlighter->setTheme(theme);
}

QString EditorStyler::renderMarkdown(const QString& markdownContent, const QFont& editorFont, const QString& currentThemeName) const
{
    QString content = markdownContent;
    Theme currentTheme = m_themes.value(currentThemeName);

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
                processedLines << QString("<table style=\"border-collapse: collapse; width: 100%; margin: 10px 0; border: 1px solid %1;\">").arg(currentTheme.mutedFg.name());
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
                    tableRow += QString("<td style=\"border: 1px solid %1; padding: 8px; text-align: left;\">%2</td>").arg(currentTheme.mutedFg.name()).arg(cellContent);
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
    content.replace(QRegularExpression(R"(\[\[([^\]]+?)\]\])"), R"(<a href="\1.md">\1</a>)");
    content.replace(QRegularExpression(R"(\[([^\]]+?)\]\(([^\)]+?)\))"), R"(<a href="\2">\1</a>)");

    // Add support for images: ![alt text](image_url)
    content.replace(QRegularExpression(R"(!\[([^\]]+?)\]\(([^\)]+?)\))"), R"(<img src="\2" alt="\1" style="max-width:100%; height:auto; display:block; margin:10px 0; border-radius:4px;" />)");

    content.replace(QRegularExpression(R"(\*\*(.*?)\*\*)"), R"(<b>\1</b>)");
    content.replace(QRegularExpression(R"(\*(.*?)\*)"), R"(<i>\1</i>)");
    content.replace(QRegularExpression(R"(`(.*?)`)"), R"(<code>\1</code>)");

    // 6. Build CSS string in chunks to avoid C2026
    QString css;
    css += "body { white-space: pre-wrap; font-family: '" + editorFont.family() + "'; background-color: " + currentTheme.windowBg.name() + "; color: " + currentTheme.textFg.name() + "; }";
    css += "h1, h2, h3, h4, h5, h6 { color: " + currentTheme.heading.name() + "; }";
    css += "a { color: " + currentTheme.accent.name() + "; text-decoration: underline; }";
    css += "blockquote { border-left: 4px solid " + currentTheme.accent.name() + "; padding-left: 10px; margin-left: 0; font-style: italic; color: " + currentTheme.textFg.name() + "; }";
    css += "code { background-color: " + currentTheme.quoteBg.name() + "; color: " + currentTheme.code.name() + "; padding: 2px 4px; border-radius: 3px; }";
    css += "pre { background-color: " + currentTheme.quoteBg.name() + "; color: " + currentTheme.code.name() + "; padding: 10px; border-radius: 5px; overflow-x: auto; white-space: pre; }";
    css += "hr { border: 0; border-top: 1px solid " + currentTheme.heading.name() + "; margin: 1em 0; }";
    css += "img { max-width: 100%; }";

    // 7. Wrap in HTML with dynamic, centralized CSS
    QString htmlTemplate = QString("<!DOCTYPE html><html><head><style>%1</style></head><body>%2</body></html>");
    QString finalHtml = htmlTemplate.arg(css).arg(content);

    return finalHtml;
}