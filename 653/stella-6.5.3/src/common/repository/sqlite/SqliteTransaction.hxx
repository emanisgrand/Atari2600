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

#ifndef SQLITE_TRANSACTION_HXX
#define SQLITE_TRANSACTION_HXX

class SqliteDatabase;

class SqliteTransaction {
  public:

    explicit SqliteTransaction(SqliteDatabase& db);

    ~SqliteTransaction();

    void commit();

    void rollback();

  private:

    SqliteDatabase& myDb;

    bool myTransactionClosed{false};

  private:

    SqliteTransaction(const SqliteTransaction&) = delete;
    SqliteTransaction(SqliteTransaction&&) = delete;
    SqliteTransaction& operator=(const SqliteTransaction&) = delete;
    SqliteTransaction& operator=(SqliteTransaction&&) = delete;
};

#endif // SQLITE_TRANSACTION_HXX
