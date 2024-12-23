#include "insert.h"

// Проверяем, заблокирована ли таблица, читая файл состояния блокировки
bool isLocked(const string& tableName, const string& schemeName) {
    int fileCount = 0;
    ifstream file(constructFilePath(schemeName, tableName, "_pk_sequence.txt", fileCount));
    if (!file.is_open()) {
        cerr << "Ошибка: файл блокировки не найден.\n";
        return false;
    }

    string current;
    file >> current;
    file.close();

    return current == "locked";
}

// Переключаем состояние блокировки таблицы между "locked" и "unlocked"
void toggleLock(const string& tableName, const string& schemeName) {
    int fileCount = 0;
    ifstream fileIn(constructFilePath(schemeName, tableName, "_lock.txt", fileCount));
    if (!fileIn.is_open()) {
        cerr << "Ошибка: файл блокировки не найден.\n";
        return;
    }

    string current;
    fileIn >> current;
    fileIn.close();

    ofstream fileOut(constructFilePath(schemeName, tableName, "_lock.txt", fileCount));
    fileOut << (current == "locked" ? "unlocked" : "locked");
    fileOut.close();
}

// Проверяем, существует ли таблица с заданным именем в связанном списке
bool isTableExist(const string& tableName, tNode* tableHead) {
    for (tNode* current = tableHead; current; current = current->next) {
        if (current->table == tableName) {
            return true;
        }
    }
    return false;
}

// Копируем названия колонок из одного CSV-файла в другой
void copyColumnsName(const string& fileFrom, const string& fileTo) {
    ifstream fileF(fileFrom);
    if (!fileF.is_open()) {
        cerr << "Ошибка: не удалось открыть файл для копирования колонок.\n";
        return;
    }

    string columns;
    getline(fileF, columns); // Чтение первой строки - заголовков
    fileF.close();

    ofstream fileT(fileTo);
    if (!fileT.is_open()) {
        cerr << "Не удалось открыть целевой файл: " << fileTo << "\n";
        return;
    }

    fileT << columns << endl; // Запись заголовков в новый файл
    fileT.close();
}

// Обрабатываем SQL-команду INSERT INTO для вставки данных в таблицу
void insert(const string& command, tableJson& tjs) {
    istringstream iss(command);
    string word;

    iss >> word; // "INSERT"
    iss >> word; // "INTO"
    if (word != "INTO") {
        cerr << "Некорректная команда: отсутствует 'INTO'.\n";
        return;
    }
    
    string tableName;
    iss >> tableName; // Чтение имени таблицы
    if (!isTableExist(tableName, tjs.tablehead)) {
        cerr << "Ошибка: таблицы не существует.\n";
        return;
    }
    
    iss >> word; // "VALUES"
    if (word != "VALUES") {
        cerr << "Некорректная команда: отсутствует 'VALUES'.\n";
        return;
    }
    
    // Извлекаем данных для вставки из команды
    string values;
    getline(iss, values, '('); // Пропуск до открывающей скобки
    getline(iss, values, ')'); // Извлечение содержимого между скобками

    if (isLocked(tableName, tjs.schemeName)) {
        cerr << "Таблица заблокирована: " << tableName << "\n";
        return;
    }

    toggleLock(tableName, tjs.schemeName); // Блокировка таблицы перед изменением

    // Читаем текущее значение первичного ключа
    int fileCount1 = 0;
    string pkFile = constructFilePath(tjs.schemeName, tableName, "_pk_sequence.txt", fileCount1);
    
    ifstream pkIn(pkFile);
    int currentPk = 0;
    if (pkIn.is_open()) {
        pkIn >> currentPk; // Чтение текущего значения первичного ключа
        pkIn.close();
    }

    // Обновление значения первичного ключа
    ofstream pkOut(pkFile);
    if (!pkOut.is_open()) {
        cerr << "Не удалось открыть файл первичных ключей: " << pkFile << "\n";
        toggleLock(tableName, tjs.schemeName);
        return;
    }
    pkOut << ++currentPk; // Инкремент и запись нового значения
    pkOut.close();

    // Поиск подходящего CSV-файла для записи данных
    int csvNum = 1;
    string csvFile;
    while (true) {
        int fileCount2 = 1;
        csvFile = constructFilePath(tjs.schemeName, tableName, ".csv", fileCount2);
        ifstream csvCheck(csvFile);

        // Если файл не существует или он пустой
        if (!csvCheck.is_open()) {
            break; // Новый файл будет создан
        }

        try {
            rapidcsv::Document doc(csvFile);
            if (doc.GetRowCount() < tjs.tableSize) {
                break; // Нашли файл с доступным местом
            }
        } catch (const std::exception& e) {
            cerr << "Ошибка при работе с файлом " << csvFile << ": " << e.what() << "\n";
            break; // Останавливаем обработку при ошибке
        }

        csvNum++;
    }

    // Копирование структуры заголовков, если файл новый
    if (rapidcsv::Document(csvFile).GetRowCount() == 0) {
        int fileCount3 = 1;
        copyColumnsName(constructFilePath(tjs.schemeName, tableName, ".csv", fileCount3), csvFile);
    }

    // Запись данных в CSV-файл
    ofstream csv(csvFile, ios::app);
    if (!csv.is_open()) {
        cerr << "Ошибка: не удалось открыть CSV файл для записи.\n";
        toggleLock(tableName, tjs.schemeName);
        return;
    }
    // Форматирование записи: первичный ключ и значения
    csv << currentPk << ",";
    for (size_t i = 0; i < values.size(); ++i) {
        if (values[i] == '\'') {
            ++i; // Пропуск символа начала значения
            while (values[i] != '\'') {
                csv << values[i++]; // Запись значения
            }
            // Добавляем либо разделитель (если есть ещё значения), либо конец строки
            if (i + 1 < values.size() && values[i + 1] == ',') {
                csv << ",";
            } else {
                csv << "\n";
            }
        }
    }
    csv.close();
    toggleLock(tableName, tjs.schemeName); // Разблокировка таблицы после изменения

}
