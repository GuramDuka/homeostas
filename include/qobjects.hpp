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
#ifndef CONNECTIONS_HPP_INCLUDED
#define CONNECTIONS_HPP_INCLUDED
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include <QObject>
#include <QDateTime>
#include <QVariant>
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
class QDirectoryTracker : public QObject {
		Q_OBJECT
        Q_PROPERTY(QString author READ author WRITE setAuthor NOTIFY authorChanged)
        Q_PROPERTY(QDateTime creationDate READ creationDate WRITE setCreationDate NOTIFY creationDateChanged)
    public:
        explicit QDirectoryTracker(QObject *parent = 0);

        QString author() const {
            return author_;
        }

        void setAuthor(const QString &author) {
            author_ = author;
            authorChanged();
        }

        QDateTime creationDate() const {
            return creationDate_;
        }

        void setCreationDate(const QDateTime &creationDate) {
            creationDate_ = creationDate;
            creationDateChanged();
        }

    signals:
        void authorChanged();
        void creationDateChanged();

    public slots:
        void doSend(int count);
    private:
        QString author_;
        QDateTime creationDate_;
};
//------------------------------------------------------------------------------
#endif // CONNECTIONS_HPP_INCLUDED
//------------------------------------------------------------------------------
