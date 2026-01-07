#include "SearchWorker.h"
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
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

        // 1. Check if filename contains the search term
        if (QFileInfo(filePath).fileName().contains(searchTerm, Qt::CaseInsensitive)) {
            matchingFiles.append(filePath);
            continue; // Found in title, move to next file
        }

        // 2. If not in title, check content
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine();
                if (line.contains(searchTerm, Qt::CaseInsensitive)) {
                    matchingFiles.append(filePath);
                    break; // Found in content, move to next file
                }
            }
            file.close();
        }
    }
    emit searchFinished(matchingFiles);
}