#include "Planet.hpp"
#include "TerrainNode.hpp"

#include <set>
#include <array>
#include <algorithm>

UINT CPlanet::GridSize = 9;

std::map<int, std::map<int, int>> Quadtree<CTerrainNode>::FaceCorrection;
std::map<int, std::map<int, int>> Quadtree<CTerrainNode>::InternalCorrection;

CPlanet::CPlanet(ID3D11DeviceContext* context, ICamera* cam)
    : Camera(cam),
      Context(context)
{
    Context->GetDevice(&Device);
}

void CPlanet::Update(float dt)
{

}

float CPlanet::GetHeight(DirectX::SimpleMath::Vector3 normal)
{
    return 0.0f;
}

void CPlanet::GeneratePermutations()
{
    typedef std::array<UINT, 5> Tri;
    std::vector<Tri> ind;

    auto add_index = [&](std::vector<Tri>& ind, UINT x, UINT y, int i)
    {
        Tri t1, t2;

        if (i % 2 == 0)
        {
            t1[0] = y * GridSize + x;
            t1[1] = (y + 1) * GridSize + x + 1;
            t1[2] = y * GridSize + x + 1;

            t2[0] = (y + 1) * GridSize + x + 1;
            t2[1] = y * GridSize + x;
            t2[2] = (y + 1) * GridSize + x;
        }
        else
        {
            t1[0] = y * GridSize + x;
            t1[1] = (y + 1) * GridSize + x;
            t1[2] = y * GridSize + x + 1;

            t2[0] = y * GridSize + x + 1;
            t2[1] = (y + 1) * GridSize + x;
            t2[2] = (y + 1) * GridSize + x + 1;
        }

        t1[3] = x;
        t1[4] = y;
        t2[3] = x;
        t2[4] = y;

        ind.push_back(t1);
        ind.push_back(t2);
    };

    auto index_loop = [=](std::vector<Tri>& ind)
    {
        int i = 0;

        for (UINT y = 0; y < GridSize - 1; ++y)
        {
            for (UINT x = 0; x < GridSize - 1; ++x)
            {
                add_index(ind, x, y, i);
                ++i;
            }

            ++i;
        }
    };

    auto convert = [](std::vector<Tri> ind)
    {
        std::vector<UINT> r;

        std::for_each(ind.begin(), ind.end(), [&r](Tri t) {
            r.push_back(t[0]);
            r.push_back(t[1]);
            r.push_back(t[2]);
        });

        return r;
    };

    /*
        None
    */

    ind.clear();
    index_loop(ind);

    IndexPerm[0] = convert(ind);

    /*
        Top
    */

    ind.clear();
    index_loop(ind);

    auto apply_top = [&](std::vector<Tri>& ind) {
        int i = -1;

        ind.erase(std::remove_if(ind.begin(), ind.end(), [&i](const Tri& t) {
            ++i;
            return i % 2 == 0 && t[4] == 0;
        }), ind.end());

        for (UINT x = 0; x < GridSize - 2; x += 2)
        {
            Tri t = { x, x + GridSize + 1, x + 2, };
            ind.push_back(t);
        }
    };

    apply_top(ind);
    IndexPerm[Top] = convert(ind);

    /*
        Bottom
    */

    ind.clear();
    index_loop(ind);

    auto apply_bottom = [&](std::vector<Tri>& ind) {
        int i = -1;

        ind.erase(std::remove_if(ind.begin(), ind.end(), [&](const Tri& t) {
            ++i;
            return i % 2 != 0 && t[4] == GridSize - 2;
        }), ind.end());

        int y0 = (GridSize - 2) * GridSize;
        int y1 = (GridSize - 1) * GridSize;

        for (UINT x = 0; x < GridSize - 2; x += 2)
        {
            Tri t = { x + y1, x + y1 + 2, x + y0 + 1 };
            ind.push_back(t);
        }
    };

    apply_bottom(ind);
    IndexPerm[Bottom] = convert(ind);

    /*
        Right
    */

    ind.clear();
    index_loop(ind);

    auto apply_right = [&](std::vector<Tri>& ind) {
        std::set<int> list;
        int i = -1;

        for (UINT z = 1; z < GridSize * 2 - 1; z += 4)
        {
            list.insert(z);
            list.insert(z + 1);
        }

        ind.erase(std::remove_if(ind.begin(), ind.end(), [&](const Tri& t) {
            if (t[3] == GridSize - 2)
            {
                ++i;
                return list.find(i) != list.end();
            }

            return false;
        }), ind.end());

        int x = GridSize - 1;

        for (UINT y = 0; y < GridSize - 2; y += 2)
        {
            Tri t = { x + y * GridSize, (x - 1) + (y + 1) * GridSize, x + (y + 2) * GridSize };
            ind.push_back(t);
        }
    };

    apply_right(ind);
    IndexPerm[Right] = convert(ind);

    /*
        Left
    */

    ind.clear();
    index_loop(ind);

    auto apply_left = [&](std::vector<Tri>& ind) {
        std::set<UINT> list;
        int i = -1;

        for (UINT z = 1; z < GridSize * 2 - 1; z += 4)
        {
            list.insert(z);
            list.insert(z + 1);
        }

        ind.erase(std::remove_if(ind.begin(), ind.end(), [&](const Tri& t) {
            if (t[3] == 0)
            {
                ++i;
                return list.find(i) != list.end();
            }

            return false;
        }), ind.end());

        int x = 0;

        for (UINT y = 0; y < GridSize - 2; y += 2)
        {
            Tri t = { x + y * GridSize, x + (y + 2) * GridSize, (x + 1) + (y + 1) * GridSize };
            ind.push_back(t);
        }
    };

    apply_left(ind);
    IndexPerm[Left] = convert(ind);

    /*
        Top + Right
    */

    ind.clear();
    index_loop(ind);

    auto apply_topright = [&](std::vector<Tri>& ind) {
        int i = -1, j = -1;
        std::set<UINT> list;

        for (UINT z = 1; z < GridSize * 2 - 1; z += 4)
        {
            list.insert(z);
            list.insert(z + 1);
        }

        ind.erase(std::remove_if(ind.begin(), ind.end(), [&](const Tri& t) {
            ++j;

            if (t[3] == GridSize - 2)
            {
                ++i;

                if (list.find(i) != list.end())
                    return true;
            }

            return j % 2 == 0 && t[4] == 0;
        }), ind.end());

        for (UINT x = 0; x < GridSize - 2; x += 2)
        {
            Tri t = { x, x + GridSize + 1, x + 2 };
            ind.push_back(t);
        }

        int x = GridSize - 1;

        for (UINT y = 0; y < GridSize - 2; y += 2)
        {
            Tri t = { x + y * GridSize, (x - 1) + (y + 1) * GridSize, x + (y + 2) * GridSize };
            ind.push_back(t);
        }
    };

    apply_topright(ind);

    IndexPerm[Top | Right] = convert(ind);

    /*
        Right + Bottom
    */

    ind.clear();
    index_loop(ind);

    auto apply_rightbottom = [&](std::vector<Tri>& ind) {
        int i = -1, j = -1;
        std::set<UINT> list;

        for (UINT z = 1; z < GridSize * 2 - 1; z += 4)
        {
            list.insert(z);
            list.insert(z + 1);
        }

        ind.erase(std::remove_if(ind.begin(), ind.end(), [&](const Tri& t) {
            ++j;

            if (t[3] == GridSize - 2)
            {
                ++i;

                if (list.find(i) != list.end())
                    return true;
            }

            return j % 2 != 0 && t[4] == GridSize - 2;
        }), ind.end());

        int y0 = (GridSize - 2) * GridSize;
        int y1 = (GridSize - 1) * GridSize;

        for (UINT x = 0; x < GridSize - 2; x += 2)
        {
            Tri t = { x + y1, x + y1 + 2, x + y0 + 1 };
            ind.push_back(t);
        }

        int x = GridSize - 1;

        for (UINT y = 0; y < GridSize - 2; y += 2)
        {
            Tri t = { x + y * GridSize, (x - 1) + (y + 1) * GridSize, x + (y + 2) * GridSize };
            ind.push_back(t);
        }
    };

    apply_rightbottom(ind);

    IndexPerm[Right | Bottom] = convert(ind);

    /*
        Bottom + Left
    */

    ind.clear();
    index_loop(ind);

    auto apply_bottomleft = [&](std::vector<Tri>& ind) {
        UINT i = -1, j = -1;
        std::set<UINT> list;

        for (UINT z = 1; z < GridSize * 2 - 1; z += 4)
        {
            list.insert(z);
            list.insert(z + 1);
        }

        ind.erase(std::remove_if(ind.begin(), ind.end(), [&](const Tri& t) {
            ++j;

            if (t[3] == 0)
            {
                ++i;

                if (list.find(i) != list.end())
                    return true;
            }

            return j % 2 != 0 && t[4] == GridSize - 2;
        }), ind.end());

        int y0 = (GridSize - 2) * GridSize;
        int y1 = (GridSize - 1) * GridSize;

        for (UINT x = 0; x < GridSize - 2; x += 2)
        {
            Tri t = { x + y1, x + y1 + 2, x + y0 + 1 };
            ind.push_back(t);
        }

        int x = 0;

        for (UINT y = 0; y < GridSize - 2; y += 2)
        {
            Tri t = { x + y * GridSize, x + (y + 2) * GridSize, (x + 1) + (y + 1) * GridSize };
            ind.push_back(t);
        }
    };

    apply_bottomleft(ind);

    IndexPerm[Bottom | Left] = convert(ind);

    /*
        Left + Top
    */

    ind.clear();
    index_loop(ind);

    auto apply_lefttop = [&](std::vector<Tri>& ind) {
        UINT i = -1, j = -1;
        std::set<UINT> list;

        for (UINT z = 1; z < GridSize * 2 - 1; z += 4)
        {
            list.insert(z);
            list.insert(z + 1);
        }

        ind.erase(std::remove_if(ind.begin(), ind.end(), [&](const Tri& t) {
            ++j;

            if (t[3] == 0)
            {
                ++i;

                if (list.find(i) != list.end())
                    return true;
            }

            return j % 2 == 0 && t[4] == 0;
        }), ind.end());

        for (UINT x = 0; x < GridSize - 2; x += 2)
        {
            Tri t = { x, x + GridSize + 1, x + 2 };
            ind.push_back(t);
        }

        int x = 0;

        for (UINT y = 0; y < GridSize - 2; y += 2)
        {
            Tri t = { x + y * GridSize, x + (y + 2) * GridSize, (x + 1) + (y + 1) * GridSize };
            ind.push_back(t);
        }
    };

    apply_lefttop(ind);

    IndexPerm[Left | Top] = convert(ind);
}