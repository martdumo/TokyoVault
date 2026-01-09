#include "MainWindow.h"
#include "MarkdownTextEdit.h"
#include "MarkdownHighlighter.h"
#include "FindReplaceDialog.h"
#include "SearchWorker.h"

#include <QApplication>
#include <QActionGroup>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextStream>
#include <QSplitter>
#include <QTreeView>
#include <QLineEdit>
#include <QStatusBar>
#include <QLabel>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QFontDialog>
#include <QInputDialog>
#include <QSettings>
#include <QStandardPaths>
#include <QShortcut>
#include <QCloseEvent>
#include <QDir>
#include <QToolButton>
#include <QRegularExpression>
#include <QUrl>
#include <QDesktopServices>
#include <QThread>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_isEditMode(true),
      m_historyIndex(-1),
      m_searchThread(nullptr),
      m_searchWorker(nullptr),
      m_searchTimer(nullptr),
      m_findReplaceDialog(nullptr)
{
    setWindowTitle("Markdown Editor [*]");
    setMinimumSize(900, 700);
    resize(1400, 900);

    setupThemes();
    createMenus();
    createToolBar();
    createStatusBar();
    setupUI();
    setupSearchThread();

    // Add global shortcut for toggling edit/preview mode
    new QShortcut(QKeySequence("Ctrl+E"), this, SLOT(toggleEditMode()));

    loadSettings();
}

MainWindow::~MainWindow()
{
    if (m_searchThread) {
        m_searchThread->quit();
        m_searchThread->wait();
    }
}

void MainWindow::setupSearchThread()
{
    m_searchThread = new QThread(this);
    m_searchWorker = new SearchWorker();
    m_searchWorker->moveToThread(m_searchThread);

    connect(m_searchWorker, &SearchWorker::searchFinished, this, &MainWindow::handleSearchResults);
    connect(m_searchThread, &QThread::finished, m_searchWorker, &QObject::deleteLater);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(800);
    connect(m_searchTimer, &QTimer::timeout, this, &MainWindow::startContentSearch);

    m_searchThread->start();
}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu("&Archivo");

    QAction *newAction = new QAction("Nuevo Archivo", this);
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::newFile);
    fileMenu->addAction(newAction);

    fileMenu->addSeparator();

    QAction *openVaultAction = new QAction("Abrir &Vault (Carpeta)...", this);
    connect(openVaultAction, &QAction::triggered, this, [this](){ openVault(); });
    fileMenu->addAction(openVaultAction);
    QAction *openFileAction = new QAction("Abrir &Archivo .md...", this);
    connect(openFileAction, &QAction::triggered, this, &MainWindow::openFile);
    fileMenu->addAction(openFileAction);
    fileMenu->addSeparator();
    QAction *saveAction = new QAction("&Guardar", this);
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    fileMenu->addAction(saveAction);
    fileMenu->addSeparator();
    QAction *exitAction = new QAction("S&alir", this);
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);
    fileMenu->addAction(exitAction);

    QMenu *editMenu = menuBar()->addMenu("&Editar");
    QAction *findAction = new QAction("&Buscar y Reemplazar...", this);
    findAction->setShortcut(QKeySequence::Find);
    connect(findAction, &QAction::triggered, this, &MainWindow::showFindReplaceDialog);
    editMenu->addAction(findAction);

    QMenu *customizeMenu = menuBar()->addMenu("&Personalizar");
    QAction *fontAction = new QAction("Cambiar &Fuente...", this);
    connect(fontAction, &QAction::triggered, this, &MainWindow::showFontDialog);
    customizeMenu->addAction(fontAction);
    customizeMenu->addSeparator();
    QMenu *themesMenu = customizeMenu->addMenu("&Temas");
    QActionGroup *themeActionGroup = new QActionGroup(this);

    for (const QString &themeName : m_themes.keys()) {
        QAction *action = new QAction(themeName, this);
        action->setCheckable(true);
        action->setData(themeName);
        themesMenu->addAction(action);
        themeActionGroup->addAction(action);
    }
    connect(themeActionGroup, &QActionGroup::triggered, this, [this](QAction *action){
        applyTheme(action->data().toString());
    });

    QMenu *helpMenu = menuBar()->addMenu("&Ayuda");
    QAction *guideAction = new QAction("&Guía de Markdown", this);
    connect(guideAction, &QAction::triggered, this, &MainWindow::showHelpDialog);
    helpMenu->addAction(guideAction);
}

