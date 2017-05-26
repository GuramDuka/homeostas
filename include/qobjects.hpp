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
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class HomeostasConfiguration : public QObject {
        Q_OBJECT
    public:
        explicit HomeostasConfiguration(QObject *parent = nullptr) : QObject(parent) {
            Q_ASSERT( singleton_ == nullptr );
            singleton_ = this;
            connect_db();
        }

        static HomeostasConfiguration & singleton() {
            return *singleton_;
        }

        void setVar(const char * varName, const QVariant & val);

        void setVar(const char * varName, const char * val) {
            setVar(varName, QVariant(QString::fromUtf8(val)));
        }

        template <typename T>
        void setVar(const std::string & varName, const T & val) {
            setVar(varName.c_str(), QVariant::fromValue(val));
        }

        template <typename T>
        void setVar(const QString & varName, const T & val) {
            setVar(varName.toStdString().c_str(), QVariant::fromValue(val));
        }

        QVariant getVar(const char * varName, const QVariant * p_defVal = nullptr);

        QVariant getVar(const std::string & varName, const QVariant * p_defVal = nullptr) {
            return getVar(varName.c_str(), p_defVal);
        }

        QVariant getVar(const QString & varName, const QVariant * p_defVal = nullptr) {
            return getVar(varName.toStdString().c_str(), p_defVal);
        }

        QVariant getVar(const char * varName, const QVariant & defVal) {
            return getVar(varName, &defVal);
        }

        QVariant getVar(const std::string & varName, const QVariant & defVal) {
            return getVar(varName.c_str(), &defVal);
        }

        struct QStringHash {
            size_t operator () (const QString & val) const {
                size_t h = 0;

                for( const auto & c : val ) {
                    h += c.unicode();
                    h += (h << 9);
                    h ^= (h >> 5);
                }

                h += (h << 3);
                h ^= (h >> 10);
                h += (h << 14);

                return h;
           }
        };

        struct QStringEqual {
           bool operator () (const QString & val1, const QString & val2) const {
              return val1.compare(val2, Qt::CaseSensitive) == 0;
           }
        };

        struct VarNode {
            uint64_t id;
            QString name;
            QVariant value;
            std::unique_ptr<std::unordered_map<QString, VarNode, QStringHash, QStringEqual>> childs;

            VarNode() {}

            VarNode(uint64_t v_id, const QString & v_name, const QVariant & v_value) :
                id(v_id),
                name(v_name),
                value(v_value) {}

            auto & operator [] (const char * name) {
                return childs->at(QString::fromUtf8(name));
            }

            const auto & operator [] (const char * name) const {
                return childs->at(QString::fromUtf8(name));
            }

            auto & operator [] (const std::string & name) {
                return childs->at(QString::fromStdString(name));
            }

            const auto & operator [] (const std::string & name) const {
                return childs->at(QString::fromStdString(name));
            }

            auto empty() const {
                return childs->empty();
            }

            auto begin() {
                return childs->begin();
            }

            auto end() {
                return childs->end();
            }
        };

        VarNode getVarTree(const char * varName);

        VarNode getVarTree(const std::string & varName) {
            return getVarTree(varName.c_str());
        }

        VarNode getVarTree(const QString & varName) {
            return getVarTree(varName.toStdString().c_str());
        }

    signals:
    public slots:
        QVariant getVar(const QString & varName, const QVariant & defVal) {
            return getVar(varName.toStdString().c_str(), &defVal);
        }

        void setVar(const QString & varName, const QVariant & val) {
            setVar(varName.toStdString().c_str(), val);
        }
    private:
        static HomeostasConfiguration * singleton_;

        uint64_t lfsr_counter_;
        std::unique_ptr<sqlite3pp::database> db_;
        std::unique_ptr<sqlite3pp::query> st_sel_;
        std::unique_ptr<sqlite3pp::query> st_sel_by_pid_;
        std::unique_ptr<sqlite3pp::command> st_ins_;
        std::unique_ptr<sqlite3pp::command> st_upd_;

        void connect_db();

        QVariant getVar(sqlite3pp::query::iterator & i, const QVariant * p_defVal = nullptr, uint64_t * p_id = nullptr);
        QVariant getVar(uint64_t pid, const char * varName, const QVariant * p_defVal = nullptr, uint64_t * p_id = nullptr);
        uint64_t getVarParentId(const char * varName, const char ** p_name = nullptr, bool * p_pid_valid = nullptr);
        void setVar(uint64_t pid, const char * varName, const QVariant & val);
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class Homeostas : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString uniqueId READ uniqueId WRITE setUniqueId)
    public:
        explicit Homeostas(QObject *parent = nullptr) : QObject(parent) {
            Q_ASSERT( singleton_ == nullptr );
            singleton_ = this;
        }

        static auto & singleton() {
            return *singleton_;
        }

        static auto & trackers() {
            return singleton_->trackers_;
        }

        QString uniqueId() const {
            return uniqueId_;
        }

        void setUniqueId(const QString & uniqueId) {
            uniqueId_ = uniqueId;
        }
    signals:
    public slots:
        QString newUniqueId();

        void startServer();
        void stopServer();

        void startTrackers();
        void stopTrackers();
    private:
        static Homeostas * singleton_;

        QString uniqueId_;
        std::vector<std::shared_ptr<homeostas::directory_tracker>> trackers_;
        std::unique_ptr<homeostas::server> server_;
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class DirectoryTracker : public QObject {
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
        explicit DirectoryTracker(QObject *parent = nullptr) : QObject(parent) {

        }

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
            return QString::fromStdString(dt_ == nullptr ? std::string() : dt_->dir_user_defined_name());
        }

        QString directoryPathName() const {
            return QString::fromStdString(dt_ == nullptr ? std::string() : dt_->dir_path_name());
        }

    signals:
        //void authorChanged();
        //void creationDateChanged();

    public slots:
        //void doSend(int count);
        void startTracker();
        void stopTracker();
    private:
        mutable std::shared_ptr<homeostas::directory_tracker> dt_;

        //QString author_;
        //QDateTime creationDate_;
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class DirectoriesTrackersModel : public QAbstractListModel {
        Q_OBJECT
    public:
        enum DirectoryTrackerRole {
            DirectoryUserDefinedNameRole = Qt::DisplayRole,
            DirectoryPathNameRole = Qt::UserRole
        };
        Q_ENUM(DirectoryTrackerRole)

        DirectoriesTrackersModel(QObject *parent = nullptr) :
            QAbstractListModel(parent),
            trackers_(Homeostas::trackers())
        {
            Q_ASSERT( singleton_ == nullptr );
            singleton_ = this;
        }

        static auto & singleton() {
            return *singleton_;
        }

        int rowCount(const QModelIndex & = QModelIndex()) const;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
        QHash<int, QByteArray> roleNames() const;

        Q_INVOKABLE QVariantMap get(int row) const;
        Q_INVOKABLE void append(const QString &directoryUserDefinedName, const QString &directoryPathName);
        Q_INVOKABLE void set(int row, const QString &directoryUserDefinedName, const QString &directoryPathName);
        Q_INVOKABLE void remove(int row);

    private:
        static DirectoriesTrackersModel * singleton_;
        //QList<QSharedPointer<DirectoryTracker>> trackers_;
        std::vector<std::shared_ptr<homeostas::directory_tracker>> & trackers_;
};
//------------------------------------------------------------------------------
#endif // QOBJECTS_HPP_INCLUDED
//------------------------------------------------------------------------------
