#ifndef NAMING_RULES_H
#define NAMING_RULES_H

// User is not allowed to define names starting with '_'.
constexpr char regexPatternForCardLabelName[] = "^[A-Z][a-zA-Z0-9_]*$";
constexpr char regexPatternForRelationshipType[] = "^[A-Z][A-Z0-9_]*$";
constexpr char regexPatternForPropertyName[] = "^[a-z][a-zA-Z0-9_]*$";

#endif // NAMING_RULES_H
