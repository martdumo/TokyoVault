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

        // --- Manual Markdown to HTML Conversion (v5.2) ---
        // Provides absolute control over visual parity for whitespace and formatting.

        // 1. Escape basic HTML characters to prevent rendering them as tags.
        content.replace("&", "&amp;");
        content.replace("<", "&lt;");
        content.replace(">", "&gt;");

        // 2. Apply simple Markdown formatting rules via Regex, in a safe order.
        // Using Raw String Literals R"(...)" to avoid escaping hell.
        content.replace(QRegularExpression(R"(\[([^\]]+)\]\(([^)]+)\))"), R"(<a href="\2">\1</a>)");      // Standard links
        content.replace(QRegularExpression(R"(\[\[([^\]]+)\]\])"), R"(<a href="\1.md">\1</a>)");        // Wiki-links
        content.replace(QRegularExpression(R"(\*\*(.*?)\*\*)"), R"(<b>\1</b>)");                         // Bold
        content.replace(QRegularExpression(R"((?<!\*)\*(.*?)\*(?!\*))"), R"(<i>\1</i>)"); // Italic (negative lookbehind/ahead)
        content.replace(QRegularExpression(R"(`(.*?)`)"), R"(<code>\1</code>)");                         // Code
        
        // 3. Convert all newlines to <br> tags for perfect 1:1 line rendering.
        content.replace("\n", "<br>");

        // 4. Wrap the content in a minimal HTML document, using a div with 'white-space: pre'
        //    to force 1:1 rendering of all whitespace, including multiple spaces.
        QString html = QString("<!DOCTYPE html><html><body><div style='white-space: pre; font-family: \"%1\";'>%2</div></body></html>")
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
void MainWindow::saveFile(){if(m_currentFilePath.isEmpty()){QString fp=QFileDialog::getSaveFileName(this,"Guardar",m_currentVaultPath,"*.md");if(fp.isEmpty())return;m_currentFilePath=fp;}QFile f(m_currentFilePath);if(!f.open(QIODevice::WriteOnly|QIODevice::Text))return;QTextStream out(&f);QString c=m_isEditMode?m_textEdit->toPlainText():m_rawMarkdownBuffer;out<<c;f.close();if(m_isEditMode)m_rawMarkdownBuffer=c;m_textEdit->document()->setModified(false);setWindowTitle(QFileInfo(m_currentFilePath).fileName()+" - ME[*]");}
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