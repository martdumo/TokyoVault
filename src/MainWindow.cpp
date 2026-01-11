#include "MainWindow.h"
#include "EditorStyler.h"
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
#include <QMenu>
#include <QContextMenuEvent>
#include <QCompleter>
#include <QAbstractItemView>
#include <QKeyEvent>
#include <QStringListModel>
#include <QDirIterator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_isEditMode(false),
      m_historyIndex(-1),
      m_searchThread(nullptr),
      m_searchWorker(nullptr),
      m_searchTimer(nullptr),
      m_findReplaceDialog(nullptr)
{
    setWindowIcon(QIcon(":/assets/icons/app.ico"));
    setWindowTitle("Markdown Editor [*]");
    setMinimumSize(900, 700);
    resize(1400, 900);

    // Hardcoded Tokyo Night theme
    QString styleSheet = QString(
        "QMainWindow,QDialog{background-color:#1a1b26;color:#c0caf5;}"
        "QMenuBar{background-color:#1a1b26;color:#c0caf5;}"
        "QMenuBar::item:selected{background-color:#7aa2f7;color:#24283b;}"
        "QMenu{background-color:#24283b;color:#c0caf5;border:1px solid #565f89;}"
        "QToolBar{background-color:#1a1b26;border:none;}"
        "QToolButton{color:#c0caf5;background-color:transparent;border:1px solid #565f89;padding:4px;border-radius:4px;}"
        "QStatusBar{color:#c0caf5;background-color:#1a1b26;}"
        "QTreeView{background-color:#1a1b26;color:#c0caf5;border:none;}"
        "QLineEdit{background-color:#24283b;color:#c0caf5;border:1px solid #565f89;border-radius:4px;padding:4px;}"
        "QTextEdit{background-color:#24283b;color:#c0caf5;border:none;margin-left:25px;padding-top:10px;}"
        "QTextEdit a{color:#7aa2f7;text-decoration:none;}"
        "QScrollBar:vertical{background:#1a1b26;width:10px;margin:0;}"
        "QScrollBar::handle:vertical{background:#565f89;min-height:20px;border-radius:5px;}"
    );
    qApp->setStyleSheet(styleSheet);

    m_styler = std::make_unique<EditorStyler>();

    createMenus();
    createToolBar();
    createStatusBar();
    setupUI();
    setupSearchThread();

    new QShortcut(QKeySequence("Ctrl+E"), this, SLOT(toggleEditMode()));
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_3), this, SLOT(toggleEditMode()));
    setupAutoCompletion();
    connect(m_textEdit, &QTextEdit::textChanged, this, &MainWindow::onTextChanged);
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

    QMenu *helpMenu = menuBar()->addMenu("&Ayuda");
    QAction *guideAction = new QAction("&Guía de Markdown", this);
    connect(guideAction, &QAction::triggered, this, &MainWindow::showHelpDialog);
    helpMenu->addAction(guideAction);

    QAction *helpAction = new QAction("&Ayuda Real", this);
    connect(helpAction, &QAction::triggered, this, &MainWindow::showHelp);
    helpMenu->addAction(helpAction);
}

void MainWindow::createToolBar()
{
    QToolBar *toolbar = addToolBar("Main Toolbar");
    toolbar->setObjectName("MainToolbar");
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

    toolbar->addSeparator();
    QToolButton *boldButton = new QToolButton(this);
    boldButton->setText("N");
    boldButton->setToolTip("Negrita (Ctrl+B)");
    connect(boldButton, &QToolButton::clicked, this, [this](){ applyFormat("**", "**"); });
    toolbar->addWidget(boldButton);

    QToolButton *italicButton = new QToolButton(this);
    italicButton->setText("I");
    italicButton->setToolTip("Itálica (Ctrl+I)");
    connect(italicButton, &QToolButton::clicked, this, [this](){ applyFormat("*", "*"); });
    toolbar->addWidget(italicButton);

    QToolButton *codeButton = new QToolButton(this);
    codeButton->setText("C");
    codeButton->setToolTip("Código (Ctrl+`)");
    connect(codeButton, &QToolButton::clicked, this, [this](){ applyFormat("`", "`"); });
    toolbar->addWidget(codeButton);
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
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &MainWindow::onTreeContextMenu);
    connect(m_treeView, &QTreeView::clicked, this, &MainWindow::onFileClicked);
    leftLayout->addWidget(m_treeView);
    m_textEdit = new MarkdownTextEdit(m_mainSplitter);
    m_textEdit->setContextMenuPolicy(Qt::DefaultContextMenu);
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

