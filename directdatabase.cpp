/*******************************************************************************
directdb.cpp
Copyright (c) Antti Merenluoto
*******************************************************************************/

#include "pch-stop.h"
#include <string.h>
#include <stdio.h>
#include <locale.h>
#ifdef DDB_USEWX
#include <wx/log.h>
#else
#include <fstream>
#include <sstream>
#endif
#include <cpp4scripts.hpp>

#include "directdatabase.hpp"

// =================================================================================================
DirectDatabase::DirectDatabase()
/*!
  This constructor just initializes all strings to zero values. Initialize function
  should be called before attempting to connect into the database.
*/
{
    feat_support=0;
    feat_on=0;
    errorId = 0;
    flags = 0;
    port = 0;

    /* Depending on the client's I18N settings the numeric values use period or comma
       as decimal separator. By default databases use the period.
     */
    struct lconv* ldata = localeconv();
    if(*ldata->decimal_point==',') {
        CS_PRINT_WARN("DirectDatabase::DirectDatabase - Current locale has , as decimal separator!");
        commaDecimal = true;
    }
    else
        commaDecimal = false;

    scratch_size = 0x200;
    scratch_buffer = new CHR_T[scratch_size];
}

// =================================================================================================
DirectDatabase::~DirectDatabase()
/*!
  Virtual destructor allows c++-system to call inherited destructor. This destructor
  has no code currently.
*/
{
    if(scratch_buffer)
        delete[] scratch_buffer;
}

// =================================================================================================
#if 0
bool DirectDatabase::Initialize(const DDBSTR &server,const DDBSTR &db,const DDBSTR &uid,const DDBSTR &pwd_in, int port_in)
/*!
  Database initialization copies connection parameters into the object itself. These
  parameters will be used in connect function.

  \param server Name (or IP address) of the database server computer on the network.
  \param db Name of the database in the server.
  \param uid User name of the person operating the client machine.
  \param pwd_in Password for the user.
  \param port_in Database IP port number. Defaults to zero = default port is used. This is database specific.

  \retval bool True if server, db parameters are not defined or connection is on.
*/
{
#ifdef DDB_USESTL
    if(server.length()==0 || db.length()==0)
#else
    if(server.IsEmpty() || db.IsEmpty())
#endif
    {
        errorId = 23;
        return false;
    }
    if(flags & DDB_FLAG_CONNECTED)
        return false;

    srvName = server;
    dbName = db;
    port = port_in;

#ifdef DDB_USESTL
    if(uid.length()!=0)
#else
    if(!uid.IsEmpty())
#endif
        userid = uid;
#ifdef DDB_USESTL
    if(pwd_in.length()!=0)
#else
    if(!pwd_in.IsEmpty())
#endif
        pwd = pwd_in;

    flags |= DDB_FLAG_INITIALIZED;
    return true;
}
#endif

// =================================================================================================
DDBSTR DirectDatabase::GetLastError()
{
#define MAX_ERRORS 24
const CHR_T *errorStr[MAX_ERRORS] = {
    /* 000 */ _T("Success"),
    /* 001 */ _T("Undefined error number"),
    /* 002 */ _T("Version 2.0 of Win socket was not found."),
    /* 003 */ _T("DB - Connect: empty or incorrect connections string."),
    /* 004 */ _T("DB - Connect: Connection failure. Check the initialization parameters."),
    /* 005 */ _T("DB - Attempt to use member functions without a connection to the database."),
    /* 006 */ _T("DB - Transaction start: Transaction is already on."),
    /* 007 */ _T("DB - Commit/RollBack: The transaction has not been started."),
    /* 008 */ _T("Rowset - Query function unsuccesfull.") ,
    /* 009 */ _T("Rowset - Query called without bound variables."),
    /* 010 */ _T("Unable to initialize Microsoft connection communication layer."),
    /* 011 */ _T("DB - Connect: Attempt to connect when database has not been initialized successfully."),
    /* 012 */ _T("Microsoft - MSBind called without calling Query first."),
    /* 013 */ _T("Microsoft - bind function did not support desired conversion."),
    /* 014 */ _T("Rowset - Requested data conversion is not supported."),
    /* 015 */ _T("Rowset - Insert execution failure."),
    /* 016 */ _T("Postgresql - Fatal error."),
    /* 017 */ _T("Rowset - Too little memory allocated for the Insert data."),
    /* 018 */ _T("DB - Modify (INSERT, UPDATE or DELETE) function was unsuccesful."),
    /* 019 */ _T("DB - Execute query function was unsuccesful."),
    /* 020 */ _T("Rowset - GetNext function was unsuccesful."),
    /* 021 */ _T("DB - Update structure (CREATE, DROP, ALTER TABLE or VIEW) command was unsuccessful."),
    /* 022 */ _T("DB - GetInsertId failed. Operation not supported or last statement was not an INSERT command."),
    /* 023 */ _T("DB - Initialization failure.")
};
    DDBSTR str;
    if(errorId >= MAX_ERRORS)
         str = errorStr[1];
    str = errorStr[errorId];
    return str;
}

