#include <iostream>
#include <string>
#include <sstream>
#include <iomanip> // For formatting output
#include <limits>  // For input validation
#include <mysql.h>
#include <windows.h> // For Sleep() and system("cls")

using namespace std;

// --- Database Connection Configuration ---
const char* HOST = "localhost";
const char* USER = "root";
const char* PW = "your password"; // <--- !!! CHANGE THIS !!!
const char* DB = "mydb";


// --- Helper Functions for Robust Input ---

// Robustly gets an integer choice from the user
int getMenuChoice() {
    int choice;
    while (true) {
        cout << "> Your choice: ";
        if (cin >> choice) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return choice;
        } else {
            cout << "Invalid input. Please enter a number." << endl;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
}

// Robustly gets a non-empty string from the user
string getStringInput(string prompt) {
    string input;
    while (true) {
        cout << prompt;
        getline(cin, input);
        if (!input.empty()) {
            return input;
        } else {
            cout << "Input cannot be empty. Please try again." << endl;
        }
    }
}

void pressEnterToContinue() {
    cout << "\nPress Enter to continue..." << endl;
    cin.get();
}

// --- Data Model Classes (from resume) ---

/**
 * @class Flight
 * @brief Models a single flight.
 */
class Flight {
public:
    int flight_id;
    string flight_number;
    string departure;
    string destination;
    int available_seats;
};

/**
 * @class Passenger
 * @brief Models a single passenger.
 */
class Passenger {
public:
    int passenger_id;
    string first_name;
    string last_name;
    string passport_number;
};

/**
 * @class Reservation
 * @brief Models a single reservation.
 */
class Reservation {
public:
    int reservation_id;
    int passenger_id;
    int flight_id;
};


/**
 * @class Database
 * @brief Encapsulates all database logic. This is the main "engine".
 */
class Database {
private:
    MYSQL* conn; // The database connection object

    // Helper to run simple, non-parameterized queries
    bool simpleQuery(string query) {
        if (mysql_query(conn, query.c_str())) {
            cout << "Error: " << mysql_error(conn) << endl;
            return false;
        }
        return true;
    }

public:
    // Constructor
    Database() {
        conn = mysql_init(NULL);
    }

    // Destructor (cleans up the connection)
    ~Database() {
        if (conn) {
            mysql_close(conn);
        }
    }

    // Connects to the database. Returns true on success.
    bool connect() {
        if (!mysql_real_connect(conn, HOST, USER, PW, DB, 3306, NULL, 0)) {
            cout << "Error connecting to database: " << mysql_error(conn) << endl;
            return false;
        } else {
            cout << "Logged In To Database!" << endl;
            return true;
        }
    }

    // Sets up the 3-table schema (as per resume)
    void setupDatabase() {
        cout << "Setting up database..." << endl;
        simpleQuery("DROP TABLE IF EXISTS Reservations");
        simpleQuery("DROP TABLE IF EXISTS Passengers");
        simpleQuery("DROP TABLE IF EXISTS Flights");

        string createFlights = R"(
            CREATE TABLE Flights (
                flight_id INT AUTO_INCREMENT PRIMARY KEY,
                flight_number VARCHAR(10) NOT NULL UNIQUE,
                departure VARCHAR(50) NOT NULL,
                destination VARCHAR(50) NOT NULL,
                available_seats INT NOT NULL
            )
        )";
        
        string createPassengers = R"(
            CREATE TABLE Passengers (
                passenger_id INT AUTO_INCREMENT PRIMARY KEY,
                first_name VARCHAR(50) NOT NULL,
                last_name VARCHAR(50) NOT NULL,
                passport_number VARCHAR(20) NOT NULL UNIQUE
            )
        )";

        string createReservations = R"(
            CREATE TABLE Reservations (
                reservation_id INT AUTO_INCREMENT PRIMARY KEY,
                passenger_id INT NOT NULL,
                flight_id INT NOT NULL,
                FOREIGN KEY (passenger_id) REFERENCES Passengers(passenger_id),
                FOREIGN KEY (flight_id) REFERENCES Flights(flight_id)
            )
        )";

        if (!simpleQuery(createFlights) || !simpleQuery(createPassengers) || !simpleQuery(createReservations)) {
            cout << "Fatal error setting up tables. Exiting." << endl;
            exit(1);
        }

        // Populate with sample data
        simpleQuery("INSERT INTO Flights (flight_number, departure, destination, available_seats) VALUES ('F101', 'UAE', 'Canada', 50)");
        simpleQuery("INSERT INTO Flights (flight_number, departure, destination, available_seats) VALUES ('F102', 'UAE', 'USA', 40)");
        simpleQuery("INSERT INTO Flights (flight_number, departure, destination, available_seats) VALUES ('F103', 'UAE', 'China', 1)"); // For testing
        
        simpleQuery("INSERT INTO Passengers (first_name, last_name, passport_number) VALUES ('John', 'Doe', 'A123')");
        simpleQuery("INSERT INTO Passengers (first_name, last_name, passport_number) VALUES ('Jane', 'Smith', 'B456')");

        cout << "Database setup complete." << endl;
        Sleep(2000);
    }

    // (READ) Displays all flights (concise CLI report)
    void displayAllFlights() {
        if (mysql_query(conn, "SELECT flight_id, flight_number, departure, destination, available_seats FROM Flights")) {
            cout << "Error: " << mysql_error(conn) << endl;
            return;
        }
        
        MYSQL_RES* res = mysql_store_result(conn);
        if (res == NULL) return;

        MYSQL_ROW row;
        cout << "\n--- Available Flights ---" << endl;
        cout << left 
             << setw(10) << "ID" 
             << setw(15) << "Flight No." 
             << setw(15) << "Departure" 
             << setw(15) << "Destination" 
             << setw(10) << "Available" 
             << endl;
        cout << string(65, '-') << endl;

        while ((row = mysql_fetch_row(res))) {
            cout << left 
                 << setw(10) << row[0] 
                 << setw(15) << row[1] 
                 << setw(15) << row[2] 
                 << setw(15) << row[3] 
                 << setw(10) << row[4] 
                 << endl;
        }
        cout << string(65, '-') << endl;
        mysql_free_result(res);
    }

    // (CREATE) Adds a new passenger (robust CRUD)
    void addNewPassenger() {
        system("cls");
        cout << "--- Add New Passenger ---" << endl;
        string fname = getStringInput("Enter first name: ");
        string lname = getStringInput("Enter last name: ");
        string passport = getStringInput("Enter passport number: ");

        // Use Prepared Statements to prevent SQL injection
        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        string query = "INSERT INTO Passengers (first_name, last_name, passport_number) VALUES (?, ?, ?)";
        
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
             cout << "Error preparing statement: " << mysql_stmt_error(stmt) << endl;
             mysql_stmt_close(stmt);
             return;
        }

        MYSQL_BIND params[3];
        memset(params, 0, sizeof(params));

        params[0].buffer_type = MYSQL_TYPE_STRING;
        params[0].buffer = (char*)fname.c_str();
        params[0].buffer_length = fname.length();

        params[1].buffer_type = MYSQL_TYPE_STRING;
        params[1].buffer = (char*)lname.c_str();
        params[1].buffer_length = lname.length();

        params[2].buffer_type = MYSQL_TYPE_STRING;
        params[2].buffer = (char*)passport.c_str();
        params[2].buffer_length = passport.length();

        mysql_stmt_bind_param(stmt, params);
        if (mysql_stmt_execute(stmt)) {
            cout << "Error adding passenger: " << mysql_stmt_error(stmt) << endl;
        } else {
            cout << "\nPassenger '" << fname << " " << lname << "' added successfully!" << endl;
            cout << "New Passenger ID: " << mysql_insert_id(conn) << endl;
        }
        mysql_stmt_close(stmt);
    }

    // (UPDATE/CREATE) Books a seat (TRANSACTION-SAFE)
    bool bookReservation(int p_id, int f_id) {
        // --- TRANSACTION START ---
        if (simpleQuery("START TRANSACTION")) {
            // 1. Check for seats AND lock the row
            string query_check = "SELECT available_seats FROM Flights WHERE flight_id = ? FOR UPDATE";
            int available_seats = 0;
            
            MYSQL_STMT* stmt_check = mysql_stmt_init(conn);
            mysql_stmt_prepare(stmt_check, query_check.c_str(), query_check.length());

            MYSQL_BIND param_check[1];
            memset(param_check, 0, sizeof(param_check));
            param_check[0].buffer_type = MYSQL_TYPE_LONG;
            param_check[0].buffer = &f_id;
            mysql_stmt_bind_param(stmt_check, param_check);

            MYSQL_BIND result_check[1];
            memset(result_check, 0, sizeof(result_check));
            result_check[0].buffer_type = MYSQL_TYPE_LONG;
            result_check[0].buffer = &available_seats;
            mysql_stmt_bind_result(stmt_check, result_check);

            mysql_stmt_execute(stmt_check);
            mysql_stmt_store_result(stmt_check);

            if (mysql_stmt_fetch(stmt_check) == 0) { // If fetch is successful
                mysql_stmt_close(stmt_check); // Close check statement

                // 2. Logic: If seats are available, book them
                if (available_seats > 0) {
                    // 2a. Update Flights table
                    string query_update = "UPDATE Flights SET available_seats = available_seats - 1 WHERE flight_id = ?";
                    MYSQL_STMT* stmt_update = mysql_stmt_init(conn);
                    mysql_stmt_prepare(stmt_update, query_update.c_str(), query_update.length());
                    mysql_stmt_bind_param(stmt_update, param_check); // Re-use param_check (f_id)
                    mysql_stmt_execute(stmt_update);
                    mysql_stmt_close(stmt_update);

                    // 2b. Insert into Reservations table
                    string query_insert = "INSERT INTO Reservations (passenger_id, flight_id) VALUES (?, ?)";
                    MYSQL_STMT* stmt_insert = mysql_stmt_init(conn);
                    mysql_stmt_prepare(stmt_insert, query_insert.c_str(), query_insert.length());

                    MYSQL_BIND param_insert[2];
                    memset(param_insert, 0, sizeof(param_insert));
                    param_insert[0].buffer_type = MYSQL_TYPE_LONG;
                    param_insert[0].buffer = &p_id;
                    param_insert[1].buffer_type = MYSQL_TYPE_LONG;
                    param_insert[1].buffer = &f_id;
                    mysql_stmt_bind_param(stmt_insert, param_insert);
                    mysql_stmt_execute(stmt_insert);

                    cout << "\nBooking successful! Reservation ID: " << mysql_insert_id(conn) << endl;
                    mysql_stmt_close(stmt_insert);

                    simpleQuery("COMMIT"); // 3. Commit transaction
                    return true;
                } else {
                    cout << "\nSorry, that flight is full. Rolling back." << endl;
                    simpleQuery("ROLLBACK");
                }
            } else {
                cout << "\nError: Flight ID not found. Rolling back." << endl;
                simpleQuery("ROLLBACK");
            }
            if(stmt_check) mysql_stmt_close(stmt_check);
        }
        return false;
    }

    // (READ) Shows reservations for a specific passenger
    void displayPassengerReservations(int p_id) {
        string query = 
            "SELECT r.reservation_id, f.flight_number, f.departure, f.destination "
            "FROM Reservations r "
            "JOIN Flights f ON r.flight_id = f.flight_id "
            "WHERE r.passenger_id = ?";

        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        mysql_stmt_prepare(stmt, query.c_str(), query.length());

        MYSQL_BIND param[1];
        memset(param, 0, sizeof(param));
        param[0].buffer_type = MYSQL_TYPE_LONG;
        param[0].buffer = &p_id;
        mysql_stmt_bind_param(stmt, param);

        // Buffers for results
        int res_id;
        char flight_num[11], departure[51], destination[51];
        unsigned long len_flight, len_dep, len_des;

        MYSQL_BIND result[4];
        memset(result, 0, sizeof(result));
        result[0].buffer_type = MYSQL_TYPE_LONG; result[0].buffer = &res_id;
        result[1].buffer_type = MYSQL_TYPE_STRING; result[1].buffer = flight_num; result[1].buffer_length = 11; result[1].length = &len_flight;
        result[2].buffer_type = MYSQL_TYPE_STRING; result[2].buffer = departure; result[2].buffer_length = 51; result[2].length = &len_dep;
        result[3].buffer_type = MYSQL_TYPE_STRING; result[3].buffer = destination; result[3].buffer_length = 51; result[3].length = &len_des;
        
        mysql_stmt_bind_result(stmt, result);
        mysql_stmt_execute(stmt);
        mysql_stmt_store_result(stmt);

        if (mysql_stmt_num_rows(stmt) == 0) {
            cout << "\nNo reservations found for Passenger ID " << p_id << "." << endl;
        } else {
            cout << "\n--- Reservations for Passenger ID " << p_id << " ---" << endl;
            cout << left << setw(10) << "Res. ID" << setw(15) << "Flight No." << setw(15) << "Departure" << setw(15) << "Destination" << endl;
            cout << string(55, '-') << endl;
            while (mysql_stmt_fetch(stmt) == 0) {
                cout << left << setw(10) << res_id << setw(15) << flight_num << setw(15) << departure << setw(15) << destination << endl;
            }
            cout << string(55, '-') << endl;
        }
        mysql_stmt_close(stmt);
    }

    // (DELETE/UPDATE) Cancels a reservation (TRANSACTION-SAFE)
    bool cancelReservation(int res_id) {
        int f_id = 0; // To store the flight_id from the reservation

        // --- TRANSACTION START ---
        if (simpleQuery("START TRANSACTION")) {
            // 1. Get flight_id from reservation AND lock the row
            string query_check = "SELECT flight_id FROM Reservations WHERE reservation_id = ? FOR UPDATE";
            MYSQL_STMT* stmt_check = mysql_stmt_init(conn);
            mysql_stmt_prepare(stmt_check, query_check.c_str(), query_check.length());

            MYSQL_BIND param_check[1];
            memset(param_check, 0, sizeof(param_check));
            param_check[0].buffer_type = MYSQL_TYPE_LONG;
            param_check[0].buffer = &res_id;
            mysql_stmt_bind_param(stmt_check, param_check);

            MYSQL_BIND result_check[1];
            memset(result_check, 0, sizeof(result_check));
            result_check[0].buffer_type = MYSQL_TYPE_LONG;
            result_check[0].buffer = &f_id;
            mysql_stmt_bind_result(stmt_check, result_check);

            mysql_stmt_execute(stmt_check);
            mysql_stmt_store_result(stmt_check);

            if (mysql_stmt_fetch(stmt_check) == 0) { // If fetch is successful
                mysql_stmt_close(stmt_check);

                // 2a. Delete the reservation
                string query_delete = "DELETE FROM Reservations WHERE reservation_id = ?";
                MYSQL_STMT* stmt_delete = mysql_stmt_init(conn);
                mysql_stmt_prepare(stmt_delete, query_delete.c_str(), query_delete.length());
                mysql_stmt_bind_param(stmt_delete, param_check); // Re-use param_check (res_id)
                mysql_stmt_execute(stmt_delete);
                mysql_stmt_close(stmt_delete);

                // 2b. Increment available_seats on the flight
                string query_update = "UPDATE Flights SET available_seats = available_seats + 1 WHERE flight_id = ?";
                MYSQL_STMT* stmt_update = mysql_stmt_init(conn);
                mysql_stmt_prepare(stmt_update, query_update.c_str(), query_update.length());

                MYSQL_BIND param_update[1];
                memset(param_update, 0, sizeof(param_update));
                param_update[0].buffer_type = MYSQL_TYPE_LONG;
                param_update[0].buffer = &f_id;
                mysql_stmt_bind_param(stmt_update, param_update);
                mysql_stmt_execute(stmt_update);
                mysql_stmt_close(stmt_update);

                // 3. Commit
                cout << "\nReservation " << res_id << " successfully cancelled." << endl;
                simpleQuery("COMMIT");
                return true;
            } else {
                cout << "\nError: Reservation ID " << res_id << " not found. Rolling back." << endl;
                simpleQuery("ROLLBACK");
            }
            if(stmt_check) mysql_stmt_close(stmt_check);
        }
        return false;
    }

};


