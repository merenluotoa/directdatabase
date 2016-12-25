/*******************************************************************************
File: ddbfirebird.hpp
$Revision: $

Copyright (c) Menacon Inc
*******************************************************************************/

#ifndef DDB_FIREBIRD_H_FILE
#define DDB_FIREBIRD_H_FILE

#include <ibase.h>

// ==================================================================================================
//! Class defines PostgreSQL specific implementation to DirectDatabase-interface.
class DdbFirebird : public DirectDatabase
{
public:
    DdbFirebird();
    ~DdbFirebird();

    bool Connect(int timeout=0);
    bool Disconnect();

    DdbRowSet* CreateRowSet();
    isc_db_handle GetFBConn();
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

protected:
    void LogError(ISC_STATUS_ARRAY);

    isc_db_handle connection;
    isc_tr_handle transaction;
};

// ==================================================================================================
//! Class defines PostgreSQL specific implementation to DdbRowSet-interface.
class DdbFirebirdRowSet : public DdbRowSet
{
    friend class DdbFirebird;
public:
    ~DdbFirebirdRowSet();

    bool Query(const DDBSTR &query);
    int GetNext();

protected:
    DdbFirebirdRowSet(DirectDatabase*);

    DdbFirebird* db;            //!< Pointer to databse object.
    int         maxRows;        //!< Total number of records in the current query.
    int         currentRow;     //!< The number of the current row in the rowset.
    isc_stmt_handle result;     //!< Query result handle.
};

inline isc_db_handle DdbFirebird::GetFBConn()
{
    return connection;
}

#endif
