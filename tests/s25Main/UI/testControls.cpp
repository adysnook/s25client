// Copyright (c) 2016 -2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "PointOutput.h"
#include "controls/ctrlDeepening.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlPreviewMinimap.h"
#include "controls/ctrlTextButton.h"
#include "controls/ctrlTextDeepening.h"
#include "driver/KeyEvent.h"
#include "driver/MouseCoords.h"
#include "helpers/mathFuncs.h"
#include "ogl/glArchivItem_Map.h"
#include "ogl/glFont.h"
#include "uiHelper/uiHelpers.hpp"
#include "libsiedler2/ArchivItem_Bitmap_Player.h"
#include "libsiedler2/ArchivItem_Font.h"
#include "libsiedler2/ArchivItem_Map.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libsiedler2/ArchivItem_Raw.h"
#include "libsiedler2/PixelBufferBGRA.h"
#include "rttr/test/random.hpp"
#include <boost/nowide/detail/utf.hpp>
#include <boost/test/unit_test.hpp>
#include <Loader.h>
#include <array>
#include <numeric>

static std::unique_ptr<glFont> createMockFont(const std::vector<char32_t>& chars)
{
    libsiedler2::ArchivItem_Font s2Font;
    s2Font.setDx(5);
    s2Font.setDy(9);
    libsiedler2::ArchivItem_Bitmap_Player glyph;
    libsiedler2::PixelBufferBGRA buffer(5, 9);
    libsiedler2::ArchivItem_Palette pal;
    glyph.create(buffer, &pal);
    const auto maxEl = *std::max_element(chars.begin(), chars.end());
    s2Font.alloc(maxEl + 1u);
    for(const auto c : chars)
        s2Font.setC(c, glyph);
    return std::make_unique<glFont>(s2Font);
}

BOOST_AUTO_TEST_SUITE(Controls)

static void resizeMap(glArchivItem_Map& glMap, const Extent& size)
{
    libsiedler2::ArchivItem_Map map;
    auto header = std::make_unique<libsiedler2::ArchivItem_Map_Header>();
    header->setWidth(size.x);
    header->setHeight(size.y);
    header->setNumPlayers(2);
    map.push(std::move(header));
    for(int i = 0; i <= MAP_TYPE; i++)
        map.push(std::make_unique<libsiedler2::ArchivItem_Raw>(std::vector<uint8_t>(prodOfComponents(size))));
    glMap.load(map);
}

BOOST_FIXTURE_TEST_CASE(PreviewMinimap, uiHelper::Fixture)
{
    DrawPoint pos(5, 12);
    Extent size(20, 10);
    ctrlPreviewMinimap mm(nullptr, 1, pos, size, nullptr);
    BOOST_TEST_REQUIRE(mm.GetCurMapSize() == Extent::all(0));
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize() == Extent::all(4)); // Padding
    // Remove padding
    mm.SetPadding(Extent::all(0));
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize() == Extent::all(0));
    glArchivItem_Map map;
    resizeMap(map, size);
    mm.SetMap(&map);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().x <= size.x); //-V807
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().y <= size.y);
    const Extent origBoundary = mm.GetBoundaryRect().getSize();
    // Width smaller -> Don't go over width
    resizeMap(map, Extent(size.x / 2, size.y));
    mm.SetMap(&map);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().x <= size.x);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().y == size.y);
    // Height smaller -> Don't go over height
    resizeMap(map, Extent(size.x, size.y / 2));
    mm.SetMap(&map);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().x == size.x);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().y <= size.y);
    // Both smaller -> Stretch to original height
    resizeMap(map, size / 2u);
    mm.SetMap(&map);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize() == origBoundary);
    // Width bigger -> Narrow map
    resizeMap(map, Extent(size.x * 2, size.y));
    mm.SetMap(&map);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().x == size.x);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().y <= size.y);
    // Height bigger -> Narrow map in x
    resizeMap(map, Extent(size.x, size.y * 2));
    mm.SetMap(&map);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().x <= size.x);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize().y == size.y);
    // Both bigger -> Stretch to original height
    resizeMap(map, size * 2u);
    mm.SetMap(&map);
    BOOST_TEST_REQUIRE(mm.GetBoundaryRect().getSize() == origBoundary);
}

