/*******************************************************************************
ddbpostgre.cpp
Copyright (c) Antti Merenluoto
*******************************************************************************/

#if defined(DDB_USEWX)
  #include <wx/wxprec.h>
  #ifndef WX_PRECOMP
    #include <wx/wx.h>
  #endif
#endif
#include "pch-stop.h"
#ifdef WIN32
  #include <windows.h>
#endif
#ifdef __linux
  #include <string.h>
  #include <stdlib.h>
  #include <errno.h>
#endif
#include <cpp4scripts.hpp>
#define __DDB_POSTGRE__
#include "directdatabase.hpp"

void DdbPQNoticeProcessor(void *, const char *message)
{
    CS_PRINT_NOTE(message);
}

// ==================================================================================================
DdbPostgre::DdbPostgre()
/*!
  Constructs database object for the connection to the PostgreSQL databases.
  In Windows environment this intitializes sockets. On Linux the only the
  initialization flag is simply set to true.
*/
{
#if defined(_WIN32) && defined(DDB_STATIC)
WORD wVersionRequested;
WSADATA wsaData;

    // Initialize the Windows sockets.
    wVersionRequested = MAKEWORD( 2, 0 );
    int err = WSAStartup( wVersionRequested, &wsaData );
    if(err)
    {
        CS_PRINT_CRIT("DdbPostgre::DdbPostgre - Socket initialization failed.");
        SetErrorId(2);
        flags &= ~DDB_FLAG_INITIALIZED;
    }
    else
        flags |= DDB_FLAG_INITIALIZED;
    feat_support = 0;
    feat_on = 0;
    feat_support |= DDB_FEATURE_TRANSACTIONS;
    feat_on      |= DDB_FEATURE_TRANSACTIONS;
#else
    flags |= DDB_FLAG_INITIALIZED;
#endif
    feat_support |= DDB_FEATURE_AUTOTRIM;
    connection = 0;
}

// ==================================================================================================
DdbPostgre::~DdbPostgre()
/*!
    Closes up the database connection and on Windows closes socket services.
*/
{
    if(flags & DDB_FLAG_CONNECTED)
        Disconnect();
#if defined(_WIN32) && defined(DDB_STATIC)
    // Close the Windows sockets interface.
    WSACleanup( );
#endif
}

// ==================================================================================================
bool DdbPostgre::Connect(const char *constr)
{
    // See section 30.1 in PostgreSQL manual
    // hostaddr=10.1.1.3 dbname=menacon user=app password=whatever connect_timeout=10
    // host=www.menacon.fi sslmode=require sslcert=client.crt sslkey=client.key
    if(!constr)
    {
        SetErrorId(3);
        return false;
    }
    connection = PQconnectdb(constr);

    // check to see that the backend connection was successfully made
    if (PQstatus(connection) == CONNECTION_BAD)
    {
        PQfinish(connection);
        connection = 0;
        SetErrorId(4);
        return false;
    }
    flags |= DDB_FLAG_CONNECTED;
    CS_VAPRT_INFO("Postgre client encoding id=%d",PQclientEncoding(connection));
    PQsetNoticeProcessor(connection, &DdbPQNoticeProcessor, 0);
    return true;
}

// ==================================================================================================
bool DdbPostgre::Disconnect()
{
    if(connection)
        PQfinish(connection);
    flags &= ~DDB_FLAG_CONNECTED;
    return true;
}
// ==================================================================================================
bool DdbPostgre::IsConnectOK()
{
    if(PQstatus(connection) == CONNECTION_OK)
        return true;
    return false;
}
// ==================================================================================================
bool DdbPostgre::ResetConnection()
{
    PQreset(connection);
    return IsConnectOK();
}
// ==================================================================================================
DdbRowSet* DdbPostgre::CreateRowSet()
{
    if(!(flags&DDB_FLAG_CONNECTED))
    {
        SetErrorId(5);
        return 0;
    }
    return new DdbPosgtgreRowSet(this);
}

// ==================================================================================================
DDBSTR DdbPostgre::GetErrorDescription(DdbRowSet *)
{
    DDBSTR errorMsg;

    errorMsg = GetLastError();
    if(connection)
    {
        errorMsg += "\n";
        errorMsg += PQerrorMessage(connection);
    }
    return errorMsg;
}

