#include "model/Adjacencies.h"

#include <map>
#include <assert.h>

namespace
{

struct Edge
{
    Edge(unsigned short _a, unsigned short _b)
    {
        assert(_a != _b);

        if (_a < _b) {
            a = _a;
            b = _b;
        } else {
            a = _b;
            b = _a;
        }
    }

    unsigned short a;
    unsigned short b;
};

struct Neighbors
{
    unsigned short n1;
    unsigned short n2;

    Neighbors()
    {
        n1 = n2 = 0xffff;
    }

    void AddNeigbor(unsigned short n)
    {
        if (n1 == 0xffff) {
            n1 = n;
        } else if (n2 == 0xffff) {
            n2 = n;
        } else {
            assert(0);
        }
    }

    unsigned short GetOther(unsigned short me) const
    {
        if (n1 == me) {
            return n2;
        } else if (n2 == me) {
            return n1;
        } else {
            assert(0);
        }

        return 0;
    }
};

struct CompareEdges
{
    bool operator()(const Edge& Edge1, const Edge& Edge2) const
    {
        if (Edge1.a < Edge2.a) {
            return true;
        } else if (Edge1.a == Edge2.a) {
            return (Edge1.b < Edge2.b);
        } else {
            return false;
        }
    }
};

}

namespace model
{

std::vector<unsigned short> Adjacencies::
Build(const std::vector<unsigned short>& tris)
{
    std::map<Edge, Neighbors, CompareEdges> index_map;

    for (size_t i = 0, n = tris.size(); i < n; i += 3)
    {
        unsigned short tri[3];
        for (size_t j = 0; j < 3; ++j) {
            tri[j] = tris[i + j];
        }

        Edge e1(tri[0], tri[1]);
        Edge e2(tri[1], tri[2]);
        Edge e3(tri[2], tri[0]);

        index_map[e1].AddNeigbor(i);
        index_map[e2].AddNeigbor(i);
        index_map[e3].AddNeigbor(i);
    }

    std::vector<unsigned short> ret;

    for (size_t i = 0, n = tris.size(); i < n; i += 3)
    {
        unsigned short tri[3];
        for (size_t j = 0; j < 3; ++j) {
            tri[j] = tris[i + j];
        }

        for (size_t j = 0 ; j < 3 ; j++) 
        {
            Edge e(tri[j], tri[(j + 1) % 3]);
            assert(index_map.find(e) != index_map.end());
            Neighbors n = index_map[e];
            unsigned short other_tri = n.GetOther(i);
            
            assert(other_tri != 0xffff);

            // get opposite index
            unsigned short opp_idx = 0xffff;
            for (size_t k = 0; k < 3; ++k)
            {
                auto idx = tris[other_tri + k];
                if (idx != e.a && idx != e.b) {
                    opp_idx = idx;
                    break;
                }
            }
         
            assert(opp_idx != 0xffff);
            ret.push_back(tri[j]);
            ret.push_back(opp_idx);
        }
    } 

    return ret;
}

}