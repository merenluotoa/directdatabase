/*******************************************************************************
sstest.cpp
String stream test
Compile with: cl /nologo /TP /W4 /GR /EHsc /D_CONSOLE /DWIN32 /D_WINDOWS /Zi /MDd /D_DEBUG sstest.cpp

Copyright (c) Antti Merenluoto
*******************************************************************************/

#include <windows.h>
#include <iostream>
#include <sstream>
using namespace std;

const char *txt[] = { "1111111111", "222222222", "333333", "444444", "55555", "6666", "777", "88", "9", "" };

int main(int argc, char *argv[])
{
    int year,month,day;
    char sep;
    stringstream ss("2006-10-15 14:50:00");
//     for(int i=0; i<10; i++)
//     {
//         ss << "Line " << txt[i];
//         cout << ss.str() << endl;
//         ss.str("");
//     }

    ss >> year >> sep >> month >> sep >> day;

    cout << year << " " << month << " " << day << endl;

    return 0;
}
