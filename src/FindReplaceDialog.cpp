#include "FindReplaceDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>

FindReplaceDialog::FindReplaceDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Buscar y Reemplazar");
    setupUi();
    setModal(false); // No modal para permitir interacción con el editor
}

void FindReplaceDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Buscar
    QHBoxLayout *findLayout = new QHBoxLayout();
    findLayout->addWidget(new QLabel("Buscar:"));
    m_findLineEdit = new QLineEdit(this);
    findLayout->addWidget(m_findLineEdit);
    mainLayout->addLayout(findLayout);

    // Reemplazar
    QHBoxLayout *replaceLayout = new QHBoxLayout();
    replaceLayout->addWidget(new QLabel("Reemplazar con:"));
    m_replaceLineEdit = new QLineEdit(this);
    replaceLayout->addWidget(m_replaceLineEdit);
    mainLayout->addLayout(replaceLayout);

    // Opciones
    m_caseSensitiveCheckBox = new QCheckBox("Distinguir mayúsculas/minúsculas", this);
    m_wholeWordsCheckBox = new QCheckBox("Palabras completas", this);
    mainLayout->addWidget(m_caseSensitiveCheckBox);
    mainLayout->addWidget(m_wholeWordsCheckBox);

    // Botones
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_findButton = new QPushButton("Buscar Siguiente", this);
    m_replaceButton = new QPushButton("Reemplazar", this);
    m_replaceAllButton = new QPushButton("Reemplazar Todo", this);

    buttonLayout->addWidget(m_findButton);
    buttonLayout->addWidget(m_replaceButton);
    buttonLayout->addWidget(m_replaceAllButton);
    mainLayout->addLayout(buttonLayout);

    // Conexiones
    connect(m_findButton, &QPushButton::clicked, this, &FindReplaceDialog::findClicked);
    connect(m_replaceButton, &QPushButton::clicked, this, &FindReplaceDialog::replaceClicked);
    connect(m_replaceAllButton, &QPushButton::clicked, this, &FindReplaceDialog::replaceAllClicked);

    m_findLineEdit->setFocus();
}

void FindReplaceDialog::findClicked()
{
    QTextDocument::FindFlags flags;
    if (m_caseSensitiveCheckBox->isChecked()) {
        flags |= QTextDocument::FindCaseSensitively;
    }
    if (m_wholeWordsCheckBox->isChecked()) {
        flags |= QTextDocument::FindWholeWords;
    }
    emit findNext(m_findLineEdit->text(), flags);
}

void FindReplaceDialog::replaceClicked()
{
    QTextDocument::FindFlags flags;
    if (m_caseSensitiveCheckBox->isChecked()) {
        flags |= QTextDocument::FindCaseSensitively;
    }
    if (m_wholeWordsCheckBox->isChecked()) {
        flags |= QTextDocument::FindWholeWords;
    }
    emit replace(m_findLineEdit->text(), m_replaceLineEdit->text(), flags);
}

void FindReplaceDialog::replaceAllClicked()
{
    QTextDocument::FindFlags flags;
    if (m_caseSensitiveCheckBox->isChecked()) {
        flags |= QTextDocument::FindCaseSensitively;
    }
    if (m_wholeWordsCheckBox->isChecked()) {
        flags |= QTextDocument::FindWholeWords;
    }
    emit replaceAll(m_findLineEdit->text(), m_replaceLineEdit->text(), flags);
}
