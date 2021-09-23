//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef KEY_VALUE_REPOSITORY_SQLITE_HXX
#define KEY_VALUE_REPOSITORY_SQLITE_HXX

#include "bspf.hxx"
#include "AbstractKeyValueRepositorySqlite.hxx"
#include "SqliteDatabase.hxx"
#include "SqliteStatement.hxx"

class KeyValueRepositorySqlite : public AbstractKeyValueRepositorySqlite
{
  public:

    KeyValueRepositorySqlite(SqliteDatabase& db, const string& tableName, const string& colKey, const string& colValue);

    void initialize();

  protected:

    SqliteStatement& stmtInsert(const string& key, const string& value) override;
    SqliteStatement& stmtSelect() override;
    SqliteStatement& stmtDelete(const string& key) override;
    SqliteDatabase& database() override;
    SqliteStatement& stmtCount(const string& key) override;
    SqliteStatement& stmtSelectOne(const string& key) override;

  private:

    SqliteDatabase& myDb;
    string myTableName;
    string myColKey;
    string myColValue;

    unique_ptr<SqliteStatement> myStmtInsert;
    unique_ptr<SqliteStatement> myStmtSelect;
    unique_ptr<SqliteStatement> myStmtDelete;
    unique_ptr<SqliteStatement> myStmtSelectOne;
    unique_ptr<SqliteStatement> myStmtCount;

  private:

    KeyValueRepositorySqlite(const KeyValueRepositorySqlite&) = delete;
    KeyValueRepositorySqlite(KeyValueRepositorySqlite&&) = delete;
    KeyValueRepositorySqlite& operator=(const KeyValueRepositorySqlite&) = delete;
    KeyValueRepositorySqlite& operator=(KeyValueRepositorySqlite&&) = delete;
};

#endif // KEY_VALUE_REPOSITORY_SQLITE_HXX
