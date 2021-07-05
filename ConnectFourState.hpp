#pragma once
#include <algorithm>
#include <array>
#include <climits>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

/**
 * Citations
 *
 *  https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector
 *      Hash function for a container
 */

class ConnectFourState {
   public:
    ConnectFourState();
    ~ConnectFourState();

    enum class Player { X, O, None };

    bool isDraw() const;
    bool isWon() const;
    bool isOver() const;
    int lastPlacedColumn() const;
    int lastPlacedRow() const;
    Player firstWinner() const;
    Player currentPlayer() const;
    int evaluate(Player maxPlayer) const;
    int evaluate(Player maxPlayer, int centreX, int centreY) const;

    void playColumn(int column);

    std::vector<int> legalMoves() const;
    std::vector<int> potentialWins(ConnectFourState::Player player) const;
    ConnectFourState applyMove(int column) const;

    std::string toString() const;
    static std::string playerToString(Player player);
    friend std::ostream& operator<<(std::ostream& os,
                                    const ConnectFourState& state);
    ConnectFourState& operator=(const ConnectFourState& state);
    bool operator==(const ConnectFourState& rhs) const;
    std::array<std::array<char, 7>, 6> state() const;

   private:
    const int _COLUMNS = 7;
    const int _ROWS = 6;
    const char _EMPTY_STATE = ' ';
    const char _PLAYER_X_STATE = 'X';
    const char _PLAYER_O_STATE = 'O';
    const int _REQUIRED_CONSECUTIVE = 4;
    const int _ROW_FILLED = -1;

    Player _current_player;
    Player _first_winner;
    std::array<std::array<char, 7>, 6> _state;
    int _lastPlacedColumn;
    int _lastPlacedRow;

    int _lowest_playable_row(int column) const;
    bool _column_playable(int column) const;
    char _map_player_to_state(Player player) const;
    void _setColumn(int column, Player player);
    void _defaultFill();
    bool _checkWinGeneral(const std::array<std::array<char, 7>, 6>& stateVector,
                          int column, int row) const;
    bool _checkWin(int column, int row) const;
    void _deepcopy(const ConnectFourState& state);
};

ConnectFourState::ConnectFourState()
    : _current_player(Player::X),
      _first_winner(Player::None),
      _lastPlacedColumn(-1),
      _lastPlacedRow(-1) {
    _defaultFill();
}

ConnectFourState::~ConnectFourState() {}

/**
 * Find if there are no more legal moves to play, and the game is not won by
 * anyone.
 */
bool ConnectFourState::isDraw() const {
    for (int col = 0; col < _COLUMNS; ++col) {
        if (_column_playable(col)) {
            return false;
        }
    }
    return true;
}

/**
 * Find if any wins have occured at all.
 */
bool ConnectFourState::isWon() const { return _first_winner != Player::None; }

bool ConnectFourState::isOver() const { return isWon() || isDraw(); }

int ConnectFourState::lastPlacedColumn() const { return _lastPlacedColumn; }
int ConnectFourState::lastPlacedRow() const { return _lastPlacedRow; }

ConnectFourState::Player ConnectFourState::firstWinner() const {
    return _first_winner;
}

ConnectFourState::Player ConnectFourState::currentPlayer() const {
    return _current_player;
}

int ConnectFourState::evaluate(Player maxPlayer) const {
    if (maxPlayer == Player::None) {
        throw std::invalid_argument(
            "None cannot be used as a player for minimax evaluation.");
    }

    int evaluation = 0;

    Player minPlayer = (maxPlayer == Player::X) ? Player::O : Player::X;
    const char MAX_CHAR =
        (maxPlayer == Player::X) ? _PLAYER_X_STATE : _PLAYER_O_STATE;

    if (firstWinner() == maxPlayer) {
        return INT_MAX;
    } else if (firstWinner() == minPlayer) {
        return INT_MIN;
    }

    return evaluation;
}

