#ifndef GAME_SIMULATOR_CORE_FIXEDWORLD_H
#define GAME_SIMULATOR_CORE_FIXEDWORLD_H

#include "common/types.h"
#include "game/simulator/core/cXObject.h"

// A whole tile: x, y, level. Built from numbers, it turns anything that doesn't fit in a byte to 0.
class CTilePt {
public:
    s8 mX;     // 0x00
    s8 mY;     // 0x01
    s8 mLevel; // 0x02

    CTilePt() { mLevel = 0; }
    CTilePt(const CTilePt &other)
    {
        mX = other.mX;
        mY = other.mY;
        mLevel = other.mLevel;
    }
    CTilePt(int x, int y, int level)
    {
        mX = x;
        mY = y;
        if (mX != x)
            mX = 0;
        if (mY != y)
            mY = 0;
        mLevel = level;
    }
    // Wii: CTilePt(const FTilePt &, int), which doesn't check.
    CTilePt(const cXObject *object, int level)
    {
        mX = object->mTileX >> 4;
        mY = object->mTileY >> 4;
        mLevel = level;
    }
    ~CTilePt() {}
    CTilePt &operator=(const CTilePt &other)
    {
        mX = other.mX;
        mY = other.mY;
        mLevel = other.mLevel;
        return *this;
    }
    CTilePt &operator+=(const CTilePt &other)
    {
        mX += other.mX;
        mY += other.mY;
        return *this;
    }
    CTilePt operator-(const CTilePt &other) const { return CTilePt(mX - other.mX, mY - other.mY, mLevel); }
    void Set(int x, int y)
    {
        mX = x;
        mY = y;
    }
    void Set(int x, int y, int level)
    {
        mX = x;
        mY = y;
        mLevel = level;
    }
    int GetX() const { return mX; }
    int GetY() const { return mY; }
    int GetLevel() const { return mLevel; }
    // The same, called as functions (0x248DB8, 0x248DC0) where they aren't inlined.
    int GetXOutOfLine() const;
    int GetYOutOfLine() const;
};

// GenericSimCall 0x3A: one tile's walls, 0x40 bytes, with an out-of-line destructor. A segment
// is one side of the tile.
class TileWalls {
public:
    u32 pad_0x0[0x10];

    TileWalls(const TileWalls &other);                    // 0x1E2AF0
    ~TileWalls();                                         // 0x1E2B20
    int HasWall(int segment) const;                       // 0x1E2B78
    void AddWall(int segment);                            // 0x1E2EF0
    void SetStyle(int style, int segment);                // 0x1E2EB8
    void SetPattern(int pattern, int segment, int side);  // 0x1E2D60
    static int GetWallSegsBetweenTiles(CTilePt *a, CTilePt *b, int &segmentA, int &segmentB); // 0x1C1F98
};

class ERoom {
public:
    static void DeleteWallAtTile(const CTilePt &tile, TileWalls &walls, int segment); // 0x31F8C8
};

// The tile flag grid: row pointers by x, a byte for each y.
struct TileFlagGrid {
    u8 pad_0x0[0xc];
    s8 **mRows; // 0x0c
};

class cFixedWorld {
public:
    u8 pad_0x0[0x20];
    TileFlagGrid *mFlags; // 0x20

    int OutOfGrid(const CTilePt &tile) const;
    TileWalls GetWall(const CTilePt &tile);              // 0x1C5470
    void SetWall(const CTilePt &tile, TileWalls walls);  // 0x1C5910
    void CommitWallChanges();                            // 0x1E4998
    int GetXSize() const; // 0x1E4378, +0x10
    int GetYSize() const; // 0x1E4380, +0x14
    int MayEditTile(const CTilePt &tile) const; // 0x1E47C0  Wii's name; PS2 by cXObject::UserCanPickup
    // Wii: GetFlags and SetFlags, inlined in GenericSimCall 0x33.
    s8 GetFlags(const CTilePt &tile) const { return mFlags->mRows[tile.GetXOutOfLine()][tile.GetYOutOfLine()]; }
    void SetFlags(const CTilePt &tile, s8 flags) { mFlags->mRows[tile.GetXOutOfLine()][tile.GetYOutOfLine()] = flags; }
};
extern cFixedWorld *gFixedWorld;

// Set by the user-placement checks: the reason the last one failed (UserCanPickup's codes
// are 0x25 to 0x27, UserCanPlace's 1 and 0xB) and the object that was in the way.
extern int gPlacementError;           // 0x492824
extern cXObject *gPlacementConflict;  // 0x492828

#endif
