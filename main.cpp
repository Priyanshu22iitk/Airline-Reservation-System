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

// --- Utility Functions ---

// Robustly gets an integer choice from the user
int getMenuChoice() {
    int choice;
    while (true) {
        cout << "> Your choice: ";
        if (cin >> choice) {
            // Clear the input buffer
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

// Pauses execution until the user presses Enter
void pressEnterToContinue() {
    cout << "\nPress Enter to continue..." << endl;
    cin.get();
}

// Executes a simple query and returns true on success
bool simpleQuery(MYSQL* conn, const string& query) {
    if (mysql_query(conn, query.c_str())) {
        cout << "Error: " << mysql_error(conn) << endl;
        return false;
    }
    return true;
}

// --- Database Setup ---

// This function sets up the correct 3-table schema.
void setupDatabase(MYSQL* conn) {
    cout << "Setting up database..." << endl;
    simpleQuery(conn, "DROP TABLE IF EXISTS Reservations");
    simpleQuery(conn, "DROP TABLE IF EXISTS Passengers");
    simpleQuery(conn, "DROP TABLE IF EXISTS Flights");

    // 1. Flights Table
    string createFlights = R"(
        CREATE TABLE Flights (
            flight_id INT AUTO_INCREMENT PRIMARY KEY,
            flight_number VARCHAR(10) NOT NULL UNIQUE,
            departure VARCHAR(50) NOT NULL,
            destination VARCHAR(50) NOT NULL,
            total_seats INT NOT NULL,
            available_seats INT NOT NULL
        )
    )";
    
    // 2. Passengers Table
    string createPassengers = R"(
        CREATE TABLE Passengers (
            passenger_id INT AUTO_INCREMENT PRIMARY KEY,
            first_name VARCHAR(50) NOT NULL,
            last_name VARCHAR(50) NOT NULL,
            passport_number VARCHAR(20) NOT NULL UNIQUE
        )
    )";

    // 3. Reservations Table (Links Flights and Passengers)
    string createReservations = R"(
        CREATE TABLE Reservations (
            reservation_id INT AUTO_INCREMENT PRIMARY KEY,
            passenger_id INT NOT NULL,
            flight_id INT NOT NULL,
            seat_number VARCHAR(4),
            FOREIGN KEY (passenger_id) REFERENCES Passengers(passenger_id),
            FOREIGN KEY (flight_id) REFERENCES Flights(flight_id)
        )
    )";

    if (!simpleQuery(conn, createFlights) || 
        !simpleQuery(conn, createPassengers) || 
        !simpleQuery(conn, createReservations)) {
        cout << "Fatal error setting up database tables." << endl;
        exit(1);
    }

    // Populate with sample data
    simpleQuery(conn, "INSERT INTO Flights (flight_number, departure, destination, total_seats, available_seats) VALUES ('F101', 'UAE', 'Canada', 50, 50)");
    simpleQuery(conn, "INSERT INTO Flights (flight_number, departure, destination, total_seats, available_seats) VALUES ('F102', 'UAE', 'USA', 40, 40)");
    simpleQuery(conn, "INSERT INTO Flights (flight_number, departure, destination, total_seats, available_seats) VALUES ('F103', 'UAE', 'China', 30, 1)"); // For testing
    
    simpleQuery(conn, "INSERT INTO Passengers (first_name, last_name, passport_number) VALUES ('John', 'Doe', 'A12345678')");
    simpleQuery(conn, "INSERT INTO Passengers (first_name, last_name, passport_number) VALUES ('Jane', 'Smith', 'B87654321')");

    cout << "Database setup complete." << endl;
    Sleep(2000);
}


// --- Core Features ---

// (READ) Displays all flights
void displayAllFlights(MYSQL* conn) {
    if (mysql_query(conn, "SELECT flight_id, flight_number, departure, destination, available_seats, total_seats FROM Flights")) {
        cout << "Error: " << mysql_error(conn) << endl;
        return;
    }
    
    MYSQL_RES* res = mysql_store_result(conn);
    if (res == NULL) {
        cout << "Error storing result: " << mysql_error(conn) << endl;
        return;
    }

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
             << setw(10) << row[0] // flight_id
             << setw(15) << row[1] // flight_number
             << setw(15) << row[2] // departure
             << setw(15) << row[3] // destination
             << setw(1) << row[4] << "/" << row[5] // available/total
             << endl;
    }
    cout << string(65, '-') << endl;

    mysql_free_result(res);
}

// (CREATE) Adds a new passenger
void addNewPassenger(MYSQL* conn) {
    system("cls");
    cout << "--- Add New Passenger ---" << endl;
    string fname = getStringInput("Enter first name: ");
    string lname = getStringInput("Enter last name: ");
    string passport = getStringInput("Enter passport number: ");

    // Use Prepared Statements to prevent SQL injection
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) {
        cout << "Error: mysql_stmt_init() failed" << endl;
        return;
    }

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

    if (mysql_stmt_bind_param(stmt, params)) {
        cout << "Error binding parameters: " << mysql_stmt_error(stmt) << endl;
        mysql_stmt_close(stmt);
        return;
    }

    if (mysql_stmt_execute(stmt)) {
        cout << "Error executing statement: " << mysql_stmt_error(stmt) << endl;
    } else {
        cout << "\nPassenger '" << fname << " " << lname << "' added successfully!" << endl;
        cout << "New Passenger ID: " << mysql_insert_id(conn) << endl;
    }

    mysql_stmt_close(stmt);
    pressEnterToContinue();
}


