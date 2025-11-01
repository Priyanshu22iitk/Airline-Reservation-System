#include <iostream>
#include <string>
#include <sstream> // For int to string conversion
#include <mysql.h>
#include <windows.h> // For Sleep() and system("cls")

using namespace std;

// --- Database Connection Configuration ---
const char* HOST = "localhost";
const char* USER = "root";
const char* PW = "your password"; // <--- !!! CHANGE THIS !!!
const char* DB = "mydb";


/**
 * @class Flight
 * @brief Models a single flight, holding its data.
 * This is a simple data-holding class.
 */
class Flight {
private:
    string fnum, dep, des;
    int seat;

public:
    // Constructor to initialize a Flight object
    Flight(string fnum, string dep, string des, int seat) {
        this->fnum = fnum;
        this->dep = dep;
        this->des = des;
        this->seat = seat;
    }

    // --- Getter Methods ---
    string getFnum() { return fnum; }
    string getDep() { return dep; }
    string getDes() { return des; }
    int getSeat() { return seat; }
};


/**
 * @class Database
 * @brief Encapsulates all database logic.
 * This class handles connecting, querying, and all business logic.
 */
class Database {
private:
    MYSQL* conn; // The database connection object

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

    // Sets up the database (one simple table)
    void setupDatabase() {
        simpleQuery("DROP TABLE IF EXISTS Airline");
        string createQuery = R"(
            CREATE TABLE Airline (
                Fnumber VARCHAR(10) PRIMARY KEY,
                Departure VARCHAR(50),
                Destination VARCHAR(50),
                Seat INT
            )
        )";
        simpleQuery(createQuery);

        // Populate with sample data
        Flight f1("F101", "UAE", "Canada", 50);
        Flight f2("F102", "UAE", "USA", 40);
        Flight f3("F103", "UAE", "China", 1); // Only 1 seat for testing
        
        insertFlight(f1);
        insertFlight(f2);
        insertFlight(f3);
        
        cout << "Database setup complete." << endl;
        Sleep(2000);
    }

    // Helper to run simple queries
    bool simpleQuery(string query) {
        if (mysql_query(conn, query.c_str())) {
            cout << "Error: " << mysql_error(conn) << endl;
            return false;
        }
        return true;
    }

    // (CREATE) Inserts a Flight object into the DB
    void insertFlight(Flight f) {
        // Convert seat from int to string
        stringstream ss;
        ss << f.getSeat();
        string strSeat = ss.str();

        // !!! VULNERABLE TO SQL INJECTION !!!
        string query = "INSERT INTO Airline (Fnumber, Departure, Destination, Seat) VALUES ('"
            + f.getFnum() + "', '"
            + f.getDep() + "', '"
            + f.getDes() + "', '"
            + strSeat + "')";

        simpleQuery(query);
    }

    // (READ) Displays all flights
    void displayFlights() {
        if (mysql_query(conn, "SELECT * FROM Airline")) {
            cout << "Error: " << mysql_error(conn) << endl;
            return;
        }
        
        MYSQL_RES* res = mysql_store_result(conn);
        if (res == NULL) return;

        MYSQL_ROW row;
        cout << "\n--- Available Flights ---" << endl;
        cout << "F-Number  |  Departure  |  Destination  |  Seats" << endl;
        cout << "--------------------------------------------------" << endl;

        while ((row = mysql_fetch_row(res))) {
            printf("%-10s|  %-10s |  %-12s |  %s\n", row[0], row[1], row[2], row[3]);
        }
        cout << "--------------------------------------------------" << endl;

        mysql_free_result(res);
    }

    // (UPDATE) Books a seat
    void bookASeat() {
        string flight;
        cout << "\nEnter Flight Number to book: ";
        cin >> flight;

        // --- THIS LOGIC HAS A RACE CONDITION ---
        
        // 1. Check for seats
        string checkQuery = "SELECT Seat FROM Airline WHERE Fnumber = '" + flight + "'";
        if (mysql_query(conn, checkQuery.c_str())) {
            cout << "Error: " << mysql_error(conn) << endl;
            return;
        }

        MYSQL_RES* res = mysql_store_result(conn);
        if (res == NULL) return;
        
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row == NULL) {
            cout << "Flight not found." << endl;
            mysql_free_result(res);
            return;
        }

        int total = atoi(row[0]); // Get current seat count
        mysql_free_result(res);

        // 2. If seats exist, book one
        if (total > 0) {
            total--; // Decrement seat count

            // Convert new total to string
            stringstream ss;
            ss << total;
            string strTotal = ss.str();

            // 3. Update the database
            string updateQuery = "UPDATE Airline SET Seat = '" + strTotal + "' WHERE Fnumber = '" + flight + "'";
            if (simpleQuery(updateQuery)) {
                cout << "\nSeat Is Reserved Successfully in: " << flight << endl;
                cout << "Remaining seats: " << total << endl;
            }
        } else {
            cout << "\nSorry! No seats available for " << flight << endl;
        }
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
        cout << "\n--- Welcome To Airlines Reservation System (OOP) ---" << endl;
        cout << "**************************************************" << endl;
        cout << "1. Display All Flights" << endl;
        cout << "2. Reserve A Seat" << endl;
        cout << "3. Exit" << endl;
        cout << "**************************************************" << endl;
        cout << "Enter Your Choice: ";

        int val;
        cin >> val;

        switch (val) {
            case 1:
                db.displayFlights();
                break;
            case 2:
                db.bookASeat();
                break;
            case 3:
                exit = true;
                cout << "\nGood Luck!" << endl;
                Sleep(2000);
                break;
            default:
                cout << "\nInvalid Input. Please choose from 1-3." << endl;
                cin.clear(); // Clear error flags
                cin.ignore(1000, '\n'); // Ignore bad input
                Sleep(2000);
                break;
        }
        
        if (!exit) {
            cout << "\nPress Enter to continue..." << endl;
            cin.ignore(); // Wait for first 'Enter'
            cin.get();    // Wait for second 'Enter'
        }
    }

    return 0; // Destructor for 'db' will automatically close the connection
}