void MainWindow::createToolBar()
{
    QToolBar *toolbar = addToolBar("Main Toolbar");
    toolbar->setObjectName("MainToolbar"); // Set object name to fix saveState warning
    toolbar->setMovable(false);

    QToolButton *newButton = new QToolButton(this);
    newButton->setText("Nuevo");
    connect(newButton, &QToolButton::clicked, this, &MainWindow::newFile);
    toolbar->addWidget(newButton);
    toolbar->addSeparator();

    m_historyBackButton = new QToolButton(this);
    m_historyBackButton->setText("<");
    m_historyBackButton->setEnabled(false);
    connect(m_historyBackButton, &QToolButton::clicked, this, &MainWindow::historyBack);
    toolbar->addWidget(m_historyBackButton);
    m_historyForwardButton = new QToolButton(this);
    m_historyForwardButton->setText(">");
    m_historyForwardButton->setEnabled(false);
    connect(m_historyForwardButton, &QToolButton::clicked, this, &MainWindow::historyForward);
    toolbar->addWidget(m_historyForwardButton);
    toolbar->addSeparator();

    QToolButton *openVaultButton = new QToolButton(this);
    openVaultButton->setText("Abrir Vault");
    connect(openVaultButton, &QToolButton::clicked, this, [this](){ openVault(); });
    toolbar->addWidget(openVaultButton);
    m_toggleButton = new QToolButton(this);
    m_toggleButton->setText("Preview Mode");
    connect(m_toggleButton, &QToolButton::clicked, this, &MainWindow::toggleEditMode);
    toolbar->addWidget(m_toggleButton);
}

void MainWindow::createStatusBar()
{
    m_statusBar = statusBar();
    m_filePathLabel = new QLabel("Ningún archivo abierto", this);
    m_wordCountLabel = new QLabel("0 palabras", this);
    m_statusBar->addPermanentWidget(m_filePathLabel, 1);
    m_statusBar->addPermanentWidget(m_wordCountLabel);
}

void MainWindow::setupUI()
{
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    QWidget *leftPanel = new QWidget(m_mainSplitter);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    m_searchBar = new QLineEdit(this);
    m_searchBar->setPlaceholderText("Buscar por título o contenido...");
    connect(m_searchBar, &QLineEdit::textChanged, this, &MainWindow::filterVault);
    leftLayout->addWidget(m_searchBar);

    m_fileSystemModel = new QFileSystemModel(this);
    m_fileSystemModel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
    m_fileSystemModel->setNameFilters(QStringList() << "*.md");
    m_fileSystemModel->setNameFilterDisables(false);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_fileSystemModel);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(0);
    m_proxyModel->setRecursiveFilteringEnabled(true);

    m_treeView = new QTreeView(m_mainSplitter);
    m_treeView->setModel(m_proxyModel);
    for (int i = 1; i < m_fileSystemModel->columnCount(); ++i) m_treeView->hideColumn(i);
    m_treeView->setHeaderHidden(true);
    connect(m_treeView, &QTreeView::clicked, this, &MainWindow::onFileClicked);
    leftLayout->addWidget(m_treeView);

    m_textEdit = new MarkdownTextEdit(m_mainSplitter);
    connect(m_textEdit, &MarkdownTextEdit::textChanged, this, &MainWindow::onDocumentModified);
    connect(m_textEdit, &MarkdownTextEdit::textChanged, this, &MainWindow::updateStatusBar);
    connect(m_textEdit, &MarkdownTextEdit::wikiLinkActivated, this, &MainWindow::handleLinkNavigation);

    m_highlighter = new MarkdownHighlighter(m_textEdit->document());
    m_mainSplitter->addWidget(leftPanel);
    m_mainSplitter->addWidget(m_textEdit);
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 3);
    m_mainSplitter->setSizes({350, 1050});
    setCentralWidget(m_mainSplitter);
}

void MainWindow::filterVault()
{
    m_searchTimer->start();
}

void MainWindow::startContentSearch()
{
    QString text = m_searchBar->text();
    m_lastSearchTerm = text;
    if (text.isEmpty()) {
        m_proxyModel->setFilterRegularExpression("");
        if (!m_currentVaultPath.isEmpty()) {
            m_treeView->setRootIndex(m_proxyModel->mapFromSource(m_fileSystemModel->index(m_currentVaultPath)));
        }
        return;
    }
    if (text.length() >= 2) {
        QMetaObject::invokeMethod(m_searchWorker, "doSearch", Qt::QueuedConnection, Q_ARG(QString, text), Q_ARG(QString, m_currentVaultPath));
    }
}

