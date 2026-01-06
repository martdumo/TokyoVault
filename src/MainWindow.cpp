#include "MainWindow.h"

// Nuevas inclusiones
#include "MarkdownTextEdit.h"
#include "MarkdownHighlighter.h"
#include "FindReplaceDialog.h"

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
#include <QSettings>
#include <QStandardPaths>
#include <QShortcut>
#include <QCloseEvent>
#include <QDir>
#include <QToolButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_isEditMode(true),
      m_historyIndex(-1),
      m_findReplaceDialog(nullptr)
{
    setWindowTitle("Markdown Editor");
    setMinimumSize(900, 700);
    resize(1400, 900);

    createMenus();
    createToolBar();
    createStatusBar();
    setupUI();

    loadSettings();
}

MainWindow::~MainWindow()
{
    // Qt gestiona la memoria de los hijos (widgets, etc.)
}

void MainWindow::createMenus()
{
    // --- Menú Archivo (Reorganizado) ---
    QMenu *fileMenu = menuBar()->addMenu("&Archivo");

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

    QAction *customizeAction = new QAction("&Personalizar...", this);
    connect(customizeAction, &QAction::triggered, this, &MainWindow::showFontDialog);
    fileMenu->addAction(customizeAction);

    fileMenu->addSeparator();

    QAction *exitAction = new QAction("S&alir", this);
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);
    fileMenu->addAction(exitAction);


    // --- Menú Editar ---
    QMenu *editMenu = menuBar()->addMenu("&Editar");
    QAction *findAction = new QAction("&Buscar y Reemplazar...", this);
    findAction->setShortcut(QKeySequence::Find);
    connect(findAction, &QAction::triggered, this, &MainWindow::showFindReplaceDialog);
    editMenu->addAction(findAction);

    // --- Menú Insertar ---
    QMenu *insertMenu = menuBar()->addMenu("&Insertar");
    QAction *insertTableAction = new QAction("Tabla", this);
    connect(insertTableAction, &QAction::triggered, this, &MainWindow::insertTableTemplate);
    insertMenu->addAction(insertTableAction);

    QAction *insertLinkAction = new QAction("Link", this);
    connect(insertLinkAction, &QAction::triggered, this, &MainWindow::insertLinkTemplate);
    insertMenu->addAction(insertLinkAction);

    QAction *insertImageAction = new QAction("Imagen", this);
    connect(insertImageAction, &QAction::triggered, this, &MainWindow::insertImageTemplate);
    insertMenu->addAction(insertImageAction);

    // --- Menú Formato ---
    QMenu *formatMenu = menuBar()->addMenu("F&ormato");
    QAction *boldAction = new QAction("&Negrita", this);
    boldAction->setShortcut(QKeySequence::Bold);
    connect(boldAction, &QAction::triggered, this, &MainWindow::applyBold);
    formatMenu->addAction(boldAction);

    QAction *italicAction = new QAction("&Cursiva", this);
    italicAction->setShortcut(QKeySequence::Italic);
    connect(italicAction, &QAction::triggered, this, &MainWindow::applyItalic);
    formatMenu->addAction(italicAction);

    QAction *underlineAction = new QAction("&Subrayado", this);
    underlineAction->setShortcut(QKeySequence::Underline);
    connect(underlineAction, &QAction::triggered, this, &MainWindow::applyUnderline);
    formatMenu->addAction(underlineAction);
}