void MainWindow::closeEvent(QCloseEvent *event) { if (maybeSave()) { saveSettings(); event->accept(); } else { event->ignore(); } }
void MainWindow::onDocumentModified() { setWindowModified(m_textEdit->document()->isModified()); }

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
        if (QFile::exists(newFilePath)) {
            QMessageBox::warning(this, "Archivo existente", "Un archivo con ese nombre ya existe.");
            return;
        }
        if (!maybeSave()) return;
        QFile file(newFilePath);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "Error", "No se pudo crear el archivo.");
            return;
        }
        file.close();
        loadFile(newFilePath);
    }
}

void MainWindow::openFile() { if(maybeSave()){QString fp=QFileDialog::getOpenFileName(this,"Abrir",m_currentVaultPath,"*.md");if(!fp.isEmpty())loadFile(fp);}}
void MainWindow::openVault(const QString &path){
    QString d=path.isEmpty()?QFileDialog::getExistingDirectory(this,"Abrir Vault"):path;
    if(!d.isEmpty()){
        m_currentVaultPath=d;
        m_fileSystemModel->setRootPath(d);
        m_treeView->setRootIndex(m_proxyModel->mapFromSource(m_fileSystemModel->index(d)));
        if (!m_defaultVaultFile.isEmpty() && QFile::exists(m_defaultVaultFile) && m_defaultVaultFile.startsWith(m_currentVaultPath)) {
            loadFile(m_defaultVaultFile);
        }
    }
}
void MainWindow::closeVault(){
    if(maybeSave()){
        m_currentVaultPath.clear();
        m_currentFilePath.clear();
        m_fileSystemModel->setRootPath("");
        m_textEdit->clear();
        m_rawMarkdownBuffer.clear();
        setWindowTitle("ME[*]");
        m_textEdit->document()->setModified(false);
        updateStatusBar();
    }
}
void MainWindow::onFileClicked(const QModelIndex &idx){
    if(!idx.isValid())return;
    auto sIdx=m_proxyModel->mapToSource(idx);
    if (!sIdx.isValid()) return;
    QString fp=m_fileSystemModel->filePath(sIdx);
    if(m_fileSystemModel->isDir(sIdx)||fp.isEmpty())return;
    if(maybeSave())loadFile(fp);
}
bool MainWindow::loadFile(const QString& fp,bool addHist){
    // Security File Guard: Check file size before opening
    QFileInfo fileInfo(fp);
    if (fileInfo.size() > 5 * 1024 * 1024) { // 5MB limit
        QMessageBox::warning(this, "Archivo demasiado grande", "Archivo demasiado grande por seguridad (Límite 5MB)");
        return false;
    }

    QFile f(fp);
    if(!f.open(QIODevice::ReadOnly|QIODevice::Text))return false;
    m_currentFilePath=fp;
    QTextStream in(&f);
    m_rawMarkdownBuffer=in.readAll();
    f.close();
    m_textEdit->setReadOnly(!m_isEditMode);
    m_highlighter->setDocument(m_isEditMode ? m_textEdit->document() : nullptr);
    if (m_isEditMode) {
        m_textEdit->setPlainText(m_rawMarkdownBuffer);
        m_highlighter->rehighlight();
        // Reset character format to ensure a clean slate when loading in edit mode
        m_textEdit->setCurrentCharFormat(QTextCharFormat());
        m_textEdit->document()->setDefaultFont(m_textEdit->font());
    } else {
        QString html = m_styler->renderMarkdown(m_rawMarkdownBuffer, m_textEdit->font());
        m_textEdit->setHtml(html);
    }
    m_toggleButton->setText(m_isEditMode ? "Preview Mode" : "Edit Mode");
    m_textEdit->document()->setModified(false);
    setWindowTitle(QFileInfo(fp).fileName()+" - ME[*]");
    updateStatusBar();
    if(addHist){
        if(m_historyIndex>=0&&m_historyIndex<m_fileHistory.size()-1)
            m_fileHistory=m_fileHistory.mid(0,m_historyIndex+1);
        if(m_fileHistory.isEmpty()||m_fileHistory.last()!=fp)
            m_fileHistory.append(fp);
        m_historyIndex=m_fileHistory.size()-1;
    }
    m_historyBackButton->setEnabled(m_historyIndex>0);
    m_historyForwardButton->setEnabled(m_historyIndex<m_fileHistory.size()-1);
    return true;
}
void MainWindow::saveFile(){
    if(m_currentFilePath.isEmpty()){
        QString fp=QFileDialog::getSaveFileName(this,"Guardar",m_currentVaultPath,"*.md");
        if(fp.isEmpty())return;
        m_currentFilePath=fp;
    }
    QFile f(m_currentFilePath);
    if(!f.open(QIODevice::WriteOnly|QIODevice::Text))return;
    QTextStream out(&f);
    QString c=m_isEditMode?m_textEdit->toPlainText():m_rawMarkdownBuffer;
    out<<c;
    f.close();
    if(m_isEditMode)m_rawMarkdownBuffer=c;
    m_textEdit->document()->setModified(false);
    setWindowTitle(QFileInfo(m_currentFilePath).fileName()+" - ME[*]");
}