void MainWindow::handleSearchResults(const QStringList &matchingFiles)
{
    if (m_searchBar->text() != m_lastSearchTerm) return;

    if (m_lastSearchTerm.isEmpty()) {
        m_proxyModel->setFilterRegularExpression("");
    } else if (matchingFiles.isEmpty()) {
        m_proxyModel->setFilterRegularExpression(QString("$."));
    } else {
        QStringList fileNames;
        for (const QString &path : matchingFiles) {
            fileNames << QRegularExpression::escape(QFileInfo(path).fileName());
        }
        m_proxyModel->setFilterRegularExpression(fileNames.join('|'));
    }
    if (!m_currentVaultPath.isEmpty()) {
        m_treeView->setRootIndex(m_proxyModel->mapFromSource(m_fileSystemModel->index(m_currentVaultPath)));
    }
}

void MainWindow::applyTheme(const QString &themeName)
{
    if (!m_themes.contains(themeName)) return;
    m_currentThemeName = themeName;
    Theme theme = m_themes[themeName];
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
    qApp->setStyleSheet(styleSheet);
    m_highlighter->setTheme(theme);
}

void MainWindow::newFile()
{
    if (m_currentVaultPath.isEmpty()) {
        QMessageBox::warning(this, "Vault no seleccionado", "Por favor, abre un Vault antes de crear un archivo.");
        return;
    }
    bool ok = false;
    QString fileName = QInputDialog::getText(this, "Nuevo Archivo", "Nombre:", QLineEdit::Normal, "", &ok);
    if (ok && !fileName.isEmpty()) {
        if (fileName.endsWith(".md")) fileName.chop(3);
        QString newFilePath = m_currentVaultPath + "/" + fileName + ".md";
        QFile file(newFilePath);
        if (file.exists()) {
            QMessageBox::warning(this, "Archivo existente", "Un archivo con ese nombre ya existe.");
            return;
        }
        if (!maybeSave()) {
            return;
        }
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "Error", "No se pudo crear el archivo.");
            return;
        }
        file.close();
        loadFile(newFilePath);
    }
}

