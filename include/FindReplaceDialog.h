#pragma once

#include <QDialog>
#include <QTextDocument> // Necesario para QTextDocument::FindFlags

class QLineEdit;
class QCheckBox;
class QPushButton;

class FindReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindReplaceDialog(QWidget *parent = nullptr);

signals:
    // Señal para buscar texto
    void findNext(const QString &text, QTextDocument::FindFlags flags);
    // Señal para reemplazar texto
    void replace(const QString &findText, const QString &replaceText, QTextDocument::FindFlags flags);
    // Señal para reemplazar todo
    void replaceAll(const QString &findText, const QString &replaceText, QTextDocument::FindFlags flags);

private slots:
    void findClicked();
    void replaceClicked();
    void replaceAllClicked();

private:
    void setupUi();

    QLineEdit *m_findLineEdit;
    QLineEdit *m_replaceLineEdit;
    QCheckBox *m_caseSensitiveCheckBox;
    QCheckBox *m_wholeWordsCheckBox;
    QPushButton *m_findButton;
    QPushButton *m_replaceButton;
    QPushButton *m_replaceAllButton;
};
