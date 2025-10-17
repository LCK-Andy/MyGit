#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

// Define a struct to hold student data
struct Student
{
    int id;
    char name[50]; // fixed-size char array for easier binary writing
    float grade;
};

// Function to save students to a file
void writeStudents(const vector<Student> &students, const string &filename)
{
    // open file in write mode and binary mode
    ofstream outFile(filename, ios::out | ios::binary);

    // check if the file opened successfully
    if (!outFile)
    {
        cerr << "Error: Cannot open file for writing.\n";
        return;
    }

    // write each student's data to the file in binary format
    for (const auto &s : students)
    {
        outFile.write(reinterpret_cast<const char *>(&s), sizeof(Student));
    }

    // close the file
    outFile.close();
    cout << "✅ Student data written to " << filename << " successfully.\n";
}

// Function to read students from a file
vector<Student> readStudents(const string &filename){
    // open file in read mode
    ifstream inFile(filename, ios::in | ios::binary);
    vector<Student> students;

    // check if the file opened successfully
    if (!inFile)
    {
        cerr << "Error: Cannot open file for reading.\n";
        return students;
    }

    // read each student's data from the file
    Student s;
    while (inFile.read(reinterpret_cast<char *>(&s), sizeof(Student)))
    {
        students.push_back(s);
    }

    inFile.close();
    return students;
}

int main()
{
    vector<Student> students = {
        {1, "Alice", 89.5},
        {2, "Bob", 76.2},
        {3, "Charlie", 92.8}};

    string filename = "students.dat"; // Binary file extension

    // Write data to file
    writeStudents(students, filename);

    // Read data back from file
    vector<Student> loadedStudents = readStudents(filename);

    // Display data
    cout << "\n📘 Loaded Student Records:\n";
    cout << "--------------------------\n";
    for (const auto &s : loadedStudents)
    {
        cout << "ID: " << s.id
             << " | Name: " << s.name
             << " | Grade: " << s.grade << endl;
    }

    return 0;
}