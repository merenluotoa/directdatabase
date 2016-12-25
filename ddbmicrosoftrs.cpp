/*******************************************************************************
ddbmicrosoftrs.cpp
Copyright (c) Antti Merenluoto
*******************************************************************************/

#ifdef WIN32
#include <windows.h>
#define	DBNTWIN32		// must identify operating system environment
#include <sqlfront.h>
#include <sqldb.h>

#ifdef DDB_USEWX
#include "wx/wxprec.h"
#include "wx/datetime.h"
#endif
#include "directdatabase.hpp"

// -----------------------------------------------------------------------------
DdbMicrosoftRowSet::DdbMicrosoftRowSet(DirectDatabase *db_in)
    :DdbRowSet()
/*!
  Initializes member variables to default values.

  \param db Pointer to database object.
*/
{
    maxRows = 0;
    currentRow = 0;

    db = (DdbMicrosoft*) db_in;
}

// -----------------------------------------------------------------------------
DdbMicrosoftRowSet::~DdbMicrosoftRowSet()
/*!
    Relases the row set connection if it still exists.
*/
{
    dbcancel(db->GetMSConn());
}

// -----------------------------------------------------------------------------
bool DdbMicrosoftRowSet::Query(const DDBSTR &query)
{
RETCODE result;

    currentRow = 0;

    queryStmt = query;
    if(queryStmt.IsEmpty())
        return false;
    if(!fieldRoot)
    {
        db->SetErrorId(9);
        return  false;
    }

    dbcmd(db->GetMSConn(),queryStmt.GetData());
    dbsqlexec(db->GetMSConn());
    result = dbresults(db->GetMSConn());
    if(result != SUCCEED)
    {
        db->SetErrorId(8);
        dbcancel(db->GetMSConn());
        return false;
    }

    if(DdbMicrosoftBind())
        return false;

    return true;
}

// -----------------------------------------------------------------------------
int DdbMicrosoftRowSet::DdbMicrosoftBind()
/*!
    Binds variables in Microsoft style. The bind-function
    should have been called before this is called. Note! Query calls this
    function automatically.
    \retval Zero on success, DirectDB error code on error.
*/
{
DdbBoundField *field;
int i = 1, vartype;
RETCODE ret;
LPBYTE  dataPtr;

    if(!fieldRoot || currentRow)
    {
        db->SetErrorId(12);
        return 12;
    }
    // While fields:
    field = fieldRoot;
    while(field)
    {
       dataPtr = (LPBYTE)field->data;

        // Switch by field type. DDB_TYPE_USED
        switch(field->type)
        {
        case DDBT_INT:
            vartype = INTBIND;
            break;
        case DDBT_STR:
#error Microsoft SQL Server strings do not work currently.
            vartype = NTBSTRINGBIND;
            break;
        case DDBT_BOOL:
            vartype = CHARBIND;
            break;
        case DDBT_TIME:
            vartype = DATETIMEBIND;
            field->msdbtime = new DBDATETIME;
            dataPtr = (LPBYTE) field->msdbtime;
            break;
        case DDBT_NUM:
            vartype = NUMERICBIND;
        default:
            db->SetErrorId(14);
            return 14;
        }
        ret = dbbind (db->GetMSConn(),i, vartype, field->length, dataPtr);
        if(ret == FAIL)
        {
            db->SetErrorId(13);
            return 13;
        }

        field = field->next;
        i++;
    }
    return 0;
}

// -----------------------------------------------------------------------------
int DdbMicrosoftRowSet::GetNext()
{
    DdbBoundField *field;
    DBDATEREC dateinfo;
    DDBTIME *timePtr;
    int fieldCount;

    if(dbnextrow(db->GetMSConn()) == NO_MORE_ROWS)
        return 0;

    // While fields:
    field = fieldRoot;
    fieldCount = 0;
    while(field)
    {
        // Use type to convert the data. DDB_TYPE_USED
        switch(field->type)
        {
        case DDBT_TIME:
            // Convert a computer-readable DBDATETIME value into user-accessible format
            dbdatecrack(db->GetMSConn(), &dateinfo,field->msdbtime);
#ifdef DDB_USESTL
            timePtr = (tm*)field->data;
            timePtr->tm_year = dateinfo.year - 1900;
            timePtr->tm_mon = dateinfo.month;
            timePtr->tm_mday = dateinfo.day;
            timePtr->tm_hour = dateinfo.hour;
            timePtr->tm_min = dateinfo.minute;
            timePtr->tm_sec = dateinfo.second;
            timePtr->tm_isdst = -1;
#else
            timePtr = (DateTime*)field->data;
            timePtr->Set(dateinfo.day,(wxDateTime::Month)dateinfo.month,dateinfo.year,
                                                dateinfo.hour,dateinfo.minute, dateinfo.second,
                                                dateinfo.millisecond);
#endif
            break;
        default:
            fieldCount--;
        }
        fieldCount++;
        field = field->next;
    }
    currentRow++;
    return ;
}

#endif
