#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include <QObject>

class connections : public QObject
{
        Q_OBJECT
    public:
        explicit connections(QObject *parent = 0);

    signals:

    public slots:
};

#endif // CONNECTIONS_H