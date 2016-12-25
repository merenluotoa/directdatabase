/*******************************************************************************
ddbrowset.cpp
Copyright (c) Antti Merenluoto
*******************************************************************************/
#include "pch-stop.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef DDB_USEWX
#include <wx/log.h>
#else
#include <fstream>
#include <sstream>
#endif
#include "directdatabase.hpp"

// ==================================================================================================
DdbBoundField::DdbBoundField(short int type_in, void *data_in)
/*!
  Simplified constructor for the situations when the field name is not needed
  and it is known that the bound values are to be used only in queries.

  \param type_in Field type. One of DDB_TYPE...
  \param data_in Pointer to client data.
*/
{
    type = type_in;
    data = data_in;
    next = 0;
}

// ==================================================================================================
DdbBoundField::~DdbBoundField()
/*!
  Empty destructor
*/
{
}

// ==================================================================================================
DdbRowSet::DdbRowSet()
/*!
  Since this is protected member function only DirectDatabase-class
  as a friend to this class can construct these (i.e. internal use only).
*/
{
    fieldRoot = 0;
    fieldCount = 0;
}

// ==================================================================================================
DdbRowSet::~DdbRowSet()
/*!
    Rowset destructor releases all the bound variables.
*/
{
DdbBoundField *field,*nextf;

    field = fieldRoot;
    while(field)
    {
        nextf = field->next;
        delete field;
        field = nextf;
    }
    fieldRoot = 0;
}

// ==================================================================================================
bool DdbRowSet::ValidateBind(short int type, void *data)
/*!
  Function quicly validates that the type is valid and data is specified.
  \param type Data type. One of DDB_TYPE...
  \param data Pointer to actual data on the client.
*/
{
    if(!data)
        return false;
    // DDB_TYPE_USED
    if(type < DDBT_MIN || type > DDBT_MAX)
        return false;
    if(type == DDBT_BIT)
        return false;
    return true;
}

// ==================================================================================================
bool DdbRowSet::Bind(short int type, void *data)
/*!
  Function binds given field variable into this rowset. It is important to call this
  function for each variable one wants to retreive or save from/into the rowset. In
  addition the order of calls is important: first field appearing in the query result
  should be bound first and so on.

  Each time client calls the GetNext the rowset copies the values from the next row into
  the bound variables.

  \param type DDBT... for the variable. This is used to convert the database variable
   into program compatible value.
  \param data Pointer to the client side data buffer.
  \retval bool if the Bind is successfull. false if not.
*/
{
    if(!ValidateBind(type,data))
        return false;
    return InsertField(new DdbBoundField(type,data));
}

// ==================================================================================================
bool DdbRowSet::InsertField(DdbBoundField *newField)
/*!
  Inserts a new field into the bound list. Function is used by Bind functions.

   \param newField Field to insert at the end of the list.
   \retval True if the insert is succesful, false if not.
*/
{
    // If this is first field:
    if(!fieldRoot)
    {
        // Make this the root and return success.
        fieldRoot = newField;
        return true;
    }

    // While existing fields:
    DdbBoundField *field = fieldRoot;
    while(field)
    {
        // If this is the last field:
        if(!field->next)
        {
            // Add new to the end and return success.
            field->next = newField;
            fieldCount++;
            return true;
        }
        // Get next field.
        field = field->next;
    }

    // Return error.
    delete newField;
    return false;
}

