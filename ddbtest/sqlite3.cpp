/*******************************************************************************
sqlite.cpp
Copyright (c) Antti Merenluoto
*******************************************************************************/

#include <windows.h>
#include <iostream>
#include <sstream>
using namespace std;

#include <sqlite3.h>

int main(int argc, char **argv)
{
    int ret;
    sqlite3 *connection;

    // Open the database.
    if(sqlite3_open(argv[1],&connection) != SQLITE_OK)
    {
        cout << sqlite3_errmsg(connection);
        return 1;
    }

    // Close the database.
    sqlite3_close(connection);
    return 0;
}
