/*******************************************************************************
directdatabase.hpp
Copyright (c) Antti Merenluoto

    Database and system independent method of accesing databases.
    Classes in this file are all abstract classes inherited by
    database dependent layer.
*******************************************************************************/

#ifndef DIRECTDB_H_FILE
#define DIRECTDB_H_FILE

#ifdef DDB_USEWX
#include "wx/string.h"
#include "wx/datetime.h"
#else
#include <string>
#include <fstream>
#endif
#include <stdint.h>

// Log feature uses STL string streams despite the library setting
#include <sstream>
using namespace std;

// DATABASE TYPES
const int DDBTYPE_POSTGRES = 1;
const int DDBTYPE_MYSQL    = 2;
const int DDBTYPE_ODBC     = 3;
const int DDBTYPE_SQLITE   = 4;
const int DDBTYPE_MSSQL    = 5;

// OBSOLETE from 2006-8-20: #include <directdb/ddb_setup.hpp>
/* You may use either the STL library or wxWidgets library type variables. Either DDB_USESTL or DDB_USEWX
   must be defined at compile command.
 */
#ifdef DDB_USESTL
 #define DDBSTR string
 #define DDBTIME tm
 #define CHR_T char
 #define DATA() c_str()
 #define UTF8() c_str()
 #define LENGTH() length()
 #define CLEAR() clear()
 #define _T
#endif
#ifdef DDB_USEWX
 #define DDBSTR wxString
 #define DDBTIME wxDateTime
 #define CHR_T wchar_t
 #define DATA() wc_str()
 #define UTF8() utf8_str()
 #define LENGTH() Length()
 #define CLEAR() Empty()
// #define _T L old definition
#endif
#ifndef DDBSTR
 #error Either DDB_USESTL or DDB_USEWX must be defined at compile time.
#endif

//#ifdef __DDB_MICROSOFT__
//#include <sqlfront.h>
//#endif
//
/* These following types are used in many places in this library. To assist
   in updating these types please mark each place you use these types with
   a comment that includes text: 'DDB_TYPE_USED'. In this way all those places
   are easier to find from the source code.
*/
const short int DDBT_MIN  = 1;
const short int DDBT_INT  = 1; // 32bit int
const short int DDBT_STR  = 2; // String wxString or STL string.
const short int DDBT_BOOL = 3;
const short int DDBT_BIT  = 4; // MSSQL: conversion to char.
const short int DDBT_TIME = 5; // Timestamp: Date and time
const short int DDBT_NUM  = 6; // Numeric (double)
const short int DDBT_DAY  = 7; // Date only
const short int DDBT_CHR  = 8; // Single character
const short int DDBT_MAX  = 8;

// Database features
const short int DDB_FEATURE_CURSOR       = 0x0001;     // Cursor library supported | on/off.
const short int DDB_FEATURE_TRANSACTIONS = 0x0002;     // Database supports transactions
const short int DDB_FEATURE_AUTOTRIM     = 0x0003;     // Automatically right trim the strings

// Database flags
const short int DDB_FLAG_INITIALIZED = 0x0001;
const short int DDB_FLAG_CONNECTED   = 0x0002;
const short int DDB_FLAG_TRANSACT_ON = 0x0004;

// =============================================================================
//  HELPER CLASSES
// =============================================================================
//! DdbRowSet class uses this to store the bound variables.
/*!
  This class is for directDB internal use only (please).
*/
class DdbBoundField
{
public:
    DdbBoundField(short int, void *);
    virtual ~DdbBoundField();

    short int type;             //!< Field type. One of DDBT... constants
    void *data;                 //!< Pointer to client data buffer.
    DdbBoundField *next;      //!< Pointer to next bound field. Null signifies end of the list.
};

// =============================================================================
//  ABSTRACT CLASSES
// =============================================================================

// Forward declarations.
class DdbRowSet;

// ==================================================================================================
//! Database class represents the connection to the database.
/*! This class wraps the connection functionality. Each database application must have at
  least one connection. Database creates RowSets (= query resut, record set, resutl set,
  etc.). Each row set created by this class therefore shares this connection to the
  database.

  This class has not been designed to be thread safe. Use from single thread only!
*/
class DirectDatabase
{
    friend class DdbPosgtgreRowSet;
public:
    DirectDatabase();
    DirectDatabase(DirectDatabase &) {};
    void operator=(DirectDatabase &) {};
    virtual ~DirectDatabase();

