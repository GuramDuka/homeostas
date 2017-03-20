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
//------------------------------------------------------------------------------
#include "port.hpp"
#include "scope_exit.hpp"
#include "locale_traits.hpp"
#include "cdc512.hpp"
#include "indexer.hpp"
//------------------------------------------------------------------------------
namespace spacenet {
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
struct file_stat :
#if _WIN32
	public _stat64
#else
	public stat
#endif
{
    file_stat(const string & file_name) noexcept {
		stat(file_name);
	}

    int stat(const string & file_name) noexcept {
#if _WIN32
        return _wstat64(file_name.c_str(), this);
#else
		return ::stat(file_name.c_str(), this);
#endif
	}
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void directory_reader::read(const string & root_path)
{
    regex mask_regex(mask_.empty() ? CPPX_U(".*") : mask_);
    regex exclude_regex(exclude_);

    path_ = root_path;

    if( path_.back() == path_delimiter[0] )
        path_.pop_back();

	struct stack_entry {
#if _WIN32
		HANDLE handle;
#else
		DIR * handle;
#endif
        string path;
	};

	std::stack<stack_entry> stack;
	
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
    auto unpack_FILETIME = [] (FILETIME & ft, uint32_t & nsec) {
        ULARGE_INTEGER q;
        q.LowPart = ft.dwLowDateTime;
        q.HighPart = ft.dwHighDateTime;
        nsec = uint32_t((q.QuadPart % 10000000) * 100); // FILETIME is in units of 100 nsec.
        constexpr const uint64_t secs_between_epochs = 11644473600; // Seconds between 1.1.1601 and 1.1.1970 */
        return time_t((q.QuadPart / 10000000) - secs_between_epochs);
    };

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
            handle = FindFirstFileW((path_ + L"\\*").c_str(), &fdw);
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
            throw std::runtime_error(
                str2utf(L"Failed to open directory: " + path_ + L", " + std::to_wstring(err)));

#else
		if( handle == nullptr ) {
			err = errno;
			if( err == ENOTDIR )
				return;
            throw std::runtime_error("Failed to open directory: " + path_ + ", " + std::to_string(err));
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

            name_ = fdw.cFileName;
#elif HAVE_READDIR_R
		for(;;) {
			if( readdir_r(handle, ent, &result) != 0 ) {
				err = errno;
				throw std::runtime_error("Failed to read directory: " + path + ", " + std::to_string(err));
			}

			if( result == nullptr )
				break;

			if( strcmp(ent->d_name, ".") == 0  && !list_dot )
				continue;
			if( strcmp(ent->d_name, "..") == 0 && !list_dotdot )
				continue;

			name = ent->d_name;
#else
		for(;;) {
			errno = 0;
			ent = readdir(handle);
            err = errno;

			if( ent == nullptr ) {
                if( err != 0 )
                    throw std::runtime_error("Failed to read directory: " + path_ + ", " + std::to_string(err));
				break;
			}

            if( strcmp(ent->d_name, ".") == 0  && !list_dot_ )
				continue;
            if( strcmp(ent->d_name, "..") == 0 && !list_dotdot_ )
				continue;

            name_ = ent->d_name;
#endif
            bool match = std::regex_match(name_, mask_regex);

            if( match && !exclude_.empty() )
                match = !std::regex_match(name_, exclude_regex);

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
                throw std::runtime_error("Failed to read file info: " + path_name_ + ", " + std::to_string(err));

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

                if( ::stat(path_name.c_str(), &st) != 0 ) {
                    err = errno;
                    throw std::runtime_error("Failed to stat entry: " + path_name + ", " + std::to_string(err));
                }

                d_type = IFTODT(st.st_mode);
			}

			if( d_type == DT_DIR ) {
#else
			struct stat st;

            if( ::stat(path_name_.c_str(), &st) != 0 ) {
				err = errno;
                throw std::runtime_error("Failed to stat entry: " + path_name_ + ", " + std::to_string(err));
			}

			if( (st.st_mode & S_IFDIR) != 0 ) {
#endif

                if( list_directories_ && match && manipulator_ )
                    manipulator_();

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
            throw std::runtime_error(
                str2utf(L"Failed to read directory: " + path_ + L", " + std::to_wstring(err)));
#endif
	}
};
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
void directory_indexer::reindex(
    sqlite3pp::database & db,
    const string & dir_path_name,
    bool * p_shutdown)
{
    db.execute_all(R"EOS(
        CREATE TABLE IF NOT EXISTS entries (
            is_alive		INTEGER NOT NULL,   /* boolean */
            /*id			BLOB PRIMARY KEY ON CONFLICT ABORT,*/
            parent_id		INTEGER NOT NULL,   /* link on entries rowid */
            name			TEXT NOT NULL,      /* file name*/
            is_dir			INTEGER,            /* boolean */
            mtime			INTEGER,            /* INTEGER as Unix Time, the number of seconds since 1970-01-01 00:00:00 UTC. and nanoseconds if supported */
            file_size		INTEGER,            /* file size in bytes */
            block_size		INTEGER,            /* file block size in bytes */
            digest			BLOB,               /* file checksum */
            UNIQUE(parent_id, name) ON CONFLICT ABORT
        ) /*WITHOUT ROWID*/;
        CREATE UNIQUE INDEX IF NOT EXISTS i1 ON entries (parent_id, name);
        CREATE INDEX IF NOT EXISTS i2 ON entries (is_alive);

        CREATE TABLE IF NOT EXISTS blocks_digests (
            entry_id		INTEGER NOT NULL,   /* link on entries rowid */
            block_no		INTEGER NOT NULL,   /* file block number starting from one */
            mtime			INTEGER,            /* INTEGER as Unix Time, the number of seconds since 1970-01-01 00:00:00 UTC. and nanoseconds if supported */
            digest			BLOB,               /* file block checksum */
            UNIQUE(entry_id, block_no) ON CONFLICT ABORT
        ) /*WITHOUT ROWID*/;
        CREATE UNIQUE INDEX IF NOT EXISTS i3 ON blocks_digests (entry_id, block_no);
    )EOS");

    sqlite3pp::query st_sel(db, R"EOS(
        SELECT
            rowid,
            mtime
        FROM
            entries
        WHERE
            parent_id = :parent_id
            AND name = :name
    )EOS");

	sqlite3pp::command st_ins(db, R"EOS(
        INSERT INTO entries (
            is_alive, parent_id, name, is_dir, mtime, file_size, block_size, digest
        ) VALUES (0, :parent_id, :name, :is_dir, NULL, :file_size, :block_size, NULL)
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
            rowid = :id
    )EOS");

    sqlite3pp::command st_upd_after(db, R"EOS(
        UPDATE entries SET
            is_alive = 0,
            mtime = :mtime,
            digest = :digest
        WHERE
            rowid = :id
    )EOS");

    sqlite3pp::query st_blk_sel(db, R"EOS(
        SELECT
            rowid, *
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

    std::unordered_map<std::string, uint64_t> parents;

    typedef std::vector<uint8_t> blob;

    auto update_block_digest = [&] (
        uint64_t entry_id,
        uint64_t blk_no,
        uint64_t mtim,
        const blob & block_digest)
    {
        auto bind = [&] (auto & st) {
            st.bind("entry_id", entry_id);
            st.bind("block_no", blk_no);
            st.bind("mtime", mtim);
            st.bind("digest", block_digest, sqlite3pp::nocopy);
        };

        bind(st_blk_ins);

        auto exceptions_safe = db.exceptions();
        at_scope_exit( db.exceptions(exceptions_safe) );
        db.exceptions(false);

        if( st_blk_ins.execute() == SQLITE_CONSTRAINT_UNIQUE ) {
            db.exceptions(true);
            bind(st_blk_upd);
            st_blk_upd.execute();
        }
    };

	auto update_entry = [&] (
        uint64_t parent_id,
		const std::string & name,
		bool is_dir,
        uint64_t mtime,
        uint64_t file_size,
        uint64_t block_size,
        uint64_t * p_mtim = nullptr)
	{
		auto bind = [&] (auto & st) {
            st.bind("parent_id", parent_id);
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
		
        auto exceptions_safe = db.exceptions();
        at_scope_exit( db.exceptions(exceptions_safe) );

        st_sel.bind("parent_id", parent_id);
        st_sel.bind("name", name, sqlite3pp::nocopy);

        uint64_t id = 0, mtim = 0;

        auto get_id_mtim = [&] {
            auto i = st_sel.begin();

            if( i ) {
                id = i->get<uint64_t>("rowid");
                mtim = i->get<uint64_t>("mtime");
            }
        };

        db.exceptions(true);
        get_id_mtim();

        // then mtime not changed, just touch entry
        if( modified_only_ && id != 0 && (mtim == mtime || mtime == 0) ) {
            st_upd_touch.bind("id", id);
            st_upd_touch.execute();
        }
        else {
            db.exceptions(false);
            bind(st_ins);

            if( st_ins.execute() == SQLITE_CONSTRAINT_UNIQUE ) {
                db.exceptions(true);
                bind(st_upd);
                st_upd.execute();
            }
        }

        if( p_mtim != nullptr )
            *p_mtim = mtim;

        if( id == 0 )
            get_id_mtim();

        return id;
    };

    directory_reader dr;

    auto update_blocks = [&] (
        cdc512 & file_ctx,
        const string & path_name,
        uint64_t entry_id,
        uint64_t mtim,
        size_t block_size)
    {
        if( p_shutdown != nullptr && *p_shutdown ) {
            dr.abort_ = true;
            return false;
        }

        int in = -1;

#if __GNUC__ >= 5 || _MSC_VER
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
        err = _wsopen_s(&in, path_name.c_str(), _O_RDONLY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
#elif __GNUC__ < 5
        in = ::open(path_name.c_str(), O_RDONLY);
        err = in == -1 ? errno : 0;
#else
        err = _sopen_s(&in, path_name.c_str(), _O_RDONLY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
#endif
        //std::wifstream in(dr.path_name, std::ios::binary);
        //in.exceptions(std::ios::failbit | std::ios::badbit);
        if( err != 0 )
            //throw std::runtime_error(
            //    "Failed open file: " + utf_path_name + ", " + std::to_string(err));
            return false;

        uint64_t blk_no = 0;
        std::vector<uint8_t> buf(block_size);
        blob block_digest;

        for(;;) {//while( !in.eof() ) {
            if( p_shutdown != nullptr && *p_shutdown ) {
                dr.abort_ = true;
                break;
            }

            blk_no++;

            //in.read(reinterpret_cast<char *>(buf), BLOCK_SIZE);
            //auto r = in.gcount();

            uint64_t bmtim = 0;

            st_blk_sel.bind("entry_id", entry_id);
            st_blk_sel.bind("block_no", blk_no);
            auto i = st_blk_sel.begin();

            if( i ) {
                bmtim = i->get<uint64_t>("mtime");
                i->get(block_digest, "digest");
            }

            if( bmtim == 0 || bmtim != mtim ) {
                auto r =
#if __GNUC__ >= 5 || _MSC_VER
                _read
#else
                ::read
#endif
                (in, &buf[0], uint32_t(block_size));

                if( r == -1 ) {
                    //err = errno;
                    //throw std::runtime_error(
                    //    "Failed read file: " + utf_path_name + ", " + std::to_string(err));
                    return false;
                }

                if( r == 0 )
                    break;

                //if( size_t(r) < block_size )
                //    std::memset(&buf[r], 0, block_size - r);

                cdc512 blk_ctx(block_digest, buf.cbegin(), buf.cbegin() + r);
                update_block_digest(entry_id, blk_no, mtim, block_digest);
            }

            file_ctx.update(block_digest.cbegin(), block_digest.cend());
        }

        st_blk_del.bind("entry_id", entry_id);
        st_blk_del.bind("block_no", blk_no);
        st_blk_del.execute();

        return true;
    };

    dr.recursive_ = dr.list_directories_ = true;
    dr.manipulator_ = [&] {
        if( p_shutdown != nullptr && *p_shutdown ) {
            dr.abort_ = true;
            return;
        }

        // skip inaccessible files or directories
        if( access(dr.path_name_, R_OK | (dr.is_dir_ ? X_OK : 0)) != 0 )
            return;

        auto utf_name = str2utf(dr.name_);
        auto utf_path = str2utf(dr.path_);

        uint64_t parent_id = [&] {
            auto pit = parents.find(utf_path);

			if( pit == parents.cend() ) {
                if( dr.level_ > 1 )
					throw std::runtime_error("Undefined behavior");

                auto id = update_entry(0, utf_path, true, 0, 0, 0, 0);
                parents.emplace(std::make_pair(utf_path, id));
                return id;
            }

            return pit->second;
        }();

        size_t block_size = 4096;

        uint64_t mtim, fmtim = dr.mtime_ * 1000000000 + dr.mtime_ns_;
        uint64_t entry_id = update_entry(
            parent_id,
            utf_name,
            dr.is_dir_,
            fmtim,
            dr.fsize_,
            block_size,
            &mtim);

        if( dr.is_dir_ )
            parents.emplace(std::make_pair(str2utf(dr.path_name_), entry_id));

        // if file modified then calculate digests

        if( dr.is_reg_ && (!modified_only_ || mtim != fmtim) ) {
            cdc512 ctx;

            if( update_blocks(ctx, dr.path_name_, entry_id, fmtim, block_size) ) {
                blob digest;
                ctx.finish(digest);

                st_upd_after.bind("id", entry_id);
                st_upd_after.bind("mtime", fmtim);
                st_upd_after.bind("digest", digest, sqlite3pp::nocopy);
                st_upd_after.execute();
            }
        }
	};

    dr.read(dir_path_name);
		
    auto cleanup_entries = [&db] {
        //sqlite3pp::query st_sel(db, R"EOS(
        //    SELECT
        //        rowid
        //    FROM
        //        entries
        //    WHERE
        //        is_alive <> 0
        //)EOS");

        //sqlite3pp::command st_del(db, R"EOS(
        //    DELETE FROM blocks_digests
        //    WHERE
        //        entry_id = :entry_id
        //)EOS");

        sqlite3pp::transaction xct(db, true);

        //for( auto i = st_sel.begin(); i != st_sel.end(); i++ ) {
        //    st_del.bind(0, i->get<uint64_t>(0));
        //    st_del.execute();
        //}

        db.execute_all(R"EOS(
            DELETE FROM blocks_digests WHERE entry_id IN (
                SELECT
                    rowid
                FROM
                    entries
                WHERE
                    is_alive <> 0
            );
            DELETE FROM entries WHERE is_alive <> 0;
            UPDATE entries SET is_alive = 1;
        )EOS");

        xct.commit();
    };

    cleanup_entries();
}
//------------------------------------------------------------------------------
} // namespace spacenet
//------------------------------------------------------------------------------