int ConnectFourState::evaluate(Player maxPlayer, int centreX,
                               int centreY) const {
    const Player minimizingPlayer =
        (maxPlayer == Player::X) ? Player::O : Player::X;
    const char maximizingState =
        (maxPlayer == Player::X) ? _PLAYER_X_STATE : _PLAYER_O_STATE;
    const char minimizingState =
        (minimizingPlayer == Player::X) ? _PLAYER_X_STATE : _PLAYER_O_STATE;

    const auto isCoordinateValid = [&](int column, int row) -> bool {
        return (column >= 0) && (column < _COLUMNS) && (row >= 0) &&
               (row < _ROWS);
    };

    const int ADJACENT[7][2] = {{-1, 1}, {-1, 0}, {-1, -1}, {0, -1},
                                {1, -1}, {1, 0},  {1, 1}};

    int maxPlayerPotentialWins = 0;
    int minPlayerPotentialWins = 0;

    std::array<std::array<char, 7>, 6> temporaryState(_state);

    for (int i = 0; i < 7; ++i) {
        int dX = ADJACENT[i][0];
        int dY = ADJACENT[i][1];
        int newX = centreX + dX;
        int newY = centreY + dY;
        if (isCoordinateValid(newX, newY) &&
            temporaryState[newY][newX] == _EMPTY_STATE) {
            // Test maximizing player
            temporaryState[newY][newX] = maximizingState;
            if (_checkWinGeneral(temporaryState, newX, newY)) {
                maxPlayerPotentialWins += 1;
            }

            // Test minimizing player
            temporaryState[newY][newX] = minimizingState;
            if (_checkWinGeneral(temporaryState, newX, newY)) {
                minPlayerPotentialWins += 1;
            }

            // Reset
            temporaryState[newY][newX] = _EMPTY_STATE;
        }
    }

    int score = maxPlayerPotentialWins - minPlayerPotentialWins;
    return score;
}

void ConnectFourState::playColumn(int column) {
    _setColumn(column, _current_player);
}

/**
 * Get the columns that can be played.
 */
std::vector<int> ConnectFourState::legalMoves() const {
    std::vector<int> moves;
    for (int column = 0; column < _COLUMNS; ++column) {
        if (_column_playable(column)) {
            moves.push_back(column);
        }
    }
    return moves;
}

/**
 * Get the columns that when played lead to a win.
 */
std::vector<int> ConnectFourState::potentialWins(
    ConnectFourState::Player player) const {
    std::vector<int> winningColumns;
    for (int column : legalMoves()) {
        ConnectFourState testState = applyMove(column);
        if (testState.firstWinner() == player) {
            winningColumns.push_back(column);
        }
    }
    return winningColumns;
}

ConnectFourState ConnectFourState::applyMove(int column) const {
    ConnectFourState newState(*this);
    newState.playColumn(column);
    return newState;
}

std::string ConnectFourState::toString() const {
    std::string representation;
    representation += "0 1 2 3 4 5 6\n";
    for (int row = 0; row < _ROWS; ++row) {
        for (int column = 0; column < _COLUMNS; ++column) {
            char EMPTY_CHAR = '-';
            char PLAYER_X_CHAR = 'X';
            char PLAYER_O_CHAR = 'O';
            char currentState = _state[row][column];
            char currentRepresentation = (currentState == _EMPTY_STATE)
                                             ? EMPTY_CHAR
                                             : (currentState == _PLAYER_X_STATE)
                                                   ? PLAYER_X_CHAR
                                                   : PLAYER_O_CHAR;
            representation += currentRepresentation;
            if (column < _COLUMNS - 1) {
                representation += ' ';
            }
        }
        if (row < _ROWS - 1) {
            representation += '\n';
        }
    }
    return representation;
}

std::string ConnectFourState::playerToString(Player player) {
    return (player == Player::X) ? "X" : ((player == Player::O) ? "O" : " ");
}

std::ostream& operator<<(std::ostream& os, const ConnectFourState& state) {
    os << state.toString();
    return os;
}

ConnectFourState& ConnectFourState::operator=(const ConnectFourState& state) {
    _deepcopy(state);
    return *this;
}

bool ConnectFourState::operator==(const ConnectFourState& rhs) const {
    return (_current_player == rhs._current_player) && (_state == rhs._state) &&
           (_lastPlacedColumn == rhs._lastPlacedColumn) &&
           (_lastPlacedRow && rhs._lastPlacedRow);
}

std::array<std::array<char, 7>, 6> ConnectFourState::state() const {
    return _state;
}

int ConnectFourState::_lowest_playable_row(int column) const {
    for (int row = _ROWS - 1; row >= 0; --row) {
        if (_state[row][column] == _EMPTY_STATE) {
            return row;
        }
    }
    return _ROW_FILLED;
}

bool ConnectFourState::_column_playable(int column) const {
    return _state[0][column] == _EMPTY_STATE;
}