    //! Returns the database type, i.e. one of the constants defined in directdatabase.hpp.
    /*! Function can be used as primitive RTTI.
      \retval int Database type.     */
    virtual int GetType()=0;

    /*! Makes a connection to the database.
        \param constr Connection string. Postgresql style string that tells connection attributes.
        \retval bool True on success, otherwise false. Use GetErrorDescription-function to get further information on error. */
    virtual bool Connect(const char *constr)=0;

    /*! Closes the database connection and releases the resources reserved for that
        purpose.
        \retval bool True always.       */
    virtual bool Disconnect()=0;

    /*! Checks the current connection status.
      \retval bool True if status is OK. False if not.       */
    virtual bool IsConnectOK()=0;

    /*! Resets the connection to database.
      \retval bool True if reset succeeds. False if not.       */
    virtual bool ResetConnection()=0;

    /*! Creates a DdbRowSet object for this database (i.e. connection). Row sets perform
        the actual work on the database, i.e. selects, inserts, updates and deletions.
        Database can have multiple row sets open.  Because each row set must have a
        connection this is the only way to create objects of that class. Please see
        DdbRowSet for more information.
        Please note that the database class does not delete DdbRowSet objects
        automatically.  Programmer must explicitly remove them with delete-command.
        \retval New DdbRowSet object.     */
    virtual DdbRowSet* CreateRowSet()=0;

    /*! This function should be called right after error has occurred in any of the
        directDB functions. In most cases the library is able to return descriptive string
        from the proprietary database interface. If databas does not support this feature
        or if the information is not available this function returns DirectDB library
        specific description of error.

        \param rs Pointer to the rowset that caused the error. NULL if not applicable.
    */
    virtual DDBSTR GetErrorDescription(DdbRowSet *rs = 0)=0;

    /*! Function will start a new transaction. Transaction should be terminated with
        either Commit- or RollBack-function. Commit will accept and save all work done
        during the transaction. RollBack will cancel all database operations done during
        the transaction. (Please see your database manual for details on transactions).

        Not all databases support transactions. In those cases this function simply
        returns error value.

        Please note that transactions are generally connection based. Starting a
        transaction affects all RowSets currently open for the connection
        (=database). There can be only one transaction in a connection. Subsequent calls to
        this function simply return error value. Current transaction is not terminated.
        \retval bool True if transaction started succesfully.
    */
    virtual bool StartTransaction()=0;

    /*! Stops the current transaction and sends 'commit' command to database. If there is
        no current transaction this function does nothing. (See Transaction-function or
        your database manual for details.
    */
    virtual bool Commit()=0;

    /*! Stops the current transaction and sends 'roll back' command to database. If there is
        no current transaction this function does nothing. (See Transaction-function or
        your database manual for details.
    */
    virtual bool RollBack()=0;

    /*! All strings are passed to the database as they are. Problems can occur if user editable
        fields are transferred directly into the database because hyphen ('), backslash (\\)
        newline (\\n) and carriage return (\\r) have special meaning in SQL statements. This function
        will clean up the given string so that no problems will occur. This is not called
        automatically to all strings because it might slow things down unnecessarily.
        \param to Resulting string after the conversion.
        \param from Original string.
     */
    virtual const CHR_T* CleanString2(const DDBSTR &str);
    virtual void CleanString(DDBSTR &str) { str = CleanString2(str); }
    virtual void CleanString(DDBSTR &to, const DDBSTR &from) { to = CleanString2(from); }

    virtual const CHR_T* GetCleanHtml(const DDBSTR &str);
    enum CLEANTYPE { CT_NORMAL, CT_HTML };
    virtual void CleanReverse(DDBSTR &str, CLEANTYPE ct=CT_NORMAL);

