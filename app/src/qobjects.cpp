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
#if QT_CORE_LIB
#   include <QString>
#   include <QtDebug>
#endif
#include <iterator>
//------------------------------------------------------------------------------
#include "std_ext.hpp"
#include "cdc512.hpp"
#include "port.hpp"
#include "configuration.hpp"
#include "discoverer.hpp"
#include "qobjects.hpp"
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
std::key512 Homeostas::newUniqueKey()
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

    auto sz = entropy.size();

    for(;;) {
        homeostas::cdc512 ctx;
        ctx.generate_entropy(&entropy);

        auto s = std::to_string(ctx);
        auto p = std::begin(s) + s.rfind('-') + 1;

        if( std::distance(p, std::end(s)) >= 9 )
            return ctx;

        entropy.resize(sz);
    }
}
//------------------------------------------------------------------------------
void Homeostas::startServer()
{
    if( server_ != nullptr )
        return;

    at_scope_exit( homeostas::configuration::instance()->detach_db() );

    server_ = std::make_unique<homeostas::server>();

    server_->host_public_key(
        homeostas::configuration::instance()->get("host.public_key"));

    server_->host_private_key(
        homeostas::configuration::instance()->get("host.private_key"));

    server_->modules().resize(1);
    server_->modules().insert(
        server_->modules().begin() + homeostas::ServerModuleRDT,
        homeostas::remote_directory_tracker::server_module);

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
void Homeostas::loadTrackers()
{
    at_scope_exit( homeostas::configuration::instance()->detach_db() );
    auto config = homeostas::configuration::instance();

    if( !config->exists("host.private_key") ) {
        auto key = Homeostas::newUniqueKey();
        config->set("host.private_key", key);
    }

    if( !config->exists("host.public_key") ) {
        auto key = Homeostas::newUniqueKey();
        config->set("host.public_key", key);
        at_scope_exit( homeostas::discoverer::instance()->detach_db() );
        auto local_p2p_key = Homeostas::newUniqueKey();
        homeostas::discoverer::instance()->announce_host(key, nullptr, &local_p2p_key);
    }

    std::cerr << "private key: "
        << config->get("host.private_key") << std::endl;
    std::cerr << "public key : "
        << config->get("host.public_key") << std::endl;
#if QT_CORE_LIB
    qDebug().noquote().nospace() << "private key: "
        << QString::fromStdString(config->get("host.private_key"));
    qDebug().noquote().nospace() << "public key : "
        << QString::fromStdString(config->get("host.public_key"));
#endif

#ifndef NDEBUG
    if( !config->exists("tracker.hiew") ) {
        auto key = Homeostas::newUniqueKey();
        config->set("tracker.hiew", key);
        config->set("tracker.hiew.name", "hiew");
        config->set("tracker.hiew.path",
#if __ANDROID_API__
            "/storage/1C72-0F0C/hiew"
#elif __linux__
            "/hiew"
#elif _WIN32
            "c:\\hiew"
#endif
        );
    }

    if( !config->exists("tracker.remote_hiew") ) {
        config->set("tracker.remote_hiew", config->get("tracker.hiew"));
        config->set("tracker.remote_hiew.name", "remote_hiew");
        config->set("tracker.remote_hiew.path", homeostas::home_path(false) + ".homeostas" + homeostas::path_delimiter + "remote_hiew");
        config->set("tracker.remote_hiew.remote", config->get("host.public_key"));
    }
#endif

    for( const auto & i : config->get_tree("tracker") ) {
        const auto & tracker = i.second;
        const auto & key = tracker.value();
        const auto & name = tracker["name"].value();
        const auto & path = tracker["path"].value();

        typedef homeostas::directory_tracker base;
        std::unique_ptr<base> p;

        if( tracker.exists("remote") ) {
            p.reset(new homeostas::remote_directory_tracker(
                key, name, path, tracker["remote"].value()));
        }
        else {
            p.reset(new base(key, name, path));
        }

        trackers_.emplace_back(std::shared_ptr<base>(std::move(p)));
    }
}
//------------------------------------------------------------------------------
void Homeostas::startTrackers()
{
    loadTrackers();

    for( const auto & t : trackers_ )
        t->startup();
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
    return int(Homeostas::trackers().size());
}
//------------------------------------------------------------------------------
QVariant DirectoriesTrackersModel::data(const QModelIndex &index, int role) const
{
    auto row = index.row();

    if( row < rowCount() ) {
        const auto & ifs = Homeostas::trackers_ifs();
        const auto & tracker = ifs.at(row);

        switch( role ) {
            case DirectoryUserDefinedNameRole :
                return QString::fromStdString(tracker->dir_user_defined_name());
            case DirectoryPathNameRole :
                return QString::fromStdString(tracker->dir_path_name());
            case DirectoryKey :
                return QString::fromStdString(std::to_string36(tracker->key().hash()) + ", " + std::to_string(tracker->key()));
            case DirectoryRemote :
                return QString::fromStdString(std::to_string36(tracker->remote().hash()) + ", " + std::to_string(tracker->remote()));
            case DirectoryIsRemote :
                return tracker->is_remote();
        }
    }

    return QVariant();
}
//------------------------------------------------------------------------------
QHash<int, QByteArray> DirectoriesTrackersModel::roleNames() const
{
    return {
        { DirectoryUserDefinedNameRole, "name"      },
        { DirectoryPathNameRole       , "path"      },
        { DirectoryKey                , "key"       },
        { DirectoryRemote             , "remote"    },
        { DirectoryIsRemote           , "is_remote" }
    };
}
//------------------------------------------------------------------------------
QVariantMap DirectoriesTrackersModel::get(int row) const
{
    //const auto tracker = Homeostas::trackers_ifs().at(row);
    const auto i = index(row);
    return {
        { "name"     , data(i, DirectoryUserDefinedNameRole) },
        { "path"     , data(i, DirectoryPathNameRole)        },
        { "key"      , data(i, DirectoryKey)                 },
        { "remote"   , data(i, DirectoryRemote)              },
        { "is_remote", data(i, DirectoryIsRemote)            }
    };
}
//------------------------------------------------------------------------------
void DirectoriesTrackersModel::load()
{
    at_scope_exit( homeostas::configuration::instance()->detach_db() );

    auto config = homeostas::configuration::instance();
    auto & ifs = Homeostas::trackers_ifs();

    for( const auto & i : config->get_tree("tracker") ) {
        const auto & tracker = i.second;
        const auto key = tracker.value().get<std::key512>();
        const auto name = tracker["name"].value().get<QString>();

        int row = 0;

        while( row < int(ifs.size())
               && name > QString::fromStdString(ifs.at(row)->dir_user_defined_name()) )
            row++;

        beginInsertRows(QModelIndex(), row, row);

        const auto path = tracker["path"].value().get<QString>();

        typedef homeostas::directory_tracker_interface base;
        std::unique_ptr<base> p;

        if( tracker.exists("remote") ) {
            p.reset(new homeostas::remote_directory_tracker_interface(
                key, name, path, tracker["remote"].value()));
        }
        else {
            p.reset(new base(key, name, path));
        }

        ifs.insert(ifs.begin() + row, std::shared_ptr<base>(std::move(p)));

        endInsertRows();
    }
}
//------------------------------------------------------------------------------
void DirectoriesTrackersModel::append_remote(const QString &directoryUserDefinedName, const QString &/*directoryPathName*/)
{
    auto & ifs = Homeostas::trackers_ifs();
    int row = 0;

    while( row < int(ifs.size())
           && directoryUserDefinedName > QString::fromStdString(ifs.at(row)->dir_user_defined_name()) )
        row++;

    beginInsertRows(QModelIndex(), row, row);

    //ifs.insert(ifs.begin() + row,
    //    std::make_shared<homeostas::directory_tracker_interface>(
    //        directoryUserDefinedName.toStdString(),
    //        directoryPathName.toStdString()));

    endInsertRows();
}
//------------------------------------------------------------------------------
void DirectoriesTrackersModel::append(const QString &directoryUserDefinedName, const QString &directoryPathName)
{
    auto & ifs = Homeostas::trackers_ifs();
    int row = 0;

    while( row < int(ifs.size())
           && directoryUserDefinedName > QString::fromStdString(ifs.at(row)->dir_user_defined_name()) )
        row++;

    beginInsertRows(QModelIndex(), row, row);

    ifs.insert(ifs.begin() + row,
        std::make_shared<homeostas::directory_tracker_interface>(
            Homeostas::newUniqueKey(),
            directoryUserDefinedName.toStdString(),
            directoryPathName.toStdString()));

    endInsertRows();
}
//------------------------------------------------------------------------------
void DirectoriesTrackersModel::set(int row, const QString &directoryUserDefinedName, const QString &directoryPathName)
{
    auto & ifs = Homeostas::trackers_ifs();

    if( row < 0 || row >= int(ifs.size()) )
        return;

    ifs[row]->dir_user_defined_name(directoryUserDefinedName.toStdString());
    ifs[row]->dir_path_name(directoryPathName.toStdString());

    dataChanged(index(row, 0), index(row, 0), {
        DirectoryUserDefinedNameRole,
        DirectoryPathNameRole
    });
}
//------------------------------------------------------------------------------
void DirectoriesTrackersModel::remove(int row)
{
    auto & ifs = Homeostas::trackers_ifs();

    if( row < 0 || row >= int(ifs.size()) )
        return;

    beginRemoveRows(QModelIndex(), row, row);
    ifs.erase(ifs.begin() + row);
    endRemoveRows();
}
//------------------------------------------------------------------------------
