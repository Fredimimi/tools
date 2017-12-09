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

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdio.h>
#include <string>

#define _USE_MATH_DEFINES
#include <math.h>

#include <objects.h>
#include "other.h"

#include <Polygon4/DataManager/Storage.h>
#include <Polygon4/DataManager/Types.h>
#include <primitives/filesystem.h>

#include <buffer.h>
#include <types.h>

using namespace polygon4;
using namespace polygon4::detail;

#define RAD2GRAD(x) (x) = (x) / M_PI * 180.0
#define ASSIGN(x, d) isnan(x) ? d : x

std::string prefix;
int inserted_all = 0;

struct mmo_storage
{
    path name;
    Objects objects;
    MechGroups mechGroups;
    MapGoods mapGoods;
    uint32_t unk0 = 0;
    MapMusic mapMusic;
    MapSounds mapSounds;
    // aim2
    Organizations orgs;
    OrganizationBases orgsBases;
    Prices prices;

    void load(const buffer &b)
    {
        objects.load(b);
        mechGroups.load(b);
        if (b.eof()) // custom maps
            return;
        mapGoods.load(b);
        READ(b, unk0);
        mapMusic.load(b);
        mapSounds.load(b);
        if (gameType == GameType::Aim2)
        {
            orgs.load(b);
            orgsBases.load(b);
            prices.load(b);
        }

        if (!b.eof())
        {
            std::stringstream ss;
            ss << std::hex << b.index() << " != " << std::hex << b.size();
            throw std::logic_error(ss.str());
        }
    }
};

mmo_storage read_mmo(const path &fn)
{
    buffer f(read_file(fn));
    mmo_storage s;
    s.name = fn;
    s.load(f);
    return s;
}

