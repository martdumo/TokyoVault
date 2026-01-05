#include "MainWindow.h"

#include <QSplitter>
#include <QTreeView>
#include <QTextEdit>
#include <QFileSystemModel>
#include <QToolBar>
#include <QToolButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QVBoxLayout>

// Constructor principal de la ventana.
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_isEditMode(true) // Inicia en modo edición.
{
    setWindowTitle("Markdown Editor");
    setMinimumSize(800, 600);
    resize(1200, 800);

    setupToolbar();
    setupUI();
}

MainWindow::~MainWindow()
{
    // Qt se encarga de la memoria de los widgets hijos. No se necesita delete manual.
}

// Crea y configura la barra de herramientas superior.
void MainWindow::setupToolbar()
{
    QToolBar *toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);

    QToolButton *openVaultButton = new QToolButton(this);
    openVaultButton->setText("Open Vault");
    connect(openVaultButton, &QToolButton::clicked, this, &MainWindow::openVault);
    toolbar->addWidget(openVaultButton);

    m_toggleButton = new QToolButton(this);
    m_toggleButton->setText("Preview Mode");
    connect(m_toggleButton, &QToolButton::clicked, this, &MainWindow::toggleEditMode);
    toolbar->addWidget(m_toggleButton);
}

// Configura la interfaz de usuario principal con el splitter, el árbol y el editor.
void MainWindow::setupUI()
{
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);

    // --- Panel Izquierdo: QTreeView con el modelo del sistema de archivos ---
    m_fileSystemModel = new QFileSystemModel(this);
    m_fileSystemModel->setFilter(QDir::NoDotAndDotDot | QDir::AllEntries); // Filtros básicos
    m_fileSystemModel->setNameFilters(QStringList() << "*.md"); // Muestra solo archivos .md
    m_fileSystemModel->setNameFilterDisables(false); // Oculta los archivos que no coinciden

    m_treeView = new QTreeView(m_mainSplitter);
    m_treeView->setModel(m_fileSystemModel);
    // Oculta columnas innecesarias (tamaño, tipo, fecha)
    for (int i = 1; i < m_fileSystemModel->columnCount(); ++i) {
        m_treeView->hideColumn(i);
    }
    m_treeView->setHeaderHidden(true); // Oculta la cabecera "Name"

    connect(m_treeView, &QTreeView::clicked, this, &MainWindow::onFileClicked);

    // --- Panel Derecho: QTextEdit para el contenido ---
    m_textEdit = new QTextEdit(m_mainSplitter);
    m_textEdit->setReadOnly(false); // Inicia en modo editable

    // --- Configuración del Splitter ---
    m_mainSplitter->addWidget(m_treeView);
    m_mainSplitter->addWidget(m_textEdit);
    m_mainSplitter->setStretchFactor(0, 1); // El panel izquierdo (árbol) ocupa 1/4 del espacio
    m_mainSplitter->setStretchFactor(1, 3); // El panel derecho (editor) ocupa 3/4
    m_mainSplitter->setSizes({250, 750}); // Tamaños iniciales

    setCentralWidget(m_mainSplitter);
}

// Abre un diálogo para seleccionar una carpeta "Vault".
void MainWindow::openVault()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                    "", // Directorio inicial (vacío)
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        m_fileSystemModel->setRootPath(dir);
        m_treeView->setRootIndex(m_fileSystemModel->index(dir));
    }
}

// Se activa al hacer clic en un elemento del árbol.
void MainWindow::onFileClicked(const QModelIndex &index)
{
    QString filePath = m_fileSystemModel->filePath(index);
    if (m_fileSystemModel->isDir(index) || filePath.isEmpty()) {
        return; // Ignora directorios o índices inválidos
    }

    loadFile(filePath);
}

// Carga y muestra el contenido de un archivo de texto.
void MainWindow::loadFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not open file: " + file.errorString());
        return;
    }

    m_currentFilePath = filePath;
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    // Actualiza el contenido del editor según el modo actual.
    if (m_isEditMode) {
        m_textEdit->setPlainText(content);
    } else {
        m_textEdit->setMarkdown(content);
    }
}

// Cambia entre modo edición y modo vista previa.
void MainWindow::toggleEditMode()
{
    m_isEditMode = !m_isEditMode;

    if (m_isEditMode) {
        m_toggleButton->setText("Preview Mode");
        m_textEdit->setReadOnly(false);
        // Recarga el contenido como texto plano
        if (!m_currentFilePath.isEmpty()) {
            loadFile(m_currentFilePath);
        }
    } else {
        m_toggleButton->setText("Edit Mode");
        m_textEdit->setReadOnly(true);
        // El contenido actual en el editor se renderiza como Markdown
        m_textEdit->setMarkdown(m_textEdit->toPlainText());
    }
}