// =================================================================================================
void DirectDatabase::reallocateScratch(size_t size) {
    size += DDB_CLEAN_MAX;
    if(size<=scratch_size) return;
    delete[] scratch_buffer;
    scratch_buffer = new CHR_T[size];
    scratch_size = size;
}
// ..........................................................................................
const char* DirectDatabase::CleanString2(const string &str)
{
    short int max=0;
    reallocateScratch(str.length());
    char *to = scratch_buffer;
    const char *from = str.DATA();
    string::const_iterator chi, end;

    chi = str.begin();
    end = str.end();
    while(chi != end) {
        if(*chi == '\'') {
            if(max<DDB_CLEAN_MAX) {
                *to++ = '\'';
                *to++ = '\'';
                max++;
            } else {
                *to++ = ' ';
            }
        }
        else if(*from != '\r') // skip the carriage return
            *to++ = *chi;
        else
            max--;
        chi++;
    }
    *to = 0;
    return scratch_buffer;
}
// ..........................................................................................
const char* DirectDatabase::CleanUtf8(const char *utf8)
{
    short int max=0;
    if(!utf8) {
        scratch_buffer[0] = 0;
        return scratch_buffer;
    }
    reallocateScratch(strlen(utf8));
    char *to = scratch_buffer;
    while(*utf8) {
        if(*utf8 == '\'') {
            if(max<DDB_CLEAN_MAX) {
                *to++ = '\'';
                *to++ = '\'';
                max++;
            } else {
                *to++ = ' ';
            }
        }
        else if(*utf8 != '\r') // skip the carriage return
            *to++ = *utf8;
        else
            max--;
        utf8++;
    }
    *to = 0;
    return scratch_buffer;
}
// ..........................................................................................
#ifdef DDB_USEWX
const wchar_t* DirectDatabase::CleanString2(const wxString &str)
{
    reallocateScratch(str.length());
    wchar_t *to = scratch_buffer;
    const wchar_t *from = str.wc_str();

    while(*from) {
        if(*from == _T('\'')) {
            if(max<DDB_CLEAN_MAX) {
                *to++ = _T('\'');
                *to++ = _T('\'');
            } else {
                *to++ = ' ';
            }
        }
        else if(*from != _T('\r')) // skip the carriage return
            *to++ = *from;
        else
            max--;
        from++;
    }
    *to = 0;
    return scratch_buffer;
}
#endif
// =================================================================================================
const CHR_T* DirectDatabase::GetCleanHtml(const DDBSTR &str)
{
    reallocateScratch(str.LENGTH());
    CHR_T *to = scratch_buffer;
    const CHR_T *from = str.DATA();
    while(*from) {
        if(*from == _T('\'')) {
            *to++ = _T('\'');
            *to++ = _T('\'');
        }
        else
            *to++ = *from;
        from++;
    }
    *to = 0;
    return scratch_buffer;
}

void DirectDatabase::CleanReverse(DDBSTR &str, CLEANTYPE /*ct*/)
{
    reallocateScratch(str.LENGTH());
    CHR_T *to = scratch_buffer;
    const CHR_T *from = str.DATA();

    // Reverse the cleaning
    while(*from) {
        if(*from == _T('\\')) {
            from++;
            if(*from == _T('n'))
                *to++ = _T('\n');
            else if(*from == _T('t'))
                *to++ = _T('\t');
            else
                *to++ = *from;
        }
        else
            *to++ = *from++;
    }
    *to = 0;
    str = scratch_buffer;
}

// =================================================================================================
int DirectDatabase::PrintNumber(char *buffer, const char *format, double number)
{
    register int i;
    int bLen=0;

    bLen = sprintf(buffer,format,number);
    if(commaDecimal) {
        for(i=0; i<bLen; i++) {
            if(buffer[i]==',') {
                buffer[i]='.';
                break;
            }
        }
    }
    return bLen;
}

// =================================================================================================
const char* DirectDatabase::PrintNumber(double number)
{
    static char buffer[30]; // It is doubtful that this many numbers can be printed from double.
    char *ptr = buffer;
    char *end = buffer + sprintf(buffer,"%f",number);
    if(commaDecimal) {
        while(ptr<end) {
            if(*ptr==',') {
                *ptr='.';
                break;
            }
            ptr++;
        }
    }
    return buffer;
}

// =================================================================================================
uint32_t DirectDatabase::ExecuteIntFunction(const DDBSTR &query)
{
    uint32_t retval;
    if(!ExecuteIntFunction(query,retval))
        return (uint32_t)-1;
    return retval;
}

// =================================================================================================
uint64_t DirectDatabase::ExecuteLongFunction(const DDBSTR &query)
{
    uint64_t retval;
    if(!ExecuteLongFunction(query,retval))
        return (uint64_t)-1;
    return retval;
}

// =================================================================================================
double DirectDatabase::ExecuteDoubleFunction(const DDBSTR &query)
{
    double retval;
    if(!ExecuteDoubleFunction(query,retval))
        return -1;
    return retval;
}
#ifdef DDB_USESTL
void DirectDatabase::TrimTail(std::string *target)
{
    std::string::reverse_iterator rit;
    for(rit=target->rbegin(); rit!=target->rend(); rit++)
        if(*rit != ' ') break;
    if(rit==target->rend())
        target->clear();
    else
        target->erase(rit.base(),target->end());
}
#endif
