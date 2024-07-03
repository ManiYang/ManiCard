#include "abstract_cards_data_access.h"


AbstractCardsDataAccessReadOnly::AbstractCardsDataAccessReadOnly(QObject *parent)
    : QObject(parent) {}

AbstractCardsDataAccess::AbstractCardsDataAccess(QObject *parent)
    : AbstractCardsDataAccessReadOnly(parent) {}
