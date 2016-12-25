/*******************************************************************************
ddbmysqlrs.cpp
Copyright (c) Antti Merenluoto
*******************************************************************************/

#ifdef WIN32
    #include <windows.h>
#endif
#include <libpq-fe.h>
#include <stdlib.h>
#include <cstdarg>
#ifdef DDB_USEWX
  #include <wx/wxprec.h>
  #ifndef WX_PRECOMP
    #include <wx/wx.h>
  #endif
  #include "wx/datetime.h"
#else
#include <fstream>
#endif
#define __DDB_MYSQL__
#include "directdatabase.hpp"

// ==================================================================================================
DdbMySqlRowSet::DdbMySqlRowSet(DdbMySql *db_in)
    :DdbRowSet()
/*!
    Initializes member variables to default values.
    \param db_in Pointer to database object.
*/
{
    maxRows = 0;
    maxFields = 0;
    currentRow = 0;
    result = 0;
    resultCleared = true;

    db = db_in;
}

// ==================================================================================================
DdbMySqlRowSet::~DdbMySqlRowSet()
/*!
    Relases the MySql result if it is still exists.
*/
{
    if(resultCleared == false)
        mysql_free_result(result);
}

// ==================================================================================================
bool DdbMySqlRowSet::Query(const DDBSTR &query)
{
    if(!fieldRoot)
    {
        db->SetErrorId(9);
        return  false;
    }
    if(query.LENGTH()==0)
        return false;
    queryStmt = query;

    if(resultCleared == false)
        mysql_free_result(result);

    if(mysql_real_query(db->GetMySConn(), queryStmt.DATA(), queryStmt.LENGTH()))
        goto MYSQL_QUERY_ERROR;
    result = mysql_use_result(db->GetMySConn());
    if (!result)
        goto MYSQL_QUERY_ERROR;
    resultCleared = false;
    maxRows = (int) mysql_num_rows(result);
    maxFields = mysql_num_fields(result);
    currentRow = 0;
    return true;

 MYSQL_QUERY_ERROR:
    ostringstream ss;
    ss << "DdbMySqlRowSet::Query" << endl
       << queryStmt << endl
       << mysql_error(db->GetMySConn());
    db->Log(ss);
    db->SetErrorId(8);
    maxRows = 0;
    resultCleared = true;
    return false;
}

// ==================================================================================================
int DdbMySqlRowSet::GetNext()
{
    int nField,count;
    DdbBoundField *field;
    char *timeStrPtr;
    tm *tPtr,tmData;

    if(resultCleared == true)
        return 0;
    MYSQL_ROW row = mysql_fetch_row(result);
    if(!row)
    {
        mysql_free_result(result);
        resultCleared = true;
        maxFields = 0;
        return 0;
    }
    field = fieldRoot;
    nField = 0;
    count=0;
    while(field && nField < maxFields)
    {
        // Use type to convert the data. DDB_TYPE_USED
        switch(field->type)
        {
        case DDBT_INT:
            if(!row[nField])
                *(static_cast<int*>(field->data)) = 0;
            else
            {
                *(static_cast<int*>(field->data)) = atoi(row[nField]);
                count++;
            }
            break;
        case DDBT_STR:
            if(!row[nField])
#ifdef DDB_USESTL
                static_cast<DDBSTR*>(field->data)->erase();
#else
                static_cast<DDBSTR*>(field->data)->Empty();
#endif
            else
            {
                *(static_cast<DDBSTR*>(field->data)) = row[nField];
#ifdef DDB_USEWX
                static_cast<DDBSTR*>(field->data)->Trim();
#endif
                count++;
            }
            break;
        case DDBT_BOOL:
            if(!row[nField])
                *(static_cast<bool*>(field->data)) = false;
            else
            {
                *(static_cast<bool*>(field->data)) = row[nField][0] == '1' ? true:false;
                count++;
            }
            break;

        case DDBT_TIME:
            if(!row[nField])
#ifdef DDB_USESTL
                memset(field->data,0,sizeof(tm));
#else
                *(static_cast<wxDateTime*>(field->data)) = wxInvalidDateTime;
#endif
            else
            {
#ifdef DDB_USESTL
                tPtr = (tm*)field->data;
#else
                tPtr = &tmData;
#endif
                memset(tPtr,0,sizeof(tm));
                timeStrPtr = row[nField];
                tPtr->tm_year = strtol(timeStrPtr,&timeStrPtr,10)-1900;
                tPtr->tm_mon = strtol(timeStrPtr+1,&timeStrPtr,10)-1;
                tPtr->tm_mday = strtol(timeStrPtr+1,&timeStrPtr,10);
                if(tPtr->tm_year==-1900 && tPtr->tm_mon==-1 && tPtr->tm_mday == 0)
                    memset(tPtr,0,sizeof(tm)); // This seems not to be time field!
                else
                {
                    tPtr->tm_hour = strtol(timeStrPtr+1,&timeStrPtr,10);
                    tPtr->tm_min = strtol(timeStrPtr+1,&timeStrPtr,10);
                    tPtr->tm_sec = strtol(timeStrPtr+1,&timeStrPtr,10);
                    tPtr->tm_isdst = -1;
                }
#ifdef DDB_USEWX
                static_cast<DDBTIME*>(field->data)->Set(tmData);
#endif
                count++;
            }
            break;

        case DDBT_DAY:
            if(!row[nField])
#ifdef DDB_USESTL
                memset(field->data,0,sizeof(tm));
#else
            *(static_cast<wxDateTime*>(field->data)) = wxInvalidDateTime;
#endif
            else
            {
#ifdef DDB_USESTL
                tPtr = (tm*)field->data;
#else
                tPtr = &tmData;
#endif
                memset(tPtr,0,sizeof(tm));
                timeStrPtr = row[nField];
                tPtr->tm_year = strtol(timeStrPtr,&timeStrPtr,10)-1900;
                tPtr->tm_mon = strtol(timeStrPtr+1,&timeStrPtr,10)-1;
                tPtr->tm_mday = strtol(timeStrPtr+1,&timeStrPtr,10);
                tPtr->tm_isdst = -1;
                if(tPtr->tm_year==-1900 && tPtr->tm_mon==-1 && tPtr->tm_mday == 0)
                    memset(tPtr,0,sizeof(tm)); // This seems not to be time field!
#ifdef DDB_USEWX
                static_cast<DDBTIME*>(field->data)->Set(tmData);
#endif
                count++;
            }
            break;

        case DDBT_NUM:
            if(!row[nField])
                *(static_cast<double*>(field->data)) = 0;
            else
            {
                if(db->IsCommaDecimal())
                {
                    char *commaPoint=strchr(row[nField],'.');
                    if(commaPoint)
                        *commaPoint=',';
                }
                *(static_cast<double*>(field->data)) = strtod(row[nField],0);
                count++;
            }
            break;
        case DDBT_CHR:
            *(static_cast<char*>(field->data)) = row[nField][0];
            count++;
            break;
        }

        field = field->next;
        nField++;
    }
    currentRow++;
    return count;
}

// ==================================================================================================
void DdbMySqlRowSet::QuitQuery()
{
    if(resultCleared)
        return;
    mysql_free_result(result);
    resultCleared = true;
    maxRows = 0;
    currentRow = 0;
}
