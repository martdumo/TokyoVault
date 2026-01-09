#pragma once

#include <QMainWindow>
#include <QModelIndex>
#include <QTextDocument>
#include <QColor>
#include <QMap>
#include <QUrl>
#include <QStringListModel>
#include <QCompleter>
#include <QDirIterator>

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

struct Theme {
    QString name;
    QColor windowBg, editorBg, textFg, mutedFg, accent;
    QColor heading, link, code, bold, italic, quoteBg;
};

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
    void applyTheme(const QString &themeName);
    void showHelpDialog();

    // --- Search ---
    void filterVault(); // Slot connected to search bar text changes, starts the debounce timer
    void startContentSearch(); // Slot connected to timer, executes the search
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

    // --- Formatting ---
    void insertTableTemplate();
    void insertLinkTemplate();
    void insertImageTemplate();
    void applyBold();
    void applyItalic();
    void applyUnderline();

    // --- New features ---
    void createFolder();
    void setDefaultFile();
    void updateLinksAfterRename(const QString &oldName, const QString &newName);
    
    // --- Autocompletion features ---
    void setupAutoCompletion();
    void insertCompletion(const QString &completion);
    QStringList getMarkdownFileNames();
    void showAutoCompletePopup(const QString &prefix);
    void updateAutoCompletionModel();
    void onTextChanged();
    void onTreeContextMenu(const QPoint &pos);
    void showRenameDialog();

private:
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void setupUI();
    void setupThemes();
    void setupSearchThread();
    bool loadFile(const QString& filePath, bool addToHistory = true);
    void saveSettings();
    void loadSettings();
    void applyTextFormatting(const QString& prefix, const QString& suffix = "");
    bool maybeSave();
    
    // New helper methods
    QString findFileInVault(const QString &fileName);
    void updateAllLinksInVault(const QString &oldLink, const QString &newLink);
    QStringList getAllMarkdownFilesInVault();

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

    // Theming
    QMap<QString, Theme> m_themes;
    QString m_currentThemeName;
    
    // Default file for vault
    QString m_defaultVaultFile;
    
    // Autocompletion
    QStringList m_markdownFiles;
    QCompleter *m_completer;
};