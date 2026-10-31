/* 
Description: Soduku Solver that reads puzzles from text files, solves them, and prints the solutions.
Inputs: Text files containing Sudoku puzzles
Outputs: Terminal messages
All Collaborators: Me, ChatGPT
Author: Bruno Barron Silva
Creation date: 10/13/2026
Revised by: Lillian Bailey
Revision date: 29OCT2026
*/
#include <array>
#include <bitset>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

class SudokuSolver {
private:
    static constexpr int SIZE = 9;
    static constexpr int BOX_SIZE = 3;
    static constexpr int CELLS = SIZE * SIZE;

    using Mask = std::uint16_t;
    static constexpr Mask ALL_DIGITS = 0x3FE;

    using Grid = std::array<int, CELLS>;
    using SolutionHandler = std::function<void(const Grid&, std::size_t)>;

    Grid grid{};
    std::array<Mask, SIZE> rowUsed{};
    std::array<Mask, SIZE> colUsed{};
    std::array<Mask, SIZE> boxUsed{};
    std::array<int, CELLS> empties{};
    int emptyCount = 0;
    std::size_t solutionCount = 0;

    int boxOf(int row, int col) const {
        return (row / BOX_SIZE) * BOX_SIZE + (col / BOX_SIZE);
    }

    Mask candidatesFor(int cell) const {
        int row = cell / SIZE;
        int col = cell % SIZE;
        Mask used = rowUsed[row] | colUsed[col] |
                    boxUsed[boxOf(row, col)];

        return static_cast<Mask>(ALL_DIGITS & ~used);
    }

    bool prepareSearch(std::string& conflictMessage) {
        rowUsed.fill(0);
        colUsed.fill(0);
        boxUsed.fill(0);
        emptyCount = 0;

        for (int cell = 0; cell < CELLS; ++cell) {
            int value = grid[cell];

            if (value == 0) {
                empties[emptyCount++] = cell;
                continue;
            }

            int row = cell / SIZE;
            int col = cell % SIZE;
            int box = boxOf(row, col);
            Mask bit = static_cast<Mask>(1u << value);

            if ((rowUsed[row] | colUsed[col] | boxUsed[box]) & bit) {
                conflictMessage = "digit " + std::to_string(value) +
                    " at row " + std::to_string(row + 1) +
                    ", column " + std::to_string(col + 1) +
                    " repeats in its row, column, or box";
                return false;
            }

            rowUsed[row] |= bit;
            colUsed[col] |= bit;
            boxUsed[box] |= bit;
        }

        return true;
    }

    // Backtracking search using the minimum-remaining-values heuristic.
    void search(int depth, const SolutionHandler& onSolution) {
        if (depth == emptyCount) {
            ++solutionCount;
            onSolution(grid, solutionCount);
            return;
        }

        int bestIndex = depth;
        int bestCount = SIZE + 1;
        Mask bestMask = 0;

        for (int i = depth; i < emptyCount; ++i) {
            Mask mask = candidatesFor(empties[i]);
            int count = static_cast<int>(std::bitset<16>(mask).count());

            if (count == 0) {
                return;
            }

            if (count < bestCount) {
                bestCount = count;
                bestIndex = i;
                bestMask = mask;

                if (count == 1) {
                    break;
                }
            }
        }

        std::swap(empties[depth], empties[bestIndex]);

        int cell = empties[depth];
        int row = cell / SIZE;
        int col = cell % SIZE;
        int box = boxOf(row, col);

        for (int digit = 1; digit <= SIZE; ++digit) {
            Mask bit = static_cast<Mask>(1u << digit);

            if (!(bestMask & bit)) {
                continue;
            }

            grid[cell] = digit;
            rowUsed[row] |= bit;
            colUsed[col] |= bit;
            boxUsed[box] |= bit;

            search(depth + 1, onSolution);

            grid[cell] = 0;
            rowUsed[row] ^= bit;
            colUsed[col] ^= bit;
            boxUsed[box] ^= bit;
        }
    }

    void printGrid(const Grid& board) const {
        for (int row = 0; row < SIZE; ++row) {
            if (row > 0 && row % BOX_SIZE == 0) {
                std::cout << "------+-------+------\n";
            }

            for (int col = 0; col < SIZE; ++col) {
                if (col > 0 && col % BOX_SIZE == 0) {
                    std::cout << "| ";
                }

                int value = board[row * SIZE + col];

                if (value == 0) {
                    std::cout << "_ ";
                } else {
                    std::cout << value << ' ';
                }
            }

            std::cout << '\n';
        }
    }

