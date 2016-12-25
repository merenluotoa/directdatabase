/*******************************************************************************
ddbodbc.cpp
Copyright (c) Antti Merenluoto
*******************************************************************************/

#ifdef __linux__
    #error Not supported currently.
#endif

#include "pch-stop.h"
#ifdef DDB_USEWX
#include <wx/wxprec.h>
  #ifndef WX_PRECOMP
    #include <wx/wx.h>
  #endif
#endif
#define __DDB_ODBCWIN__
#include "directdatabase.hpp"

// ==================================================================================================
DdbOdbc::DdbOdbc()
/*!
  Constructs database object for the connection to the ODBC databases.
  Please note that 3.x specification is followed.
*/
{
    hConnection = SQL_NULL_HANDLE;
    execStmt = SQL_NULL_HANDLE;
    flags = 0;
    feature = 0;

    SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&hEnvironment);
    SQLSetEnvAttr(hEnvironment,SQL_ATTR_ODBC_VERSION,(SQLPOINTER)SQL_OV_ODBC3,0);
}

// ==================================================================================================
DdbOdbc::~DdbOdbc()
/*!
    Closes up the database connection and on Windows closes socket services.
*/
{
    if(flags & DDB_FLAG_CONNECTED)
        Disconnect();
    SQLFreeHandle(SQL_HANDLE_ENV,hEnvironment);
}

// ==================================================================================================
bool DdbOdbc::Connect(int timeout)
{
    char connectStr[1024],connectOut[1024];
    SQLCHAR *sqlUser,*sqlPwd;
    SQLSMALLINT sqlUserLen, sqlPwdLen,conOutLen;
    SQLRETURN retval;
    ostringstream ss;

    // Safety checks
    if(!(flags&DDB_FLAG_INITIALIZED))
    {
        SetErrorId(11);
        return false;
    }
    if(srvName.LENGTH()==0 || dbName.LENGTH()==0)
    {
        SetErrorId(3);
        return false;
    }

    if(flags & DDB_FLAG_CONNECTED)
        Disconnect();

    // Allocate connection hanle
    retval = SQLAllocHandle(SQL_HANDLE_DBC,hEnvironment,&hConnection);
    if(!SQLSUCCESS(retval))
    {
        ss << "wxDdbOdbc::Connect - SQLAllocHandle failure " <<retval;
        Log(ss);
        return false;
    }
    // Set the timeout if user so desired.
    if(timeout)
    {
        SQLUINTEGER sqlTO=timeout;
        retval = SQLSetConnectAttr(hConnection,SQL_ATTR_CONNECTION_TIMEOUT,&sqlTO,0);
        if(!SQLSUCCESS(retval))
            ss << "DdbOdbc::Connect - Set connection timeout failure " << retval;
        retval = SQLSetConnectAttr(hConnection,SQL_ATTR_LOGIN_TIMEOUT,&sqlTO,0);
        if(!SQLSUCCESS(retval))
            ss << "DdbOdbc::Connect - Set connection login failure " << retval;
        Log(ss);
        return false;
    }

    bool dsnUsed = srvName == "DSN" ? true:false;
    if(dsnUsed)
        strcpy(connectStr,dbName.DATA());
    else
        sprintf(connectStr,"DRIVER={%s};DBQ=%s;",srvName.DATA(),dbName.DATA());

    // Get user id and the password
    if(userid.LENGTH()==0)
    {
        sqlUser = 0;
        sqlUserLen = 0;
    }
    else
    {
        sqlUser = (SQLCHAR*) userid.DATA();
        sqlUserLen = (SQLSMALLINT) userid.LENGTH();
        if(!dsnUsed)
        {
            strcat(connectStr,"UID=");
            strcat(connectStr,(char*)sqlUser);
            strcat(connectStr,";");
        }
    }
    if(pwd.LENGTH()==0)
    {
        sqlPwd = 0;
        sqlPwdLen = 0;
    }
    else
    {
        sqlPwd = (SQLCHAR*)pwd.DATA();
        sqlPwdLen = (SQLSMALLINT) pwd.LENGTH();
        if(!dsnUsed)
        {
            strcat(connectStr,"PWD=");
            strcat(connectStr,(char*)sqlPwd);
            strcat(connectStr,";");
        }
    }

    // Finally, the connection
    if(dsnUsed)
        retval = SQLConnect(hConnection,(SQLCHAR*)connectStr,strlen(connectStr),sqlUser,sqlUserLen,sqlPwd,sqlPwdLen);
    else
        retval = SQLDriverConnect(hConnection,0,(SQLCHAR*)connectStr,strlen(connectStr),
                                  (SQLCHAR*)connectOut,sizeof(connectOut),&conOutLen,SQL_DRIVER_NOPROMPT);

    if(!SQLSUCCESS(retval))
    {
        SetErrorId(4);
        ostringstream errorStr;
        errorStr << GetErrorDescription(0).DATA();
        Log(errorStr);
        SQLFreeHandle(SQL_HANDLE_DBC,hConnection);
        return false;
    }
    // Allocate statement handle for Exec-functions
    SQLAllocHandle(SQL_HANDLE_STMT,hConnection,&execStmt);

    flags |= DDB_FLAG_CONNECTED;
    return true;
}

