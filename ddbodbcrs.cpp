/*******************************************************************************
ddbodbcrs.cpp
Copyright (c) Antti Merenluoto
*******************************************************************************/

#ifdef __linux__
    #error Not supported currently.
#endif
#include <stdlib.h>
#ifdef DDB_USEWX
  #include <wx/wxprec.h>
  #ifndef WX_PRECOMP
    #include <wx/wx.h>
  #endif
#include "wx/datetime.h"
#endif
#define __DDB_ODBCWIN__
#include "directdatabase.hpp"

// ==================================================================================================
DdbOdbcRowSet::DdbOdbcRowSet(DirectDatabase *db_in)
    :DdbRowSet()
/*!
    Initializes member variables to default values.
    \param db_in Pointer to database object.
*/
{
    currentRow = 0;
    bindIndex = 1;
    resultCleared = true;

    db = static_cast<DdbOdbc*>(db_in);
    SQLAllocHandle(SQL_HANDLE_STMT,db->GetCon(),&hStmt);
}

// ==================================================================================================
DdbOdbcRowSet::~DdbOdbcRowSet()
/*!
    Relases the statement handle
*/
{
    if(!resultCleared)
        SQLCloseCursor(hStmt);
    if(hStmt != SQL_NULL_HANDLE)
        SQLFreeHandle(SQL_HANDLE_STMT,hStmt);
}

// ==================================================================================================
bool DdbOdbcRowSet::Query(const DDBSTR &query)
{
    SQLSMALLINT maxFields;

    if(!fieldRoot)
    {
        db->SetErrorId(9);
        return  false;
    }
    queryStmt = query;
    if(queryStmt.LENGTH()==0)
        return false;
    if(!resultCleared)
        SQLCloseCursor(hStmt);

    SQLRETURN sqlrv = SQLExecDirect(hStmt,(SQLCHAR*)query.DATA(),query.LENGTH());
    if(!SQLSUCCESS(sqlrv))
    {
        ostringstream ss;
        db->SetErrorId(8);
        ss << "DdbOdbc::Query - SQLExecDirect failure " << sqlrv << endl << db->GetErrorDescription(this).DATA();
        db->Log(ss);
        resultCleared = true;
        return false;
    }

    // If result is empty:
    sqlrv = SQLNumResultCols(hStmt,&maxFields);
    if(!SQLSUCCESS(sqlrv) || !maxFields)
    {
        // Close the cursor
        SQLCloseCursor(hStmt);
        resultCleared = true;
        return false;
    }

    resultCleared = false;
    currentRow = 0;
    return true;
}

// ==================================================================================================
int DdbOdbcRowSet::GetNext()
{
    DdbBoundField *field;
    int fieldIndex;
    char *str,boolChar;
    SQLINTEGER cb;
    SQLRETURN sqlrv;
    TIMESTAMP_STRUCT timestamp;
    DATE_STRUCT date;
    tm tmtime;

    if(resultCleared)
        return 0;

    sqlrv = SQLFetch(hStmt);
    if(sqlrv == SQL_NO_DATA)
    {
        SQLCloseCursor(hStmt);
        resultCleared = true;
        return 0;
    }
    if(!SQLSUCCESS(sqlrv))
    {
        SQLCloseCursor(hStmt);
        resultCleared = true;
        db->SetErrorId(20);
        return 0;
    }

    field = fieldRoot;
    fieldIndex = 1;
    while(field)
    {
        // Use type to convert the data. DDB_TYPE_USED
        switch(field->type)
        {
        case DDBT_INT:
            SQLGetData(hStmt,fieldIndex,SQL_C_SLONG,field->data,0,&cb);
            break;
        case DDBT_STR:
            sqlrv = SQLGetData(hStmt,fieldIndex,SQL_C_CHAR,&boolChar,0,&cb);
            if(!SQLSUCCESS(sqlrv))
            {
                ostringstream error;
                error << db->GetErrorDescription(this).DATA();
                db->Log(error);
            }
            else if(cb != SQL_NULL_DATA && cb != SQL_NO_TOTAL)
            {
#ifdef DDB_USESTL
                str = new char(cb+1);
                SQLGetData(hStmt,fieldIndex,SQL_C_CHAR,str,cb+1,&cb);
                static_cast<DDBSTR*>(field->data)->assign(str);
                delete[] str;
#else
                str = static_cast<DDBSTR*>(field->data)->GetWriteBuf(cb+1);
                SQLGetData(hStmt,fieldIndex,SQL_C_CHAR,str,cb+1,&cb);
                static_cast<DDBSTR*>(field->data)->UngetWriteBuf();
                static_cast<DDBSTR*>(field->data)->Trim();
#endif
            }
            else
#ifdef DDB_USESTL
                static_cast<DDBSTR*>(field->data)->erase();
#else
                static_cast<DDBSTR*>(field->data)->Empty();
#endif
            break;
        case DDBT_BOOL:
            SQLGetData(hStmt,fieldIndex,SQL_C_CHAR,&boolChar,1,&cb);
            *(static_cast<bool*>(field->data)) = boolChar == 't'? true : false;
            break;
        case DDBT_TIME:
            memset(&tmtime,0,sizeof(tm));
            SQLGetData(hStmt,fieldIndex,SQL_C_TYPE_TIMESTAMP,&timestamp,sizeof(TIMESTAMP_STRUCT),&cb);
            tmtime.tm_year = timestamp.year-1900;
            tmtime.tm_mon  = timestamp.month-1;
            tmtime.tm_mday = timestamp.day;
            tmtime.tm_hour = timestamp.hour;
            tmtime.tm_min  = timestamp.minute;
            tmtime.tm_sec  = timestamp.second;
            tmtime.tm_isdst = -1;
#ifdef DDB_USESTL
            memcpy(field->data,&tmtime,sizeof(tmtime));
#else
            static_cast<DDBTIME*>(field->data)->Set(tmtime);
#endif
            break;
        case DDBT_DAY:
            memset(&tmtime,0,sizeof(tm));
            SQLGetData(hStmt,fieldIndex,SQL_C_TYPE_DATE,&date,sizeof(DATE_STRUCT),&cb);
            tmtime.tm_year = date.year-1900;
            tmtime.tm_mon  = date.month-1;
            tmtime.tm_mday = date.day;
            tmtime.tm_isdst = -1;
#ifdef DDB_USESTL
            memcpy(field->data,&tmtime,sizeof(tmtime));
#else
            static_cast<DDBTIME*>(field->data)->Set(tmtime);
#endif
            break;
        case DDBT_NUM:
            SQLGetData(hStmt,fieldIndex,SQL_C_DOUBLE,field->data,0,&cb);
            break;
        case DDBT_CHR:
            SQLGetData(hStmt,fieldIndex,SQL_C_STINYINT,field->data,1,&cb);
            break;
        }

        field = field->next;
        fieldIndex++;
    }

    return ++currentRow;
}