// ==================================================================================================
bool DdbPostgre::StartTransaction()
{
    if(!(flags&DDB_FLAG_CONNECTED))
    {
        SetErrorId(5);
        return false;
    }
    if(flags&DDB_FLAG_TRANSACT_ON)
    {
        SetErrorId(6);
        return false;
    }

    PGresult *result = PQexec(connection, "BEGIN");
    PQclear(result);

    flags |= DDB_FLAG_TRANSACT_ON;
    return true;
}

// ==================================================================================================
bool DdbPostgre::Commit()
{
    if(!(flags&DDB_FLAG_CONNECTED))
    {
        SetErrorId(5);
        return false;
    }
    if(!(flags&DDB_FLAG_TRANSACT_ON))
    {
        SetErrorId(7);
        return false;
    }

    PGresult *result = PQexec(connection, "COMMIT");
    PQclear(result);

    flags &= ~DDB_FLAG_TRANSACT_ON;
    return true;
}

// ==================================================================================================
bool DdbPostgre::RollBack()
{
    if(!(flags&DDB_FLAG_CONNECTED))
    {
        SetErrorId(5);
        return false;
    }
    if(!(flags&DDB_FLAG_TRANSACT_ON))
    {
        SetErrorId(7);
        return false;
    }

    PGresult *result = PQexec(connection, "ROLLBACK");
    PQclear(result);

    flags &= ~DDB_FLAG_TRANSACT_ON;
    return true;
}

// ==================================================================================================
bool DdbPostgre::ExecuteIntFunction(const DDBSTR &query, uint32_t &val)
{
    if(query.LENGTH()==0)
        return false;
    PGresult *result = PQexec(connection, query.UTF8());

    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        CS_PRINT_ERRO("DdbPostgre::ExecuteIntFunction failed");
        errorId = 19;
        if(result)
        {
            CS_PRINT_ERRO(PQresultErrorMessage(result));
            PQclear(result);
        }
        return false;
    }
    if(PQntuples(result)==0 || PQgetisnull(result,0,0))
    {
        PQclear(result);
        return false;
    }
    val = (uint32_t) strtol(PQgetvalue(result, 0, 0),0,0);
    PQclear(result);
    return true;
}

// ==================================================================================================
bool DdbPostgre::ExecuteLongFunction(const DDBSTR &query, uint64_t &val)
{
    if(query.LENGTH()==0)
        return false;
    PGresult *result = PQexec(connection, query.UTF8());

    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        CS_PRINT_ERRO("DdbPostgre::ExecuteLongFunction failed");
        errorId = 19;
        if(result)
        {
            CS_PRINT_ERRO(PQresultErrorMessage(result));
            PQclear(result);
        }
        return false;
    }
    if(PQntuples(result)==0 || PQgetisnull(result,0,0))
    {
        PQclear(result);
        return false;
    }
    val = (uint64_t) strtol(PQgetvalue(result, 0, 0),0,0);
    PQclear(result);
    return true;
}

// ==================================================================================================
bool DdbPostgre::ExecuteDoubleFunction(const DDBSTR &query, double &val)
{
    if(query.LENGTH()==0)
        return false;
    PGresult *result = PQexec(connection, query.UTF8());

    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        CS_PRINT_ERRO("DdbPostgre::ExecuteDoubleFunction failed");
        errorId = 19;
        if(result)
        {
            CS_PRINT_ERRO(PQresultErrorMessage(result));
            PQclear(result);
        }
        return false;
    }
    if(PQntuples(result)==0 || PQgetisnull(result,0,0))
    {
        PQclear(result);
        return false;
    }
    char* resultStr = PQgetvalue(result, 0, 0);
    if(resultStr)
    {
        val = strtod(resultStr,0);
        PQclear(result);
        return true;
    }
    PQclear(result);
    return false;
}