// ==================================================================================================
bool DdbOdbc::Disconnect()
{
    if(execStmt != SQL_NULL_HANDLE)
        SQLFreeHandle(SQL_HANDLE_STMT,execStmt);

    SQLRETURN retval = SQLDisconnect(hConnection);
    if(!SQLSUCCESS(retval))
    {
        ostringstream ss;
        ss << "DdbOdbc::Disconnect - SQLDisconnect failure " << retval;
        Log(ss);
        return false;
    }

    if(hConnection != SQL_NULL_HANDLE)
        SQLFreeHandle(SQL_HANDLE_DBC,hConnection);

    flags &= ~DDB_FLAG_CONNECTED;
    return true;
}

// ==================================================================================================
DdbRowSet* DdbOdbc::CreateRowSet()
{
    if(!(flags&DDB_FLAG_CONNECTED))
    {
        SetErrorId(5);
        return 0;
    }
    return new DdbOdbcRowSet(this);
}

// ==================================================================================================
DDBSTR DdbOdbc::GetErrorDescription(DdbRowSet *rs)
{
    SQLCHAR SqlState[6], msg[SQL_MAX_MESSAGE_LENGTH];
    SQLINTEGER NativeError;
    SQLSMALLINT i, MsgLen,handleType;
    SQLRETURN rc;
    DDBSTR errorMsg;
    SQLHANDLE handle;

    errorMsg = GetLastError();
    if(hConnection != SQL_NULL_HANDLE)
    {
        errorMsg += "\n";
        if(rs)
        {
            handleType = SQL_HANDLE_STMT;
            handle     = static_cast<DdbOdbcRowSet*>(rs)->hStmt;
        }
        else if(execStmt == SQL_NULL_HANDLE)
        {
            handleType = SQL_HANDLE_DBC;
            handle     = hConnection;
        }
        else
        {
            handleType = SQL_HANDLE_STMT;
            handle     = execStmt;
        }
        i = 1;
        while ((rc = SQLGetDiagRec(handleType, handle, i, SqlState, &NativeError,
                                    msg, sizeof(msg), &MsgLen)) != SQL_NO_DATA)
        {
            errorMsg += (char*)msg;
            errorMsg += "\n";
            i++;
        }
    }
    return errorMsg;
}

// ==================================================================================================
bool DdbOdbc::StartTransaction()
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

    return false;
}

// ==================================================================================================
bool DdbOdbc::Commit()
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

    return false;
}

// ==================================================================================================
bool DdbOdbc::RollBack()
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

    return false;
}

