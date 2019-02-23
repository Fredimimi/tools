/*
 * AIM mpj_loader
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

#include <buffer.h>
#include <objects.h>
#include <types.h>

#include <primitives/filesystem.h>

#include <map>
#include <set>
#include <stdint.h>
#include <string>
#include <vector>

enum class SegmentType : uint32_t
{
    none            =   0,
    MapData         =   1,
    Surface         =   2,
    Weather         =   4,
    Objects         =   6,
    unk7            =   7,
    Water           =   8,
    unk9            =   9,
    BuildingGoods   =   10,
    MapMusic        =   11,
    Organizations   =   12,
    Goods           =   13,
};

struct segment
{
    SegmentType type;
    uint32_t unk0;
    uint32_t size;

    virtual ~segment() {}

    static segment *create_segment(const buffer &b);
    virtual void load(const buffer &b) = 0;
};

struct map_data : public segment
{
    std::vector<float> unk1;
    std::vector<uint32_t> unk2;
    std::vector<uint32_t> unk3;

    virtual void load(const buffer &b) override;
};

struct surface : public segment
{
    struct value
    {
        std::string name;
        uint32_t unk0;
    };
    std::vector<value> unk1;

    virtual void load(const buffer &b) override;
};

struct weather_data : public segment
{
    weather_group wg;

    virtual void load(const buffer &b) override;
};

struct objects_data : public segment
{
    Objects objects;

    virtual void load(const buffer &b) override;
};

struct segment7 : public segment
{
    std::vector<uint32_t> data;

    virtual void load(const buffer &b) override;
};

struct water_data : public segment
{
    water_group wg;

    virtual void load(const buffer &b) override;
};

struct segment9 : public segment
{
    std::vector<uint32_t> data;

    virtual void load(const buffer &b) override;
};

struct building_goods : public segment
{
    struct bg_internal
    {
        BuildingGoods bg;
        uint32_t unk0;

        void load(const buffer &b)
        {
            bg.load(b);
            READ(b, unk0);
        }
    };
    uint32_t unk1; // record format? aim2 ?
    uint32_t n;
    std::vector<bg_internal> bgs;

    virtual void load(const buffer &b) override;
};

struct map_music : public segment
{
    std::vector<MapMusic> mms;
    uint32_t unk1;

    virtual void load(const buffer &b) override;
};

struct organizations : public segment
{
    std::vector<Organization> orgs;
    std::vector<OrganizationBase> organizationBases;

    virtual void load(const buffer &b) override;
};

struct gliders_n_goods : public segment
{
    struct Good
    {
        std::string name;
        int unk0[3];

        void load(const buffer &b)
        {
            READ_STRING(b, name);
            READ(b, unk0);
        }
    };

    struct Goods
    {
        uint32_t n_goods;
        std::vector<Good> goods;

        void load(const buffer &b)
        {
            READ(b, n_goods);
            goods.resize(n_goods);
            for (auto &g : goods)
                g.load(b);
        }
    };

    uint32_t n_good_groups;
    uint32_t n_gliders;
    std::vector<Good> gliders;
    uint32_t unk1;
    std::vector<Goods> goods;

    virtual void load(const buffer &b) override;
};

struct header
{
    char magic[4];
    uint32_t unk0;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t unk3;
    uint32_t width;
    uint32_t height;
    char unk4[0x3F4];
    std::vector<segment*> segments;

    void load(const buffer &b);
};

struct mpj
{
    header h;

    //
    path filename;

    void load(const buffer &b);
    void load(const path &filename);
};
