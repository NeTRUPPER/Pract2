#include "delete.h"

// Проверяем наличие колонки в указанной таблице
bool isColumnExist(const string& tableName, const string& columnName, tNode* tableHead) {
    tNode* currentTable = tableHead; // Указатель на начало списка таблиц
    while (currentTable) {
        if (currentTable->table == tableName) { // Ищем заданную таблицу
            Node* currentColumn = currentTable->column; // Указатель на первую колонку таблицы
            while (currentColumn) {
                if (currentColumn->column == columnName) { // Проверяем колонку
                    return true; // Колонка найдена
                }
                currentColumn = currentColumn->next; // Переходим к следующей колонке
            }
            return false; // Колонка отсутствует в таблице
        }
        currentTable = currentTable->next; // Переходим к следующей таблице
    }
    return false; // Таблица не найдена
}

// Удаляем строки из таблицы на основании команды
void del(const string& command, tableJson& tjs) {
    istringstream iss(command); // Поток ввода для обработки команды
    string word;
    iss >> word; // Чтение "DELETE"
    iss >> word; // Чтение "FROM"
    if (word != "FROM") {
        cerr << "Некорректная команда.\n";
        return;
    }

    string tableName; // Имя таблицы
    iss >> word; // Чтение имени таблицы
    if (!isTableExist(word, tjs.tablehead)) { // Проверка существования таблицы
        cerr << "Такой таблицы нет.\n";
        return;
    }
    tableName = word;

    string secondCmd; // Вторая часть команды
    getline(cin, secondCmd);
    istringstream iss2(secondCmd); // Поток ввода для второй части команды
    string word2;
    iss2 >> word2; // Чтение "WHERE"
    if (word2 != "WHERE") {
        cerr << "Некорректная команда.\n";
        return;
    }

    iss2 >> word2; // Чтение "таблица.колонка"
    string table, column;
    bool dot = false; // Флаг для разделения таблицы и колонки
    for (size_t i = 0; i < word2.size(); i++) {
        if (word2[i] == '.') {
            dot = true;
            continue;
        }
        if (!dot) {
            table += word2[i]; // Имя таблицы
        } else {
            column += word2[i]; // Имя колонки
        }
    }

    if (!dot) { // Если разделитель отсутствует
        cerr << "Некорректная команда.\n";
        return;
    }

    if (table != tableName) { // Проверка соответствия таблицы
        cerr << "Некорректная команда.\n";
        return;
    }

    if (!isColumnExist(tableName, column, tjs.tablehead)) { // Проверка существования колонки
        cerr << "Такой колонки нет.\n";
        return;
    }

    iss2 >> word2; // Чтение оператора "="
    if (word2 != "=") {
        cerr << "Некорректная команда.\n";
        return;
    }

    string value; // Значение для удаления
    iss2 >> word2;
    if (word2[0] != '\'' || word2[word2.size() - 1] != '\'') { // Проверка на кавычки
        cerr << "Некорректная команда.\n";
        return;
    }

    // Извлечение значения из кавычек
    for (size_t i = 1; i < word2.size() - 1; i++) {
        value += word2[i];
    }

    if (isLocked(tableName, tjs.schemeName)) { // Проверка блокировки таблицы
        cerr << "Таблица заблокирована.\n";
        return;
    }

    toggleLock(tableName, tjs.schemeName); // Блокировка таблицы

    int amountCsv = 1; // Подсчет количества файлов CSV
    while (true) {
        int fileCount = 1;
        ifstream file(constructFilePath(tjs.schemeName, tableName, ".csv", amountCsv));
        if (!file.is_open()) { // Если файл не открыт, его не существует
            break;
        }
        file.close();
        amountCsv++;
    }

    bool deletedStr = false; // Флаг успешного удаления строк
    for (size_t iCsv = 1; iCsv < amountCsv; iCsv++) { // Просмотр всех CSV файлов
        int fileCount = 1;
        string filePath = constructFilePath(tjs.schemeName, tableName, ".csv", iCsv);
        rapidcsv::Document doc(filePath); // Открытие CSV файла

        int columnIndex = doc.GetColumnIdx(column); // Получение индекса колонки
        size_t amountRow = doc.GetRowCount(); // Получение количества строк

        for (size_t i = 0; i < amountRow; ++i) { // Обход строк
            if (doc.GetCell<string>(columnIndex, i) == value) { // Проверка значения в ячейке
                doc.RemoveRow(i); // Удаление строки
                deletedStr = true;
                --amountRow; // Коррекция количества строк
                --i; // Сдвиг индекса для повторной проверки
            }
        }
        doc.Save(filePath); // Сохранение изменений
    }

    if (!deletedStr) { // Если значение не найдено
        cout << "Указанное значение не найдено.\n";
    }

    toggleLock(tableName, tjs.schemeName); // Разблокировка таблицы
}
