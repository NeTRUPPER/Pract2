#pragma once

#include <iostream>
#include "pars.h"
#include "parsfile.h"
#include "rapidcsv.hpp" // библиотека для работы с CSV

using namespace std;

struct Node {          // Односвязный список для колонок
    string column;     // Название колонки
    Node* next;        // Указатель на следующую колонку
};

struct tNode {         // Односвязный список для таблиц
    string table;      // Название таблицы
    Node* column;      // Указатель на первую колонку таблицы
    tNode* next;       // Указатель на следующую таблицу
};

bool isLocked(const string& tableName, const string& schemeName);    // Проверка блокировки таблицы

void toggleLock(const string& tableName, const string& schemeName);  // Изменение состояния блокировки таблицы

bool isTableExist(const string& tableName, tNode* tableHead);        // Проверка существования таблицы

void copyColumnsName(const string& fileFrom, const string& fileTo);  // Копирование названий колонок из одного файла в другой

void insert(const string& command);                                  // Вставка данных в таблицу
