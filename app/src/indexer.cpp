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
#include "config.h"
//------------------------------------------------------------------------------
#include <stack>
#include <regex>
#if QT_CORE_LIB
#   include <QString>
#   include <QRegExp>
#endif
//------------------------------------------------------------------------------
#include "port.hpp"
#include "std_ext.hpp"
#include "cdc512.hpp"
#include "indexer.hpp"
//------------------------------------------------------------------------------
namespace homeostas {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
#if _WIN32
static inline auto unpack_FILETIME(FILETIME & ft, uint32_t & nsec)
{
    ULARGE_INTEGER q;
    q.LowPart = ft.dwLowDateTime;
    q.HighPart = ft.dwHighDateTime;
    nsec = uint32_t((q.QuadPart % 10000000) * 100); // FILETIME is in units of 100 nsec.
    constexpr const uint64_t secs_between_epochs = 11644473600; // Seconds between 1.1.1601 and 1.1.1970
    return time_t((q.QuadPart / 10000000) - secs_between_epochs);
};
#endif
//------------------------------------------------------------------------------
struct file_stat :
#if _WIN32
	public _stat64
#else
	public stat
#endif
{
#if _WIN32
    uint32_t st_atimensec;
    uint32_t st_ctimensec;
    uint32_t st_mtimensec;
#endif

    file_stat(const std::string & file_name) {
        auto err = stat(file_name);
        if( err != 0 )
            throw std::xruntime_error(strerror(err), __FILE__, __LINE__);
	}

    int stat(const std::string & file_name) noexcept {
#if _WIN32
        auto name = QString::fromStdString(file_name).toStdWString();
        int err = _wstat64(name.c_str(), this);

        if( err == 0 ) {
            st_mtimensec = st_ctimensec = st_atimensec = 0;

            auto handle = CreateFileW(
                name.c_str(),
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS,
                NULL);

            if( handle != INVALID_HANDLE_VALUE ) {
                FILETIME ftc, fta, ftw;

                // Retrieve the file times for the file.
                if( GetFileTime(handle, &ftc, &fta, &ftw) ) {
                    st_atime = unpack_FILETIME(fta, st_atimensec);
                    st_ctime = unpack_FILETIME(ftc, st_ctimensec);
                    st_mtime = unpack_FILETIME(ftw, st_mtimensec);
                }

                CloseHandle(handle);
            }
        }

        return err;
#else
		return ::stat(file_name.c_str(), this);
#endif
	}

    uint64_t atime() const {
        return 1000000000ull * st_atime +
#if __USE_XOPEN2K8
            st_atim.tv_nsec;
#else
            st_atimensec;
#endif
    }

    uint64_t ctime() const {
        return 1000000000ull * st_ctime +
#if __USE_XOPEN2K8
            st_ctim.tv_nsec;
#else
            st_ctimensec;
#endif
    }

    uint64_t mtime() const {
        return 1000000000ull * st_mtime +
#if __USE_XOPEN2K8
            st_mtim.tv_nsec;
#else
            st_mtimensec;
#endif
    }

    uint64_t fsize() const {
        return st_size;
    }

    bool is_reg() const {
        return
#if _WIN32
            (st_mode & S_IFREG) == 0;
#else
            S_ISREG(st_mode) != 0;
#endif
    }

    bool is_dir() const {
        return
#if _WIN32
            (st_mode & S_IFDIR) == 0;
#else
            S_ISDIR(st_mode) != 0;
#endif
    }
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void directory_reader::read(const std::string & root_path)
{
    struct stack_entry {
#if _WIN32
        HANDLE handle;
#else
        DIR * handle;
#endif
        std::string path;
    };

    std::stack<stack_entry> stack;
    QRegExp mask_regex(mask_.empty() ? ".*" : mask_.c_str());
    QRegExp exclude_regex(exclude_.c_str());

    path_ = root_path;

    if( path_.back() == path_delimiter[0] )
        path_.pop_back();

#if _WIN32
	at_scope_exit(
		while( !stack.empty() ) {
			FindClose(stack.top().handle);
			stack.pop();
		}
	);
#else
	at_scope_exit(
		while( !stack.empty() ) {
			::closedir(stack.top().handle);
			stack.pop();
		}
	);
#endif

#if _WIN32
    HANDLE handle = INVALID_HANDLE_VALUE;
#else
	DIR * handle = nullptr;
#endif

    abort_ = false;

	for(;;) {
        if( abort_ )
            break;
#if _WIN32
		DWORD err;
		WIN32_FIND_DATAW fdw;
#else
		int err;
		struct dirent * ent;
#if HAVE_READDIR_R
		struct dirent * result, res;
		ent = &res;
#endif
#endif

#if _WIN32
		if( handle == INVALID_HANDLE_VALUE ) {
            handle = FindFirstFileW(QString::fromStdString(path_ + "\\*").toStdWString().c_str(), &fdw);
#else
		if( handle == nullptr ) {
            handle = ::opendir(path_.c_str());
#endif
		}
		else if( stack.empty() ) {
			break;
		}
		else {
			handle = stack.top().handle;
            path_ = stack.top().path;
			stack.pop();
#if _WIN32
            lstrcpyW(fdw.cFileName, L"");
#endif
		}

        level_ = stack.size() + 1;
#if _WIN32
		if( handle == INVALID_HANDLE_VALUE ) {
			err = GetLastError();
			if( err == ERROR_PATH_NOT_FOUND )
				return;
            throw std::xruntime_error(
                "Failed to open directory: " + path_ + ", " + std::to_string(err), __FILE__, __LINE__);

#else
		if( handle == nullptr ) {
			err = errno;
			if( err == ENOTDIR )
				return;
            throw std::xruntime_error("Failed to open directory: " + path_ + ", " + std::to_string(err), __FILE__, __LINE__);
#endif
		}

#if _WIN32
		at_scope_exit(
			if( handle != INVALID_HANDLE_VALUE )
				FindClose(handle);
		);
#else
		at_scope_exit(
			if( handle != nullptr )
				::closedir(handle);
		);
#endif

#if _WIN32
		do {
            if( abort_ )
                break;

			if( lstrcmpW(fdw.cFileName, L"") == 0 )
				continue;
            if( lstrcmpW(fdw.cFileName, L".") == 0 && !list_dot_ )
				continue;
            if( lstrcmpW(fdw.cFileName, L"..") == 0 && !list_dotdot_ )
				continue;

            name_ = QString::fromWCharArray(fdw.cFileName).toStdString();
#elif HAVE_READDIR_R
		for(;;) {
			if( readdir_r(handle, ent, &result) != 0 ) {
				err = errno;
                throw std::xruntime_error("Failed to read directory: " + path_ + ", " + std::to_string(err), __FILE__, __LINE__);
			}

			if( result == nullptr )
				break;

            if( strcmp(ent->d_name, ".") == 0  && !list_dot_ )
				continue;
            if( strcmp(ent->d_name, "..") == 0 && !list_dotdot_ )
				continue;

            name_ = ent->d_name;
#else
		for(;;) {
			errno = 0;
			ent = readdir(handle);
            err = errno;

			if( ent == nullptr ) {
                if( err != 0 )
                    throw std::xruntime_error("Failed to read directory: " + path_ + ", " + std::to_string(err), __FILE__, __LINE__);
				break;
			}

            if( strcmp(ent->d_name, ".") == 0  && !list_dot_ )
				continue;
            if( strcmp(ent->d_name, "..") == 0 && !list_dotdot_ )
				continue;

            name_ = ent->d_name;
#endif
            auto qname = QString::fromStdString(name_);
            bool match = mask_regex.indexIn(qname) != -1;

            if( match && !exclude_.empty() )
                match = exclude_regex.indexIn(qname) == -1;

            path_name_ = path_ + path_delimiter + name_;

#if _WIN32
            fsize_ = fdw.nFileSizeHigh;
            fsize_ <<= 32;
            fsize_ |= fdw.nFileSizeLow;
            is_reg_ = (fdw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
            is_dir_ = (fdw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            is_lnk_ = false;

            atime_ = unpack_FILETIME(fdw.ftLastAccessTime, atime_ns_);
            ctime_ = unpack_FILETIME(fdw.ftCreationTime, ctime_ns_);
            mtime_ = unpack_FILETIME(fdw.ftLastWriteTime, mtime_ns_);
#else
            file_stat fs(path_name_);

			if( (err = errno) != 0 )
                throw std::xruntime_error("Failed to read file info: " + path_name_ + ", " + std::to_string(err), __FILE__, __LINE__);

			//if( (fs.st_mode & type_mask) == 0 )
			//	continue;
            atime_ = fs.st_atime;
            ctime_ = fs.st_ctime;
            mtime_ = fs.st_mtime;
#if __USE_XOPEN2K8
            atime_ns_ = fs.st_atim.tv_nsec;
            ctime_ns_ = fs.st_ctim.tv_nsec;
            mtime_ns_ = fs.st_mtim.tv_nsec;
#else
            atime_ns_ = fs.st_atimensec;
            ctime_ns_ = fs.st_ctimensec;
            mtime_ns_ = fs.st_mtimensec;
#endif
            fsize_ = fs.st_size;
            is_reg_ = S_ISREG(fs.st_mode);
            is_dir_ = S_ISDIR(fs.st_mode);
            is_lnk_ = S_ISLNK(fs.st_mode);
#endif
#if _WIN32
			if( (fdw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ) {
#elif __USE_MISC
//&& DT_UNKNOWN && DT_DIR && IFTODT
            auto d_type = ent->d_type;

			if( d_type == DT_UNKNOWN ) {
                struct stat st;

                if( ::stat(path_name_.c_str(), &st) != 0 ) {
                    err = errno;
                    throw std::xruntime_error("Failed to stat entry: " + path_name_ + ", " + std::to_string(err), __FILE__, __LINE__);
                }

                d_type = IFTODT(st.st_mode);
			}

			if( d_type == DT_DIR ) {
#else
			struct stat st;

            if( ::stat(path_name_.c_str(), &st) != 0 ) {
				err = errno;
                throw std::xruntime_error("Failed to stat entry: " + path_name_ + ", " + std::to_string(err), __FILE__, __LINE__);
			}

			if( (st.st_mode & S_IFDIR) != 0 ) {
#endif

                if( list_directories_ && match && manipulator_ )
                    manipulator_();

                if( skip_ ) {
                    skip_ = false;
                    break;
                }

                if( recursive_ && (max_level_ == 0 || stack.size() < max_level_ ) ) {
                    stack.push({ handle, path_ });
                    path_ += path_delimiter + name_;
#if _WIN32
					handle = INVALID_HANDLE_VALUE;
#else
					handle = nullptr;
#endif
					break;
				}
			}
			else if( match ) {
                if( manipulator_ )
                    manipulator_();
			}
		}
#if _WIN32
		while( FindNextFileW(handle, &fdw) != 0 );

        if( handle != INVALID_HANDLE_VALUE
                && (err = GetLastError()) != ERROR_NO_MORE_FILES
                && !abort_ )
            throw std::xruntime_error(
                "Failed to read directory: " + path_ + ", " + std::to_string(err), __FILE__, __LINE__);
#endif
	}
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void directory_indexer::reindex(
    sqlite3pp::database & db,
    const std::string & dir_path_name,
    bool * p_shutdown)
{
    auto exceptions_safe = db.exceptions();
    at_scope_exit( db.exceptions(exceptions_safe) );
    db.exceptions(true);

    db.execute_all(R"EOS(
        /*CREATE TABLE IF NOT EXISTS rowids (
            id              INTEGER PRIMARY KEY ON CONFLICT REPLACE,
            counter    		INTEGER NOT NULL
        ) WITHOUT ROWID;*/

        CREATE TABLE IF NOT EXISTS entries (
            id              INTEGER PRIMARY KEY ON CONFLICT ABORT,
            parent_id		INTEGER NOT NULL,   /* link on entries id */
            is_alive		INTEGER NOT NULL,   /* boolean */
            name			TEXT NOT NULL,      /* file name */
            is_dir			INTEGER,            /* boolean */
            mtime			INTEGER,            /* nanoseconds */
            file_size		INTEGER,            /* file size in bytes */
            block_size		INTEGER,            /* file block size in bytes */
            digest			BLOB                /* file checksum, or snap key for root */
            UNIQUE(parent_id, name) ON CONFLICT ABORT
        ) WITHOUT ROWID;

        CREATE UNIQUE INDEX IF NOT EXISTS i1 ON entries (parent_id, name);
        CREATE INDEX IF NOT EXISTS i2 ON entries (is_alive);

        CREATE TABLE IF NOT EXISTS blocks_digests (
            entry_id		INTEGER NOT NULL,   /* link on entries rowid */
            block_no		INTEGER NOT NULL,   /* file block number starting from one */
            mtime			INTEGER,            /* nanoseconds */
            digest			BLOB,               /* file block checksum */
            UNIQUE(entry_id, block_no) ON CONFLICT ABORT
        )/*WITHOUT ROWID*/;
        CREATE UNIQUE INDEX IF NOT EXISTS i3 ON blocks_digests (entry_id, block_no);

        CREATE TABLE IF NOT EXISTS remote_trackers (
            key             BLOB PRIMARY KEY ON CONFLICT ABORT, /* remote tracker host public key */
            mtime			INTEGER NOT NULL                    /* nanoseconds */
        ) WITHOUT ROWID;

        CREATE TABLE IF NOT EXISTS remote_tracking (
            key             BLOB NOT NULL,      /* remote tracker host public key */
            entry_id        INTEGER NOT NULL,   /* link on entries rowid */
            block_no		INTEGER NOT NULL,   /* file block number starting from one */
            deleted         INTEGER
            UNIQUE(entry_id, block_no, key) ON CONFLICT REPLACE
        ) /*WITHOUT ROWID*/;
        CREATE UNIQUE INDEX IF NOT EXISTS i4 ON remote_tracking (entry_id, block_no, key);
        CREATE INDEX IF NOT EXISTS i5 ON remote_tracking (entry_id);
        CREATE INDEX IF NOT EXISTS i6 ON remote_tracking (entry_id, block_no);

        CREATE TRIGGER IF NOT EXISTS entries_after_delete_trigger
            AFTER DELETE
            ON entries FOR EACH ROW
        BEGIN
            /*DELETE FROM remote_tracking WHERE entry_id = old.id;*/
            DELETE FROM blocks_digests WHERE entry_id = old.id;
        END;

        CREATE TRIGGER IF NOT EXISTS blocks_digests_after_delete_trigger
            AFTER DELETE
            ON blocks_digests FOR EACH ROW
        BEGIN
            REPLACE INTO remote_tracking
                SELECT
                    key, old.entry_id, old.block_no, 1
               FROM
                   remote_trackers
            /*DELETE FROM remote_tracking WHERE entry_id = old.entry_id AND block_no = old.block_no;*/
        END;

        CREATE TRIGGER IF NOT EXISTS remote_trackers_after_insert_trigger
            AFTER INSERT
            ON remote_trackers FOR EACH ROW
        BEGIN
            REPLACE INTO remote_tracking
                SELECT
                    new.key, entry_id, block_no, NULL
                FROM
                    blocks_digests
        END;

        CREATE TRIGGER IF NOT EXISTS remote_trackers_after_delete_trigger
            AFTER DELETE
            ON remote_trackers FOR EACH ROW
        BEGIN
            DELETE FROM remote_tracking WHERE key = old.key;
        END;
    )EOS");

    sqlite3pp::query row_next_id_st(db, R"EOS(
        SELECT
            id
        FROM
            entries
        WHERE
            id = :id
        UNION ALL
        SELECT
            entry_id
        FROM
            remote_tracking
        WHERE
            entry_id = :id
    )EOS");

    uint64_t row_id;

    while( (row_id = entropy_fast()) == 0 );

    auto row_next_id = [&] {
        at_scope_exit( row_next_id_st.reset() );

        for(;;) {
            row_next_id_st.bind("id", row_id);
            auto i = row_next_id_st.begin();

            if( i ) {
                while( (row_id = std::ihash<uint64_t>(std::rhash(row_id) + 1) ^ row_id) == 0 );
            }
            else
                break;
        }

        return row_id;
    };

    sqlite3pp::query st_sel(db, R"EOS(
        SELECT
            id,
            mtime,
            digest
        FROM
            entries
        WHERE
            parent_id = :parent_id
            AND name = :name
    )EOS");

	sqlite3pp::command st_ins(db, R"EOS(
        INSERT INTO entries (
            id, is_alive, parent_id, name, is_dir, mtime, file_size, block_size, digest
        ) VALUES (:id, 0, :parent_id, :name, :is_dir, NULL, :file_size, :block_size, NULL)
	)EOS");
	
	sqlite3pp::command st_upd(db, R"EOS(
        UPDATE entries SET
            is_alive = 0,
            parent_id = :parent_id,
            name = :name,
            is_dir = :is_dir,
            file_size = :file_size,
            block_size = :block_size,
            digest = NULL
		WHERE
            parent_id = :parent_id
            AND name = :name
	)EOS");

    sqlite3pp::command st_upd_touch(db, R"EOS(
        UPDATE entries SET
            is_alive = 0
        WHERE
            id = :id
    )EOS");

    sqlite3pp::command st_upd_after(db, R"EOS(
        UPDATE entries SET
            is_alive = 0,
            mtime = :mtime,
            digest = :digest
        WHERE
            id = :id
    )EOS");

    sqlite3pp::query st_blk_sel(db, R"EOS(
        SELECT
            *
        FROM
            blocks_digests
        WHERE
            entry_id = :entry_id
            AND block_no = :block_no
    )EOS");

    sqlite3pp::command st_blk_ins(db, R"EOS(
        INSERT INTO blocks_digests (
            entry_id, block_no, mtime, digest
        ) VALUES (
            :entry_id, :block_no, :mtime, :digest)
	)EOS");

	sqlite3pp::command st_blk_upd(db, R"EOS(
		UPDATE blocks_digests SET
            digest = :digest,
            mtime = :mtime
		WHERE
            entry_id = :entry_id
            AND block_no = :block_no
	)EOS");

    sqlite3pp::command st_blk_del(db, R"EOS(
        DELETE FROM blocks_digests
        WHERE
            entry_id = :entry_id
            AND block_no > :block_no
    )EOS");

    sqlite3pp::command st_rt_rpl(db, R"EOS(
        REPLACE INTO remote_tracking
            SELECT
                key, :entry_id, :block_no, NULL
            FROM
                remote_trackers
    )EOS");

    sqlite3pp::command st_rt_del(db, R"EOS(
        REPLACE INTO remote_tracking
            SELECT
                key, :entry_id, :block_no, 1
            FROM
                remote_trackers
    )EOS");

    struct parent_dir {
        uint64_t id;
        uint64_t mtim;
        uint64_t mtime;
    };

    std::unordered_map<std::string, parent_dir> parents;
    sqlite3pp::transaction tx(&db);

    auto tx_start = clock_gettime_ns();

    auto tx_deadline = [&] {
        auto now = clock_gettime_ns();
        auto deadline = tx_start + std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(50)).count();

        if( now >= deadline ) {
            tx.commit();
            tx.start();
            tx_start = now;
        }
    };

    auto update_block_digest = [&] (
        uint64_t entry_id,
        uint64_t blk_no,
        uint64_t mtim,
        const std::key512 & block_digest,
        const std::key512 & prev_block_digest)
    {
        auto bind = [&] (auto & st) {
            st.bind("entry_id", entry_id);
            st.bind("block_no", blk_no);
            st.bind("mtime"   , mtim);
            st.bind("digest"  , block_digest, sqlite3pp::nocopy);
        };

        bind(st_blk_ins);

        db.exceptions(false);
        st_blk_ins.execute();
        db.exceptions(true);

        if( st_blk_ins.rc() == SQLITE_CONSTRAINT_UNIQUE ) {
            bind(st_blk_upd);
            st_blk_upd.execute();
        }

        if( block_digest != prev_block_digest ) {
            st_rt_rpl.bind("entry_id", entry_id);
            st_rt_rpl.bind("block_no", blk_no);
            st_rt_rpl.execute();
        }

        tx_deadline();
    };

	auto update_entry = [&] (
        const parent_dir & parent,
		const std::string & name,
		bool is_dir,
        uint64_t mtime,
        uint64_t file_size,
        uint64_t block_size,
        uint64_t * p_mtim = nullptr)
	{
		auto bind = [&] (auto & st) {
            st.bind("parent_id", parent.id);
            st.bind("name", name, sqlite3pp::nocopy);

			if( is_dir )
                st.bind("is_dir", is_dir);
			else
                st.bind("is_dir", nullptr);

            if( file_size == 0 )
                st.bind("file_size", nullptr);
            else
                st.bind("file_size", file_size);

            if( block_size == 0 )
                st.bind("block_size", nullptr);
            else
                st.bind("block_size", block_size);
        };
		
        st_sel.bind("parent_id", parent.id);
        st_sel.bind("name", name, sqlite3pp::nocopy);

        uint64_t id = 0, mtim = 0;

        auto get_id_mtim = [&] {
            at_scope_exit( st_sel.reset() );

            auto i = st_sel.begin();

            if( i ) {
                id = i->get<uint64_t>("id");
                mtim = i->get<uint64_t>("mtime");
            }
        };

        get_id_mtim();

        // then mtime not changed, just touch entry
        if( modified_only_ && id != 0 && (mtim == mtime || mtime == 0) ) {
            // touch only if parent dir changed and not root
            if( parent.mtim != parent.mtime && parent.id != 0 ) {
                st_upd_touch.bind("id", id);
                st_upd_touch.execute();
            }
        }
        else {
            if( id == 0 ) {
                auto next_id = row_next_id();
                st_ins.bind("id", next_id);
                bind(st_ins);
                st_ins.execute();
                //if( st_ins.execute(SQLITE_CONSTRAINT_UNIQUE) != SQLITE_CONSTRAINT_UNIQUE )
                id = next_id;
            }
            else {
                bind(st_upd);
                st_upd.execute();
            }
        }

        if( p_mtim != nullptr )
            *p_mtim = mtim;

        if( id == 0 )
            get_id_mtim();

        tx_deadline();

        return id;
    };

    directory_reader dr;
    bool update_snap = false;

    auto update_blocks = [&] (
        cdc512 & file_ctx,
        const std::string & path_name,
        uint64_t entry_id,
        uint64_t mtim,
        size_t block_size)
    {
        if( p_shutdown != nullptr && *p_shutdown ) {
            dr.abort_ = true;
            return false;
        }

        int in = -1;

#if _MSC_VER
        at_scope_exit(
            if( in != -1 )
                ::_close(in);
        );
#else
        at_scope_exit(
            if( in != -1 )
                ::close(in);
        );

#endif

        auto err = errno;
#if _WIN32
        err = _wsopen_s(&in, QString::fromStdString(path_name).toStdWString().c_str(), _O_RDONLY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
#elif __MINGW32__
        err = _sopen_s(&in, path_name.c_str(), _O_RDONLY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
#elif __linux__
        in = ::open(path_name.c_str(), O_RDONLY | O_NOATIME);
        err = in == -1 ? errno : 0;
#elif (__GNUG__ > 0 && __GNUC__ < 5)
        in = ::open(path_name.c_str(), O_RDONLY);
        err = in == -1 ? errno : 0;
#endif
        //std::wifstream in(dr.path_name, std::ios::binary);
        //in.exceptions(std::ios::failbit | std::ios::badbit);
        if( err != 0 )
            //throw std::xruntime_error(
            //    "Failed open file: " + utf_path_name + ", " + std::to_string(err));
            return false;

        uint64_t blk_no = 0;
        std::unique_ptr<uint8_t> buf(new uint8_t [block_size]);

        for(;;) {//while( !in.eof() ) {
            if( p_shutdown != nullptr && *p_shutdown ) {
                dr.abort_ = true;
                break;
            }

            blk_no++;

            //in.read(reinterpret_cast<char *>(buf), BLOCK_SIZE);
            //auto r = in.gcount();

            uint64_t bmtim;
            cdc512 blk_ctx(std::leave_uninitialized);
            std::key512 prev_blk_digest(std::leave_uninitialized);

            st_blk_sel.bind("entry_id", entry_id);
            st_blk_sel.bind("block_no", blk_no);
            auto i = st_blk_sel.begin();

            if( i ) {
                bmtim = i->get<uint64_t>("mtime");
                prev_blk_digest = i->get<std::key512>("digest");
            }

            if( !i || bmtim != mtim ) {
                auto r =
#if _MSC_VER
                _read
#else
                ::read
#endif
                    (in, buf.get(), uint32_t(block_size));

                if( r == -1 ) {
                    //err = errno;
                    //throw std::xruntime_error(
                    //    "Failed read file: " + utf_path_name + ", " + std::to_string(err));
                    return false;
                }

                if( r == 0 )
                    break;

                //if( size_t(r) < block_size )
                //    std::memset(&buf[r], 0, block_size - r);

                blk_ctx.init();
                blk_ctx.update(std::make_range(buf, r));
                blk_ctx.final();

                update_block_digest(entry_id, blk_no, mtim, blk_ctx, prev_blk_digest);
                update_snap = true;

                file_ctx.update(std::begin(blk_ctx), std::end(blk_ctx));
            }
            else if( i )
                file_ctx.update(std::begin(prev_blk_digest), std::end(prev_blk_digest));
        }

        st_blk_del.bind("entry_id", entry_id);
        st_blk_del.bind("block_no", blk_no);
        st_blk_del.execute();

        st_rt_del.bind("entry_id", entry_id);
        st_rt_del.bind("block_no", blk_no);
        st_rt_del.execute();

        tx_deadline();

        return true;
    };

    parent_dir root = { 0, 0, 0 };

    dr.recursive_ = dr.list_directories_ = true;
    dr.manipulator_ = [&] {
        if( p_shutdown != nullptr && *p_shutdown ) {
            dr.abort_ = true;
            return;
        }

        // skip inaccessible files or directories
        if( access(dr.path_name_, R_OK | (dr.is_dir_ ? X_OK : 0)) != 0 ) {
            dr.skip_ = dr.is_dir_;
            return;
        }

        const auto & parent = [&] {
            auto pit = parents.find(dr.path_);

			if( pit == parents.cend() ) {
                if( dr.level_ > 1 )
                    throw std::xruntime_error("Undefined behavior", __FILE__, __LINE__);

                file_stat st(dr.path_);
                root.id = update_entry(root, dr.path_, true, root.mtime = st.mtime(), 0, 0, &root.mtim);
                parents.emplace(std::make_pair(dr.path_, root));

                return parents.find(dr.path_)->second;
            }

            return pit->second;
        }();

        size_t block_size = 4096;

        uint64_t mtim, fmtim = 1000000000ull * dr.mtime_ + dr.mtime_ns_;
        uint64_t entry_id = update_entry(
            parent,
            dr.name_,
            dr.is_dir_,
            fmtim,
            dr.fsize_,
            block_size,
            &mtim);

        if( dr.is_dir_ ) {
            parent_dir entry = { entry_id, mtim, fmtim };
            parents.emplace(std::make_pair(dr.path_name_, entry));
        }

        if( !modified_only_ || mtim != fmtim ) {
            // if file modified then calculate digests

            if( dr.is_reg_ ) {
                cdc512 ctx;

                if( update_blocks(ctx, dr.path_name_, entry_id, fmtim, block_size) ) {
                    ctx.final();
                    st_upd_after.bind("digest", ctx, sqlite3pp::nocopy);
                }
            }
            else {
                st_upd_after.bind("digest", nullptr);
            }

            st_upd_after.bind("id", entry_id);
            st_upd_after.bind("mtime", fmtim);
            st_upd_after.execute();
        }
    };

    dr.read(dir_path_name);

    if( !modified_only_ || root.mtim != root.mtime || update_snap ) {
        cdc512 digest(std::leave_uninitialized);

        st_sel.bind("parent_id", root.id);
        st_sel.bind("name", dr.path_, sqlite3pp::nocopy);

        auto i = st_sel.begin();

        if( i )
            digest = i->get<std::key512>("digest");

        struct {
            uint64_t shortcut;
            uint64_t entropy;
            std::key512 key;
        } a;

        a.shortcut = digest.hash();
        a.entropy = entropy_fast();
        a.key = digest;
        digest.update(&a, sizeof(a));

        st_upd_after.bind("digest", digest, sqlite3pp::nocopy);
        st_upd_after.bind("id", root.id);
        st_upd_after.bind("mtime", root.mtime);
        st_upd_after.execute();
    }

    auto cleanup_entries = [&db] {
        db.execute_all(R"EOS(
            DELETE FROM entries WHERE id IN (
               SELECT
                   e.id
               FROM
                   entries AS e
                       JOIN entries AS p
                       ON e.parent_id = p.id
               WHERE
                   e.is_alive <> 0
                   AND p.is_alive == 0
            );
            /*DELETE FROM entries WHERE is_alive <> 0;*/
            UPDATE entries SET is_alive = 1 WHERE is_alive = 0;
        )EOS");
    };

    cleanup_entries();
}
//------------------------------------------------------------------------------
} // namespace homeostas
//------------------------------------------------------------------------------