// (UPDATE/CREATE) Books a seat - TRANSACTION-SAFE
void bookASeat(MYSQL* conn) {
    system("cls");
    cout << "--- Book a Reservation ---" << endl;
    displayAllFlights(conn);
    
    int p_id = getMenuChoice(); // Re-using function, prompt is "Your choice: "
    cout << "Enter Passenger ID: ";
    cin >> p_id;
    int f_id = getMenuChoice();
    cout << "Enter Flight ID to book: ";
    cin >> f_id;
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear buffer

    // --- TRANSACTION START ---
    if (mysql_query(conn, "START TRANSACTION")) {
        cout << "Error starting transaction: " << mysql_error(conn) << endl;
        return;
    }

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

    if (mysql_stmt_fetch(stmt_check)) {
        // 2. Logic: If seats are available, book them
        if (available_seats > 0) {
            mysql_stmt_close(stmt_check); // Close check statement

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

            // 3. Commit transaction
            simpleQuery(conn, "COMMIT");
        } else {
            // 2c. No seats available
            cout << "\nSorry, that flight is full. Rolling back transaction." << endl;
            simpleQuery(conn, "ROLLBACK");
        }
    } else {
        // 2d. Flight ID not found
        cout << "\nError: Flight ID not found. Rolling back transaction." << endl;
        simpleQuery(conn, "ROLLBACK");
    }
    
    if (stmt_check) mysql_stmt_close(stmt_check); // Ensure it's closed
    pressEnterToContinue();
}

// (READ) Shows reservations for a specific passenger
void viewMyReservations(MYSQL* conn) {
    system("cls");
    cout << "--- View My Reservations ---" << endl;
    cout << "Enter Passenger ID: ";
    int p_id = getMenuChoice();

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
    char flight_num[10];
    char departure[50];
    char destination[50];
    unsigned long len_flight, len_dep, len_des;


    MYSQL_BIND result[4];
    memset(result, 0, sizeof(result));
    result[0].buffer_type = MYSQL_TYPE_LONG;
    result[0].buffer = &res_id;
    result[1].buffer_type = MYSQL_TYPE_STRING;
    result[1].buffer = flight_num;
    result[1].buffer_length = 10;
    result[1].length = &len_flight;
    result[2].buffer_type = MYSQL_TYPE_STRING;
    result[2].buffer = departure;
    result[2].buffer_length = 50;
    result[2].length = &len_dep;
    result[3].buffer_type = MYSQL_TYPE_STRING;
    result[3].buffer = destination;
    result[3].buffer_length = 50;
    result[3].length = &len_des;
    
    mysql_stmt_bind_result(stmt, result);
    mysql_stmt_execute(stmt);
    mysql_stmt_store_result(stmt);

    if (mysql_stmt_num_rows(stmt) == 0) {
        cout << "\nNo reservations found for Passenger ID " << p_id << "." << endl;
    } else {
        cout << "\n--- Reservations for Passenger ID " << p_id << " ---" << endl;
        cout << left 
             << setw(10) << "Res. ID" 
             << setw(15) << "Flight No." 
             << setw(15) << "Departure" 
             << setw(15) << "Destination" 
             << endl;
        cout << string(55, '-') << endl;
        while (mysql_stmt_fetch(stmt) == 0) {
            cout << left 
                 << setw(10) << res_id
                 << setw(15) << flight_num
                 << setw(15) << departure
                 << setw(15) << destination
                 << endl;
        }
        cout << string(55, '-') << endl;
    }

    mysql_stmt_close(stmt);
    pressEnterToContinue();
}


// (DELETE/UPDATE) Cancels a reservation - TRANSACTION-SAFE
void cancelReservation(MYSQL* conn) {
    system("cls");
    cout << "--- Cancel a Reservation ---" << endl;
    cout << "Enter Reservation ID to cancel: ";
    int res_id = getMenuChoice();
    int f_id = 0; // To store the flight_id from the reservation

    // --- TRANSACTION START ---
    simpleQuery(conn, "START TRANSACTION");

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

    if (mysql_stmt_fetch(stmt_check)) {
        // 2. If reservation exists, proceed
        mysql_stmt_close(stmt_check); // Close check statement

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
        simpleQuery(conn, "COMMIT");
    } else {
        // 2c. Reservation not found
        cout << "\nError: Reservation ID " << res_id << " not found. Rolling back." << endl;
        simpleQuery(conn, "ROLLBACK");
    }

    if (stmt_check) mysql_stmt_close(stmt_check);
    pressEnterToContinue();
}


// --- Main Application Loop ---

int main() {
    MYSQL* conn;
    conn = mysql_init(NULL);

    if (!mysql_real_connect(conn, HOST, USER, PW, DB, 3306, NULL, 0)) {
        cout << "Error: " << mysql_error(conn) << endl;
        return 1;
    } else {
        cout << "Logged In To Database!" << endl;
    }

    setupDatabase(conn); // Initialize the database schema

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
                displayAllFlights(conn);
                pressEnterToContinue();
                break;
            case 2:
                bookASeat(conn);
                break;
            case 3:
                cancelReservation(conn);
                break;
            case 4:
                viewMyReservations(conn);
                break;
            case 5:
                addNewPassenger(conn);
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

    mysql_close(conn);
    return 0;
}

