// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "TerrainBQOutput.h"
#include "helpers/containerUtils.h"
#include "legacy/TerrainData.h"
#include "lua/GameDataLoader.h"
#include "gameData/EdgeDesc.h"
#include "gameData/TerrainDesc.h"
#include "gameData/WorldDescription.h"
#include "rttr/test/LogAccessor.hpp"
#include "rttr/test/TmpFolder.hpp"
#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/test/unit_test.hpp>
#include <sstream>

namespace bfs = boost::filesystem;
namespace bnw = boost::nowide;

BOOST_AUTO_TEST_SUITE(GameData)

BOOST_AUTO_TEST_CASE(BQ_Output)
{
    std::stringstream s;
    s << TerrainBQ::Nothing;
    BOOST_TEST(s.str() == "Nothing");
    s.str("");
    s << TerrainBQ::Mine;
    BOOST_TEST(s.str() == "Mine");
}

BOOST_AUTO_TEST_CASE(LoadGameData)
{
    WorldDescription worldDesc;
    loadGameData(worldDesc);
    BOOST_TEST_REQUIRE(worldDesc.edges.size() == 3u * 5u - 1u);
    BOOST_TEST_REQUIRE(worldDesc.terrain.size() >= 3u * NUM_TTS);
    for(unsigned l = 0; l < NUM_LTS; l++)
    {
        auto lt = Landscape(l);
        // indexOf(name) in these arrays should be the original RTTR index as in TerrainData
        std::vector<std::string> tNames, eNames;
        for(DescIdx<TerrainDesc> i(0); i.value < worldDesc.terrain.size(); i.value++)
        {
            if(worldDesc.get(worldDesc.get(i).landscape).s2Id == l)
                tNames.push_back(worldDesc.get(i).name);
        }
        for(DescIdx<EdgeDesc> i(0); i.value < worldDesc.edges.size(); i.value++)
        {
            if(worldDesc.get(worldDesc.get(i).landscape).s2Id == l)
                eNames.push_back(worldDesc.get(i).name);
        }
        for(unsigned i = 0; i < NUM_TTS; i++)
        {
            auto t = TerrainType(i);
            const TerrainDesc& desc = worldDesc.terrain.get(worldDesc.terrain.getIndex(tNames[i]));
            BOOST_TEST_REQUIRE(desc.s2Id == TerrainData::GetTextureIdentifier(t));
            EdgeType newEdge =
              !desc.edgeType ? ET_NONE : EdgeType((helpers::indexOf(eNames, worldDesc.get(desc.edgeType).name) + 1));
            BOOST_TEST_REQUIRE(newEdge == TerrainData::GetEdgeType(lt, t));
            BOOST_TEST_REQUIRE(desc.GetBQ() == TerrainData::GetBuildingQuality(t));
            BOOST_TEST(!desc.Is(ETerrain::Walkable)
                       == (TerrainData::GetBuildingQuality(t) == TerrainBQ::Nothing
                           || TerrainData::GetBuildingQuality(t) == TerrainBQ::Danger));
            BOOST_TEST_REQUIRE(desc.Is(ETerrain::Mineable) == (TerrainData::GetBuildingQuality(t) == TerrainBQ::Mine));
            BOOST_TEST_REQUIRE(desc.Is(ETerrain::Buildable)
                               == (TerrainData::GetBuildingQuality(t) == TerrainBQ::Castle));
            BOOST_TEST_REQUIRE(desc.Is(ETerrain::Shippable) == TerrainData::IsUsableByShip(t));
            BOOST_TEST_REQUIRE((desc.kind == TerrainKind::Snow) == TerrainData::IsSnow(lt, t));

            BOOST_TEST_REQUIRE((desc.IsUsableByAnimals() || desc.kind == TerrainKind::Snow)
                               == (TerrainData::IsUsableByAnimals(t) || TerrainData::IsSnow(lt, t)));
            BOOST_TEST_REQUIRE(desc.IsVital() == TerrainData::IsVital(t));
            BOOST_TEST_REQUIRE(desc.minimapColor == TerrainData::GetColor(lt, t));
        }
    }
    // TerrainData::PrintEdgePrios();
}

