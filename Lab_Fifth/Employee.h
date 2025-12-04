#pragma once
#include <windows.h>

struct employee
{
    int num;
    char name[10];
    double hours;
};

enum CommandType {
    CMD_READ,
    CMD_MODIFY,
    CMD_EXIT
};

struct Request {
    CommandType cmd;
    int employeeID;
};

struct Response {
    bool status;
    employee record;
};