bool MainWindow::maybeSave(){
    if(!m_textEdit->document()->isModified())return true;
    auto r=QMessageBox::warning(this,"Guardar","Cambios sin guardar",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);
    if(r==QMessageBox::Save){ saveFile(); return!m_textEdit->document()->isModified(); }
    if(r==QMessageBox::Cancel)return false;
    return true;
}

void MainWindow::toggleEditMode()
{
    m_isEditMode = !m_isEditMode;
    bool wasModified = m_textEdit->document()->isModified();

    if (m_isEditMode) {
        m_toggleButton->setText("Preview Mode");
        m_textEdit->setReadOnly(false);
        m_highlighter->setDocument(m_textEdit->document());
        // Reset character format to neutral and ensure correct font when returning to edit mode
        m_textEdit->setCurrentCharFormat(QTextCharFormat());
        m_textEdit->document()->setDefaultFont(m_textEdit->font());
        m_textEdit->setPlainText(m_rawMarkdownBuffer);
        m_highlighter->rehighlight();
    } else {
        m_toggleButton->setText("Edit Mode");
        m_highlighter->setDocument(nullptr);
        m_rawMarkdownBuffer = m_textEdit->toPlainText();
        QString html = m_styler->renderMarkdown(m_rawMarkdownBuffer, m_textEdit->font());
        m_textEdit->setReadOnly(true);
        m_textEdit->setHtml(html);
    }
    m_textEdit->document()->setModified(wasModified);
    updateStatusBar();
}

void MainWindow::filterVault() { m_searchTimer->start(); }
void MainWindow::startContentSearch() {
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

void MainWindow::handleSearchResults(const QStringList &matchingFiles) {
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

void MainWindow::updateLinksAfterRename(const QString &oldName, const QString &newName)
{
    QStringList allFiles = getAllMarkdownFilesInVault();
    for (const QString &filePath : allFiles) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadWrite)) continue;
        QTextStream stream(&file);
        QString content = stream.readAll();
        file.close();
        QString updatedContent = content;
        updatedContent.replace("[[" + oldName + "]]", "[[" + newName + "]]");
        updatedContent.replace("[[" + oldName + ".md]]", "[[" + newName + ".md]]");
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream outStream(&file);
            outStream << updatedContent;
            file.close();
        }
    }
}