BOOST_AUTO_TEST_CASE(DetectRecursion)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp.get() / "default.lua");
        file << "include(\"foo.lua\")\n";
        bnw::ofstream file2(tmp.get() / "foo.lua");
        file2 << "include(\"default.lua\")\n";
    }
    WorldDescription desc;
    GameDataLoader loader(desc, tmp.get());
    rttr::test::LogAccessor logAcc;
    BOOST_TEST(!loader.Load());
    RTTR_REQUIRE_LOG_CONTAINS("Maximum include depth", false);
}

BOOST_AUTO_TEST_CASE(DetectInvalidFilenames)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp.get() / "default.lua");
        file << "include(\"foo(=.lua\")\n";
    }
    WorldDescription desc;
    GameDataLoader loader(desc, tmp.get());
    rttr::test::LogAccessor logAcc;
    BOOST_TEST(!loader.Load());
    RTTR_REQUIRE_LOG_CONTAINS("disallowed chars", false);
}

BOOST_AUTO_TEST_CASE(DetectNonexistingFile)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp.get() / "default.lua");
        file << "include(\"foo.lua\")\n";
    }
    WorldDescription desc;
    GameDataLoader loader(desc, tmp.get());
    rttr::test::LogAccessor logAcc;
    BOOST_TEST(!loader.Load());
    RTTR_REQUIRE_LOG_CONTAINS("File not found", false);
}

BOOST_AUTO_TEST_CASE(DetectWrongExtension)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp.get() / "default.lua");
        file << "include(\"foo.txt\")\n";
        bnw::ofstream file2(tmp.get() / "foo.txt");
    }
    WorldDescription desc;
    GameDataLoader loader(desc, tmp.get());
    rttr::test::LogAccessor logAcc;
    BOOST_TEST(!loader.Load());
    RTTR_REQUIRE_LOG_CONTAINS("File must have .lua as the extension", false);
}

BOOST_AUTO_TEST_CASE(DetectFolderEscape)
{
    rttr::test::TmpFolder tmp;
    bfs::path basePath(tmp.get() / "gameData");
    create_directories(basePath);
    {
        bnw::ofstream file(basePath / "default.lua");
        file << "include(\"../foo.lua\")\n";
        bnw::ofstream file2(tmp.get() / "foo.lua");
    }
    WorldDescription desc;
    GameDataLoader loader(desc, basePath);
    rttr::test::LogAccessor logAcc;
    BOOST_TEST(!loader.Load());
    RTTR_REQUIRE_LOG_CONTAINS("outside the lua data directory", false);
}

#ifndef BOOST_WINDOWS_API
BOOST_AUTO_TEST_CASE(DetectAbsolute)
{
    rttr::test::TmpFolder tmp;
    bfs::path basePath(tmp.get() / "gameData");
    create_directories(basePath);
    {
        bnw::ofstream file(basePath / "default.lua");
        file << "include(\"/tmp/foo.lua\")\n";
    }
    WorldDescription desc;
    GameDataLoader loader(desc, basePath);
    rttr::test::LogAccessor logAcc;
    BOOST_TEST(!loader.Load());
    RTTR_REQUIRE_LOG_CONTAINS("relative", false);
}
#endif