char ConnectFourState::_map_player_to_state(Player player) const {
    switch (player) {
        case Player::X:
            return _PLAYER_X_STATE;
        case Player::O:
            return _PLAYER_O_STATE;
        default:
            throw std::logic_error("An unknown player was passed.");
    }
}

void ConnectFourState::_setColumn(int column, Player player) {
    if (_column_playable(column)) {
        int row = _lowest_playable_row(column);
        _state[row][column] = _map_player_to_state(player);
        _lastPlacedRow = row;
        _lastPlacedColumn = column;

        if ((_first_winner == Player::None) && _checkWin(column, row)) {
            _first_winner = player;
        }

        _current_player = (player == Player::X) ? Player::O : Player::X;
    } else {
        throw std::invalid_argument(std::to_string(column) +
                                    " is not playable.\n" + toString());
    }
}

void ConnectFourState::_defaultFill() {
    for (int row = 0; row < _ROWS; ++row) {
        for (int col = 0; col < _COLUMNS; ++col) {
            _state[row][col] = _EMPTY_STATE;
        }
    }
}

bool ConnectFourState::_checkWinGeneral(
    const std::array<std::array<char, 7>, 6>& stateVector, int column,
    int row) const {
    const int PLAYER_STATE = stateVector[row][column];

    if (PLAYER_STATE == _EMPTY_STATE) {
        return false;
    }

    // Citation
    //    Own CMPT 213 Assignment 4 for win checking TicTacToe

    const int SCAN_DIRECTION[4][2] = {{0, 1}, {1, 0}, {1, 1}, {-1, 1}};

    const auto isCoordinateValid = [&](int column, int row) -> bool {
        return (column >= 0) && (column < _COLUMNS) && (row >= 0) &&
               (row < _ROWS);
    };

    for (int scan = 0; scan < 4; ++scan) {
        int columnHead = column;
        int rowHead = row;
        int consecutives = 1;

        const int POSITIVE_DX = SCAN_DIRECTION[scan][0];
        const int POSITIVE_DY = SCAN_DIRECTION[scan][1];
        const int NEGATIVE_DX = -1 * POSITIVE_DX;
        const int NEGATIVE_DY = -1 * POSITIVE_DY;

        while (isCoordinateValid(columnHead + NEGATIVE_DX,
                                 rowHead + NEGATIVE_DY) &&
               (stateVector[rowHead + NEGATIVE_DY][columnHead + NEGATIVE_DX] ==
                PLAYER_STATE)) {
            columnHead += NEGATIVE_DX;
            rowHead += NEGATIVE_DY;
        }

        while (isCoordinateValid(columnHead + POSITIVE_DX,
                                 rowHead + POSITIVE_DY) &&
               (stateVector[rowHead + POSITIVE_DY][columnHead + POSITIVE_DX] ==
                PLAYER_STATE)) {
            columnHead += POSITIVE_DX;
            rowHead += POSITIVE_DY;
            ++consecutives;
            if (consecutives >= _REQUIRED_CONSECUTIVE) {
                return true;
            }
        }
    }

    return false;
}

/**
 * Find if there is a win along the coordinate.
 */
bool ConnectFourState::_checkWin(int column, int row) const {
    return _checkWinGeneral(_state, column, row);
}

void ConnectFourState::_deepcopy(const ConnectFourState& state) {
    if (this != &state) {
        _current_player = state._current_player;
        _first_winner = state._first_winner;
        _state = state._state;
    }
}

namespace std {

template <>
struct hash<ConnectFourState> {
    size_t operator()(const ConnectFourState& state) const {
        size_t player_code =
            (state.currentPlayer() == ConnectFourState::Player::X)
                ? 1
                : (state.currentPlayer() == ConnectFourState::Player::O ? 2
                                                                        : 3);

        const std::array<std::array<char, 7>, 6> pieces = state.state();
        size_t state_code = pieces.size();
        for (int row = 0; row < pieces.size(); ++row) {
            for (int column = 0; column < pieces[row].size(); ++column) {
                // Equation from "HolKann" in the cited link at the citations
                // section.
                state_code ^= pieces[row][column] + 0x9e3779b9 +
                              (state_code << 6) + (state_code >> 2);
            }
        }
        return player_code + state_code + state.lastPlacedColumn() +
               state.lastPlacedRow();
    }
};
}  // namespace std