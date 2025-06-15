#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <fstream>
#include <filesystem>
#include <conio.h>

std::vector<std::string> fileContents = {};
std::filesystem::path currentPath = "test.txt";
int lineNumber = 0, charNumber = 0;
bool running = true;

#define HIGHLIGHT "\033[7m"
#define RESET "\033[0m"
#define CLAMP(x, low, high) std::max(std::min(x, high), low);

int getConsoleHeight() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
    return 10;
}

void loadFileContents() {
    if (std::filesystem::exists(currentPath)) {
        std::ifstream file;
        file.open(currentPath, std::ios::in);

        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                fileContents.emplace_back(line);
            }
        }
    }

    // fail-safe if loading an empty file, or file does not exist
    if (fileContents.empty()) {
        fileContents.emplace_back("");
    }
}

void saveFileContents() {
    std::ofstream file;
    file.open(currentPath);

    if (file.is_open()) {
        for (auto& line : fileContents) {
            file << line << "\n";
        }
    }

    file.close();
}

void dumpFileContents() {
    for (auto& line : fileContents) {
        std::cout << line << "\n";
    }
}

void printCurrentWindow() {
    const int startIndex = lineNumber;
    const int endIndex = startIndex + getConsoleHeight() - 2;
    for (int i = startIndex; i < endIndex; i++) {
        if (i < fileContents.size()) {
            std::string currentLine = fileContents[i];

            // only highlight the selected line
            if (i == startIndex) {
                if (charNumber == currentLine.size()) {
                    currentLine += HIGHLIGHT;
                    currentLine += " ";
                    currentLine += RESET;
                } 
                else {
                    currentLine.insert(charNumber, HIGHLIGHT);
                    currentLine.insert(charNumber + strlen(HIGHLIGHT) + 1, RESET);
                }
            }

            std::cout << std::to_string(i + 1) << "\t" << currentLine << "\n";
        } else {
            std::cout << "~\n";
        }
    }
}

void incrementLineNumber() {
    // no lines in file
    if (fileContents.empty()) {
        return;
    }

    if (lineNumber < fileContents.size() - 1) {
        lineNumber++;
        charNumber = CLAMP(charNumber, 0, (int)fileContents[lineNumber].size());
    }
}

void decrementLineNumber() {
    // no lines in file
    if (fileContents.empty()) {
        return;
    }

    if (lineNumber > 0) {
        lineNumber--;
        charNumber = CLAMP(charNumber, 0, (int)fileContents[lineNumber].size());
    }
}

void incrementCharNumber() {
    // no lines in file
    if (fileContents.empty()) {
        return;
    }

    // if within line extents, then increment cursor
    if (charNumber < fileContents[lineNumber].size()) {
        charNumber++;
    }
    // otherwise, we are at end of line and should move to start of next line
    else {
        incrementLineNumber();
    }
}

void decrementCharNumber() {
    // no lines in file, or at start of file
    if (fileContents.empty() || (lineNumber == 0 && charNumber == 0)) {
        return;
    }

    // if at start of line, move to previous line end
    if (charNumber == 0) {
        decrementLineNumber();

        if (!fileContents[lineNumber].empty()) {
            charNumber = fileContents[lineNumber].size();
        }
    }
    // otherwise, just decrement charNumber
    else {
        charNumber--;
    }
}

void addNewLine() {
    // no lines yet, just add an empty one
    if (fileContents.empty()) {
        fileContents.emplace_back("");
        lineNumber = 0;
        charNumber = 0;
    }
    else if (lineNumber < fileContents.size()) {
        // end of line, just add an empty one
        if (fileContents[lineNumber].empty()) {
            fileContents.insert(fileContents.begin() + lineNumber + 1, "");
        }
        // otherwise split the current line onto next line 
        else {
            std::string left = fileContents[lineNumber].substr(0, charNumber);
            std::string right = fileContents[lineNumber].substr(charNumber);

            fileContents[lineNumber] = left;
            fileContents.insert(fileContents.begin() + lineNumber + 1, right);
        }
        incrementLineNumber();
    }
}

void addTab() {
    if (lineNumber < fileContents.size()) {
        fileContents[lineNumber].insert(charNumber, "\t");
        incrementCharNumber();
    }
}

void removeCurrentChar() {
    // no file content, or at top-left of empty file
    if (fileContents.empty() || (lineNumber == 0 && charNumber == 0 && fileContents[0].empty())) {
        return;
    }

    // if current line is empty, remove it
    if (fileContents[lineNumber].empty()) {
        fileContents.erase(fileContents.begin() + lineNumber);
        decrementLineNumber();

        // if new current line is not empty, set the correct charNumber
        if (!fileContents[lineNumber].empty()) {
            charNumber = fileContents[lineNumber].size();
        }
    }
    // if exceeding line size, just decrement cursor
    else if (charNumber >= fileContents[lineNumber].size()) {
        decrementCharNumber();
    }
    // otherwise, just remove the current character
    else {
        fileContents[lineNumber].erase(charNumber, 1);

        // *not* at start of line
        if (charNumber > 0) {
            decrementCharNumber();
        }
    }
}

void removeNextChar() {
    // no file content, or at bottom-right of file
    if (fileContents.empty() || (lineNumber == fileContents.size() - 1 && charNumber >= fileContents.back().size() - 1)) {
        return;
    }

    // current line empty, just remove it
    if (fileContents[lineNumber].empty()) {
        fileContents.erase(fileContents.begin() + lineNumber);
        incrementLineNumber();
    }
    // combine current + next lines
    else if (charNumber >= fileContents[lineNumber].size() - 1) {
        std::string current = fileContents[lineNumber];
        std::string next = fileContents[lineNumber + 1];

        fileContents[lineNumber] += next;
        fileContents.erase(fileContents.begin() + lineNumber + 1);
    }
    // erase next char from current line
    else {
        fileContents[lineNumber].erase(charNumber + 1, 1);
    }
}

void writeCurrentChar(char _c) {
    std::string& current = fileContents[lineNumber];

    // empty or exceeding line size, append the character
    if (current.empty() || charNumber >= current.size()) {
        current += _c;
        incrementCharNumber();
    }
    // otherwise, overwrite the current char in the current line
    else {
        current[charNumber] = _c;
        incrementCharNumber();
    }
}

// returns true if a key was pressed
bool handleInput() {
    bool keyHit = _kbhit();

    if (keyHit) {
        int key = _getch();

        // special keys
        if (key == 0 || key == 0xE0) {
            key = _getch(); // read actual special key
            
            switch (key) {
                case 72: decrementLineNumber(); break; // up arrow
                case 80: incrementLineNumber(); break; // down arrow
                case 75: decrementCharNumber(); break; // left arrow
                case 77: incrementCharNumber(); break; // right arrow
                case 83: removeNextChar(); break; // delete
            }
        }
        else {
            // standard keys
            switch (key) {
                case 27: running = false; break; // escape
                case 13: addNewLine(); break; // enter
                case 9: addTab(); break; // tab
                case 8: removeCurrentChar(); break; // backspace
                default: writeCurrentChar(key); break;
            }
        }
    }

    return keyHit;
}

int main()
{
    std::cout << "Enter file path (can be absolute or relative to this *.exe): ";
    std::cin >> currentPath;

    loadFileContents();

    system("cls");
    printCurrentWindow();

    while (running) {
        // only update display if input pressed
        if (handleInput()) {
            system("cls");
            printCurrentWindow();
        }
    }

    system("cls");
    saveFileContents();

    fileContents.clear();

    return 0;
}