void MainWindow::toggleEditMode()
{
    m_isEditMode = !m_isEditMode;
    bool wasModified = m_textEdit->document()->isModified();

    if (m_isEditMode) {
        m_toggleButton->setText("Preview Mode");
        m_textEdit->setReadOnly(false);
        m_highlighter->setDocument(m_textEdit->document());
        m_textEdit->setPlainText(m_rawMarkdownBuffer);
        m_highlighter->rehighlight();
    } else {
        m_toggleButton->setText("Edit Mode");
        m_highlighter->setDocument(nullptr);
        m_rawMarkdownBuffer = m_textEdit->toPlainText();
        QString content = m_rawMarkdownBuffer;

        // --- Manual Markdown to HTML Conversion (v5.5) ---
        // Provides absolute control over visual parity for whitespace and formatting.

        // 1. Escape basic HTML characters to prevent rendering them as tags.
        content.replace("&", "&amp;");
        content.replace("<", "&lt;");
        content.replace(">", "&gt;");

        // 2. Apply Markdown formatting rules via Regex, in a safe order.
        // Using Raw String Literals R"(...)" to avoid escaping hell.
        
        // Process Setext-style headers (H1 and H2 with === and ---)
        // First, find all lines that are followed by a line of === or ---
        QRegularExpression setextH1Regex(R"((.+)\n=+\n)", QRegularExpression::MultilineOption);
        content.replace(setextH1Regex, R"(<h1>\1</h1>\n)");
        
        QRegularExpression setextH2Regex(R"((.+)\n-+\n)", QRegularExpression::MultilineOption);
        content.replace(setextH2Regex, R"(<h2>\1</h2>\n)");
        
        // Headers: # H1, ## H2, ### H3, etc.
        content.replace(QRegularExpression(R"(^#{6}\s+(.+)$)", QRegularExpression::MultilineOption), R"(<h6>\1</h6>)");
        content.replace(QRegularExpression(R"(^#{5}\s+(.+)$)", QRegularExpression::MultilineOption), R"(<h5>\1</h5>)");
        content.replace(QRegularExpression(R"(^#{4}\s+(.+)$)", QRegularExpression::MultilineOption), R"(<h4>\1</h4>)");
        content.replace(QRegularExpression(R"(^#{3}\s+(.+)$)", QRegularExpression::MultilineOption), R"(<h3>\1</h3>)");
        content.replace(QRegularExpression(R"(^#{2}\s+(.+)$)", QRegularExpression::MultilineOption), R"(<h2>\1</h2>)");
        content.replace(QRegularExpression(R"(^#{1}\s+(.+)$)", QRegularExpression::MultilineOption), R"(<h1>\1</h1>)");
        
        // Process blockquotes - handle multiple levels of nesting
        QStringList lines = content.split("\n");
        QStringList processedLines;
        int i = 0;
        
        while (i < lines.size()) {
            QString line = lines[i];
            
            // Check for nested blockquotes
            if (line.startsWith(">")) {
                // Count the depth of nesting
                int depth = 0;
                QString remainingLine = line;
                
                // Count how many '>' prefixes there are
                while (remainingLine.startsWith(">")) {
                    depth++;
                    remainingLine = remainingLine.mid(1); // Remove the '>'
                    if (remainingLine.startsWith(" ")) {
                        remainingLine = remainingLine.mid(1); // Remove the space
                    }
                }
                
                // Create opening blockquote tags
                QString blockquoteStart = "";
                for (int j = 0; j < depth; j++) {
                    blockquoteStart += "<blockquote>";
                }
                
                // Add the content
                QString blockquoteEnd = "";
                for (int j = 0; j < depth; j++) {
                    blockquoteEnd += "</blockquote>";
                }
                
                processedLines << blockquoteStart + "<p>" + remainingLine.trimmed() + "</p>" + blockquoteEnd;
            } else {
                processedLines << line;
            }
            
            i++;
        }
        
        content = processedLines.join("\n");
        
        // Process lists - need to handle them carefully to preserve line breaks
        // Split content into lines to process lists properly
        QStringList contentLines = content.split("\n");
        QStringList processedContentLines;
        
        for (int i = 0; i < contentLines.size(); i++) {
            QString line = contentLines[i];
            
            // Check if this is a list item
            if (line.trimmed().startsWith("* ") || line.trimmed().startsWith("- ") || line.trimmed().startsWith("+ ")) {
                // Determine the indentation level
                int indentLevel = 0;
                QString trimmedLine = line.trimmed();
                QString originalLine = line;
                
                // Calculate indentation by counting spaces at the beginning
                int spaceCount = 0;
                while (originalLine.length() > spaceCount && originalLine[spaceCount] == ' ') {
                    spaceCount++;
                }
                
                indentLevel = spaceCount / 2; // 2 spaces per indent level
                
                // Extract the list marker and content
                QRegularExpression listRegex(R"(^(\*|-|\+)\s+(.+)$)");
                QRegularExpressionMatch match = listRegex.match(trimmedLine);
                
                if (match.hasMatch()) {
                    QString marker = match.captured(1);
                    QString content = match.captured(2);
                    
                    // Generate proper indentation for nested lists
                    QString indent = "";
                    for (int j = 0; j < indentLevel; j++) {
                        indent += "  ";
                    }
                    
                    // Determine if it's an unordered list
                    processedContentLines << indent + "<li>" + content + "</li>";
                } else {
                    processedContentLines << line;
                }
            } else if (QRegularExpression(R"(^\d+\.\s+.+$)").match(line.trimmed()).hasMatch()) {
                // Handle ordered lists
                int indentLevel = 0;
                QString trimmedLine = line.trimmed();
                QString originalLine = line;
                
                // Calculate indentation by counting spaces at the beginning
                int spaceCount = 0;
                while (originalLine.length() > spaceCount && originalLine[spaceCount] == ' ') {
                    spaceCount++;
                }
                
                indentLevel = spaceCount / 2; // 2 spaces per indent level
                
                QRegularExpression orderedListRegex(R"(^(\d+)\.\s+(.+)$)");
                QRegularExpressionMatch match = orderedListRegex.match(trimmedLine);
                
                if (match.hasMatch()) {
                    QString number = match.captured(1);
                    QString content = match.captured(2);
                    
                    // Generate proper indentation for nested lists
                    QString indent = "";
                    for (int j = 0; j < indentLevel; j++) {
                        indent += "  ";
                    }
                    
                    processedContentLines << indent + "<li>" + content + "</li>";
                } else {
                    processedContentLines << line;
                }
            } else {
                processedContentLines << line;
            }
        }
        
        content = processedContentLines.join("\n");
        
        // Now wrap consecutive list items in proper ul/ol tags
        QStringList finalLines = content.split("\n");
        QStringList resultLines;
        int idx = 0;
        
        while (idx < finalLines.size()) {
            QString line = finalLines[idx];
            
            if (line.contains("<li>")) {
                // Determine if this is part of an ordered or unordered list
                bool isOrdered = false;
                
                // Check if this looks like an ordered list by looking at the original markdown
                // For now, we'll determine this by checking the context
                if (idx > 0 && finalLines[idx-1].contains("<ol>")) {
                    isOrdered = true;
                } else if (idx > 0 && finalLines[idx-1].contains("<ul>")) {
                    isOrdered = false;
                } else {
                    // Look ahead to see if we're in a sequence of list items
                    int nextIdx = idx;
                    bool hasOrdered = false;
                    bool hasUnordered = false;
                    
                    while (nextIdx < finalLines.size() && finalLines[nextIdx].contains("<li>")) {
                        // Check if this line originally had an ordered list marker
                        // Since we've already converted, we need to infer from context
                        hasOrdered = true; // Default assumption for now
                        nextIdx++;
                    }
                    
                    // For simplicity, let's assume unordered if we can't determine
                    isOrdered = false;
                }
                
                // Collect all consecutive list items
                QString listTag = isOrdered ? "<ol>" : "<ul>";
                QString endTag = isOrdered ? "</ol>" : "</ul>";
                
                resultLines << listTag;
                resultLines << line;
                
                idx++;
                while (idx < finalLines.size() && finalLines[idx].contains("<li>")) {
                    resultLines << finalLines[idx];
                    idx++;
                }
                
                resultLines << endTag;
            } else {
                resultLines << line;
                idx++;
            }
        }
        
        content = resultLines.join("\n");
        
        // Code blocks (indented with 4 spaces or a tab)
        // Process code blocks by identifying lines that start with 4 spaces or a tab
        QStringList codeLines = content.split("\n");
        QStringList processedCodeLines;
        bool inCodeBlock = false;
        
        for (int i = 0; i < codeLines.size(); i++) {
            QString line = codeLines[i];
            
            // Check if the line starts with 4 spaces or a tab
            if (line.startsWith("    ") || line.startsWith("\t")) {
                if (!inCodeBlock) {
                    // Start a new code block
                    processedCodeLines << "<pre><code>";
                    inCodeBlock = true;
                }
                // Add the line content (removing the indentation)
                QString codeLine = line.startsWith("\t") ? line.mid(1) : line.mid(4);
                processedCodeLines << codeLine;
            } else {
                if (inCodeBlock) {
                    // End the current code block
                    processedCodeLines << "</code></pre>";
                    inCodeBlock = false;
                }
                processedCodeLines << line;
            }
        }
        
        // Close any remaining open code block
        if (inCodeBlock) {
            processedCodeLines << "</code></pre>";
        }
        
        content = processedCodeLines.join("\n");
        
        // Horizontal rules
        content.replace(QRegularExpression(R"(^(\* \*){3,}$)", QRegularExpression::MultilineOption), R"(<hr>)");
        content.replace(QRegularExpression(R"(^(\*\*){3,}$)", QRegularExpression::MultilineOption), R"(<hr>)");
        content.replace(QRegularExpression(R"(^(- ){3,}$)", QRegularExpression::MultilineOption), R"(<hr>)");
        content.replace(QRegularExpression(R"(^(-){3,}$)", QRegularExpression::MultilineOption), R"(<hr>)");
        content.replace(QRegularExpression(R"(^(_ ){3,}$)", QRegularExpression::MultilineOption), R"(<hr>)");
        content.replace(QRegularExpression(R"(^(_){3,}$)", QRegularExpression::MultilineOption), R"(<hr>)");
        
        // Images - Fixed regex patterns to avoid illegal escape sequences
        content.replace(QRegularExpression(R"(!\[([^\]]*)\]\(([^)]+)\))"), QString("<img src=\"%2\" alt=\"%1\" />"));
        content.replace(QRegularExpression(R"(!\[([^\]]*)\]\(([^)]+)\s+\"([^\"]+)\"\))"), QString("<img src=\"%2\" alt=\"%1\" title=\"%3\" />"));
        
        // Standard links
        content.replace(QRegularExpression(R"(\[([^\]]+)\]\(([^)]+)\))"), QString("<a href=\"%2\">%1</a>"));
        // Wiki-links
        content.replace(QRegularExpression(R"(\[\[([^\]]+)\]\])"), QString("<a href=\"%1.md\">%1</a>"));
        // Bold
        content.replace(QRegularExpression(R"(\*\*(.*?)\*\*)"), QString("<b>%1</b>"));
        // Italic (with negative lookbehind/ahead to avoid matching inside words)
        content.replace(QRegularExpression(R"((?<!\*)\*(.*?)\*(?!\*))"), QString("<i>%1</i>"));
        // Code (inline)
        content.replace(QRegularExpression(R"(`(.*?)`)"), QString("<code>%1</code>"));

        // Replace remaining newlines with <br> tags for paragraphs
        content.replace("\n", "<br>");

        // 4. Wrap the content in a minimal HTML document with proper styling for word wrap
        QString html = QString("<!DOCTYPE html><html><head><style>"
                               "body { font-family: \"%1\"; margin: 10px; line-height: 1.6; } "
                               "h1, h2, h3, h4, h5, h6 { margin: 10px 0; } "
                               "p { margin: 10px 0; } "
                               "code { background-color: #f0f0f0; padding: 2px 4px; border-radius: 3px; } "
                               "pre { background-color: #f5f5f5; padding: 10px; border-radius: 5px; overflow-x: auto; } "
                               "blockquote { border-left: 4px solid #ccc; margin: 10px 0; padding-left: 16px; color: #666; } "
                               "blockquote blockquote { margin: 5px 0; padding-left: 12px; border-left: 3px solid #ddd; } "
                               "blockquote blockquote blockquote { border-left: 2px solid #eee; } "
                               "ul, ol { margin: 10px 0; padding-left: 20px; } "
                               "li { margin: 5px 0; } "
                               "hr { margin: 20px 0; border: 0; border-top: 1px solid #ccc; } "
                               "img { max-width: 100%; height: auto; } "
                               "</style></head>"
                               "<body>%2</body></html>")
                           .arg(m_textEdit->font().family())
                           .arg(content);

        m_textEdit->setReadOnly(true);
        m_textEdit->setHtml(html);
    }
    m_textEdit->document()->setModified(wasModified);
}

