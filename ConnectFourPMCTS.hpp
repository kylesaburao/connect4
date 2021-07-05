#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include "ConnectFourState.hpp"

/**
 * Citations
 *
 *  https://en.cppreference.com/w/cpp/chrono/high_resolution_clock
 *      High resolution clock for accurate time keeping.
 *
 */

enum class PlaythroughMode { RANDOM, HEURISTIC };
enum class DecisionCutoff { TIME, ITERATIONS };

struct Decision {
    Decision(ConnectFourState::Player player, PlaythroughMode mode,
             DecisionCutoff cutoff, int column, int possibleColumns, int score,
             long playthroughs, double time)
        : player(player),
          mode(mode),
          cutoff(cutoff),
          column(column),
          possibleColumns(possibleColumns),
          score(score),
          playthroughs(playthroughs),
          time(time),
          turn(-1) {}
    ~Decision() {}

    const ConnectFourState::Player player;
    const PlaythroughMode mode;
    const DecisionCutoff cutoff;
    const int column;
    const int possibleColumns;
    const int score;
    const long playthroughs;
    const double time;
    int turn;

    std::string toCSV() const {
        const long double PLAYTHROUGHS_PER_SECOND = playthroughs / time;
        const std::string PLAYER_REPR =
            ConnectFourState::playerToString(player);
        const std::string MODE_REPR =
            (mode == PlaythroughMode::RANDOM) ? "RANDOM" : "HEURISTIC";
        const std::string CUTOFF_REPR =
            (cutoff == DecisionCutoff::ITERATIONS) ? "ITERATIONS" : "TIME";
        return std::to_string(turn) + "," + PLAYER_REPR + "," + MODE_REPR +
               "," + CUTOFF_REPR + "," + std::to_string(column) + "," +
               std::to_string(possibleColumns) + "," + std::to_string(score) +
               "," + std::to_string(playthroughs) + "," + std::to_string(time) +
               "," + std::to_string(PLAYTHROUGHS_PER_SECOND);
    }

    friend std::ostream& operator<<(std::ostream& os,
                                    const Decision& decision) {
        const long double PLAYTHROUGHS_PER_SECOND =
            decision.playthroughs / decision.time;
        const std::string PLAYER_REPR =
            ConnectFourState::playerToString(decision.player);
        const std::string MODE_REPR =
            (decision.mode == PlaythroughMode::RANDOM) ? "RANDOM" : "HEURISTIC";
        const std::string CUTOFF_REPR =
            (decision.cutoff == DecisionCutoff::ITERATIONS) ? "ITERATIONS"
                                                            : "TIME";
        const std::string REPR =
            "Decision:\n\tTurn:              " + std::to_string(decision.turn) +
            "\n\tPlayer:           " + PLAYER_REPR +
            "\n\tMode:             " + MODE_REPR +
            "\n\tCutoff:           " + CUTOFF_REPR +
            "\n\tColumn:           " + std::to_string(decision.column) +
            "\n\tPossible Columns: " +
            std::to_string(decision.possibleColumns) +
            "\n\tScore:            " + std::to_string(decision.score) +
            "\n\tPlaythroughs:     " + std::to_string(decision.playthroughs) +
            "\n\tTime (seconds):   " + std::to_string(decision.time) +
            "\n\tPlaythroughs/sec: " + std::to_string(PLAYTHROUGHS_PER_SECOND);

        os << REPR;
        return os;
    }
};

template <typename T>
T randomElement(const std::vector<T>& container) {
    if (!container.empty()) {
        return container[rand() % container.size()];
    }
    throw std::runtime_error("The container is empty.");
}

double millisecondsSince(
    const std::chrono::high_resolution_clock::time_point& TIME) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::high_resolution_clock::now() - TIME)
        .count();
}

ConnectFourState pMCTS_RandomPlaythrough(const ConnectFourState& START_STATE) {
    ConnectFourState runningState(START_STATE);

    while (!runningState.isOver()) {
        const std::vector<int> LEGAL_COLUMNS = runningState.legalMoves();
        int randomColumn = LEGAL_COLUMNS[rand() % LEGAL_COLUMNS.size()];
        runningState.playColumn(randomColumn);
    }

    return runningState;
}

