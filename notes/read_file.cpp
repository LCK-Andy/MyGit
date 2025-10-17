#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main(){
    // open file for reading
    ifstream inFile("example.txt");
    
    // check if the file opened successfully
    if (!inFile) {
        cerr << "Error opening file!" << endl;
        return 1;
    }

    // read line by line
    string line;
    while (getline(inFile, line)) {
        cout << line << endl;
    }

    // close file
    inFile.close();
}