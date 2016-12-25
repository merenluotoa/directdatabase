/*******************************************************************************
File: ddb_setup.hpp
$Revision: 1.2 $
Copyright (c) Menacon Inc

This file has wxDirectDatabase setup definitions. Comment/Uncomment the
necessary definitions.

*******************************************************************************/

#ifndef MC_DIRECTDB_SETUP_H
#define MC_DIRECTDB_SETUP_H

// Currently only static compilations is possible.
#define DDB_STATIC

// Enable following to include PostgreSQL modules into the library and into the application.
#define __DDB_POSTGRE__

// Enable following to include Firebird modules into the library and into the application.
#define __DDB_FIREBIRD__

// Win32 only ----------------------------------------------------------------------------------------
#ifdef WIN32

// Enable following to include ODBC-Windows modules into the library and into the application.
//#define __DDB_ODBCWIN__

// Enable following to include Microsoft SQL server  modules into the library and into the application.
// Please note that the Microsoft support has lagged behind in development. Not supported currently.
//#define __DDB_MICROSOFT__

#endif // WIN32 ---------------------------------------------------------------------------------

/* You may use either the STL library or wxWidgets library type variables. Either DDB_USESTL or DDB_USEWX
   must be defined at compile command.
 */
#ifdef DDB_USESTL
 #define DDBSTR string
 #define DDBTIME tm
 #define DATA() data()
 #define LENGTH() length()
#elif DDB_USEWX
 #define DDBSTR wxString
 #define DDBTIME wxDateTime
 #define DATA() GetData()
 #define LENGTH() Length()
#else
#error Either DDB_USESTL or DDB_USEWX must be defined at compile time.
#endif

#endif
