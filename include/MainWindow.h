#pragma once

#include <QMainWindow>
#include <QModelIndex>
#include <QTextDocument>
#include <memory> // For std::unique_ptr

// Forward declarations
class QSplitter;
class QTreeView;
class QFileSystemModel;
class QToolButton;
class QLineEdit;
class QStatusBar;
class QLabel;
class QSortFilterProxyModel;
class QCloseEvent;
class QActionGroup;
class QApplication;
class QThread;
class QTimer;
class MarkdownTextEdit;
class MarkdownHighlighter;
class FindReplaceDialog;
class SearchWorker;
class QCompleter;
class EditorStyler; // Forward-declare the new class

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // --- UI Actions ---
    void openVault(const QString &path = "");
    void toggleEditMode();
    void historyBack();
    void historyForward();
    void newFile();
    void openFile();
    void saveFile();
    void closeVault();
    void showFindReplaceDialog();
    void showFontDialog();
    void applyTheme(QAction* action); // Modified to take QAction
    void showHelpDialog();

    // --- Search ---
    void filterVault(); 
    void startContentSearch();
    void handleSearchResults(const QStringList &matchingFiles);

    // --- State & Navigation ---
    void onFileClicked(const QModelIndex &index);
    void updateStatusBar();
    void handleLinkNavigation(const QString &link);
    void onDocumentModified();

    // --- Find/Replace Dialog ---
    void findNextInEditor(const QString &text, QTextDocument::FindFlags flags);
    void replaceInEditor(const QString &findText, const QString &replaceText, QTextDocument::FindFlags flags);
    void replaceAllInEditor(const QString &findText, const QString &replaceText, QTextDocument::FindFlags flags);

    // --- Vault/File Management ---
    void createFolder();
    void setDefaultFile();
    void updateLinksAfterRename(const QString &oldName, const QString &newName);
    void deleteFileOrFolder();
    void showRenameDialog();
    
    // --- Autocompletion features ---
    void onTextChanged();
    void onTreeContextMenu(const QPoint &pos);
    void insertCompletion(const QString &completion);
    void showAutoCompletePopup(const QString &prefix);

private:
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void setupUI();
    void setupSearchThread();
    bool loadFile(const QString& filePath, bool addToHistory = true);
    void saveSettings();
    void loadSettings();
    bool maybeSave();
    
    // --- Autocompletion Helpers ---
    void setupAutoCompletion();
    void updateAutoCompletionModel();
    QStringList getMarkdownFileNames();
    
    // --- Other Helpers ---
    QString findFileInVault(const QString &fileName);

    // UI Widgets
    QSplitter *m_mainSplitter;
    QTreeView *m_treeView;
    MarkdownTextEdit *m_textEdit;
    QToolButton *m_toggleButton;
    QLineEdit *m_searchBar;
    QStatusBar *m_statusBar;
    QLabel *m_filePathLabel;
    QLabel *m_wordCountLabel;
    QToolButton *m_historyBackButton;
    QToolButton *m_historyForwardButton;

    // Models and state
    QFileSystemModel *m_fileSystemModel;
    QSortFilterProxyModel *m_proxyModel;
    MarkdownHighlighter *m_highlighter;

    QString m_currentFilePath;
    QString m_currentVaultPath;
    bool m_isEditMode;
    QString m_rawMarkdownBuffer;

    // Search thread and debounce timer
    QThread* m_searchThread;
    SearchWorker* m_searchWorker;
    QTimer* m_searchTimer;
    QString m_lastSearchTerm;

    // History
    QList<QString> m_fileHistory;
    int m_historyIndex;

    // Dialogs
    FindReplaceDialog *m_findReplaceDialog;

    // Theming & Styling
    std::unique_ptr<EditorStyler> m_styler;
    QString m_currentThemeName;
    
    // Default file for vault
    QString m_defaultVaultFile;
    
    // Autocompletion
    QStringList m_markdownFiles;
    QCompleter *m_completer;
};