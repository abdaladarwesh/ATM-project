#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sqlite3.h>
using namespace std;

// Database initialization
sqlite3* db;

void executeSQL(const string& sql) {
    char* errMsg = 0;
    if (sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg) != SQLITE_OK) {
        cerr << "SQL Error: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
}

void initializeDatabase() {
    if (sqlite3_open("bank.db", &db)) {
        cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
        exit(1);
    }
    string sql = "CREATE TABLE IF NOT EXISTS users ("
                 "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "first_name TEXT,"
                 "last_name TEXT,"
                 "phone_num TEXT UNIQUE,"
                 "money INTEGER,"
                 "password TEXT);";
    executeSQL(sql);
}

struct User {
    int id;
    string first_name;
    string last_name;
    string phone_num;
    int money;
    string password;
};

bool getUser(const string& phone, User& user) {
    string sql = "SELECT * FROM users WHERE phone_num='" + phone + "';";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            user.id = sqlite3_column_int(stmt, 0);
            user.first_name = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            user.last_name = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            user.phone_num = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            user.money = sqlite3_column_int(stmt, 4);
            user.password = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
            sqlite3_finalize(stmt);
            return true;
        }
    }
    sqlite3_finalize(stmt);
    return false;
}

void addUser(User& user) {
    string sql = "INSERT INTO users (first_name, last_name, phone_num, money, password) VALUES ('" +
                 user.first_name + "', '" + user.last_name + "', '" + user.phone_num + "', " + to_string(user.money) + ", '" + user.password + "');";
    executeSQL(sql);
}

bool verifyPassword(const string& storedPassword) {
    int attempts = 3;
    while (attempts > 0) {
        string inputPassword;
        cout << "Enter your password: ";
        cin >> inputPassword;
        if (inputPassword == storedPassword) {
            return true;
        }
        attempts--;
        cout << "Incorrect password! You have " << attempts << " attempts remaining.\n";
    }
    cout << "Too many incorrect attempts. Returning to main menu.\n";
    return false;
}

void transferMoney(User& sender) {
    string receiverPhone;
    cout << "Enter recipient phone number (must be 11 digits): ";
    cin >> receiverPhone;
    if (receiverPhone.length() != 11) {
        cout << "Invalid phone number! It must be exactly 11 digits.\n";
        return;
    }

    User receiver;
    if (!getUser(receiverPhone, receiver)) {
        cout << "Recipient not found!\n";
        return;
    }

    int amount;
    cout << "Enter amount to transfer: ";
    cin >> amount;

    if (amount > sender.money) {
        cout << "Insufficient balance!\n";
        cout << "Current balance: " << sender.money << " EGP\n";
        return;
    }

    if (!verifyPassword(sender.password)) {
        return;
    }

    sender.money -= amount;
    receiver.money += amount;

    executeSQL("UPDATE users SET money = " + to_string(sender.money) + " WHERE phone_num = '" + sender.phone_num + "';");
    executeSQL("UPDATE users SET money = " + to_string(receiver.money) + " WHERE phone_num = '" + receiver.phone_num + "';");
    cout << "Transfer successful!\n";
    cout << "Current balance: " << sender.money << " EGP\n";

}

void changePassword(User& user) {
    if (!verifyPassword(user.password)) {
        return;
    }
    cout << "Enter new password: ";
    cin >> user.password;
    executeSQL("UPDATE users SET password = '" + user.password + "' WHERE phone_num = '" + user.phone_num + "';");
    cout << "Password changed successfully!\n";
}

void showProfile(const User& user) {
    cout << "\n=========== User Profile ===========\n";
    cout << "Name: " << user.first_name << " " << user.last_name << "\n";
    cout << "Phone: " << user.phone_num << "\n";
    cout << "Balance: " << user.money << " EGP\n";
}

void userOperations(User& user) {
    int operation;
    do {
        cout << "\nChoose your operation:\n";
        cout << "1- Check Balance\n2- Deposit\n3- Withdraw\n4- Transfer Money\n5- Change Password\n6- Show Profile\n7- Return to Main Menu\n8- Exit\nEnter your choice: ";
        cin >> operation;

        switch (operation) {
        case 1:
            cout << "Your current balance: " << user.money << " EGP\n";
            break;
        case 2: {
            int deposit;
            cout << "Enter the amount to deposit: ";
            cin >> deposit;
            user.money += deposit;
            executeSQL("UPDATE users SET money = " + to_string(user.money) + " WHERE phone_num = '" + user.phone_num + "';");
            cout << "Cpdated balance: " << user.money << " EGP\n";
            cout << "Deposit successful!\n";
            break;
        }
        case 3: {
            int withdraw;
            cout << "Enter the amount to withdraw: ";
            cin >> withdraw;
            if (withdraw <= user.money) {
                if (!verifyPassword(user.password)) {
                    return;
                }
                user.money -= withdraw;
                executeSQL("UPDATE users SET money = " + to_string(user.money) + " WHERE phone_num = '" + user.phone_num + "';");
                cout << "Withdraw successful!\n";
                cout << "Current balance: " << user.money << " EGP\n";
            } else {
                cout << "Insufficient balance!\n";
                cout << "Current balance: " << user.money << " EGP\n";
            }
            break;
        }
        case 4:
            transferMoney(user);
            break;
        case 5:
            changePassword(user);
            break;
        case 6:
            showProfile(user);
            break;
        case 7:
            cout << "Returning to Main Menu...\n";
            return;
        case 8:
            cout << "Exiting... Goodbye!\n";
            exit(0);
        default:
            cout << "Invalid choice! Try again.\n";
        }
    } while (operation != 7);
}

int main() {
    initializeDatabase();
    while (true) {
        string phone;
        cout << "Enter your Phone Number (must be 11 digits): ";
        cin >> phone;
        if (phone.length() != 11) {
            cout << "Invalid phone number! It must be exactly 11 digits.\n";
            continue;
        }

        User user;
        if (getUser(phone, user)) {
            string password;
            cout << "Enter your password: ";
            cin >> password;
            if (password == user.password) {
                cout << "\nWelcome back, " << user.first_name << "!\n";
                userOperations(user);
            } else {
                cout << "Incorrect password!\n";
            }
        } else {
            cout << "New account detected. Enter details:\n";
            user.phone_num = phone;
            cout << "Enter first name: ";
            cin >> user.first_name;
            cout << "Enter last name: ";
            cin >> user.last_name;
            cout << "Enter password: ";
            cin >> user.password;
            user.money = 0;
            addUser(user);
            cout << "Account created successfully!\n";
            userOperations(user);
        }
    }
    sqlite3_close(db);
    return 0;
}