void MainWindow::updateStatusBar(){
    m_filePathLabel->setText(m_currentFilePath.isEmpty() ? "" : m_currentFilePath);
    QString t=m_isEditMode?m_textEdit->toPlainText():m_rawMarkdownBuffer;
    m_wordCountLabel->setText(QString("%1w").arg(t.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts).count()));
}
void MainWindow::historyBack(){ if(m_historyIndex>0&&maybeSave()){ m_historyIndex--; loadFile(m_fileHistory[m_historyIndex],false); } }
void MainWindow::historyForward(){ if(m_historyIndex<m_fileHistory.size()-1&&maybeSave()){ m_historyIndex++; loadFile(m_fileHistory[m_historyIndex],false); } }
void MainWindow::handleLinkNavigation(const QString &link) {
    QString finalLink = link.trimmed();
    if (finalLink.contains("://")) { QDesktopServices::openUrl(QUrl(finalLink)); return; }
    if (m_currentVaultPath.isEmpty()) return;
    QString filePath = findFileInVault(finalLink);
    if (!filePath.isEmpty()) { if (maybeSave()) loadFile(filePath); } 
    else {
        QString newFilePath = finalLink.endsWith(".md") ? m_currentVaultPath + "/" + finalLink : m_currentVaultPath + "/" + finalLink + ".md";
        auto reply = QMessageBox::question(this, "Crear archivo", "El archivo '" + QFileInfo(newFilePath).fileName() + "' no existe.\n¿Quieres crearlo?", QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            if (!maybeSave()) return;
            QFile file(newFilePath);
            if (file.open(QIODevice::WriteOnly)) { file.close(); loadFile(newFilePath); } 
            else { QMessageBox::warning(this, "Error", "No se pudo crear el archivo."); }
        }
    }
}
QString MainWindow::findFileInVault(const QString &fileName) {
    if (m_currentVaultPath.isEmpty()) return QString();
    QString searchName = fileName;
    if (!searchName.endsWith(".md")) searchName += ".md";
    QDirIterator it(m_currentVaultPath, QStringList() << searchName, QDir::Files, QDirIterator::Subdirectories);
    if (it.hasNext()) return it.next();
    return QString();
}
void MainWindow::showFontDialog() { bool ok; QFont font = QFontDialog::getFont(&ok, m_textEdit->font(), this); if (ok) m_textEdit->setFont(font); }
void MainWindow::showHelpDialog() { QMessageBox::information(this, "Guía de Markdown", "<h2>Guía Rápida</h2><p><code>**Negrita**</code>, <code>*Cursiva*</code>, <code>`Código`</code>, <code># Título</code>, <code>[[Link Interno]]</code></p>"); }
void MainWindow::showFindReplaceDialog() {
    if (!m_isEditMode) { QMessageBox::information(this, "Modo Preview", "La búsqueda solo está disponible en modo edición."); return; }
    if (!m_findReplaceDialog) {
        m_findReplaceDialog = new FindReplaceDialog(this);
        connect(m_findReplaceDialog, &FindReplaceDialog::findNext, this, &MainWindow::findNextInEditor);
        connect(m_findReplaceDialog, &FindReplaceDialog::replace, this, &MainWindow::replaceInEditor);
        connect(m_findReplaceDialog, &FindReplaceDialog::replaceAll, this, &MainWindow::replaceAllInEditor);
    }
    m_findReplaceDialog->show(); m_findReplaceDialog->raise(); m_findReplaceDialog->activateWindow();
}
void MainWindow::findNextInEditor(const QString &text, QTextDocument::FindFlags flags) { if (text.isEmpty() || !m_isEditMode) return; if (!m_textEdit->find(text, flags)) { if(QMessageBox::question(this, "Fin de la búsqueda", "¿Continuar desde el principio?", QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) { QTextCursor cursor=m_textEdit->textCursor(); cursor.movePosition(QTextCursor::Start); m_textEdit->setTextCursor(cursor); m_textEdit->find(text, flags); } } }
void MainWindow::replaceInEditor(const QString &findText, const QString &replaceText, QTextDocument::FindFlags flags) { if (findText.isEmpty() || !m_isEditMode) return; QTextCursor cursor = m_textEdit->textCursor(); if (cursor.hasSelection()) { if (cursor.selectedText() == findText || ((flags & QTextDocument::FindCaseSensitively)==0 && cursor.selectedText().compare(findText, Qt::CaseInsensitive)==0) ) cursor.insertText(replaceText); } findNextInEditor(findText, flags); }
void MainWindow::replaceAllInEditor(const QString &findText, const QString &replaceText, QTextDocument::FindFlags flags) { if (findText.isEmpty() || !m_isEditMode) return; int count = 0; QTextCursor cursor = m_textEdit->textCursor(); cursor.movePosition(QTextCursor::Start); m_textEdit->setTextCursor(cursor); while(m_textEdit->find(findText, flags)) { m_textEdit->textCursor().insertText(replaceText); count++; } QMessageBox::information(this, "Reemplazar todo", QString("%1 coincidencias reemplazadas.").arg(count)); }
void MainWindow::saveSettings() { QSettings s("MS","ME"); s.beginGroup("MW"); s.setValue("g",saveGeometry()); s.setValue("s",saveState()); s.endGroup(); s.beginGroup("ED"); s.setValue("f",m_textEdit->font()); s.setValue("vp",m_currentVaultPath); s.setValue("dvf",m_defaultVaultFile); s.endGroup(); }

void MainWindow::loadSettings() {
    QSettings s("MS","ME");
    s.beginGroup("MW");
    restoreGeometry(s.value("g").toByteArray());
    restoreState(s.value("s").toByteArray());
    s.endGroup();
    s.beginGroup("ED");
    m_textEdit->setFont(s.value("f",QFont("JetBrains Mono")).value<QFont>());
    QString lv = s.value("vp","").toString();
    m_defaultVaultFile = s.value("dvf", "").toString();
    if(!lv.isEmpty() && QDir(lv).exists()) openVault(lv);
    s.endGroup();
}

void MainWindow::setupAutoCompletion() { m_completer = new QCompleter(this); m_completer->setModel(new QStringListModel(m_markdownFiles, m_completer)); m_completer->setCaseSensitivity(Qt::CaseInsensitive); m_completer->setCompletionMode(QCompleter::PopupCompletion); m_completer->setWidget(m_textEdit); connect(m_completer, QOverload<const QString &>::of(&QCompleter::activated), this, &MainWindow::insertCompletion); }
void MainWindow::insertCompletion(const QString &completion) {
    if (m_completer->widget() != m_textEdit) return;
    QTextCursor tc = m_textEdit->textCursor();
    int extra = completion.length() - m_completer->completionPrefix().length();
    tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, m_completer->completionPrefix().length());
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra) + "]]" );
    m_textEdit->setTextCursor(tc);
}
QStringList MainWindow::getMarkdownFileNames() { QStringList fileNames; if (!m_currentVaultPath.isEmpty()) { QDirIterator it(m_currentVaultPath, QStringList() << "*.md", QDir::Files, QDirIterator::Subdirectories); while (it.hasNext()) { fileNames << QFileInfo(it.next()).baseName(); } } return fileNames; }
void MainWindow::updateAutoCompletionModel() { m_markdownFiles = getMarkdownFileNames(); static_cast<QStringListModel*>(m_completer->model())->setStringList(m_markdownFiles); }
void MainWindow::updateCompleterModel() {
    if (!m_currentVaultPath.isEmpty()) {
        QStringList fileNames;
        QDirIterator it(m_currentVaultPath, QStringList() << "*.md", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            QString fileName = QFileInfo(filePath).baseName();
            fileNames << fileName;
        }

        QStringListModel *model = new QStringListModel(fileNames, m_completer);
        m_completer->setModel(model);
    }
}

