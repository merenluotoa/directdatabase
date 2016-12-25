/*******************************************************************************
ddbmicrosoft.cpp
Copyright (c) Antti Merenluoto
*******************************************************************************/

// Microsoft only works from Win32 systems.
#ifdef WIN32

#include <windows.h>
#define	DBNTWIN32		// must identify operating system environment
#include <sqlfront.h>
#include <sqldb.h>
#include <stdio.h>

#include "directdatabase.hpp"

DDBSTR DdbMicrosoft::g_errorStr;

// ==================================================================================================
int err_handler(PDBPROCESS dbproc, int severity, int dberr, int oserr, char * dberrstr, char * oserrstr)
/*!
  Error handler call-back function. SQL Server library calls this function on errors. We simply
  store the description. Use GetErrorDescription-function to display it.

   \param dbproc (See SQL Server documentation)
   \param severity (See SQL Server documentation)
   \param dberr (See SQL Server documentation)
   \param oserr (See SQL Server documentation)
   \param dberrstr (See SQL Server documentation)
   \param oserrstr (See SQL Server documentation)
   \retval (See SQL Server documentation)
*/
{
	if (dberrstr != NULL)
    {
		DdbMicrosoft::g_errorStr += "DB-LIBRARY error:\n";
        DdbMicrosoft::g_errorStr += dberrstr;
    }

	if (oserr != DBNOERR)
    {
		DdbMicrosoft::g_errorStr += "OS error:\n";
        DdbMicrosoft::g_errorStr += oserrstr;
    }

	if ((dbproc == NULL) ||	(DBDEAD(dbproc)))
		return(INT_EXIT);
	else
		return(INT_CANCEL);
}

// ==================================================================================================
int msg_handler(PDBPROCESS dbproc, DBINT msgno, int msgstate, int severity, char * msgtext)
/*!
  ###TODO: Describe this function better. (See SQL Server documentation)

   \param dbproc (See SQL Server documentation)
   \param msgno (See SQL Server documentation)
   \param msgstate (See SQL Server documentation)
   \param severity (See SQL Server documentation)
   \param msgtext (See SQL Server documentation)
   \retval (See SQL Server documentation)
*/
{
    if(severity>1)
	    DdbMicrosoft::g_errorStr.Printf("SQL Server message %ld, state %d, severity %d:\n\t%s\n",
			    msgno, msgstate, severity, msgtext);
	return(0);
}

// ==================================================================================================
DdbMicrosoft::DdbMicrosoft()
/*!
    Constructs database object for the connection to the Microsoft databases.
*/
{
    connection = 0;

    // Initialize database libarary.
	if (dbinit() == (char *)NULL)
	{
        SetErrorId(10);
        flags &= ~DDBFLAG_INITIALIZED;
    }
    flags |= DDBFLAG_INITIALIZED;
}

// ==================================================================================================
DdbMicrosoft::~DdbMicrosoft()
/*!
    Closes up the database connection.
*/
{
    if(flags & DDBFLAG_CONNECTED)
        Disconnect();
}

// ==================================================================================================
bool DdbMicrosoft::Connect(int /*timeout*/)
{
PLOGINREC  loginrec;

    /*### timeout value is not used by microsoft, yet */

    if(!(flags&DDBFLAG_INITIALIZED))
    {
        SetErrorId(11);
        return false;
    }

    if(srvName.IsEmpty() || dbName.IsEmpty())
    {
        SetErrorId(3);
        return false;
    }

    loginrec = dblogin();

    if(!userid.IsEmpty())
	    DBSETLUSER (loginrec, userid.GetData());
    if(!pwd.IsEmpty())
        DBSETLPWD (loginrec, pwd.GetData());

	DBSETLAPP (loginrec, (char *)"DirectDB");
	DBSETLVERSION(loginrec,DBVER60);

	dberrhandle((DBERRHANDLE_PROC)err_handler);
	dbmsghandle((DBMSGHANDLE_PROC)msg_handler);

	if ((connection	= dbopen (loginrec, srvName.GetData())) == NULL)
	{
        SetErrorId(4);
        return false;
	}

    dbuse (connection, dbName.GetData());

    flags |= DDBFLAG_CONNECTED;
    return true;
}

// ==================================================================================================
bool DdbMicrosoft::Disconnect()
{
    if(connection)
       	dbexit();

    flags &= ~DDBFLAG_CONNECTED;
    return true;
}

// ==================================================================================================
DdbRowSet* DdbMicrosoft::CreateRowSet()
{
    if(!(flags&DDBFLAG_CONNECTED))
    {
        SetErrorId(5);
        return 0;
    }
    return new xwDdbMicrosoftRowSet(this);
}

// ==================================================================================================
DDBSTR DdbMicrosoft::GetErrorDescription(int id)
{
    DDBSTR errorMsg;

    errorMsg = DirectDatabase::GetErrorDescription(id);
    if(connection && !g_errorStr.IsEmpty())
    {
        errorMsg += "\n";
        errorMsg += g_errorStr;
        g_errorStr.Empty();
    }
    return errorMsg;
}

// ==================================================================================================
bool DdbMicrosoft::StartTransaction()
{
    if(!(flags&DDBFLAG_CONNECTED))
    {
        SetErrorId(5);
        return false;
    }
    if(flags&DDBFLAG_TRANSACT_ON)
    {
        SetErrorId(6);
        return false;
    }

    //###TODO: Transaction support
    // flags |= DDBFLAG_TRANSACT_ON;
    return false;
}

// ==================================================================================================
bool DdbMicrosoft::Commit()
{
    if(!(flags&DDBFLAG_CONNECTED))
    {
        SetErrorId(5);
        return false;
    }
    if(!(flags&DDBFLAG_TRANSACT_ON))
    {
        SetErrorId(7);
        return false;
    }

    //###TODO: Transaction support
    // flags &= ~DDBFLAG_TRANSACT_ON;
    return true;
}

// ==================================================================================================
bool DdbMicrosoft::RollBack()
{
    if(!(flags&DDBFLAG_CONNECTED))
    {
        SetErrorId(5);
        return false;
    }
    if(!(flags&DDBFLAG_TRANSACT_ON))
    {
        SetErrorId(7);
        return false;
    }

    //###TODO: Transaction support
    // flags &= ~DDBFLAG_TRANSACT_ON;
    return true;
}

// -----------------------------------------------------------------------------
int xwDdbMicrosoft::ExecuteIntFunction(char *)
{
    // ### NOT IMPLEMENTED!
    return 0;
}

// -----------------------------------------------------------------------------
double xwDdbMicrosoft::ExecuteDoubleFunction(char *)
{
    // ### NOT IMPLEMENTED!
    return 0;
}

// -----------------------------------------------------------------------------
int xwDdbMicrosoft::ExecuteModify(char *)
{
    // ### NOT IMPLEMENTED!
    return -1;
}

// -----------------------------------------------------------------------------
bool xwDdbMicrosoft::ExecuteStrFunction(DDBSTR &query, DDBSTR &result)
{
    // ### NOT IMPLEMENTED!
    return false;
}

#endif