void MainWindow::createToolBar()
{
    QToolBar *toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);

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

    // --- Panel Izquierdo: Buscador y Árbol de archivos ---
    QWidget *leftPanel = new QWidget(m_mainSplitter);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    m_searchBar = new QLineEdit(this);
    m_searchBar->setPlaceholderText("Buscar en el vault...");
    connect(m_searchBar, &QLineEdit::textChanged, this, &MainWindow::filterVault);
    leftLayout->addWidget(m_searchBar);

    m_fileSystemModel = new QFileSystemModel(this);
    m_fileSystemModel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
    m_fileSystemModel->setNameFilters(QStringList() << "*.md");
    m_fileSystemModel->setNameFilterDisables(false);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_fileSystemModel);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(0); // Filtra por la columna 0 (nombre)

    m_treeView = new QTreeView(m_mainSplitter);
    m_treeView->setModel(m_proxyModel);
    for (int i = 1; i < m_fileSystemModel->columnCount(); ++i) {
        m_treeView->hideColumn(i);
    }
    m_treeView->setHeaderHidden(true);
    connect(m_treeView, &QTreeView::clicked, this, &MainWindow::onFileClicked);
    leftLayout->addWidget(m_treeView);

    // --- Panel Derecho: Editor de Texto ---
    m_textEdit = new MarkdownTextEdit(m_mainSplitter);
    connect(m_textEdit, &MarkdownTextEdit::textChanged, this, &MainWindow::onDocumentModified);
    connect(m_textEdit, &MarkdownTextEdit::textChanged, this, &MainWindow::updateStatusBar);
    connect(m_textEdit, &MarkdownTextEdit::wikiLinkActivated, this, &MainWindow::handleWikiLinkActivated);
    
    // Configura el resaltador de sintaxis
    m_highlighter = new MarkdownHighlighter(m_textEdit->document());

    // --- Configuración del Splitter ---
    m_mainSplitter->addWidget(leftPanel);
    m_mainSplitter->addWidget(m_textEdit);
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 3);
    m_mainSplitter->setSizes({350, 1050});

    setCentralWidget(m_mainSplitter);
}


// --- Lógica de Slots ---

void MainWindow::openFile()
{
    if (maybeSave()) {
        QString filePath = QFileDialog::getOpenFileName(this, "Abrir Archivo Markdown", m_currentVaultPath, "Markdown (*.md)");
        if (!filePath.isEmpty()) {
            loadFile(filePath);
        }
    }
}

void MainWindow::openVault(const QString &path)
{
    QString dir = path;
    if (dir.isEmpty()) {
        dir = QFileDialog::getExistingDirectory(this, "Abrir Vault");
    }

    if (!dir.isEmpty()) {
        m_currentVaultPath = dir;
        m_fileSystemModel->setRootPath(dir);
        QModelIndex rootIndex = m_proxyModel->mapFromSource(m_fileSystemModel->index(dir));
        m_treeView->setRootIndex(rootIndex);

        // Cargar archivo de inicio si está configurado
        QSettings settings("MySoft", "MarkdownEditor");
        QString startupFile = settings.value("startup_file").toString();
        if (!startupFile.isEmpty()) {
            QString startupFilePath = m_currentVaultPath + "/" + startupFile;
            if (QFile::exists(startupFilePath)) {
                loadFile(startupFilePath);
            }
        }
    }
}

void MainWindow::closeVault() {
    if (maybeSave()) {
        m_currentVaultPath = "";
        m_currentFilePath = "";
        m_fileSystemModel->setRootPath("");
        m_textEdit->clear();
        setWindowTitle("Markdown Editor");
        m_textEdit->document()->setModified(false);
        updateStatusBar();
    }
}

void MainWindow::onFileClicked(const QModelIndex &proxyIndex)
{
    if (!proxyIndex.isValid()) return; 
    
    QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);
    QString filePath = m_fileSystemModel->filePath(sourceIndex);

    if (m_fileSystemModel->isDir(sourceIndex) || filePath.isEmpty()) {
        return;
    }

    if (maybeSave()) {
        loadFile(filePath);
    }
}

bool MainWindow::loadFile(const QString& filePath, bool addToHistory)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "No se pudo abrir el archivo: " + file.errorString());
        return false;
    }
    
    m_currentFilePath = filePath;
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    if (m_isEditMode) {
        m_textEdit->setPlainText(content);
    } else {
        m_textEdit->setMarkdown(content);
    }
    
    setWindowTitle(QFileInfo(filePath).fileName() + " - Markdown Editor [*]");
    m_textEdit->document()->setModified(false);
    updateStatusBar();

    // Gestión del historial
    if (addToHistory) {
        if(m_historyIndex >= 0 && m_historyIndex < m_fileHistory.size() - 1) {
            m_fileHistory = m_fileHistory.mid(0, m_historyIndex + 1);
        }
        if (m_fileHistory.isEmpty() || m_fileHistory.last() != filePath) {
            m_fileHistory.append(filePath);
        }
        m_historyIndex = m_fileHistory.size() - 1;
    }
    m_historyBackButton->setEnabled(m_historyIndex > 0);
    m_historyForwardButton->setEnabled(m_historyIndex < m_fileHistory.size() - 1);
    
    return true;
}

