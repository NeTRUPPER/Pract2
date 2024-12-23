#include "pars.h"
#include "parsfile.h"
#include "insert.h"
#include "delete.h"
#include "selectWere.h"

#include "pars.cpp"
#include "parsfile.cpp"
#include "insert.cpp"
#include "delete.cpp"
#include "selectWere.cpp"

#include <iostream>
#include <sys/socket.h>  // Работа с сокетами
#include <arpa/inet.h>   // Для преобразования IP-адресов
#include <netinet/in.h>  // Структуры данных для портов
#include <unistd.h>      // Работа с системными вызовами
#include <string.h>
#include <thread>
#include <sstream>
#include <mutex>
#include <vector>

using namespace std;

// Порт, на котором будет работать сервер
const int SERVER_PORT = 7432;

// Глобальный мьютекс для синхронизации доступа к данным
mutex mtx;

// Функция обработки команды
void processCommand(const string& command, tableJson& tjs, string& response) {
    lock_guard<mutex> lock(mtx); // Блокируем доступ к структуре данных

    try {
        if (command.find("INSERT") == 0) {
            insert(command, tjs);  // Выполняем вставку
            response = "INSERT выполнен успешно.\n";
        } else if (command.find("DELETE") == 0) {
            del(command, tjs);     // Выполняем удаление
            response = "DELETE выполнен успешно.\n";
        } else if (command.find("SELECT") == 0) {
            stringstream ss;
            select(command, tjs, ss); // Выполняем выборку
            response = ss.str();
        } else if (command == "EXIT") {
            response = "EXIT: завершение соединения.\n";
        } else {
            response = "Неизвестная команда.\n";
        }
    } catch (const exception& e) {
        response = string("Ошибка: ") + e.what() + "\n";
    }
}

// Обновленная функция для обработки клиентов
void handleClient(int clientSocket, sockaddr_in clientAddress, tableJson& tjs) {
    char buffer[1024];
    string response;

    // Получение IP-адреса и порта клиента
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddress.sin_addr, clientIP, INET_ADDRSTRLEN);
    int clientPort = ntohs(clientAddress.sin_port);

    // Вывод информации о подключении в консоль администратора
    cout << "Новое подключение от клиента: " << clientIP 
         << " на порту " << clientPort << "\n";

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0) {
            cout << "Клиент " << clientIP << ":" << clientPort << " отключился.\n";
            close(clientSocket);
            break;
        }

        string command(buffer);
        command.erase(command.find_last_not_of("\n\r") + 1); // Убираем переносы строк
        if (command == "EXIT") {
            response = "EXIT: завершение соединения.\n";
            write(clientSocket, response.c_str(), response.size());
            cout << "Клиент " << clientIP << ":" << clientPort << " завершил соединение.\n";
            close(clientSocket);
            break;
        }

        processCommand(command, tjs, response);
        write(clientSocket, response.c_str(), response.size());
    }
}

int main() {
    tableJson tjs;
    parsing(tjs); // Парсинг схемы

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == 0) {
        cerr << "Ошибка создания сокета.\n";
        return 1;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "Ошибка привязки сокета.\n";
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 5) < 0) {
        cerr << "Ошибка при переходе в режим ожидания.\n";
        close(serverSocket);
        return 1;
    }

    cout << "Сервер запущен и ожидает подключения на порту " << SERVER_PORT << "...\n";

    vector<thread> clientThreads;

    while (true) {
        sockaddr_in clientAddress;
        socklen_t clientLen = sizeof(clientAddress);

        // Принятие соединения с клиента
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientLen);
        if (clientSocket < 0) {
            cerr << "Ошибка при принятии соединения.\n";
            continue;
        }

        // Создаем новый поток для обработки клиента
        clientThreads.emplace_back(handleClient, clientSocket, clientAddress, ref(tjs));
    }

    for (auto& th : clientThreads) {
        if (th.joinable()) {
            th.join();
        }
    }

    close(serverSocket);
    return 0;
}
