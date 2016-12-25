/*******************************************************************************
mysqltest.cpp
Copyright (c) Antti Merenluoto

Call the program e.g. with following values: gsapp scoregym 10.1.1.37 gymscore
*******************************************************************************/

#include <windows.h>
#include "mysql.h"
#include <iostream>
#include <sstream>
using namespace std;

const char *g_create_table =
"CREATE TABLE ddb_demo ("\
"id int NOT NULL AUTO_INCREMENT"\
",ts timestamp"\
",data varchar(255)"\
",tf boolean"\
",PRIMARY KEY(id)"\
") ENGINE=InnoDB";
const char *g_create_index =
"CREATE UNIQUE INDEX demo_ndx ON ddb_demo(id)";

void Usage()
{
    cout << "Usage: " << endl;
    cout << "  mysqltest [uid] [pwd] [host] [database]" << endl;
}

int main(int argc, char **argv)
{
    int i;
    stringstream query;
    my_ulonglong rows,insert_id;
    MYSQL connection;
    MYSQL_RES *result = NULL; /* To be used to fetch information into */
    MYSQL_ROW row;

    if(argc<4){
        Usage();
        return 1;
    }

    // Initialize connection structure
    mysql_init(&connection);

    // Set options
    unsigned int timeout = 5;
    mysql_options(&connection, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&timeout);

    if(!mysql_real_connect(&connection, argv[3], argv[1], argv[2], argv[4],0,0,0)){
        cerr << mysql_error(&connection) << endl;
        return 1;
    }

    cout << endl << "Connection OK.\nCreating table: " << g_create_table << endl;
    if(mysql_real_query(&connection, g_create_table, strlen(g_create_table)))
    {
        cout << "#!# Unable to create table:\n  " << mysql_error(&connection) << endl;
        goto CLOSE;
    }

//     cout << "Creating index: " << g_create_index << endl;
//     if(mysql_real_query(&connection, g_create_index, strlen(g_create_index)))
//     {
//         cout << "#!# Unable to create index:\n  " << mysql_error(&connection) << endl;
//         goto CLOSE;
//     }

    // ................................................................................
    cout << endl << "Inserting 20 items... ";
    for(i=0; i<20; i++)
    {
        query << "INSERT INTO ddb_demo(ts, data, tf) values(CURRENT_TIMESTAMP(), 'Test item "<< i+1 << "', ";
        query << (int)((i%2)?1:0) << ")";
        if(mysql_real_query(&connection, query.str().c_str(), query.str().length()))
        {
            cout << "#!# Insert failure:\n  " << mysql_error(&connection) << endl;
            goto CLOSE;
        }
        rows = mysql_affected_rows(&connection);
        if(rows != 1)
            cout << endl << "Affected rows " << rows << " on line " << i+1 << endl;
        insert_id = mysql_insert_id(&connection);
        cout << insert_id << ' ';
        query.str("");
    }
    cout << endl;

    // ................................................................................
    cout << endl << "Reading items..." << endl;
    query << "SELECT id, ts, data, tf FROM ddb_demo";
    if(mysql_real_query(&connection, query.str().c_str(), query.str().length()))
    {
        cout << "#!# Query failure:\n  " << mysql_error(&connection) << endl;
        goto CLOSE;
    }
    query.str("");
    result = mysql_use_result(&connection);
    if(!result)
    {
        cout << "#!# Unable to get result:\n  " << mysql_error(&connection) << endl;
        goto CLOSE;
    }
    if(mysql_num_fields(result) < 4)
    {
        cout << "#!# Too few fields in the query.\n";
        goto CLOSE;
    }
    while( (row=mysql_fetch_row(result)) )
    {
        cout << row[0] << " " << row[1] << " " << row[2] << " " << row[3] << endl;
    }
    mysql_free_result(result);
    result = 0;

    // ................................................................................
    cout << endl << "Deleting table and closing connection...";
    query << "DROP TABLE ddb_demo";
    if(mysql_real_query(&connection, query.str().c_str(), query.str().length()))
    {
        cout << "#!# Unable drop the table 'ddb_demo':\n  " << mysql_error(&connection) << endl;
        goto CLOSE;
    }

 CLOSE:
    if(result)
        mysql_free_result(result);
    mysql_close(&connection);
    cout << endl << "Connection closed. Bye" << endl;
    return 0;
}
