#pragma once

#include <QMainWindow>
#include <QModelIndex>
#include <QTextDocument>
#include <QColor>
#include <QMap>

// Forward declarations to reduce compile times and header dependencies.
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

// New classes
class MarkdownTextEdit;
class MarkdownHighlighter;
class FindReplaceDialog;

// Structure to define the colors of a theme
struct Theme {
    QString name;
    // General UI colors
    QColor windowBg;
    QColor editorBg;
    QColor textFg;
    QColor mutedFg;
    QColor accent;
    // Highlighter colors
    QColor heading;
    QColor link;
    QColor code;
    QColor bold;
    QColor italic;
    QColor quoteBg;
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override; // To save settings on close

private slots:
    // Toolbar slots
    void openVault(const QString &path = "");
    void toggleEditMode();
    void historyBack();     // Navigate back in history
    void historyForward();  // Navigate forward in history

    // File menu slots
    void openFile();
    void saveFile();
    void closeVault();

    // Edit menu slots
    void showFindReplaceDialog();
    void findNextInEditor(const QString &text, QTextDocument::FindFlags flags);
    void replaceInEditor(const QString &findText, const QString &replaceText, QTextDocument::FindFlags flags);
    void replaceAllInEditor(const QString &findText, const QString &replaceText, QTextDocument::FindFlags flags);

    // Insert menu slots
    void insertTableTemplate();
    void insertLinkTemplate();
    void insertImageTemplate();

    // Format slots (Shortcuts)
    void applyBold();
    void applyItalic();
    void applyUnderline();

    // Customization slots
    void showFontDialog();
    void applyTheme(const QString &themeName);


    // QTreeView and Editor slots
    void onFileClicked(const QModelIndex &index);
    void updateStatusBar(); // Updates word count and path.
    void filterVault(const QString &text); // For the search QLineEdit
    void handleWikiLinkActivated(const QString &linkName); // Manages Ctrl+Click on wiki-links
    void onDocumentModified(); // Activates when text changes

private:
    // --- Configuration functions ---
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void setupUI();
    void setupThemes(); // Loads theme definitions
    bool loadFile(const QString& filePath, bool addToHistory = true);
    void saveSettings(); // Saves configuration (last vault, font)
    void loadSettings(); // Loads configuration
    void applyTextFormatting(const QString& prefix, const QString& suffix = "");
    bool maybeSave(); // Asks the user if they want to save changes

    // --- UI Widgets ---
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

    // --- Models and state ---
    QFileSystemModel *m_fileSystemModel;
    QSortFilterProxyModel *m_proxyModel;
    MarkdownHighlighter *m_highlighter;

    QString m_currentFilePath;
    QString m_currentVaultPath;
    bool m_isEditMode;

    // History of open files
    QList<QString> m_fileHistory;
    int m_historyIndex;

    // Find and Replace Dialog
    FindReplaceDialog *m_findReplaceDialog;

    // Theming System
    QMap<QString, Theme> m_themes;
    QString m_currentThemeName;
};