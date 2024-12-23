#include "selectWere.h"

// Разделение строки вида <таблица>.<колонка> на таблицу и колонку
void splitDot(const string& word, string& table, string& column, tableJson& tjs) { 
    bool dot = false;  // Поиск точки
    for (size_t i = 0; i < word.size(); i++) {
        if (word[i] == '.') {
            dot = true;
            continue;
        }
        if (word[i] == ',') {
            continue;
        }
        if (!dot) {  // Разделяем таблицу и колонку
            table += word[i];
        } else {
            column += word[i];
        }
    }
    if (!dot) {  // Если точка не найдена
        cerr << "Некорректная команда.\n";
        return;
    }
    if (isTableExist(table, tjs.tablehead) == false) {  // Проверка на существование таблицы
        cerr << "Такой таблицы нет.\n";
        return;
    }
    if (isColumnExist(table, column, tjs.tablehead) == false) {  // Проверка на существование колонки
        cerr << "Такой колонки нет.\n";
        return;
    }
}

// Удаление одинарных кавычек из строки
std::string ignoreQuotes(const std::string& word) {
    std::string result;
    for (char ch : word) {
        if (ch != '\'') {
            result += ch;
        }
    }
    return result;
}

// Удаление пробелов, табуляций и переносов строк с начала и конца строки
std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    size_t end = str.find_last_not_of(" \t\r\n");
    return (start == std::string::npos || end == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

// Проверка на наличие точки в строке
bool findDot(const std::string& word) {
    return word.find('.') != std::string::npos;
}

// Подсчет количества CSV-файлов, относящихся к таблице
int countCsv(tableJson& tjs, const std::string& table) {
    int csvCount = 1;

    while (true) {
        
        std::string filePath = "/home/vlad/Documents/VC Code/SecondSemestr/TEST/" + tjs.schemeName + "/" + table + "/" + to_string(csvCount) + ".csv";

        std::ifstream file(filePath);
        if (!file) {
            break; // Файл не существует
        }
        file.close();
        csvCount++;
    }

    return csvCount;
}

// Выполнение CROSS JOIN между двумя таблицами по заданным колонкам
void crossJoin(tableJson& tjs, const std::string& table1, const std::string& table2, const std::string& column1, const std::string& column2, stringstream& ss) {
    size_t csvCount1 = countCsv(tjs, table1);
    size_t csvCount2 = countCsv(tjs, table2);

    for (size_t csvIndex1 = 1; csvIndex1 < csvCount1; csvIndex1++) {

        int fileCount = 1;
        rapidcsv::Document doc1(constructFilePath(tjs.schemeName, table1, ".csv", fileCount));
        size_t columnIndex1 = doc1.GetColumnIdx(column1);

        if (columnIndex1 == -1) {
            std::cerr << "Колонка \"" << column1 << "\" не найдена в таблице \"" << table1 << "\".\n";
            return;
        }

        size_t rowCount1 = doc1.GetRowCount();

        for (size_t row1 = 0; row1 < rowCount1; row1++) {
            for (size_t csvIndex2 = 1; csvIndex2 < csvCount2; csvIndex2++) {

                rapidcsv::Document doc2(constructFilePath(tjs.schemeName, table2, ".csv", fileCount));
                size_t columnIndex2 = doc2.GetColumnIdx(column2);

                if (columnIndex2 == -1) {
                    std::cerr << "Колонка \"" << column2 << "\" не найдена в таблице \"" << table2 << "\".\n";
                    return;
                }

                size_t rowCount2 = doc2.GetRowCount();

                for (size_t row2 = 0; row2 < rowCount2; row2++) {
                    // Сравнение значений в указанных колонках
                    if (trim(doc1.GetCell<std::string>(columnIndex1, row1)) == trim(doc2.GetCell<std::string>(columnIndex2, row2))) {
                        ss << doc1.GetCell<std::string>(0, row1) << ": ";
                        ss << doc1.GetCell<std::string>(columnIndex1, row1) << "  |   ";
                        ss << doc2.GetCell<std::string>(0, row2) << ": ";
                        ss << doc2.GetCell<std::string>(columnIndex2, row2) << "\n";
                        
                    }
                }
            }
        }
    }
}

// Функция проверки условий для SQL-выражения WHERE
bool checkCond(tableJson& tjs, const string& table, const string& column, const string& tcond, const string& ccond, const string& s) {
    // Проверка простого условия (прямое сравнение)
    if (!s.empty()) {
        int amountCsv = countCsv(tjs, table); // Подсчёт количества CSV-файлов для таблицы
        for (int iCsv = 1; iCsv <= amountCsv; iCsv++) {
            // Формирование пути к текущему CSV-файлу
            int fileCount = 1;
            rapidcsv::Document doc(constructFilePath(tjs.schemeName, table, ".csv", fileCount));
            int columnIndex = doc.GetColumnIdx(column); // Получение индекса указанной колонки
            int amountRow = doc.GetRowCount();

            // Проверка каждой строки на совпадение значения
            for (int i = 0; i < amountRow; ++i) {
                if (doc.GetCell<string>(columnIndex, i) == s) {
                    return true; // Условие выполнено
                }
            }
        }
    } else {
        // Сложное условие (сравнение между двумя колонками из возможных разных таблиц)
        bool condition = true;
        int amountCsv = countCsv(tjs, table);

        for (int iCsv = 1; iCsv <= amountCsv; iCsv++) {
            // Пути к файлам первичных ключей для двух сравниваемых таблиц
            int fileCount = 0;
            string pk1, pk2;
            ifstream file1(constructFilePath(tjs.schemeName, table, "_pk_sequence.txt", fileCount)), file2(constructFilePath(tjs.schemeName, tcond, "_pk_sequence.txt", fileCount));

            // Убедиться, что оба файла первичных ключей открыты
            if (!file1.is_open() || !file2.is_open()) {
                cerr << "Не удалось открыть файлы первичных ключей для проверки.\n";
                return false;
            }

            file1 >> pk1;
            file2 >> pk2;

            // Если первичные ключи не совпадают, условие не выполнено
            if (pk1 != pk2) {
                return false;
            }

            // Пути к текущим CSV-файлам для двух таблиц
            int fileCount2 = 1;
            rapidcsv::Document doc1(constructFilePath(tjs.schemeName, table, ".csv", fileCount2)), doc2(constructFilePath(tjs.schemeName, tcond, ".csv", fileCount2));
            int columnIndex1 = doc1.GetColumnIdx(column);
            int columnIndex2 = doc2.GetColumnIdx(ccond);
            int amountRow1 = doc1.GetRowCount();

            // Сравнение соответствующих строк между двумя колонками
            for (int i = 0; i < amountRow1; i++) {
                if (doc1.GetCell<string>(columnIndex1, i) != doc2.GetCell<string>(columnIndex2, i)) {
                    condition = false;
                }
            }
        }
        if (condition) {
            return true;
        }
    }
    return false;
}

// Функция обработки команды SQL SELECT
void select(const string& command, tableJson& tjs, stringstream& ss) {
    istringstream iss(command);
    string word;
    iss >> word;  // Ожидается "SELECT"
    if (word != "SELECT") {
        cerr << "Неверная команда.\n";
        return;
    }

    // Разбор первой таблицы и колонки
    iss >> word;
    string table1, column1;
    splitDot(word, table1, column1, tjs);

    // Разбор второй таблицы и колонки
    iss >> word;
    string table2, column2;
    splitDot(word, table2, column2, tjs);

    // Чтение и разбор второй строки команды
    string secondCmd;
    getline(cin, secondCmd);
    istringstream iss2(secondCmd);

    iss2 >> word;  // Ожидается "FROM"
    if (word != "FROM") {
        cerr << "Неверная команда.\n";
        return;
    }

    iss2 >> word;
    string tab1;
    for (char ch : word) {
        if (ch != ',') tab1 += ch; // Удаление запятых из имени таблицы
    }
    if (tab1 != table1) {
        cerr << "Неверная команда.\n";
        return;
    }

    iss2 >> word;
    if (word != table2) {
        cerr << "Неверная команда.\n";
        return;
    }

    // Чтение и разбор третьей строки команды
    string thirdCmd;
    getline(cin, thirdCmd);
    istringstream iss3(thirdCmd);

    iss3 >> word;  // Ожидается "WHERE" или ничего
    if (word != "WHERE") {
        crossJoin(tjs, table1, table2, column1, column2, ss); // Выполнить CROSS JOIN, если нет условия WHERE
        return;
    }

    // Разбор первого условия в выражении WHERE
    iss3 >> word;
    string t1, c1;
    splitDot(word, t1, c1, tjs);

    iss3 >> word;  // Ожидается "="
    if (word != "=") {
        cerr << "Неверная команда.\n";
        return;
    }

    iss3 >> word;
    string t1cond = "", c1cond = "", s1 = "";
    if (findDot(word)) {
        splitDot(word, t1cond, c1cond, tjs);
    } else {
        s1 = ignoreQuotes(word); // Извлечение значения без кавычек
    }

    // Разбор опционального оператора AND/OR
    iss3 >> word;
    string oper = word;
    if (oper != "AND" && oper != "OR") {
        // Случай одного условия
        if (checkCond(tjs, t1, c1, t1cond, c1cond, s1)) {
            crossJoin(tjs, table1, table2, column1, column2, ss);
        } else {
            cout << "Условие не выполнено.\n";
        }
        return;
    }

    // Разбор второго условия в выражении WHERE
    iss3 >> word;
    string t2, c2;
    splitDot(word, t2, c2, tjs);

    iss3 >> word;  // Ожидается "="
    if (word != "=") {
        cerr << "Неверная команда.\n";
        return;
    }

    iss3 >> word;
    string t2cond = "", c2cond = "", s2 = "";
    if (findDot(word)) {
        splitDot(word, t2cond, c2cond, tjs);
    } else {
        s2 = ignoreQuotes(word); // Извлечение значения без кавычек
    }

    // Оценка условий AND/OR
    if (oper == "AND") {
        if (checkCond(tjs, t1, c1, t1cond, c1cond, s1) && checkCond(tjs, t2, c2, t2cond, c2cond, s2)) {
            crossJoin(tjs, table1, table2, column1, column2, ss);
        } else {
            cout << "Условие не выполнено.\n";
        }
    } else if (oper == "OR") {
        if (checkCond(tjs, t1, c1, t1cond, c1cond, s1) || checkCond(tjs, t2, c2, t2cond, c2cond, s2)) {
            crossJoin(tjs, table1, table2, column1, column2, ss);
        } else {
            cout << "Условие не выполнено.\n";
        }
    }
}