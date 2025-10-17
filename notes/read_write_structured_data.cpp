#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

// Define a struct to hold student data
struct Student
{
    int id;
    string name;
    float grade;
};

// Function to save students to a file
void writeStudents(const vector<Student> &students, const string &filename)
{
    // open file in write mode
    ofstream outFile(filename, ios::out);

    // check if the file opened successfully
    if (!outFile)
    {
        cerr << "Error: Cannot open file for writing.\n";
        return;
    }

    // write each student's data to the file
    for (const auto &s : students)
    {
        // Write each student’s data in a single line
        outFile << s.id << " " << s.name << " " << s.grade << "\n";
    }

    // close the file
    outFile.close();
    cout << "✅ Student data written to " << filename << " successfully.\n";
}

// Function to read students from a file
vector<Student> readStudents(const string &filename){
    // open file in read mode
    ifstream inFile(filename, ios::in);
    vector<Student> students;

    // check if the file opened successfully
    if (!inFile)
    {
        cerr << "Error: Cannot open file for reading.\n";
        return students;
    }

    // read each student's data from the file
    Student s;
    while (inFile >> s.id >> s.name >> s.grade)
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

    string filename = "students.txt";

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