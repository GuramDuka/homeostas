/*-
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Guram Duka
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
//------------------------------------------------------------------------------
#ifndef QOBJECTS_HPP_INCLUDED
#define QOBJECTS_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVariant>
#include <QNetworkInterface>
#include <QSharedPointer>
#include <QAbstractListModel>
//------------------------------------------------------------------------------
#include "tracker.hpp"
#include "server.hpp"
#include "client.hpp"
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class Homeostas : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString uniqueId READ uniqueId WRITE setUniqueId)
public:
    explicit Homeostas(QObject * parent = nullptr) : QObject(parent) {}

    static auto instance() {
        static Homeostas singleton;
        return &singleton;
    }

    static auto & trackers() {
        return instance()->trackers_;
    }

    static auto & trackers_ifs() {
        return instance()->trackers_ifs_;
    }

    QString uniqueId() const {
        return uniqueId_;
    }

    void setUniqueId(const QString & uniqueId) {
        uniqueId_ = uniqueId;
    }
signals:
public slots:
    static std::key512 newUniqueKey();

    void startServer();
    void stopServer();

    void loadTrackers();
    void startTrackers();
    void stopTrackers();
private:
    QString uniqueId_;
    std::vector<std::shared_ptr<homeostas::directory_tracker>> trackers_;
    std::vector<std::shared_ptr<homeostas::directory_tracker_interface>> trackers_ifs_;
    std::unique_ptr<homeostas::server> server_;
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
/*class DirectoryTracker : public QObject {
    Q_OBJECT
    //Q_PROPERTY(QString author READ author WRITE setAuthor NOTIFY authorChanged)
    //Q_PROPERTY(QDateTime creationDate READ creationDate WRITE setCreationDate NOTIFY creationDateChanged)
    Q_PROPERTY(
        QString directoryUserDefinedName
        READ directoryUserDefinedName
        )
    Q_PROPERTY(
        QString directoryPathName
        READ directoryPathName
        )
public:
    explicit DirectoryTracker(QObject *parent = nullptr) : QObject(parent) {}

    //QString author() const {
    //    return author_;
    //}

    //void setAuthor(const QString &author) {
    //    author_ = author;
    //    authorChanged();
    //}

    //QDateTime creationDate() const {
    //    return creationDate_;
    //}

    //void setCreationDate(const QDateTime &creationDate) {
    //    creationDate_ = creationDate;
    //    creationDateChanged();
    //}

    QString directoryUserDefinedName() const {
        return QString::fromStdString(
            dt_ ? dt_->dir_user_defined_name() : std::string());
    }

    QString directoryPathName() const {
        return QString::fromStdString(
            dt_ ? dt_->dir_path_name() : std::string());
    }

signals:
    //void authorChanged();
    //void creationDateChanged();

public slots:
    //void doSend(int count);
    void startTracker();
    void stopTracker();
private:
    std::unique_ptr<homeostas::directory_tracker> dt_;

    //QString author_;
    //QDateTime creationDate_;
};*/
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class DirectoriesTrackersModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum DirectoryTrackerRole {
        DirectoryUserDefinedNameRole = Qt::DisplayRole,
        DirectoryKey = Qt::UserRole,
        DirectoryPathNameRole,
        DirectoryRemote,
        DirectoryIsRemote
    };
    Q_ENUM(DirectoryTrackerRole)

    explicit DirectoriesTrackersModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    static auto instance() {
        static DirectoriesTrackersModel singleton;
        return &singleton;
    }

    int rowCount(const QModelIndex & = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int, QByteArray> roleNames() const;

    Q_INVOKABLE void load();
    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE void append(const QString &directoryUserDefinedName, const QString &directoryPathName);
    Q_INVOKABLE void append_remote(const QString &directoryUserDefinedName, const QString &directoryPathName);
    Q_INVOKABLE void set(int row, const QString &directoryUserDefinedName, const QString &directoryPathName);
    Q_INVOKABLE void remove(int row);
private:
};
//------------------------------------------------------------------------------
#endif // QOBJECTS_HPP_INCLUDED
//------------------------------------------------------------------------------
