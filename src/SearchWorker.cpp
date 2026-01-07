#include "SearchWorker.h"
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QDebug>

SearchWorker::SearchWorker(QObject *parent) : QObject(parent)
{
}

void SearchWorker::doSearch(const QString &searchTerm, const QString &vaultPath)
{
    QStringList matchingFiles;
    if (searchTerm.isEmpty() || vaultPath.isEmpty()) {
        emit searchFinished(matchingFiles);
        return;
    }

    QDirIterator it(vaultPath, QStringList() << "*.md", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            // Leer el archivo en trozos para no consumir demasiada memoria con archivos grandes
            while (!in.atEnd()) {
                QString line = in.readLine();
                if (line.contains(searchTerm, Qt::CaseInsensitive)) {
                    matchingFiles.append(filePath);
                    break; // Encontramos una coincidencia, pasamos al siguiente archivo
                }
            }
            file.close();
        }
    }
    emit searchFinished(matchingFiles);
}