void MainWindow::saveFile()
{
    if (m_currentFilePath.isEmpty()) {
        return; // No hay archivo que guardar
    }

    QFile file(m_currentFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "No se pudo guardar el archivo: " + file.errorString());
        return;
    }

    QTextStream out(&file);
    out << m_textEdit->toPlainText();
    file.close();
    m_textEdit->document()->setModified(false);
}

void MainWindow::onDocumentModified() {
    setWindowModified(m_textEdit->document()->isModified());
}

bool MainWindow::maybeSave()
{
    if (!m_textEdit->document()->isModified()) {
        return true;
    }

    const QMessageBox::StandardButton ret =
        QMessageBox::warning(this, "Cambios sin guardar",
                             "El documento tiene cambios sin guardar.\n"
                             "¿Quieres guardar los cambios?",
                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    switch (ret) {
    case QMessageBox::Save:
        saveFile();
        return true;
    case QMessageBox::Cancel:
        return false;
    default: // QMessageBox::Discard
        break;
    }
    return true;
}

void MainWindow::toggleEditMode()
{
    m_isEditMode = !m_isEditMode;
    QString content = m_textEdit->toPlainText();

    if (m_isEditMode) {
        m_toggleButton->setText("Preview Mode");
        m_textEdit->setReadOnly(false);
        m_textEdit->setPlainText(content); // Vuelve a texto plano
        m_highlighter->rehighlight(); // Activa el resaltador
    } else {
        m_toggleButton->setText("Edit Mode");
        m_textEdit->setReadOnly(true);
        m_textEdit->setMarkdown(content); // Renderiza como Markdown
    }
}

void MainWindow::updateStatusBar()
{
    if (m_currentFilePath.isEmpty()) {
        m_filePathLabel->setText("Ningún archivo abierto");
    } else {
        m_filePathLabel->setText(m_currentFilePath);
    }

    QString text = m_textEdit->toPlainText();
    int wordCount = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).count();
    m_wordCountLabel->setText(QString("%1 palabras").arg(wordCount));
}

void MainWindow::filterVault(const QString &text)
{
    // Aún no se implementa la búsqueda por contenido.
    // Esta es solo la búsqueda por nombre de archivo.
    m_proxyModel->setFilterRegularExpression(QRegularExpression(text, QRegularExpression::CaseInsensitiveOption));
}

void MainWindow::historyBack() {
    if (m_historyIndex > 0) {
        if (maybeSave()) {
            m_historyIndex--;
            loadFile(m_fileHistory[m_historyIndex], false); // No añadir al historial de nuevo
        }
    }
}

void MainWindow::historyForward() {
    if (m_historyIndex < m_fileHistory.size() - 1) {
        if (maybeSave()) {
            m_historyIndex++;
            loadFile(m_fileHistory[m_historyIndex], false);
        }
    }
}

void MainWindow::handleWikiLinkActivated(const QString &linkName) {
    if (m_currentVaultPath.isEmpty()) return;

    QString newFilePath = m_currentVaultPath + "/" + linkName.trimmed() + ".md";
    QFileInfo fileInfo(newFilePath);
    
    if (fileInfo.exists()) {
        if(maybeSave()) {
            loadFile(newFilePath);
        }
    } else {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Crear archivo",
                                       "El archivo '" + linkName.trimmed() + ".md' no existe.\n"
                                       "¿Quieres crearlo?",
                                       QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            if (maybeSave()) {
                QFile file(newFilePath);
                if (file.open(QIODevice::WriteOnly)) { // Crea el archivo vacío
                    file.close();
                    loadFile(newFilePath); // Carga el nuevo archivo
                } else {
                    QMessageBox::warning(this, "Error", "No se pudo crear el archivo.");
                }
            }
        }
    }
}

// --- Menús y Formato ---

