/*! \file ddbpostgrers.cpp
 * \brief [short description] */
// Copyright (c) Menacon Oy
/********************************************************************************/

#if defined(DDB_USEWX)
  #include <wx/wxprec.h>
  #ifndef WX_PRECOMP
    #include <wx/wx.h>
  #endif
#endif
#include "pch-stop.h"

#if defined(DDB_USEWX)
  #include "wx/datetime.h"
#else
  #include <fstream>
#endif
#ifdef WIN32
    #include <windows.h>
#endif
#ifdef __linux
  #include <string.h>
#endif
#include <libpq-fe.h>
#include <stdlib.h>
#include <cstdarg>
#include <cpp4scripts.hpp>
#define __DDB_POSTGRE__
#include "directdatabase.hpp"

// ==================================================================================================
DdbPosgtgreRowSet::DdbPosgtgreRowSet(DirectDatabase *db_in)
    :DdbRowSet()
/*!
    Initializes member variables to default values.
    \param db_in Pointer to database object.
*/
{
    maxRows = 0;
    currentRow = 0;
    result = 0;
    resultCleared = true;

    db = (DdbPostgre*) db_in;
}

// ==================================================================================================
DdbPosgtgreRowSet::~DdbPosgtgreRowSet()
/*!
    Relases the PostGre result if it still exists.
*/
{
    if(resultCleared == false)
        PQclear(result);
}

// ==================================================================================================
bool DdbPosgtgreRowSet::Query(const DDBSTR &query)
{
    if(!fieldRoot) {
        CS_PRINT_NOTE("DdbPosgtgreRowSet::Query - Query called without binding variables.");
        db->SetErrorId(9);
        return  false;
    }
    if(query.LENGTH()==0) {
        CS_PRINT_WARN("DdbPosgtgreRowSet::Query - Empty query string. Aborted.");
        return false;
    }
    queryStmt = query;
    if(resultCleared == false)
        PQclear(result);

    result = PQexec(db->GetPGConn(), queryStmt.UTF8());
    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        CS_VAPRT_ERRO("DdbPosgtgreRowSet::Query failed: %s", PQresultErrorMessage(result));
        db->SetErrorId(8);
        PQclear(result);
        maxRows = 0;
        resultCleared = true;
        return false;
    }
    resultCleared = false;
    maxRows = PQntuples(result);
    currentRow = 0;
    return true;
}

// ==================================================================================================
int DdbPosgtgreRowSet::GetNext()
{
    int maxFields,nField,count;
    DdbBoundField *field;
    char *resultStr;

    if(resultCleared == true)
        return 0;

    // If the result of the query was empty:
    if(!maxRows)
    {
        // Clear the result and return false.
        PQclear(result);
        resultCleared = true;
        return 0;
    }
    bool trim = db->IsFeatureOn(DDB_FEATURE_AUTOTRIM);
    maxFields = PQnfields(result);
    field = fieldRoot;
    nField = 0;
    count=0;
    while(field)
    {
        resultStr = PQgetvalue(result, currentRow, nField);
        if(resultStr)
        {
            // Use type to convert the data. DDB_TYPE_USED
            switch(field->type)
            {
            case DDBT_INT:
                if(resultStr[0]=='\0')
                    *(static_cast<int*>(field->data)) = 0;
                else
                {
                    *(static_cast<int*>(field->data)) = strtol(resultStr,0,10);
                    count++;
                }
                break;
            case DDBT_STR:
                if(resultStr[0]=='\0')
                    static_cast<DDBSTR*>(field->data)->CLEAR();
                else
                {
#ifdef DDB_USESTL
                    *(static_cast<std::string*>(field->data)) = resultStr;
                    if(trim)
                        DirectDatabase::TrimTail(static_cast<std::string*>(field->data));
#else
                    *(static_cast<wxString*>(field->data)) = wxString::FromUTF8Unchecked(resultStr);
                    if(trim)
                        static_cast<wxString*>(field->data)->Trim();
#endif
                    count++;
                }
                break;
            case DDBT_BOOL:
                if(resultStr[0]=='\0')
                    *(static_cast<bool*>(field->data)) = 0;
                else
                {
                    *(static_cast<bool*>(field->data)) = resultStr[0] == 't' ? true:false;
                    count++;
                }
                break;

            case DDBT_TIME:
            case DDBT_DAY:
#ifdef DDB_USESTL
                if(resultStr[0]=='\0')
                    memset(field->data,0,sizeof(tm));
                else {
                    DdbPostgre::ExtractTimestamp(resultStr,(tm*)field->data);
                    count++;
                }
#else
                if(resultStr[0]=='\0')
                    *(static_cast<wxDateTime*>(field->data)) = wxInvalidDateTime;
                else {
                    wxString::const_iterator end;
                    if(field->type==DDBT_DAY) {
                        if(! ((wxDateTime*)field->data)->ParseFormat(resultStr,"%Y-%m-%d",&end))
                            CS_VAPRT_WARN("DdbPosgtgreRowSet::GetNext - Date parse failed for %s",resultStr);
                    }
                    else {
                        if(! ((wxDateTime*)field->data)->ParseFormat(resultStr,"%Y-%m-%d %H:%M:%S",&end))
                            CS_VAPRT_WARN("DdbPosgtgreRowSet::GetNext - Timestamp parse failed for %s",resultStr);
                    }
                }
#endif
                break;

            case DDBT_NUM:
                if(resultStr[0]=='\0')
                    *(static_cast<double*>(field->data)) = 0;
                else
                {
                    if(db->IsCommaDecimal())
                    {
                        char *commaPoint=strchr(resultStr,'.');
                        if(commaPoint)
                            *commaPoint=',';
                    }
                    *(static_cast<double*>(field->data)) = strtod(resultStr,0);
                    count++;
                }
                break;
            case DDBT_CHR:
#ifdef DDB_USESTL
                *(static_cast<char*>(field->data)) = resultStr[0];
#else
                *(static_cast<wxUniChar*>(field->data)) = resultStr[0];
#endif
                count++;
                break;
            }
        }

        field = field->next;
        nField++;
        if(!field || nField == maxFields)
            break;
    }
    currentRow++;
    if(currentRow == maxRows)
    {
        PQclear(result);
        resultCleared = true;
    }
    return count;
}

// ==================================================================================================
void DdbPosgtgreRowSet::QuitQuery()
{
    if(resultCleared)
        return;
    PQclear(result);
    resultCleared = true;
    maxRows = 0;
    currentRow = 0;
}
