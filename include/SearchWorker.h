#pragma once

#include <QObject>
#include <QStringList>

class SearchWorker : public QObject
{
    Q_OBJECT

public:
    explicit SearchWorker(QObject *parent = nullptr);

public slots:
    // Realiza la búsqueda de `searchTerm` en todos los archivos .md dentro de `vaultPath`
    void doSearch(const QString &searchTerm, const QString &vaultPath);

signals:
    // Emitida cuando la búsqueda ha terminado, con la lista de archivos que coinciden
    void searchFinished(const QStringList &matchingFiles);
};
