#ifndef MAP_UPDATE_H
#define MAP_UPDATE_H

#include <QHash>
#include <QSet>

template <class K, class V>
class MapUpdate
{
private:
    enum class Type { UpdateKeys, SetWhole };
    explicit MapUpdate(const Type type_) : type(type_) {}

public:
    //!
    //! Creates an empty (i.e., containing no update) MapUpdate object.
    //!
    explicit MapUpdate() : type(Type::UpdateKeys) {}

    static MapUpdate UpdatingKeys(const QHash<K, V> &keysToUpdate, const QSet<K> &keysToRemove) {
        MapUpdate update {Type::UpdateKeys};
        update.keysToUpdate = keysToUpdate;
        update.keysToRemove = keysToRemove;
        return update;
    }

    static MapUpdate SettingWhole(const QHash<K, V> &updatedMap) {
        MapUpdate update {Type::SetWhole};
        update.updatedWholeMap = updatedMap;
        return update;
    }

    QHash<K, V> applyTo(const QHash<K, V> &map) const {
        switch (type) {
        case Type::UpdateKeys:
            if (keysToUpdate.isEmpty() && keysToRemove.isEmpty()) {
                return map;
            }
            else {
                QHash<K, V> result;
                for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
                    const auto &key = it.key();
                    if (keysToRemove.contains(key))
                        continue;
                    if (keysToUpdate.contains(key))
                        result.insert(key, keysToUpdate.value(key));
                    else
                        result.insert(key, it.value());
                }
                return result;
            }

        case Type::SetWhole:
            return updatedWholeMap;
        }
        Q_ASSERT(false); // case not implemented
    }

private:
    const Type type;
    QHash<K, V> keysToUpdate;
    QSet<K> keysToRemove;
    QHash<K, V> updatedWholeMap;
};

#endif // MAP_UPDATE_H