void MainWindow::handleLinkNavigation(const QString &link) {
    QString finalLink = link.trimmed();

    // Handle external web links
    if (finalLink.contains("://")) {
        QDesktopServices::openUrl(QUrl(finalLink));
        return;
    }

    if (m_currentVaultPath.isEmpty()) return;

    // Construct the full path for the local markdown file
    QString newFilePath = finalLink.endsWith(".md")
        ? m_currentVaultPath + "/" + finalLink
        : m_currentVaultPath + "/" + finalLink + ".md";

    QFileInfo fileInfo(newFilePath);

    // If the file doesn't exist, ask the user if they want to create it.
    if (!fileInfo.exists()) {
        auto reply = QMessageBox::question(
            this,
            "Crear archivo",
            "El archivo '" + fileInfo.fileName() + "' no existe.\n¿Quieres crearlo?",
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            // CRITICAL: First, ensure the current work is saved if needed.
            if (!maybeSave()) {
                return; // Stop if user cancels saving the current file.
            }

            // ONLY if maybeSave() was successful, create the new file.
            QFile file(newFilePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.close();
                loadFile(newFilePath); // Load the new, empty file.
            } else {
                QMessageBox::warning(this, "Error", "No se pudo crear el archivo.");
            }
        }
    } else {
        // If the file exists, just try to save the current one and then load it.
        if (maybeSave()) {
            loadFile(newFilePath);
        }
    }
}

