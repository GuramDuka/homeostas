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
#include <QtDebug>
//------------------------------------------------------------------------------
#include "locale_traits.hpp"
#include "cdc512.hpp"
#include "port.hpp"
#include "qobjects.hpp"
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
HomeostasConfiguration * HomeostasConfiguration::singleton_ = nullptr;
//------------------------------------------------------------------------------
void HomeostasConfiguration::connect_db()
{
    if( db_ == nullptr ) {
        std::unique_ptr<sqlite3pp::database> db(new sqlite3pp::database);
        sqlite3_enable_shared_cache(1);

        db->connect(
            homeostas::home_path(false)
                + ".homeostas" + homeostas::path_delimiter + "homeostas.sqlite",
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_SHAREDCACHE
        );

        sqlite3pp::command(*db, R"EOS(
            PRAGMA page_size = 4096;
            PRAGMA journal_mode = WAL;
            PRAGMA count_changes = OFF;
            PRAGMA auto_vacuum = NONE;
            PRAGMA cache_size = -2048;
            PRAGMA synchronous = NORMAL;
            /*PRAGMA temp_store = MEMORY;*/
        )EOS").execute_all();

        sqlite3pp::command(*db, R"EOS(
            CREATE TABLE IF NOT EXISTS rowids (
                id              INTEGER PRIMARY KEY ON CONFLICT REPLACE,
                counter    		INTEGER NOT NULL
            ) WITHOUT ROWID;

            CREATE TABLE IF NOT EXISTS config (
                id              INTEGER PRIMARY KEY ON CONFLICT ABORT,
                parent_id		INTEGER NOT NULL,   /* link on entries id */
                name            TEXT NOT NULL,
                value_type		INTEGER,	/* 1 - boolean, 2 - integer, 3 - double, 4 - string */
                value_b			INTEGER,	/* boolean */
                value_i			INTEGER,	/* integer */
                value_n			REAL,       /* double */
                value_s			TEXT,		/* string */
                UNIQUE(parent_id, name) ON CONFLICT ABORT
            ) WITHOUT ROWID;

            CREATE TRIGGER IF NOT EXISTS config_after_insert_trigger
                AFTER INSERT
                ON config
            BEGIN
                REPLACE INTO rowids(id, counter) VALUES (1, new.id);
            END;

            CREATE UNIQUE INDEX IF NOT EXISTS i1 ON config (parent_id, name);
        )EOS").execute_all();

        st_sel_.reset(new sqlite3pp::query(*db, R"EOS(
            SELECT
                id                                              AS id,
                name                                            AS name,
                value_type                                      AS value_type,
                COALESCE(value_b, value_i, value_n, value_s)    AS value
            FROM
                config
            WHERE
                parent_id = :parent_id
                AND name = :name
        )EOS"));

        st_sel_by_pid_.reset(new sqlite3pp::query(*db, R"EOS(
            SELECT
                id                                              AS id,
                name                                            AS name,
                value_type                                      AS value_type,
                COALESCE(value_b, value_i, value_n, value_s)    AS value
            FROM
                config
            WHERE
                parent_id = :parent_id
        )EOS"));

        st_ins_.reset(new sqlite3pp::command(*db, R"EOS(
            INSERT INTO config
                (id, parent_id, name, value_type, value_b, value_i, value_n, value_s)
                VALUES
                (:id, :parent_id, :name, :value_type, :value_b, :value_i, :value_n, :value_s)
        )EOS"));

        st_upd_.reset(new sqlite3pp::command(*db, R"EOS(
            UPDATE config SET
                name        = :name,
                value_type  = :value_type,
                value_b     = :value_b,
                value_i     = :value_i,
                value_n     = :value_n,
                value_s     = :value_s
        )EOS"));

        row_next_id_ = [&] {
            sqlite3pp::query st(*db, R"EOS(
                SELECT
                    counter
                FROM
                    rowids
                WHERE
                    id = :id
            )EOS");

            st.bind("id", 1);

            auto i = st.begin();

            return std::rhash(i ? i->get<uint64_t>(0) : 0) + 1;
        }();

        db_.reset(db.release());
    }
}
//------------------------------------------------------------------------------
QVariant HomeostasConfiguration::getVar(sqlite3pp::query::iterator & i, const QVariant * p_defVal, uint64_t * p_id)
{
    if( i ) {
        int vt = i->column_isnull("value_type") ? 0 : i->get<int>("value_type");

        switch( vt ) {
            default:
            case 0 :
                break;
            case 1 :
                return i->get<int>("value") ? true : false;
            case 2 :
                return i->get<int64_t>("value");
            case 3 :
                return i->get<double>("value");
            case 4 :
                return QString::fromUtf8(i->get<const char *>("value"));
        }

        if( p_id != nullptr )
            *p_id = i->get<uint64_t>("id");
    }
    else if( p_id != nullptr )
        *p_id = 0;

    if( p_defVal != nullptr )
        return *p_defVal;

    return QVariant();
}
//------------------------------------------------------------------------------
QVariant HomeostasConfiguration::getVar(uint64_t pid, const char * varName, const QVariant * p_defVal, uint64_t * p_id)
{
    st_sel_->bind("parent_id", pid);
    st_sel_->bind("name", varName, sqlite3pp::nocopy);
    auto i = st_sel_->begin();
    return getVar(i, p_defVal, p_id);
}
//------------------------------------------------------------------------------
uint64_t HomeostasConfiguration::getVarParentId(const char * varName, const char ** p_name, bool * p_pid_valid)
{
    uint64_t pid = 0;
    bool pid_valid = true;
    const char * p = varName;
    char * n = (char *) alloca((homeostas::slen(varName) + 1) * sizeof(char));

    while( *p != '\0' && pid_valid ) {
        const char * a = p;

        while( *a != '\0' ) {
            if( *a == '.' ) {
                size_t l = (a - p) * sizeof(char);
                memcpy(n, p, l);
                n[l] = '\0';

                st_sel_->bind("parent_id", pid);
                st_sel_->bind("name", (const char *) n, sqlite3pp::nocopy);

                auto i = st_sel_->begin();

                if( i )
                    pid = i->get<uint64_t>("id");
                else
                    pid_valid = false;

                p = a + 1;
                break;
            }

            a++;
        }

        if( *a == '\0' )
            break;
    }

    if( p_name != nullptr )
        *p_name = p;

    if( p_pid_valid != nullptr )
        *p_pid_valid = pid_valid;

    return pid;
}
//------------------------------------------------------------------------------
QVariant HomeostasConfiguration::getVar(const char * varName, const QVariant * p_defVal)
{
    bool pid_valid = true;
    const char * p = nullptr;
    uint64_t pid = getVarParentId(varName, &p, &pid_valid);

    if( pid_valid )
        return getVar(pid, p, p_defVal);

    if( p_defVal != nullptr )
        return *p_defVal;

    return QVariant();
}
//------------------------------------------------------------------------------
void HomeostasConfiguration::setVar(uint64_t pid, const char * varName, const QVariant & val)
{
    auto bindex = [&] (auto & st) {
        st.bind("parent_id", pid);
        st.bind("name", varName, sqlite3pp::nocopy);
        st.bind("value_b", nullptr);
        st.bind("value_i", nullptr);
        st.bind("value_n", nullptr);
        st.bind("value_s", nullptr);

        switch( val.type() ) {
            case QVariant::Type::Bool       :
                st.bind("value_type", 1);
                st.bind("value_b", val.toBool());
                break;
            case QVariant::Type::Double     :
                st.bind("value_type", 3);
                st.bind("value_n", val.toDouble());
                break;
            case QVariant::Type::Int        :
                st.bind("value_type", 2);
                st.bind("value_i", val.toInt());
                break;
            case QVariant::Type::UInt       :
                st.bind("value_type", 2);
                st.bind("value_i", val.toUInt());
                break;
            case QVariant::Type::LongLong   :
                st.bind("value_type", 2);
                st.bind("value_i", val.toLongLong());
                break;
            case QVariant::Type::ULongLong  :
                st.bind("value_type", 2);
                st.bind("value_i", val.toULongLong());
                break;
            case QVariant::Type::String     :
            default                         :
                st.bind("value_type", 4);
                st.bind("value_s", val.toString().toStdString().c_str(), sqlite3pp::copy);
                break;
        }

        st.execute();
    };

    st_sel_->bind("parent_id", pid);
    st_sel_->bind("name", varName, sqlite3pp::nocopy);
    auto i = st_sel_->begin();

    if( i ) {
        bindex(*st_upd_);
    }
    else {
        st_ins_->bind("id", std::rhash(row_next_id_));
        bindex(*st_ins_);
        row_next_id_++;
    }
}
//------------------------------------------------------------------------------
void HomeostasConfiguration::setVar(const char * varName, const QVariant & val)
{
    uint64_t pid = 0;
    const char * p = varName;
    char * n = (char *) alloca((strlen(varName) + 1) * sizeof(char));

    while( *p != '\0' ) {
        const char * a = p;

        while( *a != '\0' ) {
            if( *a == '.' ) {
                size_t l = (a - p) * sizeof(char);
                memcpy(n, p, l);
                n[l] = '\0';

                st_sel_->bind("parent_id", pid);
                st_sel_->bind("name", (const char *) n, sqlite3pp::nocopy);

                auto i = st_sel_->begin();

                if( i ) {
                    pid = i->get<uint64_t>("id");
                }
                else {
                    uint64_t id = std::rhash(row_next_id_);

                    st_ins_->bind("id", id);
                    st_ins_->bind("parent_id", pid);
                    st_ins_->bind("name", (const char *) n, sqlite3pp::nocopy);
                    st_ins_->bind("value_type", nullptr);
                    st_ins_->bind("value_b", nullptr);
                    st_ins_->bind("value_i", nullptr);
                    st_ins_->bind("value_n", nullptr);
                    st_ins_->bind("value_s", nullptr);
                    st_ins_->execute();

                    pid = id;
                    row_next_id_++;
                }

                p = a + 1;
                break;
            }

            a++;
        }

        if( *a == '\0' )
            break;
    }

    setVar(pid, p, val);
}
//------------------------------------------------------------------------------
HomeostasConfiguration::var_node HomeostasConfiguration::getVarTree(const char * varName)
{
    var_node var;

    bool pid_valid = true;
    const char * p = nullptr;
    uint64_t pid = getVarParentId(varName, &p, &pid_valid);

    if( pid_valid ) {
        var.name = QString::fromUtf8(p);
        var.value = getVar(pid, p, nullptr, &var.id);

        std::function<void (var_node &)> g = [&] (var_node & var) {
            st_sel_by_pid_->bind("parent_id", var.id);

            for( auto i = st_sel_by_pid_->begin(); i; ++i ) {
                const auto name = QString::fromUtf8(i->get<const char *>("name"));

                if( var.childs == nullptr )
                    var.childs.reset(new var_node::childs_type);

                var.childs->emplace(std::make_pair(
                   name, var_node(i->get<uint64_t>("id"), name, getVar(i))
                ));
            }

            if( var.empty() )
                return;

            for( auto & i : var )
                g(i.second);
        };

        g(var);
    }

    return var;
}
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
Homeostas * Homeostas::singleton_ = nullptr;
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
        ctx.to_short_string("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ", '-', 9));
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
    auto & config = HomeostasConfiguration::singleton();

#ifndef NDEBUG
    if( config.getVarTree("tracker").empty() ) {
        config.setVar("tracker.hiew.path", "c:\\hiew");
        //std::shared_ptr<homeostas::directory_tracker> dt(new homeostas::directory_tracker);
        //dt->dir_user_defined_name("hiew");
        //dt->dir_path_name("c:\\hiew");
        //trackers_.emplace_back(dt);
    }
#endif

    for( auto & i : config.getVarTree("tracker") ) {
        const auto & tracker = i.second;
        DirectoriesTrackersModel::singleton().append(
           tracker.name,
           tracker["path"].value.toString()
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
void DirectoryTracker::startTracker()
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
}
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
DirectoriesTrackersModel * DirectoriesTrackersModel::singleton_ = nullptr;
//------------------------------------------------------------------------------
int DirectoriesTrackersModel::rowCount(const QModelIndex &) const
{
    return int(trackers_.size());
}
//------------------------------------------------------------------------------
QVariant DirectoriesTrackersModel::data(const QModelIndex &index, int role) const
{
    auto row = index.row();

    if( row < rowCount() ) {
        auto tracker = trackers_.at(row);

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
        { DirectoryPathNameRole, "path" }
    };
}
//------------------------------------------------------------------------------
QVariantMap DirectoriesTrackersModel::get(int row) const
{
    const auto tracker = trackers_.at(row);
    return {
        { "name", QString::fromStdString(tracker->dir_user_defined_name()) },
        { "path", QString::fromStdString(tracker->dir_path_name()) }
    };
}
//------------------------------------------------------------------------------
void DirectoriesTrackersModel::append(const QString &directoryUserDefinedName, const QString &directoryPathName)
{
    int row = 0;

    while( row < int(trackers_.size())
           && directoryUserDefinedName > QString::fromStdString(trackers_.at(row)->dir_user_defined_name()) )
        ++row;

    beginInsertRows(QModelIndex(), row, row);
    trackers_.insert(trackers_.begin() + row,
        std::shared_ptr<homeostas::directory_tracker>(
            new homeostas::directory_tracker(
                directoryUserDefinedName.toStdString(),
                directoryPathName.toStdString()
    )));
    endInsertRows();
}
//------------------------------------------------------------------------------
void DirectoriesTrackersModel::set(int row, const QString &directoryUserDefinedName, const QString &directoryPathName)
{
    if( row < 0 || row >= int(trackers_.size()) )
        return;

    trackers_[row] = std::shared_ptr<homeostas::directory_tracker>(
        new homeostas::directory_tracker(
            directoryUserDefinedName.toStdString(),
            directoryPathName.toStdString()
    ));

    dataChanged(index(row, 0), index(row, 0), {
        DirectoryUserDefinedNameRole,
        DirectoryPathNameRole
    });
}
//------------------------------------------------------------------------------
void DirectoriesTrackersModel::remove(int row)
{
    if( row < 0 || row >= int(trackers_.size()) )
        return;

    beginRemoveRows(QModelIndex(), row, row);
    trackers_.erase(trackers_.begin() + row);
    endRemoveRows();
}
//------------------------------------------------------------------------------
