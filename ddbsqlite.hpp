/*******************************************************************************
File: ddbsqlite.hpp
$Revision: $

Copyright (c) Menacon Inc
*******************************************************************************/

#ifndef DDB_SQLITE_H_FILE
#define DDB_SQLITE_H_FILE

#include <ibase.h>

// ==================================================================================================
//! Class defines PostgreSQL specific implementation to DirectDatabase-interface.
class DdbSqlite : public DirectDatabase
{
public:
    DdbSqlite();
    ~DdbSqlite();

    bool Connect(int timeout=0);
    bool Disconnect();

    DdbRowSet* CreateRowSet();
    DDBSTR GetErrorDescription(DdbRowSet *rs);

    bool StartTransaction();
    bool Commit();
    bool RollBack();

    bool ExecuteIntFunction(const DDBSTR &query,int &val);
    bool ExecuteDoubleFunction(const DDBSTR &query, double &val);
    bool ExecuteBoolFunction(const DDBSTR &query, bool &val);
    bool ExecuteStrFunction(const DDBSTR &query, DDBSTR &result);
    bool ExecuteDateFunction(const DDBSTR &query, DDBTIME &val);
    int ExecuteModify(const DDBSTR &query);
    bool UpdateStructure(const DDBSTR &command);

protected:
    void LogError(ISC_STATUS_ARRAY);

};

// ==================================================================================================
//! Class defines PostgreSQL specific implementation to DdbRowSet-interface.
class DdbSqliteRowSet : public DdbRowSet
{
    friend class DdbSqlite;
public:
    ~DdbSqliteRowSet();

    bool Query(const DDBSTR &query);
    int GetNext();

protected:
    DdbSqliteRowSet(DirectDatabase*);

    DdbSqlite* db;            //!< Pointer to databse object.
    int         maxRows;        //!< Total number of records in the current query.
    int         currentRow;     //!< The number of the current row in the rowset.
    isc_stmt_handle result;     //!< Query result handle.
};

inline isc_db_handle DdbSqlite::GetFBConn()
{
    return connection;
}

#endif
