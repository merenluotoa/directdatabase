/*******************************************************************************
sqlite3_test.cpp
Copyright (c) Antti Merenluoto
*******************************************************************************/

#include <iostream>
#include <sstream>
using namespace std;

#include <sqlite3.h>

const char *g_create_table =
"CREATE TABLE ddb_demo ("\
"id int NOT NULL"\
",ts timestamp"\
",data varchar(255)"\
",tf boolean"\
",PRIMARY KEY(id)"\
")";

int Sqlite3CallBack(void*,int count, char **value, char **colname)
{
    const char *val;
    cout << count << " cols as result\n";
    for(int i=0; i<count; i++)
    {
        val = value[i] ? value[i] : "null";
        cout << colname[i] << " = " << val << endl;
    }
    cout << endl;
    return 0;
}

void CreateTable(sqlite3 *con)
{
    char *emsg;
    int rv = sqlite3_exec(con,g_create_table,Sqlite3CallBack,0,&emsg);
    if(rv != SQLITE_OK)
        cout << "Create table failure: "<<rv<<"\n"<<sqlite3_errmsg(con) << endl;
    else
        cout << "Test table created\n";
}

int main(int argc, char **argv)
{
    int ret;
    sqlite3 *connection;

    if(argc == 1)
    {
        cout << "Missing database argument.\n";
        return 1;
    }
    // Open the database.
    if(sqlite3_open(argv[1],&connection) != SQLITE_OK)
    {
        cout << sqlite3_errmsg(connection);
        return 1;
    }
    CreateTable(connection);

    // Close the database.
    sqlite3_close(connection);
    return 0;
}