    // Write each solution using the same format as the puzzle files.
    void writeGrid(std::ostream& output, const Grid& board) const {
        for (int row = 0; row < SIZE; ++row) {
            for (int col = 0; col < SIZE; ++col) {
                int value = board[row * SIZE + col];

                output << (value == 0 ? '_' :
                          static_cast<char>('0' + value));
            }

            output << '\n';
        }
    }

public:
    bool loadFromFile(const std::string& filename) {
        std::ifstream inputFile(filename);

        if (!inputFile) {
            std::cerr << "Error: could not open " << filename << ".\n";
            return false;
        }

        Grid loaded{};
        int cellCount = 0;
        char character;

        while (inputFile.get(character)) {
            if (std::isspace(static_cast<unsigned char>(character))) {
                continue;
            }

            if (cellCount == CELLS) {
                std::cerr << "Error: " << filename
                          << " contains more than 81 cells.\n";
                return false;
            }

            int value = 0;

            if (character == '_') {
                value = 0;
            } else if (character >= '1' && character <= '9') {
                value = character - '0';
            } else {
                std::cerr << "Error: invalid character '" << character
                          << "' in " << filename
                          << " (row " << cellCount / SIZE + 1
                          << ", column " << cellCount % SIZE + 1
                          << ").\n";
                return false;
            }

            loaded[cellCount++] = value;
        }

        if (cellCount != CELLS) {
            std::cerr << "Error: " << filename
                      << " must contain exactly 81 cells but has "
                      << cellCount << ".\n";
            return false;
        }

        grid = loaded;
        return true;
    }

    bool solveAndPrint(const std::string& filename,
                       const std::string& solutionFilename) {
        solutionCount = 0;

        std::cout << "\nPuzzle file: " << filename << "\n";
        std::cout << "Original puzzle:\n";
        printGrid(grid);

        std::ofstream solutionFile(solutionFilename);

        if (!solutionFile) {
            std::cerr << "Error: could not create "
                      << solutionFilename << ".\n";
            return false;
        }

        std::string conflictMessage;

        if (!prepareSearch(conflictMessage)) {
            std::cout << "No solution found (the given clues conflict: "
                      << conflictMessage << ").\n";

            solutionFile << "No solution found.\n";
            return static_cast<bool>(solutionFile);
        }

        auto printSolution = [this, &solutionFile](
            const Grid& solved, std::size_t number) {

            std::cout << "\nSolution " << number << ":\n";
            printGrid(solved);

            if (number > 1) {
                solutionFile << '\n';
            }

            writeGrid(solutionFile, solved);
        };

        search(0, printSolution);

        if (solutionCount == 0) {
            std::cout << "No solution found.\n";
            solutionFile << "No solution found.\n";
        } else {
            std::cout << "\nNumber of solutions found: "
                      << solutionCount << "\n";

            solutionFile << "Solutions found: "
                         << solutionCount << '\n';
        }

        return static_cast<bool>(solutionFile);
    }
};

// Create the corresponding solution filename.
std::string makeSolutionFilename(
    const std::string& puzzleFilename,
    std::size_t puzzleNumber) {

    std::size_t slashPosition = puzzleFilename.find_last_of("/\\");
    std::size_t basePosition =
        (slashPosition == std::string::npos) ? 0 : slashPosition + 1;

    std::string outputFilename = puzzleFilename;

    if (outputFilename.compare(basePosition, 6, "puzzle") == 0) {
        outputFilename.replace(basePosition, 6, "solution");
        return outputFilename;
    }

    return "solution" + std::to_string(puzzleNumber) + ".txt";
}

int main(int argc, char* argv[]) {
    std::vector<std::string> filenames;

    for (int i = 1; i < argc; ++i) {
        filenames.push_back(argv[i]);
    }

    if (filenames.empty()) {
        filenames = {
            "puzzle1.txt",
            "puzzle2.txt",
            "puzzle3.txt",
            "puzzle4.txt",
            "puzzle5.txt"
        };
    }

    SudokuSolver solver;
    bool allSuccessful = true;

    for (std::size_t i = 0; i < filenames.size(); ++i) {
        const std::string& filename = filenames[i];
        std::string solutionFilename =
            makeSolutionFilename(filename, i + 1);

        if (solver.loadFromFile(filename)) {
            if (!solver.solveAndPrint(filename, solutionFilename)) {
                allSuccessful = false;
            }
        } else {
            std::cout << "Skipping " << filename
                      << " because it could not be loaded.\n";
            allSuccessful = false;
        }
    }

    return allSuccessful ? 0 : 1;
}