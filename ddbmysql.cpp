/*******************************************************************************
ddbmysql.cpp
Copyright (c) Antti Merenluoto
*******************************************************************************/

#ifdef WIN32
    #include <windows.h>
#endif
#ifdef __linux
    #include <string.h>
#endif

#ifdef DDB_USEWX
  #include <wx/wxprec.h>
  #ifndef WX_PRECOMP
    #include <wx/wx.h>
  #endif
#endif
#define __DDB_MYSQL__
#include "directdatabase.hpp"

// ==================================================================================================
DdbMySql::DdbMySql()
/*!
  Constructs database object for the connection to the PostgreSQL databases.
  In Windows environment this intitializes sockets. On Linux the only the
  initialization flag is simply set to true.
*/
{
    mysql_init(&connection);
    flags = 0;
    feature = DDB_FEATURE_TRANSACTIONS;
}

// ==================================================================================================
DdbMySql::~DdbMySql()
/*!
    Closes up the database connection and on Windows closes socket services.
*/
{
    if( (flags & DDB_FLAG_CONNECTED) > 0)
        Disconnect();
}

// ==================================================================================================
bool DdbMySql::Connect(int timeout)
{
#ifdef DDB_USESTL
    char const *srv  = srvName.length()==0 ? 0 : srvName.data();
    char const *db   = dbName.length()==0 ? 0 : dbName.data();
    char const *user = userid.length()==0 ? 0 : userid.data();
    char const *pass = pwd.length()==0 ? 0 : pwd.data();
#else
    char const *srv  = srvName.IsEmpty() ? 0 : srvName.GetData();
    char const *db   = dbName.IsEmpty() ? 0 : dbName.GetData();
    char const *user = userid.IsEmpty() ? 0 : userid.GetData();
    char const *pass = pwd.IsEmpty() ? 0 : pwd.GetData();
#endif
    if(!(flags&DDB_FLAG_INITIALIZED))
    {
        SetErrorId(11);
        return false;
    }
    if(!srv || !db)
    {
        SetErrorId(3);
        return false;
    }

    unsigned int to = timeout;
    mysql_options(&connection, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&to);

    if(!mysql_real_connect(&connection, srv, user, pass, db, port, 0, 0))
    {
        ostringstream ss;
        SetErrorId(4);
        ss << "DdbMySql::Connect - " << mysql_error(&connection);
        Log(ss);
        return false;
    }
    flags |= DDB_FLAG_CONNECTED;
    return true;
}

// ==================================================================================================
bool DdbMySql::Disconnect()
{
    if( (flags&DDB_FLAG_CONNECTED) > 0)
        mysql_close(&connection);
    flags &= ~DDB_FLAG_CONNECTED;
    return true;
}

// ==================================================================================================
DdbRowSet* DdbMySql::CreateRowSet()
{
    if(!(flags&DDB_FLAG_CONNECTED))
    {
        SetErrorId(5);
        return 0;
    }
    return new DdbMySqlRowSet(this);
}

// ==================================================================================================
DDBSTR DdbMySql::GetErrorDescription(DdbRowSet *)
{
    DDBSTR errorMsg;

    errorMsg = GetLastError();
    if((flags&DDB_FLAG_CONNECTED)>0)
    {
        errorMsg += "\n";
        errorMsg += mysql_error(&connection);
    }
    return errorMsg;
}

// ==================================================================================================
bool DdbMySql::StartTransaction()
{
    if(!(flags&DDB_FLAG_CONNECTED))
    {
        SetErrorId(5);
        return false;
    }
    if((flags&DDB_FLAG_TRANSACT_ON)>0)
    {
        SetErrorId(6);
        return false;
    }

    if(mysql_real_query(&connection, "START TRANSACTION", 17))
    {
        ostringstream ss;
        ss << "DdbMySql::StartTransaction - failure: " << mysql_error(&connection) << endl;
        Log(ss);
        return false;
    }
    flags |= DDB_FLAG_TRANSACT_ON;
    return true;
}

// ==================================================================================================
bool DdbMySql::Commit()
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

    if(!mysql_commit(&connection))
    {
        flags &= ~DDB_FLAG_TRANSACT_ON;
        return true;
    }
    ostringstream ss;
    ss << "DdbMySql::Commit - failure: " << mysql_error(&connection) << endl;
    Log(ss);
    return false;
}

// ==================================================================================================
bool DdbMySql::RollBack()
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

    if(mysql_rollback(&connection))
    {
        flags &= ~DDB_FLAG_TRANSACT_ON;
        return true;
    }
    ostringstream ss;
    ss << "DdbMySql::RollBack - failure: " << mysql_error(&connection) << endl;
    Log(ss);
    return false;
}