BOOST_FIXTURE_TEST_CASE(EditShowsCorrectChars, uiHelper::Fixture)
{
    using utfDecoder = boost::nowide::detail::utf::utf_traits<char>;
    // Use a 1 Byte and a 2 Byte UTF8 "char"
    const std::array<std::string, 6> chars = {u8"a", u8"b", u8"c", u8"\u0424", u8"\u041A", u8"\u043b"};
    std::vector<char32_t> codepoints(chars.size() + 1);
    std::transform(chars.begin(), chars.end(), codepoints.begin(), [](const auto& c) {
        auto it = c.begin();
        return utfDecoder::decode_valid(it);
    });
    codepoints.back() = '?';
    const auto font = createMockFont(codepoints);
    ctrlEdit edt(nullptr, 0, DrawPoint(0, 0), Extent(90, 15), TC_GREEN1, font.get());
    ctrlEdit edt2(nullptr, 0, DrawPoint(0, 0), Extent(90, 15), TC_GREEN1, font.get());
    const ctrlBaseText* txt = edt.GetCtrl<ctrlBaseText>(0);
    const ctrlBaseText* txt2 = edt2.GetCtrl<ctrlBaseText>(0);
    BOOST_TEST_REQUIRE(txt);
    BOOST_TEST_REQUIRE(txt2);
    BOOST_TEST_REQUIRE(txt->GetText() == "");
    // Width available for text excludes the border left&right and 1 "Dx" for new chars
    const auto allowedWidth = edt.GetSize().x - ctrlDeepening::borderSize.x * 2;
    std::vector<std::string> curChars;
    // Add random chars and observe front chars beeing removed
    for(int i = 0; i < 30; i++)
    {
        curChars.push_back(chars[rttr::test::randomValue<size_t>(0u, chars.size() - 1)]);
        auto it = curChars.back().begin();
        const char32_t c = utfDecoder::decode_valid(it);
        std::string curText = std::accumulate(curChars.begin(), curChars.end(), std::string{});
        edt.SetText(curText);
        // Activate
        edt2.Msg_LeftDown(MouseCoords(edt2.GetPos(), true));
        edt2.Msg_PaintAfter();
        edt2.Msg_KeyDown(KeyEvent{KT_CHAR, c, false, false, false});
        // Remove chars from front until in size
        auto itFirst = curChars.begin();
        while(font->getWidth(curText) > allowedWidth)
        {
            curText = std::accumulate(++itFirst, curChars.end(), std::string{});
        }
        BOOST_TEST_REQUIRE(txt->GetText() == curText);
        BOOST_TEST_REQUIRE(txt2->GetText() == curText);
    }
    // Check navigating of cursor
    edt.Msg_LeftDown(MouseCoords(edt.GetPos(), true)); // Activate
    edt.Msg_PaintAfter();
    int curCursorPos = curChars.size(); // Current cursor should be at end
    while(!curChars.empty())
    {
        int moveOffset = rttr::test::randomValue<int>(-curCursorPos - 1, curChars.size() - curCursorPos + 1); //+-1 to check for "overrun"
        for(; moveOffset < 0; ++moveOffset, --curCursorPos)
            edt.Msg_KeyDown(KeyEvent{KT_LEFT, 0, false, false, false});
        for(; moveOffset > 0; --moveOffset, ++curCursorPos)
            edt.Msg_KeyDown(KeyEvent{KT_RIGHT, 0, false, false, false});
        curCursorPos = helpers::clamp(curCursorPos, 0, static_cast<int>(curChars.size()));
        // Erase one char (currently only good way to check where the cursor is
        edt.Msg_KeyDown(KeyEvent{KT_BACKSPACE, 0, false, false, false});
        if(curCursorPos > 0)
        {
            curChars.erase(curChars.begin() + --curCursorPos);
        }
        const auto curText = std::accumulate(curChars.begin(), curChars.end(), std::string{});
        BOOST_TEST_REQUIRE(edt.GetText() == curText);
    }
    // Check "movement" of text when using cursor
    // First create a text bigger than fit
    curChars.clear();
    std::string curText;
    do
    {
        curChars.push_back(chars[rttr::test::randomValue<size_t>(0u, chars.size() - 1)]);
        curText += curChars.back();
    } while(font->getWidth(curText) <= allowedWidth);
    edt.SetText(curText);
    // Now cursor is at end and first "char" is not showing
    curCursorPos = curChars.size();
    const auto txtWithoutFirst = curText.substr(curChars.front().size());
    BOOST_TEST_REQUIRE(font->getWidth(txtWithoutFirst) <= allowedWidth); // Sanity check
    // At least 5 chars before cursor should be shown -> No change until curCursorPos <= 5
    do
    {
        BOOST_TEST_REQUIRE(txt->GetText() == txtWithoutFirst);
        edt.Msg_KeyDown(KeyEvent{KT_LEFT, 0, false, false, false});
        --curCursorPos;
    } while(curCursorPos > 5);
    while(curCursorPos-- >= 0)
    {
        BOOST_TEST_REQUIRE(txt->GetText() == curText); // Trailing chars are removed by font rendering
        edt.Msg_KeyDown(KeyEvent{KT_LEFT, 0, false, false, false});
    }
    // Moving fully right shows txt again
    curCursorPos = 0;
    while(static_cast<unsigned>(curCursorPos++) < curChars.size())
        edt.Msg_KeyDown(KeyEvent{KT_RIGHT, 0, false, false, false});
    BOOST_TEST_REQUIRE(txt->GetText() == txtWithoutFirst);
}

BOOST_AUTO_TEST_CASE(AdjustWidthForMaxChars_SetsCorrectSize)
{
    auto font = createMockFont({'?', 'a', 'z'});
    {
        ctrlTextDeepening txt(nullptr, 1, rttr::test::randomPoint<DrawPoint>(), rttr::test::randomPoint<Extent>(), TC_GREEN1, "foo",
                              font.get(), COLOR_BLACK);
        const Extent sizeBefore = txt.GetSize();
        // Don't assume size, so get size for 0 chars
        txt.ResizeForMaxChars(0);
        const Extent sizeZero = txt.GetSize();
        BOOST_TEST(sizeZero.y == sizeBefore.y);
        const auto numChars = rttr::test::randomValue(1u, 20u);
        txt.ResizeForMaxChars(numChars);
        BOOST_TEST(txt.GetSize() == Extent(sizeZero.x + numChars * font->getDx(), sizeBefore.y));
    }
    {
        ctrlTextButton txt(nullptr, 1, rttr::test::randomPoint<DrawPoint>(), rttr::test::randomPoint<Extent>(), TC_GREEN1, "foo",
                           font.get(), "tooltip");
        const Extent sizeBefore = txt.GetSize();
        // Don't assume size, so get size for 0 chars
        txt.ResizeForMaxChars(0);
        const Extent sizeZero = txt.GetSize();
        BOOST_TEST(sizeZero.y == sizeBefore.y);
        const auto numChars = rttr::test::randomValue(1u, 20u);
        txt.ResizeForMaxChars(numChars);
        BOOST_TEST(txt.GetSize() == Extent(sizeZero.x + numChars * font->getDx(), sizeBefore.y));
    }
}

BOOST_AUTO_TEST_SUITE_END()
