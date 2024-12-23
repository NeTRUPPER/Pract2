#pragma once
#include <iostream>
#include <fstream>
#include "insert.h"

using namespace std;


void del(const string& command, tableJson& tjs); // Удаление строк из таблицы на основе команды


bool isColumnExist(const string& tableName, const string& columnName, tNode* tableHead); // Проверка наличия колонки в таблице