void MainWindow::showFontDialog()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_textEdit->font(), this, "Seleccionar Fuente");
    if (ok) {
        m_textEdit->setFont(font);
        // The font will be saved via saveSettings() on close
    }
}

void MainWindow::showHelpDialog()
{
    QString helpText =
        "<h2>Guía Rápida de Markdown</h2>"
        "<p>Usa estos sencillos códigos para dar formato a tu texto.</p>"
        "<hr>"
        "<h3>Negrita</h3>"
        "<p>Envuelve el texto con dos asteriscos: <code>**texto en negrita**</code></p>"
        "<h3>Cursiva</h3>"
        "<p>Envuelve el texto con un asterisco: <code>*texto en cursiva*</code></p>"
        "<h3>Código en línea</h3>"
        "<p>Envuelve el texto con acentos graves: <code>`código en línea`</code></p>"
        "<h3>Enlaces (Links)</h3>"
        "<p>Sintaxis: <code>[Texto a mostrar](https://www.ejemplo.com)</code></p>"
        "<h3>Enlaces Internos (Wiki-links)</h3>"
        "<p>Crea un enlace a otra nota envolviendo su nombre con dos corchetes: <code>[[NombreDeLaNota]]</code></p>";

    QMessageBox::information(this, "Guía de Markdown", helpText);
}