BOOST_AUTO_TEST_CASE(TextureCoords)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp.get() / "default.lua");
        file << "rttr:AddLandscape{\
            name = \"testland\",\
            mapGfx = \"<RTTR_GAME>/DATA/MAP_0_Z.LST\",\
            roads = {\
                normal = { texture = \"<RTTR_GAME>/foo\"},\
                upgraded = { texture = \"<RTTR_GAME>/foo\"},\
                boat = { texture = \"<RTTR_GAME>/foo\"},\
                mountain = { texture = \"<RTTR_GAME>/foo\"}\
            }}";
        file << "rttr:AddTerrainEdge{ name = \"edge\", landscape = \"testland\", texture = \"<RTTR_GAME>/foo\" }";
        file << "rttr:AddTerrain{\
            name = \"terrain1\",\
            landscape = \"testland\", edgeType = \"edge\", texture = \"<RTTR_GAME>/foo\", color = 0, \
            pos = { 10, 20, 32, 31 }, texType = \"overlapped\" }";
        file << "rttr:AddTerrain{\
            name = \"terrain2\",\
            landscape = \"testland\", edgeType = \"edge\", texture = \"<RTTR_GAME>/foo\", color = 0, \
            pos = { 10, 20, 32, 31 }, texType = \"stacked\" }";
        file << "rttr:AddTerrain{\
            name = \"terrain3\",\
            landscape = \"testland\", edgeType = \"edge\", texture = \"<RTTR_GAME>/foo\", color = 0, \
            pos = { 10, 20, 32, 31 }, texType = \"rotated\" }";
    }
    WorldDescription desc;
    GameDataLoader loader(desc, tmp.get());
    BOOST_TEST_REQUIRE(loader.Load());
    using PointF = TerrainDesc::PointF;
    // Border points are inset by half a pixel for OpenGL (sample middle of pixel!)
    // Overlapped uses the full rectangle
    const TerrainDesc::Triangle rsuO = desc.terrain.tryGet("terrain1")->GetRSUTriangle();
    BOOST_TEST_REQUIRE(rsuO.tip == PointF(10 + 32 / 2.f, 20 + 0.5f));
    BOOST_TEST_REQUIRE(rsuO.left == PointF(10 + 0.5f, 20 + 31 - 0.5f));
    BOOST_TEST_REQUIRE(rsuO.right == PointF(10 + 32 - 0.5f, 20 + 31 - 0.5f));
    const TerrainDesc::Triangle usdO = desc.terrain.tryGet("terrain1")->GetUSDTriangle();
    BOOST_TEST_REQUIRE(usdO.tip == PointF(10 + 32 / 2.f, 20 + 31 - 0.5f));
    BOOST_TEST_REQUIRE(usdO.left == PointF(10 + 0.5f, 20 + 0.5f));
    BOOST_TEST_REQUIRE(usdO.right == PointF(10 + 32 - 0.5f, 20 + 0.5f));

    // Stacked has RSU over USD
    const TerrainDesc::Triangle rsuS = desc.terrain.tryGet("terrain2")->GetRSUTriangle();
    BOOST_TEST_REQUIRE(rsuS.tip == rsuO.tip);
    BOOST_TEST_REQUIRE(rsuS.left == PointF(10 + 0.5f, 20 + 31 / 2.f));
    BOOST_TEST_REQUIRE(rsuS.right == PointF(10 + 32 - 0.5f, 20 + 31 / 2.f));
    const TerrainDesc::Triangle usdS = desc.terrain.tryGet("terrain2")->GetUSDTriangle();
    BOOST_TEST_REQUIRE(usdS.tip == usdO.tip);
    BOOST_TEST_REQUIRE(usdS.left == PointF(10 + 0.5f, 20 + 31 / 2.f));
    BOOST_TEST_REQUIRE(usdS.right == PointF(10 + 32 - 0.5f, 20 + 31 / 2.f));

    // Rotated is stacked sideways (note that order stays the same)
    const TerrainDesc::Triangle rsuR = desc.terrain.tryGet("terrain3")->GetRSUTriangle();
    BOOST_TEST_REQUIRE(rsuR.tip == rsuS.left);
    BOOST_TEST_REQUIRE(rsuR.left == rsuS.right);
    BOOST_TEST_REQUIRE(rsuR.right == rsuS.tip);
    const TerrainDesc::Triangle usdR = desc.terrain.tryGet("terrain3")->GetUSDTriangle();
    BOOST_TEST_REQUIRE(usdR.tip == usdS.right);
    BOOST_TEST_REQUIRE(usdR.left == usdS.tip);
    BOOST_TEST_REQUIRE(usdR.right == usdS.left);
}

BOOST_AUTO_TEST_SUITE_END()