// ==================================================================================================
bool DdbPostgre::ExecuteBoolFunction(const DDBSTR &query, bool &val)
{
    if(query.LENGTH()==0)
        return false;
    PGresult *result = PQexec(connection, query.UTF8());

    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        CS_PRINT_ERRO("DdbPostgre::ExecuteBoolFunction failed");
        errorId = 19;
        if(result)
        {
            CS_PRINT_ERRO(PQresultErrorMessage(result));
            PQclear(result);
        }
        return false;
    }
    if(PQntuples(result)==0 || PQgetisnull(result,0,0))
    {
        PQclear(result);
        return false;
    }
    char* resultStr = PQgetvalue(result, 0, 0);
    if(resultStr)
    {
        val = resultStr[0] == 't' ? true:false;
        PQclear(result);
        return true;
    }
    PQclear(result);
    return false;
}

// ==================================================================================================
bool DdbPostgre::ExecuteStrFunction(const DDBSTR &query, DDBSTR &answer)
{
    if(query.LENGTH()==0)
        return false;
    PGresult *result = PQexec(connection, query.UTF8());
    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
        CS_PRINT_ERRO("DdbPostgre::ExecuteStrFunction failed.");
        errorId = 19;
        if(result) {
            CS_PRINT_ERRO(PQresultErrorMessage(result));
            PQclear(result);
        }
        return false;
    }
    if(PQntuples(result)==0 || PQgetisnull(result,0,0))
    {
        PQclear(result);
        return false;
    }
    errorId = 0;
    char* resultStr = PQgetvalue(result, 0, 0);
    if(resultStr) {
#ifdef DDB_USESTL
        answer = resultStr;
        if( (feat_on&DDB_FEATURE_AUTOTRIM)>0 ) {
            DirectDatabase::TrimTail(&answer);
        }
#else
        answer = wxString::FromUTF8Unchecked(resultStr);
        if( (feat_on&DDB_FEATURE_AUTOTRIM)>0 ) {
            answer.Trim();
        }
#endif
        PQclear(result);
        return true;
    }
    answer.CLEAR();
    PQclear(result);
    return false;
}

// ==================================================================================================
bool DdbPostgre::ExecuteDateFunction(const DDBSTR &query, DDBTIME &val)
{
    tm *tmPtr;
    if(query.LENGTH()==0)
        return false;
    PGresult *result = PQexec(connection, query.UTF8());

    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        CS_PRINT_ERRO("DdbPostgre::ExecuteDateFunction failed");
        errorId = 19;
        if(result)
        {
            CS_PRINT_ERRO(PQresultErrorMessage(result));
            PQclear(result);
        }
        return false;
    }
    if(PQntuples(result)==0 || PQgetisnull(result,0,0))
    {
        PQclear(result);
        return false;
    }
    char* resultStr = PQgetvalue(result, 0, 0);
    if(resultStr)
    {
#ifdef DDB_USESTL
        tmPtr = &val;
        ExtractTimestamp(resultStr,tmPtr);
#else
        tm tmData;
        tmPtr = &tmData;
        bool rv = ExtractTimestamp(resultStr,tmPtr);
        if(rv) val.Set(tmData);
#endif
        PQclear(result);
        return true;
    }
    PQclear(result);
    return false;
}

// ------------------------------------------------------------------------------------------
bool DdbPostgre::ExtractTimestamp(const char *result, struct tm *tmPtr)
{
    char *dummy;
    unsigned int len = strlen(result);
    memset(tmPtr,0,sizeof(tm));
    if(len>=10) {
        tmPtr->tm_year = strtol(result,&dummy,10)-1900;
        tmPtr->tm_mon  = strtol(result+5,&dummy,10)-1;
        tmPtr->tm_mday = strtol(result+8,&dummy,10);
        if(len >=19)
        {
            tmPtr->tm_hour = strtol(result+11,&dummy,10);
            tmPtr->tm_min  = strtol(result+14,&dummy,10);
            tmPtr->tm_sec  = strtol(result+17,&dummy,10);
            tmPtr->tm_isdst = -1;
        }
        return true;
    }
    CS_PRINT_NOTE("DdbPostgre::ExtractDate - Empty date detected.");
    return false;
}


