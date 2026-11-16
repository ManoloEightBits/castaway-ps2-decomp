#ifndef GAME_WEATHER_H
#define GAME_WEATHER_H

#include "common/eastl.h"
#include "common/types.h"

namespace Weather {
enum eDamageType { DAMAGE_TYPE_COUNT = 3 };
// Wii: Weather::DamageEvent. One amount per damage type for the clothes, and one for the
// skin. Nothing else is known.
struct DamageEvent {
    s16 amounts[DAMAGE_TYPE_COUNT]; // 0x00
    s16 skinAmount;                 // 0x06
};
// Wii: Weather::Armor. One value per damage type, the same shape as DamageEvent. Its
// constructor is a memset, which cXObject::ApplyDamage inlines.
struct Armor {
    s16 values[4];
    Armor() { memset(values, 0, sizeof(values)); }
};
}  // namespace Weather

#endif