// ==================================================================================================
bool DdbOdbc::ExecuteIntFunction(const DDBSTR &query, int &val)
{
    ostringstream ss;
    int retval;
    SQLRETURN sqlrv;
    SQLINTEGER cbint=0;

    if(query.LENGTH()==0 || execStmt == SQL_NULL_HANDLE)
        return false;

    sqlrv = SQLBindCol(execStmt,1,SQL_C_SLONG,&retval,0,&cbint);
    if(!SQLSUCCESS(sqlrv))
    {
        ss << "DdbOdbc::ExecuteIntFunction - SQLBindCol failure " << sqlrv;
        ss << endl << GetErrorDescription(0);
        Log(ss);
        errorId = 19;
        return false;
    }
    sqlrv = SQLExecDirect(execStmt,(SQLCHAR*)query.DATA(),SQL_NTS);
    if(!SQLSUCCESS(sqlrv))
    {
        ss << "DdbOdbc::ExecuteIntFunction - SQLExecDirect failure " << sqlrv << endl << query.DATA();
        Log(ss);
        goto ODBC_EXE_INT_ERROR;
    }
    sqlrv = SQLFetch(execStmt);
    if(sqlrv != SQL_NO_DATA && !SQLSUCCESS(sqlrv))
    {
        ss << "DdbOdbc::ExecuteIntFunction - SQLFetch failure " << sqlrv;
        Log(ss);
        goto ODBC_EXE_INT_ERROR;
    }
    FreeExecStmt();
    if(sqlrv == SQL_NO_DATA)
        return false;
    val = retval;
    return true;

 ODBC_EXE_INT_ERROR:
    ostringstream errorStr;
    errorId = 19;
    errorStr << GetErrorDescription(0).DATA();
    Log(errorStr);
    FreeExecStmt();
    return false;
}

// ==================================================================================================
bool DdbOdbc::ExecuteDoubleFunction(const DDBSTR &query, double &val)
{
    ostringstream ss;
    double retval;
    SQLRETURN sqlrv;
    SQLINTEGER cb=0;

    if(query.LENGTH()==0 || execStmt == SQL_NULL_HANDLE)
        return false;

    sqlrv = SQLBindCol(execStmt,1,SQL_C_DOUBLE,&retval,0,&cb);
    if(!SQLSUCCESS(sqlrv))
    {
        ss << "DdbOdbc::ExecuteDoubleFunction - SQLBindCol failure " << sqlrv;
        ss << endl << GetErrorDescription(0).DATA();
        Log(ss);
        errorId = 19;
        return false;
    }
    sqlrv = SQLExecDirect(execStmt,(SQLCHAR*)query.DATA(),SQL_NTS);
    if(!SQLSUCCESS(sqlrv))
    {
        ss << "DdbOdbc::ExecuteDoubleFunction - SQLExecDirect failure " << sqlrv << endl << query.DATA();
        Log(ss);
        goto ODBC_EXE_DBL_ERROR;
    }
    sqlrv = SQLFetch(execStmt);
    if(sqlrv != SQL_NO_DATA && !SQLSUCCESS(sqlrv))
    {
        ss << "DdbOdbc::ExecuteDoubleFunction - SQLFetch failure " << sqlrv;
        Log(ss);
        goto ODBC_EXE_DBL_ERROR;
    }
    FreeExecStmt();
    if(sqlrv == SQL_NO_DATA)
        return false;
    val = retval;
    return true;

 ODBC_EXE_DBL_ERROR:
    ostringstream errorStr;
    errorId = 19;
    errorStr << GetErrorDescription(0).DATA();
    Log(errorStr);
    FreeExecStmt();
    return false;
}