void MainWindow::showHelp() {
    QString helpFilePath = "assets/help.md";
    QFile helpFile(helpFilePath);

    QString helpContent;

    if (!helpFile.exists()) {
        helpContent = "# Guía de Usuario del Markdown Editor\n\n"
                     "## Atajos de Teclado\n"
                     "- `Ctrl+N`: Nuevo archivo\n"
                     "- `Ctrl+O`: Abrir archivo\n"
                     "- `Ctrl+S`: Guardar archivo\n"
                     "- `Ctrl+E`: Cambiar entre modo edición y vista previa\n"
                     "- `Ctrl+F`: Buscar y reemplazar\n"
                     "- `Ctrl++`: Acercar zoom\n"
                     "- `Ctrl+-`: Alejar zoom\n\n"

                     "## Funcionalidades\n"
                     "- **Wiki-links**: Escribe `[[nombre]]` para crear enlaces internos\n"
                     "- **Formato**: Usa `**negrita**`, `*cursiva*`, y `` `código` ``\n"
                     "- **Encabezados**: Usa `# H1`, `## H2`, ..., `###### H6`\n"
                     "- **Imágenes**: Inserta con `![alt text](ruta_imagen)`\n"
                     "- **Tablas**: Crea tablas con el formato `| col1 | col2 |`\n\n"

                     "## Navegación\n"
                     "- Haz clic en los archivos del vault para abrirlos\n"
                     "- Usa los botones de navegación para moverte entre archivos recientes\n"
                     "- Mantén presionado `Ctrl` y haz clic en un wiki-link para navegar\n\n"

                     "¡Disfruta escribiendo tus documentos en Markdown!";
    } else {
        if (helpFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&helpFile);
            helpContent = in.readAll();
            helpFile.close();
        } else {
            helpContent = "# Guía de Usuario del Markdown Editor\n\n"
                         "No se pudo leer el archivo de ayuda. Aquí tienes una guía básica:\n\n"

                         "## Atajos de Teclado\n"
                         "- `Ctrl+E`: Cambiar entre modo edición y vista previa\n"
                         "- `Ctrl+F`: Buscar y reemplazar\n\n"

                         "## Funcionalidades\n"
                         "- **Wiki-links**: Escribe `[[nombre]]` para crear enlaces internos\n"
                         "- **Formato**: Usa `**negrita**`, `*cursiva*`, y `` `código` ``\n";
        }
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Ayuda del Editor");
    msgBox.setTextFormat(Qt::RichText);

    QString htmlContent = m_styler->renderMarkdown(helpContent, font());
    msgBox.setText(htmlContent);
    msgBox.setDetailedText(helpContent);
    msgBox.exec();
}
void MainWindow::showAutoCompletePopup(const QString &prefix) { if (m_completer->completionCount() > 0) { m_completer->setCompletionPrefix(prefix); m_completer->complete(); } }
void MainWindow::onTextChanged() {
    if (!m_isEditMode) return;
    QTextCursor cursor = m_textEdit->textCursor();
    int pos = cursor.position();
    QString text = m_textEdit->toPlainText();
    if (pos <= 0 || pos > text.length()) return;
    QString prefix = text.left(pos);
    int bracketPos = prefix.lastIndexOf("[[");
    if (bracketPos != -1) {
        QString completionPrefix = prefix.mid(bracketPos + 2);
        updateCompleterModel();
        if (!completionPrefix.isEmpty()) showAutoCompletePopup(completionPrefix);
    }
}

