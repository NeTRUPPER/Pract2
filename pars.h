#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>  // Директории
#include "json.hpp"    // JSON
#include "insert.h"    // Структура таблиц

using namespace std;
using json = nlohmann::json; 
namespace fs = filesystem;

struct tableJson { 
    int tableSize;          // Размер колонок
    string schemeName;      // Имя схемы
    tNode* tablehead;       // Указатель на голову таблицы
};

void removeDirectory(const fs::path& directoryPath); // Удаление директории

void createDirectoriesAndFiles(const fs::path& schemePath, const json& structure, tableJson& tjs); // Создание полной директории и файлов

void parsing(tableJson& tjs); // Парсинг схемы
