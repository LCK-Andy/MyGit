#include <iostream>
#include <fstream>
using namespace std;

int main()
{
    fstream file("data.txt", ios::in | ios::out | ios::app);
    // ios::in  = read mode
    // ios::out = write mode
    // ios::app = append mode (optional)
    // ios::trunc = erase existing content 
    // ios::binary = binary mode
    // ios::ate = open and move pointer to end

    // Check if file opened successfully
    if (!file)
    {
        cout << "File could not be opened!";
        return 1;
    }

    // Write data to the file
    file << "Additional line of text.\n";
    file.seekg(0); // Move file pointer to beginning


    // Read and display the file content
    string line;
    while (getline(file, line))
    {
        cout << line << endl;
    }

    // Close the file
    file.close();
    return 0;
}