// ==================================================================================================
int DdbPostgre::ExecuteModify(const DDBSTR &modify)
{
    if(modify.LENGTH()==0)
        return -1;
    PGresult *result = PQexec(connection, modify.UTF8());
    if (!result || PQresultStatus(result) != PGRES_COMMAND_OK)
    {
        CS_VAPRT_ERRO("DdbPostgre::ExecuteModify - Failed. PQStatus=%d",PQresultStatus(result));
        if(result)
        {
            CS_PRINT_ERRO(PQresultErrorMessage(result));
            PQclear(result);
        }
        return -1;
    }

    char *resultStr = PQcmdTuples(result);
    int retval = strtol(resultStr,0,10);
    PQclear(result);
    return retval;
}

// ==================================================================================================
bool DdbPostgre::UpdateStructure(const DDBSTR &command)
{
    if(command.LENGTH()==0)
        return false;
    PGresult *result = PQexec(connection, command.UTF8());

    if (!result || PQresultStatus(result) != PGRES_COMMAND_OK)
    {
        CS_PRINT_ERRO("DdbPostgre::UpdateStructure failed");
        if(result)
        {
            CS_PRINT_ERRO(PQresultErrorMessage(result));
            PQclear(result);
        }
        return false;
    }
    return true;
}

// ==================================================================================================
unsigned long DdbPostgre::GetInsertId()
{
    PGresult *result = PQexec(connection,"SELECT lastval()");
    if (!result) {
        CS_PRINT_WARN("DdbPostgre::GetInsertId - unable to get result.");
        return 0;
    }
    if(PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        CS_VAPRT_ERRO("DdbPostgre::GetInsertId failure:%s",PQresultErrorMessage(result));
        PQclear(result);
        return 0;
    }
    char *endptr;
    char* resultStr = PQgetvalue(result, 0, 0);
    errno = 0;
    unsigned long rv = strtoul(resultStr,&endptr,10);
    if(errno)
        return 0;
    return rv;
}

// ==========================================================================================
// $$$$ ADMIN COMMANDS $$$
// ------------------------------------------------------------------------------------------
bool DdbPostgre::CreateUser(const DDBSTR &uid, const DDBSTR &pwd)
{
    char query[100];
    uint32_t oid;
    sprintf(query,"SELECT oid FROM pg_roles WHERE rolname='%s'",uid.c_str());
    if(!ExecuteIntFunction(query,oid)) {
        if(pwd.empty())
            sprintf(query,"CREATE ROLE %s CREATEROLE LOGIN",uid.c_str());
        else
            sprintf(query,"CREATE ROLE %s CREATEROLE LOGIN ENCRYPTED PASSWORD '%s'",uid.c_str(),pwd.c_str());
        if(!UpdateStructure(query)) {
            CS_VAPRT_ERRO("Unable to create non-existing user '%s'",uid.c_str());
            return false;
        }
        CS_VAPRT_INFO("DdbPostgre::CreateUser - created role %s",uid.c_str());
    }
    return true;
}
// ------------------------------------------------------------------------------------------
bool DdbPostgre::CreateDatabase(const DDBSTR &dbname, const DDBSTR &owner)
{
    char query[100];
    DDBSTR rsname;
    // Check the existence first.
    DdbRowSet *rs = CreateRowSet();
    rs->Bind(DDBT_STR,&rsname);
    rs->Query("SELECT datname FROM pg_database");
    while(rs->GetNext()) {
        if(!rsname.compare(dbname))
            return true;
    }
    // Create database
    sprintf(query,"CREATE DATABASE %s OWNER=%s",dbname.c_str(), owner.c_str());
    if(!UpdateStructure(query)) {
        CS_VAPRT_ERRO("Unable to create %s database.",dbname.c_str());
        return false;
    }
    CS_VAPRT_INFO("Database %s created.",dbname.c_str());
    return true;
}

// ------------------------------------------------------------------------------------------
bool DdbPostgre::CreateTable(const DDBSTR &name, const DDBSTR &sql)
{
    DDBSTR rstbl;
    char query[100];
    sprintf(query,"SELECT relname FROM pg_class WHERE relname='%s' AND relkind='r'",name.c_str());
    if(!ExecuteStrFunction(query,rstbl)) {
        if(errorId>0) return false;
        if(!UpdateStructure(sql)) return false;
    }
    return true;
}