void MainWindow::applyFormat(const QString &prefix, const QString &suffix) {
    if (!m_isEditMode) return;

    QTextCursor cursor = m_textEdit->textCursor();
    QString selectedText = cursor.selectedText();

    if (selectedText.isEmpty()) {
        cursor.insertText(prefix + suffix);
        cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor, suffix.length());
        m_textEdit->setTextCursor(cursor);
    } else {
        cursor.insertText(prefix + selectedText + suffix);
    }
}

QStringList MainWindow::getAllMarkdownFilesInVault() {
    QStringList filePaths;
    if (!m_currentVaultPath.isEmpty()) {
        QDirIterator it(m_currentVaultPath, QStringList() << "*.md", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            filePaths << it.next();
        }
    }
    return filePaths;
}

void MainWindow::onTreeContextMenu(const QPoint &pos) {
    QModelIndex proxyIndex = m_treeView->indexAt(pos);
    if (!proxyIndex.isValid()) return;

    QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);
    if (!sourceIndex.isValid()) return;

    QString filePath = m_fileSystemModel->filePath(sourceIndex);
    QFileInfo fileInfo(filePath);

    QMenu contextMenu(this);
    if (fileInfo.isFile()) {
        QAction *setDefaultAction = contextMenu.addAction("Establecer como archivo por defecto");
        connect(setDefaultAction, &QAction::triggered, this, [this, filePath]() { m_defaultVaultFile = filePath; });
    }

    QAction *renameAction = contextMenu.addAction("Renombrar");
    connect(renameAction, &QAction::triggered, this, [this, sourceIndex]() { showRenameDialog(); });

    QAction *deleteAction = contextMenu.addAction("Eliminar");
    connect(deleteAction, &QAction::triggered, this, [this, sourceIndex]() { deleteFileOrFolder(); });

    if (m_fileSystemModel->isDir(sourceIndex)) {
        QAction *createFileAction = contextMenu.addAction("Crear archivo .md");
        connect(createFileAction, &QAction::triggered, this, [this, filePath]() { newFile(); });
    }

    contextMenu.exec(m_treeView->mapToGlobal(pos));
}