ConnectFourState pMCTS_HeuristicPlaythrough(
    const ConnectFourState& START_STATE) {
    ConnectFourState runningState(START_STATE);

    // todo: remove CURRENT_PLAYER parameter because it should be dictated by
    // the playout current state

    /*
       Instead of letting heuristics dictate the playthroughs, random choices
       are made by default and specific win/denial plays occur only when they
       are possible. Using cheap heuristics seems to be an important factor in a
       pure-MCTS with heuristics search. The power of pMCTS comes from the sheer
       amount of information it can gather from quick playthroughs, so ensuring
       that the heuristic version is not significantly slower than the pure
       version is a requirement.

       Heuristics:
        Instantaneous choices:
            - If a winning move exists, pick it.
            - If the opponent is about to win, deny it.
     */

    while (!runningState.isOver()) {
        int bestColumn = -1;
        ConnectFourState::Player curr = runningState.currentPlayer();
        ConnectFourState::Player other = (curr == ConnectFourState::Player::X)
                                             ? ConnectFourState::Player::O
                                             : ConnectFourState::Player::X;

        // Only generate vectors when required.
        const std::vector<int> CURRENT_PLAYER_POTENTIAL_WINS =
            runningState.potentialWins(curr);

        if (!CURRENT_PLAYER_POTENTIAL_WINS.empty()) {
            bestColumn = randomElement(CURRENT_PLAYER_POTENTIAL_WINS);
        } else {
            const std::vector<int> OTHER_PLAYER_POTENTIAL_WINS =
                runningState.potentialWins(other);
            if (!OTHER_PLAYER_POTENTIAL_WINS.empty()) {
                bestColumn = randomElement(OTHER_PLAYER_POTENTIAL_WINS);
            } else {
                bestColumn = randomElement(runningState.legalMoves());
            }
        }

        runningState.playColumn(bestColumn);
    }

    return runningState;
}

Decision pMCTS_DecideColumn(const ConnectFourState& STATE,
                            const PlaythroughMode MODE,
                            const double MAX_SECONDS = 5.0,
                            const DecisionCutoff CUTOFF = DecisionCutoff::TIME,
                            const long MINIMUM_ITERATIONS = 20000,
                            const bool PRINT_STATISTICS = false) {
    if (STATE.isOver()) {
        throw std::runtime_error(
            "The game cannot be played further. (It is in a draw.)");
    }

    if (MAX_SECONDS < 0.1) {
        throw std::invalid_argument(
            "The maximum time must be at least 0.1 seconds.");
    }

    const ConnectFourState::Player DECIDING_PLAYER = STATE.currentPlayer();
    const ConnectFourState::Player OTHER_PLAYER =
        (ConnectFourState::Player::X == DECIDING_PLAYER)
            ? ConnectFourState::Player::O
            : ConnectFourState::Player::X;
    const double MAX_MILLISECONDS = MAX_SECONDS * 1000;
    const bool CUTOFF_ON_TIME = CUTOFF == DecisionCutoff::TIME;

    std::vector<std::pair<int, std::pair<ConnectFourState, int>>> childStates;
    for (int playableColumn : STATE.legalMoves()) {
        childStates.push_back(
            {playableColumn, {STATE.applyMove(playableColumn), 0}});
    }
    childStates.shrink_to_fit();

    const std::chrono::high_resolution_clock::time_point START_TIME =
        std::chrono::high_resolution_clock::now();

    long playthroughs = 0;

    for (long iteration = 0;
         (CUTOFF_ON_TIME &&
          (millisecondsSince(START_TIME) <= MAX_MILLISECONDS)) ||
         (!CUTOFF_ON_TIME && (iteration < MINIMUM_ITERATIONS));
         ++iteration) {
        for (int i = 0; i < childStates.size(); ++i) {
            const int CURRENT_COLUMN = childStates[i].first;
            const ConnectFourState& CURRENT_CHILD_STATE =
                childStates[i].second.first;
            int& currentColumnScore = childStates[i].second.second;

            const ConnectFourState::Player FIRST_WINNER =
                (MODE == PlaythroughMode::RANDOM)
                    ? pMCTS_RandomPlaythrough(CURRENT_CHILD_STATE).firstWinner()
                    : pMCTS_HeuristicPlaythrough(CURRENT_CHILD_STATE)
                          .firstWinner();

            if (FIRST_WINNER == DECIDING_PLAYER) {
                currentColumnScore += 1;
            } else if (FIRST_WINNER == OTHER_PLAYER) {
                currentColumnScore += -1;
            } else {
                currentColumnScore += 1;
            }

            ++playthroughs;
        }
    }

    const long double MS_TIME_SPENT =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - START_TIME)
            .count();

    int bestColumn = -1;
    int bestScore = INT_MIN;

    for (int i = 0; i < childStates.size(); ++i) {
        const int COLUMN = childStates[i].first;
        const int SCORE = childStates[i].second.second;
        if (SCORE > bestScore || (SCORE == bestScore && (rand() % 2 == 0))) {
            bestColumn = COLUMN;
            bestScore = SCORE;
        }
    }

    if (PRINT_STATISTICS) {
        std::cout << "========================================\n";
        std::cout << "Playthroughs:     " << playthroughs << '\n'
                  << "Playthroughs/sec: "
                  << (playthroughs /
                      static_cast<long double>(MS_TIME_SPENT / 1000))
                  << '\n'
                  << "Time:             " << (MS_TIME_SPENT / 1000) << "s"
                  << '\n';
        std::cout << "========================================\n";
    }

    return Decision(DECIDING_PLAYER, MODE, CUTOFF, bestColumn,
                    childStates.size(), bestScore, playthroughs,
                    MS_TIME_SPENT / 1000);
}