    /*! Prints floating point numbers in a safe manner. Comma is swapped to dot to accommodate
        SQL standards if the current locale uses comma as decimal separator.
        \param buffer Pointer to resulting number string.
        \param format Printf-style format for the number.
        \double number The number that sould be printed.
        \retval int Number of characters printed into buffer.
    */
    virtual int PrintNumber(char *buffer, const char *format, double number);

    /*! Prints floating point numbers in a safe manner. Comma is swapped to dot to accommodate
        SQL standards if the current locale uses comma as decimal separator.
        \param number Number that should be printed.
        \retval DDBSTR& Reference to text representation of the number.
     */
    virtual const char* PrintNumber(double number);

    /*! Exectes given SQL SELECT-statement in the database and returns the result as an
        integer.  This function supposes that SELECT has only one integer field specified.
        \param query SQL SELECT-statement to be executed.
        \retval int Query result. Returns value is -1 on error.*/
    virtual uint32_t ExecuteIntFunction(const DDBSTR &query);

    /*! Exectes given SQL SELECT-statement in the database and returns the result as an
        integer.  This function supposes that SELECT has only one integer field specified.
        \param query SQL SELECT-statement to be executed.
        \param val Ref to interger where the value will be stored into.
        \retval bool True if query was successful, false if not. In latter case the val is unchanged.*/
    virtual bool ExecuteIntFunction(const DDBSTR &query,uint32_t &val)=0;

    /*! Exectes given SQL SELECT-statement in the database and returns the result as a
        long integer.  This function supposes that SELECT has only one integer field specified.
        \param query SQL SELECT-statement to be executed.
        \retval int Query result. Returns value is -1 on error.*/
    virtual uint64_t ExecuteLongFunction(const DDBSTR &query);

    /*! Exectes given SQL SELECT-statement in the database and returns the result as a
        long integer.  This function supposes that SELECT has only one integer field specified.
        \param query SQL SELECT-statement to be executed.
        \param val Ref to interger where the value will be stored into.
        \retval bool True if query was successful, false if not. In latter case the val is unchanged.*/
    virtual bool ExecuteLongFunction(const DDBSTR &query,uint64_t &val)=0;

    /*! Exectes given SQL SELECT-statement in the database and returns the result as a
        double.  This function supposes that SELECT has only one numeric field specified.

        \param query SQL SELECT-statement to be executed. Returns value is -1 on error.
     */
    virtual double ExecuteDoubleFunction(const DDBSTR &query);

    /*! Exectes given SQL SELECT-statement in the database and returns the result as a
        double.  This function supposes that SELECT has only one numeric field specified.

        \param query SQL SELECT-statement to be executed. Returns value is -1 on error.
        \param val Ref to double where the value will be stored into.
        \retval bool True if query was successful, false if not. In latter case the val is unchanged.
     */
    virtual bool ExecuteDoubleFunction(const DDBSTR &query, double &val)=0;

    /*! Exectes given SQL SELECT-statement in the database and returns the result as a
        boolean.  This function supposes that SELECT has only one boolean field specified.

        \param query SQL SELECT-statement to be executed. Returns value is -1 on error.
        \param val Ref to boolean where the value will be stored into.
        \retval bool True if query was successful, false if not. In latter case the val is unchanged.
     */
    virtual bool ExecuteBoolFunction(const DDBSTR &query, bool &val)=0;

    /*! Exectes given SQL SELECT-statement in the database and returns the result as a string.
        This function supposes that SELECT has only one char or varchar field specified.
        If query returns NULL then the result is cleared/emptied.

        \param query SQL SELECT-statement to be executed.
        \param result Reference to string where the answer is copied into.
        \retval bool True if query is succesful. False if result is NULL or error occurred.
     */
    virtual bool ExecuteStrFunction(const DDBSTR &query, DDBSTR &result)=0;

    /*! Exectes given SQL SELECT-statement in the database and returns the result as a timestamp.
        This function supposes that SELECT has only one char or varchar field specified.

        \param query SQL SELECT-statement to be executed.
        \param val Reference to date where the answer is copied into.
        \retval bool True if query is succesful, false if not.
     */
    virtual bool ExecuteDateFunction(const DDBSTR &query, DDBTIME &val)=0;

