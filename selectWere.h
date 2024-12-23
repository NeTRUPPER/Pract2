#pragma once

#include <iostream>
#include <sstream>
#include "insert.h"
#include "delete.h"

using namespace std;

void splitDot(const string& word, string& table, string& column, tableJson& tjs); // Разделяет строку на таблицу и столбец, если в строке есть точка

string ignoreQuotes(const string& word); // Убирает кавычки из начала и конца строки

string trim(const string& str); // Убирает пробелы и табуляции в начале и конце строки

bool findDot(const string& word); // Проверяет, содержит ли строка точку

int countCsv(tableJson& tjs, const string& table); // Подсчитывает количество строк в CSV-файле, связанном с указанной таблицей

void crossJoin(tableJson& tjs, const string& table1, const string& table2, const string& column1, const string& column2, bool detect, stringstream& result); // Выполняет операцию кросс-джойна между двумя таблицами по указанным столбцам

bool checkCond(tableJson& tjs, const string& table, const string& column, const string& tcond, const string& ccond, const string& s); // Проверяет условие для указанной таблицы и столбца

void select(const string& command, tableJson& tjs, stringstream& result); // Выполняет SQL-запрос SELECT и возвращает результат