void MainWindow::applyTextFormatting(const QString& prefix, const QString& suffix) {
    QTextCursor cursor = m_textEdit->textCursor();
    if (cursor.hasSelection()) {
        QString selectedText = cursor.selectedText();
        cursor.insertText(prefix + selectedText + (suffix.isEmpty() ? prefix : suffix));
    }
}

void MainWindow::applyBold() { applyTextFormatting("**"); }
void MainWindow::applyItalic() { applyTextFormatting("*"); }
void MainWindow::applyUnderline() { applyTextFormatting("<u>", "</u>"); }

void MainWindow::insertTableTemplate() {
    m_textEdit->textCursor().insertText("\n| Cabecera 1 | Cabecera 2 |\n|------------|------------|\n| Celda 1    | Celda 2    |\n| Celda 3    | Celda 4    |\n");
}
void MainWindow::insertLinkTemplate() {
    m_textEdit->textCursor().insertText("[texto del link](http://url.com)");
}
void MainWindow::insertImageTemplate() {
    m_textEdit->textCursor().insertText("![texto alternativo](http://url/imagen.jpg)");
}

void MainWindow::showFindReplaceDialog()
{
    if (!m_findReplaceDialog) {
        m_findReplaceDialog = new FindReplaceDialog(this);
        connect(m_findReplaceDialog, &FindReplaceDialog::findNext, this, &MainWindow::findNextInEditor);
        connect(m_findReplaceDialog, &FindReplaceDialog::replace, this, &MainWindow::replaceInEditor);
        connect(m_findReplaceDialog, &FindReplaceDialog::replaceAll, this, &MainWindow::replaceAllInEditor);
    }
    m_findReplaceDialog->show();
    m_findReplaceDialog->raise();
    m_findReplaceDialog->activateWindow();
}

void MainWindow::findNextInEditor(const QString &text, QTextDocument::FindFlags flags) {
    if (!m_textEdit->find(text, flags)) {
        QMessageBox::information(this, "No encontrado", "No se encontraron más coincidencias.");
    }
}

void MainWindow::replaceInEditor(const QString &findText, const QString &replaceText, QTextDocument::FindFlags flags) {
    QTextCursor cursor = m_textEdit->textCursor();
    if(cursor.hasSelection() && cursor.selectedText().compare(findText, flags.testFlag(QTextDocument::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive) == 0) {
        cursor.insertText(replaceText);
    }
    findNextInEditor(findText, flags);
}

void MainWindow::replaceAllInEditor(const QString &findText, const QString &replaceText, QTextDocument::FindFlags flags) {
    m_textEdit->moveCursor(QTextCursor::Start);
    int count = 0;
    while(m_textEdit->find(findText, flags)) {
        m_textEdit->textCursor().insertText(replaceText);
        count++;
    }
    QMessageBox::information(this, "Reemplazar Todo", QString("%1 coincidencias reemplazadas.").arg(count));
}


// --- Configuración ---

void MainWindow::showFontDialog()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_textEdit->font(), this, "Elige una fuente para el editor");
    if (ok) {
        m_textEdit->setFont(font);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        saveSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::saveSettings()
{
    QSettings settings("MySoft", "MarkdownEditor");
    settings.beginGroup("MainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.endGroup();

    settings.beginGroup("Editor");
    settings.setValue("font", m_textEdit->font());
    settings.setValue("lastVaultPath", m_currentVaultPath);
    settings.endGroup();
}

void MainWindow::loadSettings()
{
    QSettings settings("MySoft", "MarkdownEditor");
    settings.beginGroup("MainWindow");
    const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    const QByteArray state = settings.value("windowState", QByteArray()).toByteArray();
    if (!state.isEmpty()) {
        restoreState(state);
    }
    settings.endGroup();

    settings.beginGroup("Editor");
    QFont defaultFont("JetBrains Mono");
    m_textEdit->setFont(settings.value("font", defaultFont).value<QFont>());
    QString lastVault = settings.value("lastVaultPath", "").toString();
    if (!lastVault.isEmpty() && QDir(lastVault).exists()) {
        openVault(lastVault);
    }
    settings.endGroup();
}