    /*! Function executes given INSERT, MODIFY or DELETE SQL-statement and returns the result
        as integer. Generally most databases return the number of fields affected by the
        modification.

       \param query SQL SELECT-statement to be executed.
       \retval int Number of rows modified if the database supports the feature.
                   -1 on error. Please note that MySQL can return zero on succesful update if nothing was changed.
    */
    virtual int ExecuteModify(const DDBSTR &query)=0;

    /*! Function returns automatically assigned field value (id) from the last insert statement.
        In PostgreSQL field type is SEQUENCE,
        in MySql the field type is AUTO INCREMENT.
        For other databases this function returns 0. Please see further details from your database manual.
        \retval unsigned int Auto increment field value from last insert.
     */
    virtual unsigned long GetInsertId()=0;

    /*! Used to update database structure commands like CREATE, DROP and ALTER TABLE.
      \param command SQL command to execute.
      \retval bool True if command is successful, false otherwise.
     */
    virtual bool UpdateStructure(const DDBSTR &command)=0;

    /*! Returns true if comma is used as a decimal separator in running environment. This means that when
        floating point numbers are printed the comma should be changed to period. PrintNumber-function does this
        automatically depending on the running environemnt.
    */
    virtual bool IsCommaDecimal() { return commaDecimal; };
    /*! Returns true if the transaction has been started i.e. is current on.
     */
    bool IsTransaction() { return (flags&DDB_FLAG_TRANSACT_ON)!=0; }

    bool IsConnected();
    bool IsFeatureOn(const unsigned short int ft) { return (ft&feat_on)>0?true:false; }
    virtual int IsFeatureSupported(const int);

    DDBSTR GetServerName();
    DDBSTR GetDbName();
    DDBSTR GetLastError();
    //! Returns latest error code. Zero on success.
    int GetErrorID() { return errorId; }
    //! Returns the connection port number;
    int GetPort() { return port; }
    //! Turns on one of several library features.
    bool SetFeature(const int);

#ifdef DDB_USESTL
    static void TrimTail(std::string*);
#endif

protected:
    void SetErrorId(int id);
    void reallocateScratch(size_t size) {
        if(size<=scratch_size) return;
        delete[] scratch_buffer;
        scratch_buffer = new CHR_T[size];
        scratch_size = size;
    }

    DDBSTR srvName;           //!< Name of the server machine or it's ip address.
    DDBSTR dbName;            //!< Name of the database in the server.
    DDBSTR userid;            //!< Name of the user who owns the connection.
    DDBSTR pwd;               //!< Password for the user.
    int port;                 //!< Database port number. If 0 then default is used.

    unsigned short feat_support; //!< Supported features. A bit field of feature bits.
    unsigned short feat_on;   //!< Currently selected features.
    unsigned long errorId;    //!< Error id from the last database operation. Zero if all OK.
    short int flags;          //!< Operation flags. Combination of DDBFLAG_...
    bool commaDecimal;        //!< True if comma is decimal separator, false otherwise.
    CHR_T *scratch_buffer;    //!< Buffer for the string cleaning
    size_t scratch_size;      //!< size for the current buffer.
};

