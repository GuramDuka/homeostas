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
#if QT_CORE_LIB && __ANDROID__
#   include <QString>
#   include <QtDebug>
#endif
//------------------------------------------------------------------------------
#include "locale_traits.hpp"
#include "cdc512.hpp"
#include "port.hpp"
#include "configuration.hpp"
#include "qobjects.hpp"
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
QString Homeostas::newUniqueId()
{
    std::vector<uint8_t> entropy;

    for( auto iface : QNetworkInterface::allInterfaces() ) {
        uint8_t c, q = 0, i = 0;

        for( auto qc : iface.hardwareAddress() ) {
            if( !qc.isLetterOrNumber() )
                continue;

            c = qc.toLatin1();
            q <<= 4;

            if( qc.isDigit() )
                q |= c - '0';
            else if( qc.isLower() )
                q |= c - 'a';
            else
                q |= c - 'A';

            if( ++i == 2 ) {
                entropy.push_back(q);
                q = i = 0;
            }
        }

        if( i )
            entropy.push_back(q);
    }

    homeostas::cdc512 ctx;
    ctx.generate_entropy(&entropy);

    return QString::fromStdString(
        std::blob2string(std::blob(std::begin(ctx), std::end(ctx))));
}
//------------------------------------------------------------------------------
void Homeostas::startServer()
{
    if( server_ != nullptr )
        return;

    server_.reset(new homeostas::server);
    server_->startup();
}
//------------------------------------------------------------------------------
void Homeostas::stopServer()
{
    if( server_ == nullptr )
        return;

    server_ = nullptr;
}
//------------------------------------------------------------------------------
void Homeostas::startTrackers()
{
    auto config = homeostas::configuration::instance();

    if( !config->exists("host.private_id") ) {
        const auto id = Homeostas::instance()->newUniqueId().toStdString();
        config->set("host.private_id", std::string2blob(id));
    }

    if( !config->exists("host.public_id") ) {
        const auto id = Homeostas::instance()->newUniqueId().toStdString();
        config->set("host.public_id", std::string2blob(id));
    }

    std::cerr << "private_id: " << std::blob2string(config->get("host.private_id").get<std::blob>()) << std::endl;
    std::cerr << "public_id : " << std::blob2string(config->get("host.public_id").get<std::blob>()) << std::endl;
#if QT_CORE_LIB && __ANDROID__
    qDebug().noquote() << "private_id: " << QString::fromStdString(std::blob2string(config->get("host.private_id").get<std::blob>()));
    qDebug().noquote() << "public_id : " << QString::fromStdString(std::blob2string(config->get("host.public_id").get<std::blob>()));
#endif

#ifndef NDEBUG
    if( !config->exists("tracker.hiew.path") ) {
        config->set("tracker.hiew", "hiew");
        config->set("tracker.hiew.path",
#if __ANDROID_API__
            "/storage/1C72-0F0C/hiew"
#elif _WIN32
            "c:\\hiew"
#endif
        );
        //std::shared_ptr<homeostas::directory_tracker> dt(new homeostas::directory_tracker);
        //dt->dir_user_defined_name("hiew");
        //dt->dir_path_name("c:\\hiew");
        //trackers_.emplace_back(dt);
    }
#endif

    for( const auto & i : config->get_tree("tracker") ) {
        const auto & tracker = i.second;
        const auto & path = tracker["path"];
        DirectoriesTrackersModel::instance()->append(
           tracker.value().get<QString>(),
           path.value().get<QString>()
        );
    }
}
//------------------------------------------------------------------------------
void Homeostas::stopTrackers()
{
    trackers_.clear();
}
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
//void DirectoryTracker::doSend(int count)
//{
//    qDebug() << "Received in C++ from QML:" << count;
//}
//------------------------------------------------------------------------------
/*void DirectoryTracker::startTracker()
{
    if( dt_ == nullptr )
        return;

    dt_->startup();
}
//------------------------------------------------------------------------------
void DirectoryTracker::stopTracker()
{
    if( dt_ == nullptr )
        return;

    dt_ = nullptr;
}*/
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
int DirectoriesTrackersModel::rowCount(const QModelIndex &) const
{
    return int(Homeostas::instance()->trackers().size());
}
//------------------------------------------------------------------------------
QVariant DirectoriesTrackersModel::data(const QModelIndex &index, int role) const
{
    auto row = index.row();

    if( row < rowCount() ) {
        auto tracker = Homeostas::instance()->trackers().at(row);

        switch( role ) {
            case DirectoryUserDefinedNameRole :
                return QString::fromStdString(tracker->dir_user_defined_name());
            case DirectoryPathNameRole :
                return QString::fromStdString(tracker->dir_path_name());
        }
    }

    return QVariant();
}
//------------------------------------------------------------------------------
QHash<int, QByteArray> DirectoriesTrackersModel::roleNames() const
{
    return {
        { DirectoryUserDefinedNameRole, "name" },
        { DirectoryPathNameRole       , "path" }
    };
}
//------------------------------------------------------------------------------
QVariantMap DirectoriesTrackersModel::get(int row) const
{
    const auto tracker = Homeostas::instance()->trackers().at(row);
    return {
        { "name", QString::fromStdString(tracker->dir_user_defined_name()) },
        { "path", QString::fromStdString(tracker->dir_path_name()) }
    };
}
//------------------------------------------------------------------------------
void DirectoriesTrackersModel::append(const QString &directoryUserDefinedName, const QString &directoryPathName)
{
    auto & tracks = Homeostas::instance()->trackers();
    int row = 0;

    while( row < int(tracks.size())
           && directoryUserDefinedName > QString::fromStdString(tracks.at(row)->dir_user_defined_name()) )
        ++row;

    beginInsertRows(QModelIndex(), row, row);

    auto dt = std::make_shared<homeostas::directory_tracker>(
        directoryUserDefinedName.toStdString(),
        directoryPathName.toStdString());

    tracks.insert(tracks.begin() + row, dt);

    dt->startup();

    endInsertRows();
}
//------------------------------------------------------------------------------
void DirectoriesTrackersModel::set(int row, const QString &directoryUserDefinedName, const QString &directoryPathName)
{
    auto & tracks = Homeostas::instance()->trackers();

    if( row < 0 || row >= int(tracks.size()) )
        return;

    tracks[row] = std::make_shared<homeostas::directory_tracker>(
        directoryUserDefinedName.toStdString(),
        directoryPathName.toStdString()
    );

    dataChanged(index(row, 0), index(row, 0), {
        DirectoryUserDefinedNameRole,
        DirectoryPathNameRole
    });
}
//------------------------------------------------------------------------------
void DirectoriesTrackersModel::remove(int row)
{
    auto & tracks = Homeostas::instance()->trackers();

    if( row < 0 || row >= int(tracks.size()) )
        return;

    beginRemoveRows(QModelIndex(), row, row);
    tracks.erase(tracks.begin() + row);
    endRemoveRows();
}
//------------------------------------------------------------------------------
