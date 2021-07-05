#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <unordered_set>
#include "ConnectFourPMCTS.hpp"
#include "ConnectFourState.hpp"
#include "FileIO.hpp"

/**
 * The computer will spend all of its allowed time running pMCTS playthroughs.
 *
 * Citations
 *  https://stackoverflow.com/questions/3273993/how-do-i-validate-user-input-as-a-double-in-c
 *      Taking valid input from std::cin.
 */

void print(const std::string& str = "") { std::cout << str; }

void myprintln(const std::string& str = "") {
    print(str);
    std::cout << std::endl;
}

std::string toLower(const std::string& str) {
    std::string transformed;
    for (char c : str) {
        transformed += tolower(c);
    }
    return transformed;
}

std::string getInput(const std::string& question,
                     const std::unordered_set<std::string>& options) {
    std::cout << question << '\n';
    while (true) {
        std::string response;
        std::cout << "> ";
        std::cin >> response;
        response = toLower(response);

        if (options.find(response) != options.end()) {
            return response;
        }
    }
}

double askBoundedDouble(const std::string& question, double lower,
                        double upper) {
    std::cout << question << '\n';
    while (true) {
        double response;
        std::cout << "> ";
        if (std::cin >> response) {
            if (response >= lower && response <= upper) {
                return response;
            }
        } else {
            std::cin.clear();
            while (std::cin.get() != '\n')
                ;
        }
    }
}

std::pair<std::pair<ConnectFourState::Player, PlaythroughMode>,
          std::vector<Decision>>
testRandomVsHeuristic() {
    const PlaythroughMode X_MODE = (rand() % 2 == 0)
                                       ? PlaythroughMode::RANDOM
                                       : PlaythroughMode::HEURISTIC;
    const PlaythroughMode O_MODE = (X_MODE == PlaythroughMode::RANDOM)
                                       ? PlaythroughMode::HEURISTIC
                                       : PlaythroughMode::RANDOM;
    const double MAX_TIME = 1.0;
    const long MIN_ITERATIONS = 20000;
    const DecisionCutoff CUTOFF_TYPE = DecisionCutoff::TIME;

    ConnectFourState game;
    std::vector<Decision> gameDecisions;
    int turn = 1;

    while (!game.isOver()) {
        const PlaythroughMode CURRENT_MODE =
            (game.currentPlayer() == ConnectFourState::Player::X) ? X_MODE
                                                                  : O_MODE;
        const std::string PLAYER_REPR =
            game.playerToString(game.currentPlayer());
        const std::string MODE_REPR =
            (CURRENT_MODE == PlaythroughMode::RANDOM) ? "Random" : "Heuristic";

        std::cout << "-----------------------------------------------------\n";
        std::cout << "Turn: " << turn << '\n';
        std::cout << "Player " << PLAYER_REPR << " (" << MODE_REPR << ")\n";

        Decision currentDecision = pMCTS_DecideColumn(
            game, CURRENT_MODE, MAX_TIME, CUTOFF_TYPE, MIN_ITERATIONS, true);
        currentDecision.turn = turn;
        game.playColumn(currentDecision.column);
        std::cout << "Column " << currentDecision.column << " chosen\n";
        std::cout << game << "\n\n";

        gameDecisions.push_back(currentDecision);

        ++turn;
    }

    PlaythroughMode lastMode;

    if (!game.isDraw()) {
        lastMode =
            (game.firstWinner() == ConnectFourState::Player::X) ? X_MODE
                                                                : O_MODE;
        const std::string WINNING_MODE_REPR =
            (lastMode == PlaythroughMode::RANDOM) ? "RANDOM" : "HEURISTIC";

        std::cout << "Player " << game.playerToString(game.firstWinner())
                  << " (" << WINNING_MODE_REPR << ") won\n";
    } else {
        std::cout << "Draw\n";
    }

    return {{game.firstWinner(), lastMode}, gameDecisions};
}

