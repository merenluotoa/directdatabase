/*******************************************************************************
File: ddbodbc.hpp
$Revision: 1.2 $
Copyright (c) Menacon Inc
*******************************************************************************/

#ifndef DDB_ODBC_H_FILE
#define DDB_ODBC_H_FILE

#include <windows.h>
#include <sql.h>
#include <sqlext.h>

#define SQLSUCCESS(rc) ((rc==SQL_SUCCESS)||(rc==SQL_SUCCESS_WITH_INFO))

// ==================================================================================================
//! Class defines ODBC specific implementation to DirectDatabase-interface.
/*! This class ignores the database name. Set the DSN to server name.
 */
class DdbOdbc : public DirectDatabase
{
public:
    DdbOdbc();
    ~DdbOdbc();

    int GetType() { return DDBTYPE_ODBC; }
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
    unsigned long GetInsertId() { return 0; } // NOT SUPPORTED
    bool UpdateStructure(const DDBSTR &command);

    SQLHANDLE GetEnv() { return hEnvironment; }
    SQLHANDLE GetCon() { return hConnection; }

protected:
    void FreeExecStmt();

    SQLHANDLE hEnvironment;  //!< Environment handle.
    SQLHANDLE hConnection;   //!< Connection handle.
    SQLHANDLE execStmt;      //!< Statement handle that is used for Exec-functions.
};

// ==================================================================================================
//! Class defines PostgreSQL specific implementation to DdbRowSet-interface.
class DdbOdbcRowSet : public DdbRowSet
{
    friend class DdbOdbc;
public:
    ~DdbOdbcRowSet();

    bool Query(const DDBSTR &query);
    int GetNext();

protected:
    DdbOdbcRowSet(DirectDatabase*);

    DdbOdbc*   db;              //!< Pointer to databse object.
    int        currentRow;      //!< The number of the current row in the
    bool       resultCleared;   //!< True if the result has been cleared.
    int        bindIndex;       //!< Index nubmer of the current bind. Starts from 1.
    SQLHANDLE  hStmt;           //!< Statement handle for this row set.
};

#endif