void MainWindow::createFolder() {
    if (m_currentVaultPath.isEmpty()) return;
    bool ok = false;
    QString folderName = QInputDialog::getText(this, "Nueva Carpeta", "Nombre:", QLineEdit::Normal, "", &ok);
    if (ok && !folderName.isEmpty()) {
        QDir(m_currentVaultPath).mkdir(folderName);
    }
}

void MainWindow::setDefaultFile() {
    QModelIndex proxyIndex = m_treeView->currentIndex();
    if (!proxyIndex.isValid()) return;
    QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);
    if (!sourceIndex.isValid()) return;
    m_defaultVaultFile = m_fileSystemModel->filePath(sourceIndex);
}

void MainWindow::showRenameDialog() {
    QModelIndex proxyIndex = m_treeView->currentIndex();
    if (!proxyIndex.isValid()) return;
    QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);
    if (!sourceIndex.isValid()) return;

    QString oldPath = m_fileSystemModel->filePath(sourceIndex);
    QFileInfo fileInfo(oldPath);
    bool ok = false;
    QString newName = QInputDialog::getText(this, "Renombrar", "Nuevo nombre:", QLineEdit::Normal, fileInfo.fileName(), &ok);

    if (ok && !newName.isEmpty() && newName != fileInfo.fileName()) {
        QString newPath = fileInfo.absolutePath() + "/" + newName;
        if (QFile::rename(oldPath, newPath)) {
            if (!fileInfo.isDir()) {
                updateLinksAfterRename(fileInfo.baseName(), QFileInfo(newPath).baseName());
            }
        }
    }
}

void MainWindow::deleteFileOrFolder() {
    QModelIndex proxyIndex = m_treeView->currentIndex();
    if (!proxyIndex.isValid()) return;
    QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);
    if (!sourceIndex.isValid()) return;

    QString path = m_fileSystemModel->filePath(sourceIndex);
    auto reply = QMessageBox::question(this, "Eliminar", "¿Estás seguro de eliminar '" + QFileInfo(path).fileName() + "'?", QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (QFileInfo(path).isDir()) {
            QDir(path).removeRecursively();
        } else {
            QFile::remove(path);
        }
    }
}