// --- Main Application Loop ---

int main() {
    Database db; // Create the Database object

    if (!db.connect()) {
        return 1; // Exit if connection fails
    }

    db.setupDatabase(); // Set up tables and data

    bool exit = false;
    while (!exit) {
        system("cls");
        cout << "\n--- Welcome To Airlines Reservation System ---" << endl;
        cout << "********************************************" << endl;
        cout << "1. Display All Flights" << endl;
        cout << "2. Book a Seat" << endl;
        cout << "3. Cancel a Reservation" << endl;
        cout << "4. View My Reservations (by Passenger ID)" << endl;
        cout << "5. Add New Passenger" << endl;
        cout << "6. Exit" << endl;
        cout << "********************************************" << endl;

        int val = getMenuChoice();

        switch (val) {
            case 1:
                db.displayAllFlights();
                pressEnterToContinue();
                break;
            case 2:
                { // Braces create a new scope for variables
                    system("cls");
                    cout << "--- Book a Reservation ---" << endl;
                    db.displayAllFlights();
                    cout << "Enter Your Passenger ID: ";
                    int p_id = getMenuChoice();
                    cout << "Enter Flight ID to book: ";
                    int f_id = getMenuChoice();
                    db.bookReservation(p_id, f_id);
                    pressEnterToContinue();
                }
                break;
            case 3:
                {
                    system("cls");
                    cout << "--- Cancel a Reservation ---" << endl;
                    cout << "Enter Reservation ID to cancel: ";
                    int r_id = getMenuChoice();
                    db.cancelReservation(r_id);
                    pressEnterToContinue();
                }
                break;
            case 4:
                {
                    system("cls");
                    cout << "--- View My Reservations ---" << endl;
                    cout << "Enter Your Passenger ID: ";
                    int p_id = getMenuChoice();
                    db.displayPassengerReservations(p_id);
                    pressEnterToContinue();
                }
                break;
            case 5:
                db.addNewPassenger();
                pressEnterToContinue();
                break;
            case 6:
                exit = true;
                cout << "\nGood Luck!" << endl;
                Sleep(2000);
                break;
            default:
                cout << "\nInvalid Input. Please choose from 1-6." << endl;
                Sleep(2000);
                break;
        }
    }

    return 0; // Destructor for 'db' will automatically close the connection
}
