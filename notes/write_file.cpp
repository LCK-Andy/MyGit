#include <iostream>
#include <fstream>

using namespace std;

int main(){
    // Create and open a file 
    ofstream outFile("example.txt");

    // check if the file opened successfully
    if (!outFile) {
        cerr << "Error creating file!" << endl;
        return 1;
    }

    // write text to file 
    outFile << "Hello, World!" << endl;
    outFile << "This is a sample file created using C++." << endl;

    // close the file
    outFile.close();
    cout << "File created and data written successfully." << endl;

    return 0;
}