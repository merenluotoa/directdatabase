/*******************************************************************************
File: ddbmicrosoft.h
$Revision: 1.1.1.1 $
Copyright (c) 2002, Menacon Inc
*******************************************************************************/

#ifndef DDB_MICROSOFT_H_FILE
#define DDB_MICROSOFT_H_FILE

#include <sqlfront.h>

#ifndef _LIB
    struct DBPROCESS;
    typedef DBPROCESS *PDBPROCESS;
#endif

class DdbMSField : public DdbBoundField
{
 public:
    DdbMSField(short int type, void *data) : DdbBoundField(type,data),msdbtime(0) {}
    ~DdbMSField() { if(msdbtime) delete msdbtime; }

    DBDATETIME *msdbtime;       //!< Field is required by Microsoft SQL library functions
};

// ==================================================================================================
//! Class defines Microsoft SQL Server specific implementation to DirectDatabase-interface.
class DdbMicrosoft : public DirectDatabase
{
public:
    DdbMicrosoft();
    ~DdbMicrosoft();

    int GetType() { return DDBTYPE_MSSQL; }
    bool Connect(int timeout=0);
    bool Disconnect();

    DdbRowSet* CreateRowSet();
    PDBPROCESS GetMSConn();
    DDBSTR GetErrorDescription(int id=0);

    bool StartTransaction();
    bool Commit();
    bool RollBack();

    int ExecuteIntFunction(const DDBSTR &query);
    double ExecuteDoubleFunction(const DDBSTR &query);
    bool ExecuteStrFunction(const DDBSTR &query, DDBSTR &result);
    int ExecuteModify(const DDBSTR &query);
    bool UpdateStructure(const DDBSTR &command);

    static DDBSTR g_errorStr; //!< SQL server library use call-back functions to report errors, hence we need static string to store it into.

protected:
    PDBPROCESS connection;      //!< Pointer to DB-LIB process structure
};

// ==================================================================================================
//! Class defines  Microsoft SQL Server specific implementation to DdbRowSet-interface.
class xwDdbMicrosoftRowSet : public DdbRowSet
{
    friend class DdbMicrosoft;
public:
    ~xwDdbMicrosoftRowSet();

    bool Bind(short int type, void *data) { InsertField(new DdbMSField(type,data)); }
    bool Query(const DDBSTR &query);
    int GetNext();

    PDBPROCESS GetMSConn();

protected:
    xwDdbMicrosoftRowSet(DirectDatabase *db);
    int xwDdbMicrosoftBind();

    DdbMicrosoft* db;         //!< Pointer to databse object.
    int         maxRows;        //!< Total number of records in the current query. Note, this is not accurate!
    int         currentRow;     //!< The number of the current row in the rowset.
};


inline PDBPROCESS DdbMicrosoft::GetMSConn()
{
    return connection;
}

#endif
