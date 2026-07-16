#include "core/Economy.hpp"

void Resources::operator+=(const Resources &other)
{
    this->science += other.science;
    this->diplomacy += other.diplomacy;
    this->growth += other.growth;
    this->industry += other.industry;
    this->military += other.military;
    this->trade += other.trade;
}

Resources Resources::operator*(float num)
{
    this->science *= num;
    this->diplomacy *= num;
    this->growth *= num;
    this->industry *= num;
    this->military *= num;
    this->trade *= num;

    return *this;
}