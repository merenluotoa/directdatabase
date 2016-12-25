/*! \file ddbpostgre.hpp
 * \brief [short description] */
// Copyright (c) Menacon Oy
/********************************************************************************/

#ifndef DDB_POSTGRE_H_FILE
#define DDB_POSTGRE_H_FILE

#include <libpq-fe.h>

// ==================================================================================================
//! Class defines PostgreSQL specific implementation to DirectDatabase-interface.
class DdbPostgre : public DirectDatabase
{
public:
    DdbPostgre();
    ~DdbPostgre();

    int GetType() { return DDBTYPE_POSTGRES; }
    bool Connect(const char *constr);
    bool Disconnect();
    bool IsConnected() { return connection==0?false:true; }
    bool IsConnectOK();
    bool ResetConnection();

    DdbRowSet* CreateRowSet();
    PGconn* GetPGConn();
    DDBSTR GetErrorDescription(DdbRowSet *rs);

    bool StartTransaction();
    bool Commit();
    bool RollBack();

    bool ExecuteIntFunction(const DDBSTR &query,uint32_t &val);
    bool ExecuteLongFunction(const DDBSTR &query,uint64_t &val);
    bool ExecuteDoubleFunction(const DDBSTR &query, double &val);
    bool ExecuteBoolFunction(const DDBSTR &query, bool &val);
    bool ExecuteStrFunction(const DDBSTR &query, DDBSTR &result);
    bool ExecuteDateFunction(const DDBSTR &query, DDBTIME &val);
    int ExecuteModify(const DDBSTR &query);
    unsigned long GetInsertId();
    bool UpdateStructure(const DDBSTR &command);

    // Admin commands
    bool CreateUser(const DDBSTR &uid, const DDBSTR &pwd);
    bool CreateDatabase(const DDBSTR &dbname, const DDBSTR &owner);
    bool CreateTable(const DDBSTR &name, const DDBSTR &sql);

    static DdbPostgre* getAdminConn() {
        DdbPostgre *pg = new DdbPostgre();
        pg->Connect("host=/var/run dbname=postgres user=postgres connect_timeout=10");
        return pg;
    }
    static bool ExtractTimestamp(const char *result, struct tm *);
protected:
    PGconn     *connection;

};

// ==================================================================================================
//! Class defines PostgreSQL specific implementation to DdbRowSet-interface.
class DdbPosgtgreRowSet : public DdbRowSet
{
    friend class DdbPostgre;
public:
    ~DdbPosgtgreRowSet();

    bool Query(const DDBSTR &query);
    int GetNext();
    void QuitQuery();

protected:
    DdbPosgtgreRowSet(DirectDatabase*);

    DdbPostgre* db;             //!< Pointer to databse object.
    int         maxRows;        //!< Total number of records in the current query.
    int         currentRow;     //!< The number of the current row in the rowset.
    PGresult   *result;         //!< Pointer to the result structure.
    bool        resultCleared;  //!< True if the result has been cleared.
};


inline PGconn* DdbPostgre::GetPGConn()
{
    return connection;
}

#endif
