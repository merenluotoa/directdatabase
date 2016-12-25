/*******************************************************************************
File: ddbmysql.h
$Revision:$

Copyright (c) Menacon Inc
*******************************************************************************/

#ifndef DDB_MYSQL_H_FILE
#define DDB_MYSQL_H_FILE

#include <mysql.h>

// ==================================================================================================
//! Class defines MySQL specific implementation to DirectDatabase-interface.
class DdbMySql : public DirectDatabase
{
public:
    DdbMySql();
    ~DdbMySql();

    int GetType() { return DDBTYPE_MYSQL; }
    bool Connect(int timeout=0);
    bool Disconnect();

    DdbRowSet* CreateRowSet();
    MYSQL* GetMySConn();
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
    unsigned long GetInsertId();
    bool UpdateStructure(const DDBSTR &command);

protected:
    bool ExecuteFunction(const DDBSTR &query, stringstream &ss);

    MYSQL connection;
};

// ==================================================================================================
//! Class defines PostgreSQL specific implementation to DdbRowSet-interface.
class DdbMySqlRowSet : public DdbRowSet
{
    friend class DdbMySql;
public:
    ~DdbMySqlRowSet();

    bool Query(const DDBSTR &query);
    int GetNext();
    void QuitQuery();

protected:
    DdbMySqlRowSet(DdbMySql*);

    DdbMySql*   db;             //!< Pointer to databse object.
    int         maxRows;        //!< Total number of records in the current query.
    int         maxFields;      //!< Total number of fields in current query result.
    int         currentRow;     //!< The number of the current row in the rowset.
    MYSQL_RES  *result;         //!< Pointer to the result structure.
    bool        resultCleared;  //!< True if the result has been cleared.
};


inline MYSQL* DdbMySql::GetMySConn()
{
    return &connection;
}

#endif
