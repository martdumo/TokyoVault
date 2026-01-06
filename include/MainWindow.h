#pragma once

#include <QMainWindow>
#include <QModelIndex>
#include <QTextDocument> // Para QTextDocument::FindFlags

// Forward declarations para reducir tiempos de compilación y dependencias en cabeceras.
class QSplitter;
class QTreeView;
class QFileSystemModel;
class QToolButton;
class QLineEdit;
class QStatusBar;
class QLabel;
class QSortFilterProxyModel; // Para el filtro del QTreeView
class QCloseEvent; // Para el evento de cierre de ventana

// Nuevas clases
class MarkdownTextEdit; // Nuestro QTextEdit personalizado
class MarkdownHighlighter; // Para el resaltado de sintaxis
class FindReplaceDialog; // Diálogo de buscar y reemplazar

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override; // Para guardar la configuración al cerrar

private slots:
    // Slots de la barra de herramientas
    void openVault(const QString &path = "");
    void toggleEditMode();
    void historyBack();     // Navegar hacia atrás en el historial
    void historyForward();  // Navegar hacia adelante en el historial

    // Slots del menú Archivo
    void openFile();
    void saveFile();
    void closeVault();

    // Slots del menú Editar
    void showFindReplaceDialog();
    void findNextInEditor(const QString &text, QTextDocument::FindFlags flags);
    void replaceInEditor(const QString &findText, const QString &replaceText, QTextDocument::FindFlags flags);
    void replaceAllInEditor(const QString &findText, const QString &replaceText, QTextDocument::FindFlags flags);

    // Slots del menú Insertar
    void insertTableTemplate();
    void insertLinkTemplate();
    void insertImageTemplate();

    // Slots de Formato (Shortcuts)
    void applyBold();
    void applyItalic();
    void applyUnderline();

    // Slots de Configuración
    void showFontDialog();

    // Slots del QTreeView y Editor
    void onFileClicked(const QModelIndex &index);
    void updateStatusBar(); // Actualiza el conteo de palabras y la ruta.
    void filterVault(const QString &text); // Para el QLineEdit de búsqueda
    void handleWikiLinkActivated(const QString &linkName); // Gestiona el Ctrl+Click en wiki-links
    void onDocumentModified(); // Se activa cuando el texto cambia

private:
    // --- Funciones de configuración ---
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void setupUI();
    bool loadFile(const QString& filePath, bool addToHistory = true);
    void saveSettings(); // Guarda la configuración (última vault, fuente)
    void loadSettings(); // Carga la configuración
    void applyTextFormatting(const QString& prefix, const QString& suffix = "");
    bool maybeSave(); // Pregunta al usuario si quiere guardar los cambios

    // --- Widgets de la UI ---
    QSplitter *m_mainSplitter;
    QTreeView *m_treeView;
    MarkdownTextEdit *m_textEdit; // Ahora usa nuestra clase personalizada
    QToolButton *m_toggleButton;
    QLineEdit *m_searchBar; // Nueva barra de búsqueda
    QStatusBar *m_statusBar; // Nueva barra de estado
    QLabel *m_filePathLabel; // Muestra la ruta del archivo actual
    QLabel *m_wordCountLabel; // Muestra el conteo de palabras
    QToolButton *m_historyBackButton;
    QToolButton *m_historyForwardButton;

    // --- Modelos y estado ---
    QFileSystemModel *m_fileSystemModel;
    QSortFilterProxyModel *m_proxyModel; // Para filtrar el QTreeView
    MarkdownHighlighter *m_highlighter; // Resaltador de sintaxis

    QString m_currentFilePath;
    QString m_currentVaultPath; // Ruta del vault activo
    bool m_isEditMode;

    // Historial de archivos abiertos
    QList<QString> m_fileHistory;
    int m_historyIndex;

    // Diálogo de Buscar y Reemplazar
    FindReplaceDialog *m_findReplaceDialog;
};
