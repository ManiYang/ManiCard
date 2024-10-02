#ifndef SETS_UTIL_H
#define SETS_UTIL_H

#include <initializer_list>
#include <type_traits>
#include <QSet>

//!
//! The enum type \e EnumType must have \c int as the underlying type.
//!
template <class EnumType>
class SetOfEnumItems
{
public:
    SetOfEnumItems() {}

    SetOfEnumItems(std::initializer_list<EnumType> list) {
        for (const EnumType item: list)
            set << static_cast<int>(item);
    }

    template <class Container>
    SetOfEnumItems(const Container &c) {
        static_assert(std::is_same_v<typename Container::value_type, EnumType>);
        for (const EnumType item: c)
            set << static_cast<int>(item);
    }

    template <class InputIt>
    SetOfEnumItems(InputIt first, InputIt last) {
        static_assert(std::is_same_v<typename InputIt::value_type, EnumType>);
        for (auto it = first; it != last; it++)
            set << static_cast<int>(*it);
    }

    //

    bool isEmpty() const {
        return set.isEmpty();
    }

    int count() const {
        return set.count();
    }

    bool contains(const EnumType item) const {
        return set.contains(static_cast<int>(item));
    }

    bool contains(const SetOfEnumItems<EnumType> &other) const {
        return set.contains(other.set);
    }

    bool intersects(const SetOfEnumItems<EnumType> &other) const {
        return set.intersects(other.set);
    }

    void clear() {
        set.clear();
    }

    SetOfEnumItems &operator << (const EnumType item) {
        set << static_cast<int>(item);
        return *this;
    }

private:
    QSet<int> set;
};


#endif // SETS_UTIL_H
