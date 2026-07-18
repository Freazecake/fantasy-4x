#include "core/GameState.hpp"

void GameState::addPlayer(const Player &player) {
    m_players.push_back(player);
}

void GameState::setMap(const Map &map) {
    m_map = map;
}

void GameState::nextTurn() {
    ++m_turnNumber;
}