// ==================================================================================================
bool DdbMySql::ExecuteFunction(const DDBSTR &query, stringstream &ss)
/*!
  Most execute functions differ only by how the result string is translated to C/C++ datatype.
  Common query logic for all most execute functions is here and the individual functions
  take care of parsing the result.
  \param query Query to perform
  \param ss Stream where the result will be stored in.
  \retval bool True on succes, false on error.
*/
{
    if(query.LENGTH()==0)
        return false;

    if(mysql_real_query(&connection, query.DATA(), query.LENGTH()))
    {
        ostringstream ss;
        ss << "DdbMySql::ExecuteFunction - " << query.DATA() << endl;
        ss << mysql_error(&connection);
        Log(ss);
        return false;
    }
    MYSQL_RES *result = mysql_store_result(&connection);
    if(!result)
    {
        ostringstream ss;
        ss << "DdbMySql::ExecuteFunction - mysql_store_result failure" << endl;
        ss << mysql_error(&connection);
        Log(ss);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if(row && row[0])
    {
        ss.str(row[0]);
        mysql_free_result(result);
        return true;
    }
    mysql_free_result(result);
    return false;
}

// ==================================================================================================
bool DdbMySql::ExecuteIntFunction(const DDBSTR &query, int &val)
{
    stringstream ss;
    if(!ExecuteFunction(query,ss))
        return false;
    ss >> val;
    return true;
}

// ==================================================================================================
bool DdbMySql::ExecuteDoubleFunction(const DDBSTR &query, double &val)
{
    stringstream ss;
    if(!ExecuteFunction(query,ss))
        return false;
    ss >> val;
    return true;
}

// ==================================================================================================
bool DdbMySql::ExecuteBoolFunction(const DDBSTR &query, bool &val)
{
    int boolval;
    stringstream ss;
    if(!ExecuteFunction(query,ss))
        return false;
    ss >> boolval;
    val = boolval ? true:false;
    return true;
}

// ==================================================================================================
bool DdbMySql::ExecuteStrFunction(const DDBSTR &query, DDBSTR &answer)
{
    stringstream ss;
    if(!ExecuteFunction(query,ss))
        return false;
    answer = ss.str().c_str();
    return true;
}

// ==================================================================================================
bool DdbMySql::ExecuteDateFunction(const DDBSTR &query, DDBTIME &val)
{
    tm *tmPtr;
    char separator;
    stringstream ss;

    if(!ExecuteFunction(query,ss))
        return false;

#ifdef DDB_USESTL
    tmPtr = &val;
#else
    tm tmData;
    tmPtr = &tmData;
#endif
    memset(tmPtr,0,sizeof(tm));
    ss >> tmPtr->tm_year >> separator;
    ss >> tmPtr->tm_mon  >> separator;
    ss >> tmPtr->tm_mday;
    if(ss.str().length() > 10)
    {
        ss >> tmPtr->tm_hour >> separator;
        ss >> tmPtr->tm_min >> separator;
        ss >> tmPtr->tm_sec;
        tmPtr->tm_isdst = -1;
    }
#ifdef DDB_USEWX
    val.Set(tmData);
#endif
    return true;
}


// ==================================================================================================
int DdbMySql::ExecuteModify(const DDBSTR &modify)
{
    if(modify.LENGTH()==0)
        return -1;

    if(mysql_real_query(&connection, modify.DATA(), modify.LENGTH()))
    {
        errorId = 18;
        ostringstream ss;
        ss << "DdbMySql::ExecuteModify - " << modify.DATA() << endl;
        ss << mysql_error(&connection);
        Log(ss);
        return -1;
    }
    return (int) mysql_affected_rows(&connection);
}

// ==================================================================================================
bool DdbMySql::UpdateStructure(const DDBSTR &command)
{
    if(command.LENGTH()==0)
        return false;

    if(mysql_real_query(&connection, command.DATA(), command.LENGTH()))
    {
        errorId = 21;
        ostringstream ss;
        ss << "DdbMySql::UpdateStructure - " << command.DATA() << endl;
        ss << mysql_error(&connection);
        Log(ss);
        return false;
    }
    return true;
}

// ==================================================================================================
unsigned long DdbMySql::GetInsertId()
{
    if( (flags & DDB_FLAG_CONNECTED) > 0)
        return (unsigned long) mysql_insert_id(&connection);
    return 0;
}
