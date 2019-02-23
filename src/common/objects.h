/*
 * AIM obj_extractor
 * Copyright (C) 2015 lzwdgc
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "buffer.h"
#include "types.h"

enum class ObjectType : uint32_t
{
    TEXTURE         =   1,
    //MODEL,
    //SURFACE,
    STONE           =   3,
    TREE            =   4,

    //GLIDER,
    HELPER          =   7,
    ROAD            =   8,
    //WEAPON,
    //CONFIG,

    //SHELL, // buildings
    BUILDING        =   11,
    IMAGE,
    LAMP            =   13,
    //EXPLOSION, // road lights
    //EQUIPMENT,
    //ORGANIZATION,

    //COVERING        =   18,
    SOUND           =   19,
    //GOODS, // anomaly?
    ANOMALY         =   20, // radiation?

    unk0            =   21, // anomaly?
    TOWER           =   22,
    BOUNDARY        =   23,
    SOUND_ZONE      =   24,

    TANK            =   27,
};

struct Segment
{
    ObjectType segment_type;
    uint32_t segment_len = 0;
    uint32_t n_objects = 0;

    virtual ~Segment(){}
    static Segment *create_segment(const buffer &b);
    virtual void load(const buffer &b) = 0;
};

template <class T>
struct SegmentObjects : public Segment
{
    std::vector<T> objects;

    virtual void load(const buffer &b)
    {
        for (uint32_t i = 0; i < n_objects; i++)
        {
            T o;
            o.load(b);
            objects.push_back(o);
        }
    }
};

struct Common
{
    // m_rotate_z[2].z - model scale on the map
    vector4 m_rotate_z[3];
    vector4 position;

    void load(const buffer &b)
    {
        READ(b, m_rotate_z);
        READ(b, position);
    }
};

struct MapObject : public Common
{
    std::string name1;
    std::string name2;

    void load(const buffer &b)
    {
        Common::load(b);

        READ_STRING(b, name1);
        READ_STRING(b, name2);
    }
};

struct MapObjectWithArray : public MapObject
{
    uint32_t len = 0;
    std::vector<uint32_t> unk0;

    void load(const buffer &b)
    {
        MapObject::load(b);

        READ(b, len);
        unk0.resize(len);
        for (uint32_t i = 0; i < len; i++)
            READ(b, unk0[i]);
    }
};

struct Sound : public Common
{
    uint32_t unk1[11];
    char name1[0x14];

    void load(const buffer &b)
    {
        Common::load(b);

        READ(b, unk1);
        READ(b, name1);
    }
};

struct Road : public MapObjectWithArray {};
struct Boundary : public MapObjectWithArray {};

#define KNOWN_OBJECT(name) \
    struct name : public MapObject {}

KNOWN_OBJECT(Stone);
KNOWN_OBJECT(Helper);
KNOWN_OBJECT(Building);
KNOWN_OBJECT(Tree);
KNOWN_OBJECT(Lamp);
KNOWN_OBJECT(Image);
KNOWN_OBJECT(Anomaly);
KNOWN_OBJECT(Tower);
KNOWN_OBJECT(SoundZone);
KNOWN_OBJECT(Tank);

#define UNKNOWN_OBJECT(name) \
    struct name : public MapObject { void load(const buffer &b){ int pos = b.index(); assert(false); } }

UNKNOWN_OBJECT(unk0);

struct Objects
{
    std::vector<Segment *> segments;

    void load(const buffer &b);
};
