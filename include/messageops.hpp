#ifndef MESSAGEOPS_HPP 
#define MESSAGEOPS_HPP

#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <sqlite3.h>

std::atomic<bool> command_entered = false;

void execute_sql(sqlite3* DB, const std::string& sql) {
    char* errmsg;
    int rc = sqlite3_exec(DB, sql.c_str(), 0, 0, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errmsg << std::endl;
        sqlite3_free(errmsg);
    }
}

int adjust_indexes(sqlite3* DB) {
    std::string sql = "WITH RECURSIVE cnt(x) AS ( "
                      "SELECT 1 UNION ALL SELECT x+1 FROM cnt LIMIT (SELECT COUNT(*) FROM people) "
                      ") "
                      "UPDATE people "
                      "SET id = (SELECT x FROM cnt WHERE cnt.x = (SELECT COUNT(*) FROM people p2 WHERE p2.rowid <= people.rowid));";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(DB, sql.c_str(), -1, &stmt, NULL);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? 0 : 1;
}

void displayMessageHistory(const std::string& dbname) {
    sqlite3* DB;
    sqlite3_open(dbname.c_str(), &DB);

    std::string sql = "SELECT * FROM MSG_LOGS;";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(DB, sql.c_str(), -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char* person = sqlite3_column_text(stmt, 1);
            const unsigned char* message = sqlite3_column_text(stmt, 2);
            const unsigned char* timestamp = sqlite3_column_text(stmt, 3);

            std::cout << id << " " << person << " " << message << " " << timestamp << "\n";
        }
        std::cout << "-----------------\n";
    } else {
        std::cerr << "Failed to retrieve data from the database.\n";
    }

    sqlite3_finalize(stmt);
    sqlite3_close(DB);
}

void edit_message(const std::string& dbname, std::size_t id) {
    std::cout << "New Message: ";
    std::string newmsg;
    std::getline(std::cin, newmsg);

    sqlite3* DB;
    sqlite3_open(dbname.c_str(), &DB);

    std::string sql = "UPDATE MSG_LOGS SET MESSAGE = '" + newmsg + "' WHERE ID = " + std::to_string(id) + ";";
    execute_sql(DB, sql);

    sqlite3_close(DB);
}

void delete_message(const std::string& dbname, std::size_t id)
{
    sqlite3* DB;
    sqlite3_open(dbname.c_str(), &DB);

    std::string sql = "DELETE FROM MSG_LOGS WHERE ID = " + std::to_string(id) + ";";
    execute_sql(DB,sql);
    adjust_indexes(DB);

    sqlite3_close(DB);
}

bool executeCommands(std::string message, const std::string& dbname) {
    message.erase(std::remove_if(message.begin(), message.end(), ::isspace), message.end());
    if (message == ":e") 
    {
        displayMessageHistory(dbname);
        std::string id;
        std::cout << "Enter the index of the message you want to edit: ";
        std::getline(std::cin, id);
        edit_message(dbname, std::stoi(id));
        std::cout << "Updated!\n";
        return true;
    } 
    else if (message == ":v") 
    {
        displayMessageHistory(dbname);
        return true;
    } 
    else if (message == ":d")
    {
        displayMessageHistory(dbname);
        std::string id;
        std::cout << "Enter the index of the message you want to delete: ";
        std::getline(std::cin, id);
        delete_message(dbname, std::stoi(id));
        std::cout << "Deleted!\n";
        return true;
    }
    else if (message == ":h") 
    {
        std::cout << ":v - View Message History\n";
        std::cout << ":e - Edit Message\n";
        std::cout << ":d - Delete Message\n";
        std::cout << ":q - Quit\n";
        return true;
    } else if (message == ":q")
        exit(0);
    return false;
}

void insert_message(sqlite3* DB, const std::string& person, const std::string& message, const std::string& timestamp) {
    std::string sql = "INSERT INTO MSG_LOGS (PERSON, MESSAGE, TIME) VALUES ('" + person + "', '" + message + "', '" + timestamp + "');";
    execute_sql(DB, sql);
}

#endif