// ==================================================================================================
bool DdbOdbc::ExecuteStrFunction(const DDBSTR &query, DDBSTR &answer)
{
    SQLRETURN sqlrv;
    SQLINTEGER resLen=0, cb=0;
    char *str=0, buffer[1024];
    ostringstream ss;

    if(!query.LENGTH())
        return false;

    if(query.LENGTH()==0 || execStmt == SQL_NULL_HANDLE)
        return false;

    sqlrv = SQLExecDirect(execStmt,(SQLCHAR*)query.DATA(),SQL_NTS);
    if(!SQLSUCCESS(sqlrv))
    {
        ss << "DdbOdbc::ExecuteStrFunction - SQLExecDirect failure " << sqlrv << endl << query.DATA();
        Log(ss);
        goto ODBC_EXE_STR_ERROR;
    }

    SQLColAttribute (execStmt,1,SQL_DESC_DISPLAY_SIZE,0,0,0,&resLen);
    if(resLen >= 1024)
        str = new char[resLen+1];
    else
        str = buffer;
    sqlrv = SQLBindCol(execStmt,1,SQL_C_CHAR,str,resLen,&cb);
    if(!SQLSUCCESS(sqlrv))
    {
        ss << "DdbOdbc::ExecuteStrFunction - SQLBindCol failure " << sqlrv;
        Log(ss);
        goto ODBC_EXE_STR_ERROR;
    }
    sqlrv = SQLFetch(execStmt);
    answer = str;

    if(resLen >= 1024 && str)
        delete[] str;
    FreeExecStmt();

    if(sqlrv == SQL_NO_DATA)
        return false;
    if(!SQLSUCCESS(sqlrv))
    {
        ss << "DdbOdbc::ExecuteStrFunction - SQLFetch failure " << sqlrv;
        Log(ss);
        return false;
    }
    return true;

ODBC_EXE_STR_ERROR:
    ostringstream  errorStr;
    errorId = 19;
    errorStr << GetErrorDescription(0).DATA();
    Log(errorStr);
    if(resLen >= 1024 && str)
        delete[] str;
    return false;
}

// ==================================================================================================
bool DdbOdbc::ExecuteDateFunction(const DDBSTR &, DDBTIME &)
{
    ostringstream ss;
    ss << "DdbOdbc::ExecuteDateFunction - NOT implemented";
    Log(ss);
    return false;
}

// ==================================================================================================
bool DdbOdbc::ExecuteBoolFunction(const DDBSTR &, bool &)
{
    ostringstream ss;
    ss << "DdbOdbc::ExecuteDateFunction - NOT implemented";
    Log(ss);
    return false;
}

// ==================================================================================================
int DdbOdbc::ExecuteModify(const DDBSTR &modify)
{
    SQLINTEGER rowCount;
    SQLRETURN sqlrv;
    ostringstream ss;

    if(modify.LENGTH()==0)
        return 0;

    sqlrv = SQLExecDirect(execStmt,(SQLCHAR*)modify.DATA(),SQL_NTS);
    if(sqlrv == SQL_NO_DATA)
        return 0;
    if(!SQLSUCCESS(sqlrv))
    {
        ss << "DdbOdbc::ExecuteModify - SQLExecDirect failure"<<endl<<modify.DATA();
        ss << endl << GetErrorDescription(0).DATA();
        Log(ss);
        errorId = 18;
        return -1;
    }
    sqlrv = SQLRowCount(execStmt,&rowCount);
    if(!SQLSUCCESS(sqlrv))
    {
        ss << "DdbOdbc::ExecuteModify - SQLRowCount failure";
        Log(ss);
        rowCount = -1;
    }

    return rowCount;
}

// ==================================================================================================
bool DdbOdbc::UpdateStructure(const DDBSTR &command)
{
    SQLRETURN sqlrv;
    ostringstream ss;

    if(command.LENGTH()==0)
        return false;

    sqlrv = SQLExecDirect(execStmt,(SQLCHAR*)command.DATA(),SQL_NTS);
    if(sqlrv == SQL_NO_DATA)
        return false;
    if(!SQLSUCCESS(sqlrv))
    {
        ss << "DdbOdbc::UpdateStructure - SQLExecDirect failure"<<endl<<command.DATA();
        ss << endl << GetErrorDescription(0).DATA();
        Log(ss);
        errorId = 21;
        return false;
    }
    return true;
}

// ==================================================================================================
void DdbOdbc::FreeExecStmt()
/*!
  Closes the cursor and unbinds exeStmt.
*/
{
    ostringstream ss;
    SQLRETURN sqlrv = SQLCloseCursor(execStmt);
    if(!SQLSUCCESS(sqlrv))
    {
        ss << "DdbOdbc::FreeExecStmt - SQLCloseCursor failure " << sqlrv;
        Log(ss);
    }
    sqlrv = SQLFreeStmt(execStmt,SQL_UNBIND);
    if(!SQLSUCCESS(sqlrv))
    {
        ss << "DdbOdbc::FreeExecStmt - SQLFreeStmt failure " << sqlrv;
        Log(ss);
    }
}
