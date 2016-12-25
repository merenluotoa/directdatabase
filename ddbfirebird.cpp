/*******************************************************************************
ddbfirebird.cpp
Copyright (c) Antti Merenluoto
*******************************************************************************/

#ifdef WIN32
    #include <windows.h>
#endif
#ifdef __linux
    #include <string.h>
#endif
#include <memory.h>
#ifdef DDB_USEWX
  #include <wx/wxprec.h>
  #ifndef WX_PRECOMP
    #include <wx/wx.h>
  #endif
#endif

#define __DDB_FIREBIRD__
#include "directdatabase.hpp"

// ==================================================================================================
DdbFirebird::DdbFirebird()
/*!
  Constructs database object for the connection to the Firebird databases.
*/
{
    flags = DDB_FLAG_INITIALIZED;
    connection = 0;
    transaction = 0;
}

// ==================================================================================================
DdbFirebird::~DdbFirebird()
/*!
    Closes up the database connection and on Windows closes socket services.
*/
{
    if( (flags & DDB_FLAG_CONNECTED) > 0)
        Disconnect();
}

// ==================================================================================================
bool DdbFirebird::Connect(int) // Firebird does not use timeouts during connection.
{
    char dpbBuffer[500],*pdb;
    int len;
    ISC_STATUS_ARRAY statusArray;

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
    // Check for initialization.
    if(!(flags&DDB_FLAG_INITIALIZED))
    {
        SetErrorId(11);
        return false;
    }
    // Check that we have something to connect to.
    if(!db)
    {
        SetErrorId(3);
        return false;
    }

    // Set connection parameters
    pdb = dpbBuffer;
    len = 1;
    *pdb++ = isc_dpb_version1;
    isc_expand_dpb(&pdb, (short *) &len,
                   isc_dpb_user_name, user,
                   isc_dpb_password, pass,  NULL);
    /*
    *pdb++ = isc_dpb_user_name;
    len = strlen(user);
    memcpy(pdb,user,len);
    pdb += len;
    *pdb++ = isc_dpb_password;
    len = strlen(pass);
    memcpy(pdb,pass,len);
    pdb += len;
    */

    if(isc_attach_database(statusArray,0,db,&connection,(short)len,pdb))
    {
        LogError(statusArray);
        return false;
    }

    delete pdb;
    flags |= DDB_FLAG_CONNECTED;
    return true;
}

// ==================================================================================================
bool DdbFirebird::Disconnect()
{
    ISC_STATUS_ARRAY statusArray;
    if(connection)
    {
        if(isc_detach_database(statusArray,&connection))
        {
            LogError(statusArray);
            return false;
        }
        flags &= ~DDB_FLAG_CONNECTED;
    }
    return true;
}

// ==================================================================================================
void DdbFirebird::LogError(ISC_STATUS_ARRAY statusArray)
{
        char errStr[512];
        int index;
        ostringstream ss;
        if(!statusArray[0])
            return;
        ss << "Firebird database error first long = " << statusArray[0] << endl;
        /*
        for(index=0; statusArray[index] && index<ISC_STATUS_LENGTH; index++)
        {
            isc_sql_code();
            isc_sql_interprete(statusArray[index],errStr,sizeof(errStr));
            ss << errStr << endl;
        }
        ss << ">>" << endl;
        */
        Log(ss);
}

// ==================================================================================================
DdbRowSet* DdbFirebird::CreateRowSet()
{
    if(!(flags&DDB_FLAG_CONNECTED))
    {
        SetErrorId(5);
        return 0;
    }
    return 0; //new DdbPosgtgreRowSet(this);
}

// ==================================================================================================
DDBSTR DdbFirebird::GetErrorDescription(DdbRowSet *)
{
    DDBSTR errorMsg;
    errorMsg = GetLastError();
    return errorMsg;
}

// ==================================================================================================
bool DdbFirebird::StartTransaction()
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

    flags |= DDB_FLAG_TRANSACT_ON;
    return true;
}

// ==================================================================================================
bool DdbFirebird::Commit()
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

    flags &= ~DDB_FLAG_TRANSACT_ON;
    return true;
}

// ==================================================================================================
bool DdbFirebird::RollBack()
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

    flags &= ~DDB_FLAG_TRANSACT_ON;
    return true;
}

// ==================================================================================================
bool DdbFirebird::ExecuteIntFunction(const DDBSTR &query, int &val)
{
    if(query.LENGTH()==0)
        return false;
    return true;
}

// ==================================================================================================
bool DdbFirebird::ExecuteDoubleFunction(const DDBSTR &query, double &val)
{
    if(query.LENGTH()==0)
        return false;
    return true;
}

// ==================================================================================================
bool DdbFirebird::ExecuteBoolFunction(const DDBSTR &query, bool &val)
{
    if(query.LENGTH()==0)
        return false;
    return false;
}

// ==================================================================================================
bool DdbFirebird::ExecuteStrFunction(const DDBSTR &query, DDBSTR &answer)
{
    if(query.LENGTH()==0)
        return false;
    return true;
}

// ==================================================================================================
bool DdbFirebird::ExecuteDateFunction(const DDBSTR &query, DDBTIME &val)
{
    tm *tmPtr;

    if(query.LENGTH()==0)
        return false;
    if(true)
    {
#ifdef DDB_USESTL
        tmPtr = &val;
#else
        tm tmData;
        tmPtr = &tmData;
#endif
#ifdef DDB_USEWX
        val.Set(tmData);
#endif
    }
    return false;
}


// ==================================================================================================
int DdbFirebird::ExecuteModify(const DDBSTR &modify)
{
    int retval;

    if(modify.LENGTH()==0)
        return false;
    return true;
}