void write_mmo(Storage *storage, const mmo_storage &s)
{
    std::string map_name = s.name.filename().stem().string();
    if (!prefix.empty())
        map_name = prefix + "." + map_name;
    std::transform(map_name.begin(), map_name.end(), map_name.begin(), ::tolower);

    int map_id = 0;
    for (auto &m : storage->maps)
    {
        if (m.second->text_id == map_name)
        {
            map_id = m.first;
            break;
        }
    }

    if (map_id == 0)
    {
        printf("error: this map is not found in the database\n");
        return;
    }

    auto this_map = storage->maps[map_id];

    auto check_val = [](auto &v)
    {
        if (v > 1)
        {
            v = v - floor(v);
            std::cerr << /*object->name2 << */": yaw > 1\n";
        }
        if (v < -1)
        {
            v = v - ceil(v);
            std::cerr << /*object->name2 << */": yaw < -1\n";
        }
    };

    auto calc_yaw = [&check_val](auto &v)
    {
        //check_val(v[0].x);
        //check_val(v[1].x);

        auto yaw = atan2(v[1].x, v[0].x);
        yaw = RAD2GRAD(yaw);
        return -(yaw - 90);
    };

    int inserted = 0;
    int exist = 0;
    for (auto &seg : s.objects.segments)
    {
        if (seg->segment_type == ObjectType::BUILDING ||
            seg->segment_type == ObjectType::TOWER)
        {
            SegmentObjects<::MapObject> *segment = (SegmentObjects<::MapObject> *)seg;
            std::set<std::string> objs;
            std::map<std::string, int> bld_ids;
            for (auto &object : segment->objects)
                objs.insert(object->name1);
            for (auto &o : objs)
            {
                auto iter = std::find_if(storage->buildings.begin(), storage->buildings.end(), [&](const auto &p)
                {
                    return p.second->text_id == o;
                });
                if (iter == storage->buildings.end())
                {
                    auto bld = storage->addBuilding();
                    bld->text_id = o;
                    bld_ids[o] = bld->getId();
                }
                else
                {
                    bld_ids[o] = iter->second->getId();
                }
            }
            for (auto &object : segment->objects)
            {
                MapBuilding mb;
                mb.text_id = object->name2;
                mb.building = storage->buildings[bld_ids[object->name1]];
                mb.map = this_map;
                mb.x = ASSIGN(object->position.x, 0);
                mb.y = ASSIGN(object->position.y, 0);
                mb.z = ASSIGN(object->position.z, 0);
                mb.roll = 0;
                mb.pitch = 0;
                /*auto yaw = ASSIGN(object->m_rotate_z[0].x, 0);
                if (yaw > 1)
                {
                    yaw = yaw - floor(yaw);
                    std::cerr << object->name2 << ": yaw > 1\n";
                }
                if (yaw < -1)
                {
                    yaw = yaw - ceil(yaw);
                    std::cerr << object->name2 << ": yaw < -1\n";
                }
                mb.yaw = acos(yaw);
                RAD2GRAD(mb.yaw);
                // wrong 1
                /*if (mb.yaw < 90)
                    mb.yaw += 90;
                else
                {
                    mb.yaw -= 90;
                    mb.yaw = -mb.yaw;
                }*/
                mb.yaw = calc_yaw(object->m_rotate_z);
                mb.scale = ASSIGN(object->m_rotate_z[2].z, 1);
                auto i = find_if(storage->mapBuildings.begin(), storage->mapBuildings.end(), [&](const auto &p)
                {
                    return *p.second == mb;
                });
                if (i == storage->mapBuildings.end())
                {
                    auto mb2 = storage->addMapBuilding(storage->maps[map_id]);
                    mb.setId(mb2->getId());
                    *mb2 = mb;
                    inserted++;
                }
                else
                {
                    exist++;
                }
            }
        }

        if (seg->segment_type == ObjectType::TREE       ||
            seg->segment_type == ObjectType::STONE      ||
            seg->segment_type == ObjectType::LAMP       ||
            seg->segment_type == ObjectType::BOUNDARY)
        {
            SegmentObjects<::MapObject> *segment = (SegmentObjects<::MapObject> *)seg;
            std::set<std::string> objs;
            std::map<std::string, int> bld_ids;
            for (auto &object : segment->objects)
                objs.insert(object->name1);
            for (auto &o : objs)
            {
                auto iter = find_if(storage->objects.begin(), storage->objects.end(), [&](const auto &p)
                {
                    return p.second->text_id == o;
                });
                if (iter == storage->objects.end())
                {
                    auto bld = storage->addObject();
                    bld->text_id = o;
                    bld_ids[o] = bld->getId();
                }
                else
                    bld_ids[o] = iter->second->getId();
            }
            for (auto &object : segment->objects)
            {
                detail::MapObject mb;
                mb.text_id = object->name2;
                mb.map = this_map;
                mb.object = storage->objects[bld_ids[object->name1]];
                mb.x = ASSIGN(object->position.x, 0);
                mb.y = ASSIGN(object->position.y, 0);
                mb.z = ASSIGN(object->position.z, 0);
                mb.roll = 0;
                mb.pitch = 0;
                mb.yaw = calc_yaw(object->m_rotate_z) - 90;
                mb.scale = ASSIGN(object->m_rotate_z[2].z, 1);
                auto i = find_if(storage->mapObjects.begin(), storage->mapObjects.end(), [&](const auto &p)
                {
                    return *p.second == mb;
                });
                if (i == storage->mapObjects.end())
                {
                    auto mb2 = storage->addMapObject(storage->maps[map_id]);
                    mb.setId(mb2->getId());
                    *mb2 = mb;
                    inserted++;
                }
                else
                {
                    exist++;
                }
            }
        }
    }
    inserted_all += inserted;
    std::cout << "inserted: " << inserted << ", exist: " << exist << "\n";
}

int main(int argc, char *argv[])
try
{
    if (argc != 4)
    {
        std::cout << "Usage:\n" << argv[0] << " db.sqlite {file.mmo,mmo_dir} prefix" << "\n";
        return 1;
    }
    prefix = argv[3];
    if (prefix == "m1")
        gameType = GameType::Aim1;
    else if (prefix == "m2")
        gameType = GameType::Aim2;
    else
        throw std::runtime_error("unknown prefix (game type)");

    auto storage = initStorage(argv[1]);
    storage->load();

    path p = argv[2];
    if (fs::is_regular_file(p))
        write_mmo(storage.get(), read_mmo(p));
    else if (fs::is_directory(p))
    {
        auto files = enumerate_files_like(p, ".*\\.mmo", false);
        for (auto &f : files)
        {
            std::cout << "processing: " << f << "\n";
            write_mmo(storage.get(), read_mmo(f));
        }
    }
    else
        throw std::runtime_error("Bad fs object");

    if (inserted_all)
        storage->save();

    return 0;
}
catch (std::exception &e)
{
    printf("error: %s\n", e.what());
    return 1;
}
catch (...)
{
    printf("error: unknown exception\n");
    return 1;
}