// ==================================================================================================
class DdbRowSet
/*!
  This class is the work horse of the DirectDB library. It has functions to
  send SQL statements to the database and to retrieve the results from these
  statements.

  Use Bind - SetQueryStmt - Query - GetNext function sequence to perform queries (SELECT
  -SQL statements) on the database. Each field specified in select must be bound.

  Use ExecuteModify-function for INSERT, UPDATE, DELETE SQL-statements. This function
  does not require you to bind variable beforehand.

  Use other Execute...-functions to perform simple SELECTs where the result consists only
  one field.
*/
{
    friend class DirectDatabase;
public:
    virtual ~DdbRowSet();

    virtual bool Bind(short int type, void *data);

    /*! Sends the query statement to the database and waits for it to execute.
        Caller should have bound all fields from the SELECT-clause. Please note that
        this function does not actually retreive the records from the query. Each call to
        GetNext-function will move one record from the result set into bound variables
        i.e. you have to call GetNext at least once.

        \param query Query string that is passed to the database.
        \retval bool True on success, false on error.
        \sa Bind, GetNext, QuitQuery
      */
    virtual bool Query(const DDBSTR &query) = 0;

    /*! Gets the next row of information from the query result. This is called right after
        Query-function.  Field values from the row are stored into bound variables. Bound
        variables are written over the next time this function is called.

        This function should be called repeatedly until it returns zero, i.e. all results
        have been fetched. If partial result set is read QuitQuery-function should be called
        to make sure row set is not left into unsyncronized state (problem with MySql especially)

        \retval Number of fields converted. Note that this can be less than was specified in
        the query since some of the field values could have been NULLs. In this case the bound
        data is cleared (actual operation depends on the data type). If return value is zero
        then there is no more rows in the result set (= no fields converted). Bound variables
        remain unaltered in this case.
        \sa QuitQuery
      */
    virtual int GetNext() = 0;

    /*! Releases the query results. GetNext calls this automatically once query results have been
        read to the end. If partial result set is read this function should be called to make
        sure the result set is left into proper state (MySql needs this)
     */
    virtual void QuitQuery() {}

    /*! Returns number of fields currently bound */
    int GetFieldCount() { return fieldCount; }

protected:
    DdbRowSet();
    bool InsertField(DdbBoundField *newField);
    bool ValidateBind(short int type, void *data);

    DDBSTR queryStmt;            //!< Query statement.
    DdbBoundField *fieldRoot;    //!< First field of the bound field list.
    int fieldCount;              //!< Number of fields bound for this row set.
};

// =============================================================================
//  INLINE FUNCTIONS
// =============================================================================

// ==================================================================================================
inline DDBSTR DirectDatabase::GetServerName()
/*!
    Returns server name if the connection is on. Otherwice
    returns '\<No connection\>'
    \retval DDBSTR server name
*/
{
    if(flags&DDB_FLAG_CONNECTED)
        return srvName;
    else
        return DDBSTR(_T("<No connection>"));
}

// ==================================================================================================
inline DDBSTR DirectDatabase::GetDbName()
/*!
    Returns database name if the connection is on. Otherwice
    returns '\<No connection\>'
    \retval DDBSTR Database name.
*/
{
    if(flags&DDB_FLAG_CONNECTED)
        return dbName;
    else
        return DDBSTR(_T("<No connection>"));
}

// ==================================================================================================
inline void DirectDatabase::SetErrorId(int id)
/*!
    This protected function is used only by library functions. It is used
    to store the most recent error code into the database object.
    \param id Error code of the most recent error.
*/
{
    errorId = id;
}

// ==================================================================================================
inline bool DirectDatabase::IsConnected()
/*!
    \retval bool True if the database is in connected state.
*/
{
    return (flags&DDB_FLAG_CONNECTED)? true:false;
}

// ==================================================================================================
inline int DirectDatabase::IsFeatureSupported(const int featureId)
/*!
  This function is not supported currently.
  \param featureId Feature constants from the directdb.h
  \retval bool True if the given feature is supported. Otherwice function returns false.
*/
{
    return (feat_support&featureId)>0 ? true:false;
}

// ==================================================================================================
inline bool DirectDatabase::SetFeature(const int feature_new)
/*!
  Some database features (e.g. transactions) are not on by default. They need to be
  explicitly turned on with this function. Use logical OR '|' to combine more than one
  feature.

  \param feature_new     Feature to set.
  \retval bool True if the database supports the given features and the features have been
  successfully set on.
*/
{
    if(IsFeatureSupported(feature_new))
    {
        feat_on |= feature_new;
        return true;
    }
    return false;
}

#endif // if defined DIRECTDB_H_FILE

#ifdef __DDB_POSTGRE__
#include "ddbpostgre.hpp"
#endif

#ifdef __DDB_ODBCWIN__
#include "ddbodbc.hpp"
#endif

#ifdef __DDB_FIREBIRD__
#include "ddbfirebird.hpp"
#endif

#ifdef __DDB_SQLLITE__
#include "ddbsqllite.hpp"
#endif

#if defined(__DDB_MICROSOFT__) && defined(_WIN32)
#include "ddbmicrosoft.hpp"
#endif

#ifdef __DDB_MYSQL__
#include "ddbmysql.hpp"
#endif