void playGame() {
    const PlaythroughMode pMCTS_MODE =
        (getInput(
             "Set computer playthrough to pure random or heuristics? (r/h)",
             {"r", "h"}) == "r")
            ? PlaythroughMode::RANDOM
            : PlaythroughMode::HEURISTIC;
    myprintln();

    const DecisionCutoff AI_CUTOFF =
        (getInput("Hard limit computer decision by time, or playthrough "
                  "iterations? [t, i]",
                  {"t", "i"}) == "t")
            ? DecisionCutoff::TIME
            : DecisionCutoff::ITERATIONS;

    double MAX_DECISION_TIME = 1.0;
    const long iterations = 20000;
    if (AI_CUTOFF == DecisionCutoff::TIME) {
        MAX_DECISION_TIME = askBoundedDouble(
            "How many seconds can the computer take to decide? [0.1, 100]", 0.1,
            100.0);
    } else {
        std::cout << "Defaulting to " << iterations
                  << " playthroughs per possible move.\n";
    }

    myprintln();
    myprintln();

    ConnectFourState game;
    int turn = 1;

    std::cout << game << "\n\n";
    std::cout << std::string(40, '-') << "\n";

    while (!game.isOver()) {
        std::cout << "<Turn " << turn++ << ">\n";
        int chosenColumn;
        if (game.currentPlayer() == ConnectFourState::Player::X) {
            std::unordered_set<std::string> allowedOptions;
            for (int column : game.legalMoves()) {
                allowedOptions.insert(std::to_string(column));
            }
            std::string option = getInput("Select a column", allowedOptions);
            chosenColumn = std::stoi(option);

        } else {
            print("Deciding...\r");

            Decision computerDecision =
                pMCTS_DecideColumn(game, pMCTS_MODE, MAX_DECISION_TIME,
                                   AI_CUTOFF, iterations, true);
            chosenColumn = computerDecision.column;

            std::cout << "Computer O ("
                      << (pMCTS_MODE == PlaythroughMode::HEURISTIC ? "Heuristic"
                                                                   : "Random")
                      << ") chose column " << chosenColumn << '\n';
        }

        game.playColumn(chosenColumn);

        std::cout << '\n' << game << "\n\n";
        std::cout << std::string(40, '-') << '\n';

        if (game.isWon()) {
            if (game.firstWinner() == ConnectFourState::Player::X) {
                myprintln("Player X Won");
            } else {
                myprintln("Player O Won");
            }

            return;
        }
    }
}

void collectRandomVsHeuristicData(const std::string& filename, int tests = 10) {
    int randomScore = 0;
    int heuristicScore = 0;
    int drawScore = 0;

    FileManager fileWriter(filename, false);
    fileWriter.write(
        "turn,player,mode,cutoff,column,possible_columns,score,playthroughs,time,playthroughs_"
        "per_second\n");

    for (int i = 0; i < tests; ++i) {
        auto gameData = testRandomVsHeuristic();
        ConnectFourState::Player winningPlayer = gameData.first.first;
        PlaythroughMode lastMoveMode = gameData.first.second;
        const std::vector<Decision>& DECISIONS = gameData.second;

        if (winningPlayer != ConnectFourState::Player::None) {
            if (lastMoveMode == PlaythroughMode::RANDOM) {
                ++randomScore;
            } else if (lastMoveMode == PlaythroughMode::HEURISTIC) {
                ++heuristicScore;
            }
        } else {
            ++drawScore;
        }

        for (const Decision& d : DECISIONS) {
            fileWriter.write(d.toCSV() + '\n');
        }
        fileWriter.flush();

        std::cout << "Random: " << randomScore
                  << ", Heuristic: " << heuristicScore
                  << ", Draw: " << drawScore << '\n';
    }
    fileWriter.write("\nRandom:" + std::to_string(randomScore) +
                     ",Heuristic:" + std::to_string(heuristicScore) + ",Draw:" +
                     std::to_string(drawScore) + '\n');
}

int main() {
    srand(time(NULL));

    // collectRandomVsHeuristicData("data/RVH_DATA_TIME_100R", 100);
    playGame();

    std::string temp;
    print("Enter any key to quit: ");
    std::cin >> temp;
    return 0;
}