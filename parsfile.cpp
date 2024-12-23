#include "parsfile.h"

// Функция для создания пути к файлам таблицы
string constructFilePath(const string& schemeName, const string& tableName, const string& fileType, int fileNumber = 0) {
    string basePath = "/home/vlad/Documents/VC Code/SecondSemestr/TEST/" + schemeName + "/" + tableName + "/";
    if (fileNumber > 0) {
        return basePath + to_string(fileNumber) + fileType; // Путь к CSV-файлу
    } else {
        return basePath + tableName + fileType; // Путь к файлам pk_sequence или lock
    }
}