// --- Rest of the functions ---
void MainWindow::openFile() { if(maybeSave()){QString fp=QFileDialog::getOpenFileName(this,"Abrir",m_currentVaultPath,"*.md");if(!fp.isEmpty())loadFile(fp);}}
void MainWindow::openVault(const QString &path){QString d=path.isEmpty()?QFileDialog::getExistingDirectory(this,"Abrir Vault"):path;if(!d.isEmpty()){m_currentVaultPath=d;m_fileSystemModel->setRootPath(d);m_treeView->setRootIndex(m_proxyModel->mapFromSource(m_fileSystemModel->index(d)));}}
void MainWindow::closeVault(){if(maybeSave()){m_currentVaultPath.clear();m_currentFilePath.clear();m_fileSystemModel->setRootPath("");m_textEdit->clear();m_rawMarkdownBuffer.clear();setWindowTitle("ME[*]");m_textEdit->document()->setModified(false);updateStatusBar();}}
void MainWindow::onFileClicked(const QModelIndex &idx){if(!idx.isValid())return;auto sIdx=m_proxyModel->mapToSource(idx);QString fp=m_fileSystemModel->filePath(sIdx);if(m_fileSystemModel->isDir(sIdx)||fp.isEmpty())return;if(maybeSave())loadFile(fp);}
bool MainWindow::loadFile(const QString& fp,bool addHist){QFile f(fp);if(!f.open(QIODevice::ReadOnly|QIODevice::Text))return false;m_currentFilePath=fp;QTextStream in(&f);m_rawMarkdownBuffer=in.readAll();m_textEdit->setPlainText(m_rawMarkdownBuffer);f.close();if(!m_isEditMode){m_isEditMode=true;toggleEditMode();m_isEditMode=true;}
m_textEdit->setPlainText(m_rawMarkdownBuffer);
m_textEdit->document()->setModified(false);setWindowTitle(QFileInfo(fp).fileName()+" - ME[*]");updateStatusBar();if(addHist){if(m_historyIndex>=0&&m_historyIndex<m_fileHistory.size()-1)m_fileHistory=m_fileHistory.mid(0,m_historyIndex+1);if(m_fileHistory.isEmpty()||m_fileHistory.last()!=fp)m_fileHistory.append(fp);m_historyIndex=m_fileHistory.size()-1;}m_historyBackButton->setEnabled(m_historyIndex>0);m_historyForwardButton->setEnabled(m_historyIndex<m_fileHistory.size()-1);return true;}
void MainWindow::saveFile(){if(m_currentFilePath.isEmpty()){QString fp=QFileDialog::getOpenFileName(this,"Guardar",m_currentVaultPath,"*.md");if(fp.isEmpty())return;m_currentFilePath=fp;}QFile f(m_currentFilePath);if(!f.open(QIODevice::WriteOnly|QIODevice::Text))return;QTextStream out(&f);QString c=m_isEditMode?m_textEdit->toPlainText():m_rawMarkdownBuffer;out<<c;f.close();if(m_isEditMode)m_rawMarkdownBuffer=c;m_textEdit->document()->setModified(false);setWindowTitle(QFileInfo(m_currentFilePath).fileName()+" - ME[*]");}
void MainWindow::onDocumentModified(){setWindowModified(m_textEdit->document()->isModified());}
bool MainWindow::maybeSave(){if(!m_textEdit->document()->isModified())return true;auto r=QMessageBox::warning(this,"Guardar","Cambios sin guardar",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);if(r==QMessageBox::Save){saveFile();return!m_textEdit->document()->isModified();}if(r==QMessageBox::Cancel)return false;return true;}
void MainWindow::updateStatusBar(){m_filePathLabel->setText(m_currentFilePath.isEmpty() ? "" : m_currentFilePath);QString t=m_isEditMode?m_textEdit->toPlainText():m_rawMarkdownBuffer;m_wordCountLabel->setText(QString("%1w").arg(t.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts).count()));}
void MainWindow::historyBack(){if(m_historyIndex>0&&maybeSave()){m_historyIndex--;loadFile(m_fileHistory[m_historyIndex],false);}}
void MainWindow::historyForward(){if(m_historyIndex<m_fileHistory.size()-1&&maybeSave()){m_historyIndex++;loadFile(m_fileHistory[m_historyIndex],false);}}
void MainWindow::closeEvent(QCloseEvent *e){if(maybeSave()){saveSettings();e->accept();}else{e->ignore();}}
void MainWindow::saveSettings(){QSettings s("MS","ME");s.beginGroup("MW");s.setValue("g",saveGeometry());s.setValue("s",saveState());s.endGroup();s.beginGroup("ED");s.setValue("f",m_textEdit->font());s.setValue("vp",m_currentVaultPath);s.setValue("th",m_currentThemeName);s.endGroup();}
void MainWindow::loadSettings(){QSettings s("MS","ME");s.beginGroup("MW");restoreGeometry(s.value("g").toByteArray());restoreState(s.value("s").toByteArray());s.endGroup();s.beginGroup("ED");m_textEdit->setFont(s.value("f",QFont("JetBrains Mono")).value<QFont>());QString th=s.value("th","Tokyo Night").toString();applyTheme(th);QString lv=s.value("vp","").toString();if(!lv.isEmpty()&&QDir(lv).exists())openVault(lv);s.endGroup();}

