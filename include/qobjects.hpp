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
//------------------------------------------------------------------------------
#include "tracker.hpp"
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class QHomeostas : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString uniqueId READ uniqueId WRITE setUniqueId)
    public:
        explicit QHomeostas(QObject *parent = 0) : QObject(parent) {}

        QString uniqueId() const {
            return uniqueId_;
        }

        void setUniqueId(const QString & uniqueId) {
            uniqueId_ = uniqueId;
        }
    signals:
    public slots:
        QVariant getVar(const QString & varName);
        void setVar(const QString & varName, const QVariant & val);
        QString newUniqueId();
    private:
        std::unique_ptr<sqlite3pp::database> db_;
        QString uniqueId_;
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class QDirectoryTracker : public QObject {
		Q_OBJECT
        //Q_PROPERTY(QString author READ author WRITE setAuthor NOTIFY authorChanged)
        //Q_PROPERTY(QDateTime creationDate READ creationDate WRITE setCreationDate NOTIFY creationDateChanged)
        Q_PROPERTY(
            QString directoryUserDefinedName
            READ directoryUserDefinedName
            WRITE setDirectoryUserDefinedName)
        Q_PROPERTY(
            QString directoryPathName
            READ directoryPathName
            WRITE setDirectoryPathName)
    public:
        explicit QDirectoryTracker(QObject *parent = 0) : QObject(parent) {}

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
            return QString::fromStdString(dt_.dir_user_defined_name());
        }

        void setDirectoryUserDefinedName(const QString & directoryUserDefinedName) {
            auto started = dt_.started();

            dt_.shutdown();
            dt_.dir_user_defined_name(directoryUserDefinedName.toStdString());

            if( started )
                dt_.startup();
        }

        QString directoryPathName() const {
            return QString::fromStdString(dt_.dir_path_name());
        }

        void setDirectoryPathName(const QString & directoryPathName) {
            auto started = dt_.started();

            dt_.shutdown();
            dt_.dir_path_name(directoryPathName.toStdString());

            if( started )
                dt_.startup();
        }

    signals:
        //void authorChanged();
        //void creationDateChanged();

    public slots:
        //void doSend(int count);
        void startTracker();
        void stopTracker();
    private:
        homeostas::directory_tracker dt_;

        QString author_;
        QDateTime creationDate_;
};
//------------------------------------------------------------------------------
#endif // QOBJECTS_HPP_INCLUDED
//------------------------------------------------------------------------------
