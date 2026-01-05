#pragma once

#include <QMainWindow>
#include <QModelIndex>

// Forward declarations para reducir tiempos de compilación y dependencias en cabeceras.
class QSplitter;
class QTreeView;
class QTextEdit;
class QFileSystemModel;
class QToolButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Slot para manejar la acción de "Abrir Vault".
    void openVault();

    // Slot para cambiar entre el modo de edición y el de solo lectura (vista previa).
    void toggleEditMode();

    // Slot para cargar el contenido de un archivo cuando se hace clic en él en el QTreeView.
    void onFileClicked(const QModelIndex &index);

private:
    // --- Funciones de configuración ---
    void setupUI();
    void setupToolbar();
    void loadFile(const QString& filePath);

    // --- Widgets de la UI ---
    QSplitter *m_mainSplitter;
    QTreeView *m_treeView;
    QTextEdit *m_textEdit;
    QToolButton *m_toggleButton;

    // --- Modelos y estado ---
    QFileSystemModel *m_fileSystemModel;
    QString m_currentFilePath;
    bool m_isEditMode;
};