void MainWindow::setupThemes() {
    m_themes.clear();
    // 1. Tokyo Night (Dark)
    m_themes["Tokyo Night"] = {"Tokyo Night", "#1a1b26", "#24283b", "#a9b1d6", "#565f89", "#7aa2f7", "#7dcfff", "#bb9af7", "#ff9e64", "#c0caf5", "#c0caf5", "#2a2e42"};
    // 2. Nord (Dark)
    m_themes["Nord"] = {"Nord", "#2E3440", "#3B4252", "#D8DEE9", "#4C566A", "#88C0D0", "#81A1C1", "#B48EAD", "#EBCB8B", "#ECEFF4", "#ECEFF4", "#434C5E"};
    // 3. Catppuccin (Mocha)
    m_themes["Catppuccin"] = {"Catppuccin", "#1e1e2e", "#181825", "#cdd6f4", "#585b70", "#89b4fa", "#94e2d5", "#cba6f7", "#fab387", "#bac2de", "#bac2de", "#313244"};
    // 4. Gruvbox (Dark)
    m_themes["Gruvbox"] = {"Gruvbox", "#282828", "#3c3836", "#ebdbb2", "#7c6f64", "#83a598", "#8ec07c", "#d3869b", "#fe8019", "#d5c4a1", "#d5c4a1", "#504945"};
    // 5. One Dark
    m_themes["One Dark"] = {"One Dark", "#282c34", "#21252b", "#abb2bf", "#5c6370", "#61afef", "#98c379", "#c678dd", "#e5c07b", "#e06c75", "#e06c75", "#3f4451"};
    // 6. Light Mode
    m_themes["Light Mode"] = {"Light Mode", "#ffffff", "#f8f9fa", "#212529", "#adb5bd", "#0d6efd", "#198754", "#6f42c1", "#fd7e14", "#dc3545", "#dc3545", "#e9ecef"};
}

void MainWindow::insertTableTemplate(){}
void MainWindow::insertLinkTemplate(){}
void MainWindow::insertImageTemplate(){}
void MainWindow::applyBold(){}
void MainWindow::applyItalic(){}
void MainWindow::applyUnderline(){}
void MainWindow::applyTextFormatting(const QString&,const QString&){}
void MainWindow::findNextInEditor(const QString&,QTextDocument::FindFlags){}
void MainWindow::replaceInEditor(const QString&,const QString&,QTextDocument::FindFlags){}

void MainWindow::replaceAllInEditor(const QString &findText, const QString &replaceText, QTextDocument::FindFlags flags)
{
    // This function's body is intentionally left empty for now,
    // but its signature is corrected to prevent compilation errors.
}

void MainWindow::showFindReplaceDialog(){}