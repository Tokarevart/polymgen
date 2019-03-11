#include "spatial-objs/shell/shell-facet.h"
#include "algorithm"
#include "helpers/spatial-algs/spatial-algs.h"

#include "helpers/cosd-values.h"

using namespace pmg;
using pair_dd = std::pair<double, double>;
using pair_ff = std::pair<pmg::Facet*, pmg::Facet*>;
using pair_ee = std::pair<pmg::Edge*, pmg::Edge*>;
using tva::Vec;
using tva::Point;


#define DEG_1_IN_RAD 0.0174532925199432957


#define NINE_DIV_SIXTEEN 0.5625
#define SIXTEEN_DIV_NINE 1.7777777777777777
#define SQRT3_2          0.8660254037844386

#define NOT_TOO_CLOSE  1e-1
#define FROM_VERT_COEF 1e-2
#define EDGES_INTERS_DIST_COEF 1e-4


template <typename T>
constexpr double degToRad(T value)
{
    return value * DEG_1_IN_RAD;
}


using FrPlEdge   = front::plane::Edge;
using FrPlVertex = front::plane::Vertex;




double shell::Facet::preferredLength() const
{
    return m_preferredLength;
}




const std::list<pmg::Facet*>& shell::Facet::innerFacets() const
{
    return m_innerFacets;
}


const std::list<pmg::Edge*>& shell::Facet::innerEdges() const
{
    return m_innerEdges;
}


const std::vector<Vertex*>& shell::Facet::innerVerts() const
{
    return m_innerVerts;
}


const std::list<FrPlEdge*>& shell::Facet::frontEdges() const
{
    return m_frontEdges;
}


shell::Vertex* shell::Facet::findVertNot(const shell::Edge* edge) const
{
    for (auto& facet_edge : edges)
    {
        if (edge != facet_edge)
        {
            if (!edge->contains(facet_edge->verts[0]))
                return facet_edge->verts[0];
            else
                return facet_edge->verts[1];
        }
    }

    return nullptr;
}


shell::Edge* shell::Facet::findShellEdgeContaining(const pmg::Edge* edge) const
{
    for (auto& sedge : edges)
        if (sedge->contains(edge))
            return sedge;

    return nullptr;
}




void shell::Facet::triangulate(double preferredLen)
{
    m_preferredLength = preferredLen;
    initializeFront();
    computeFrontNormals();
    processAngles();
//    if (globalIntersectionCheck())
//        throw std::logic_error("Intersection error.\nCrystallite3::globalIntersectionCheck returned true.");

    smoothMesh(20);
    for (int i = 0; i < 3; i++)
    {
        delaunayPostproc();
        smoothMesh(20);
    }
}




bool shell::Facet::contains(const shell::Edge* edge) const
{
    for (auto& edge_ : edges)
        if (edge_ == edge)
            return true;

    return false;
}


bool shell::Facet::contains(const shell::Vertex* vert) const
{
    for (auto& edge : edges)
        if (edge->contains(vert))
            return true;

    return false;
}




shell::Facet::Facet(
    const shell::Edge* edge0,
    const shell::Edge* edge1,
    const shell::Edge* edge2)
{
    edges[0] = const_cast<shell::Edge*>(edge0);
    edges[1] = const_cast<shell::Edge*>(edge1);
    edges[2] = const_cast<shell::Edge*>(edge2);
}




FrPlVertex* shell::Facet::findFrontVert(const pmg::Vertex* vert) const
{
    for (auto& fvert : m_frontVerts)
        if (fvert->vert == vert)
            return fvert;

    return nullptr;
}




FrPlEdge* shell::Facet::addToFront(const pmg::Edge* edge)
{
    FrPlEdge* new_f_edge = new FrPlEdge(this, edge);
    m_frontEdges.push_back(new_f_edge);
    m_innerEdges.push_back(new_f_edge->edge);
    return new_f_edge;
}


FrPlVertex* shell::Facet::addToFront(const pmg::Vertex* vert)
{
    FrPlVertex* new_f_vert = new FrPlVertex(this, vert);
    m_frontVerts.push_back(new_f_vert);
    m_innerVerts.push_back(new_f_vert->vert);
    return new_f_vert;
}




void shell::Facet::removeFromFront(FrPlEdge* fEdge)
{
    m_frontEdges.erase(std::find(m_frontEdges.begin(), m_frontEdges.end(), fEdge));
    delete fEdge;
}


void shell::Facet::removeFromFront(FrPlVertex* fVert)
{
    m_frontVerts.erase(std::find(m_frontVerts.begin(), m_frontVerts.end(), fVert));
    delete fVert;
}




bool shell::Facet::anyVertInsidePotentialTriangCheck(FrPlVertex* fVert) const
{
    auto opp_verts = fVert->findOppVerts();
    pmg::Vertex* tr[3]
    {
        fVert->vert,
        opp_verts.first,
        opp_verts.second
    };

    for (auto& fvert : m_frontVerts)
        if (fvert->vert != tr[0] && fvert->vert != tr[1] && fvert->vert != tr[2] &&
            tva::spatalgs::isPointOnTriangle(fvert->vert->pos(), tr[0]->pos(), tr[1]->pos(), tr[2]->pos()))
            return true;

    return false;
}


bool shell::Facet::doesSegmentIntersectsWithFront(const Point& p0, const Point& p1) const
{
    for (auto& fedge : m_frontEdges)
        if (tva::spatalgs::segmentsDistance(
                fedge->edge->verts[0]->pos(), fedge->edge->verts[1]->pos(),
                p0, p1) < EDGES_INTERS_DIST_COEF * m_preferredLength)
            return true;

    return false;
}




Vec shell::Facet::computeNormalInTriang(FrPlEdge* fEdge, const Point& oppVertPos)
{
    Point p0 = fEdge->edge->verts[0]->pos();
    Point p1 = fEdge->edge->verts[1]->pos();
    return (tva::spatalgs::project(oppVertPos, p0, p1) - oppVertPos).normalize();
}




bool shell::Facet::tryComputeNewVertPosType2(FrPlEdge* fEdge, Point& out_pos)
{
    FrPlVertex* main_f_verts[2];
    main_f_verts[0] = findFrontVert(fEdge->edge->verts[0]);
    main_f_verts[1] = findFrontVert(fEdge->edge->verts[1]);

    auto adj_f_edges0 = main_f_verts[0]->findAdjEdges();
    auto adj_f_edges1 = main_f_verts[1]->findAdjEdges();
    auto en0 = std::get<0>(adj_f_edges0) == fEdge ? std::get<1>(adj_f_edges0) : std::get<0>(adj_f_edges0);
    auto en1 = std::get<0>(adj_f_edges1) == fEdge ? std::get<1>(adj_f_edges1) : std::get<0>(adj_f_edges1);

    Point vn0_pos = en0->edge->findNot(fEdge->edge)->pos();
    Point vn1_pos = en1->edge->findNot(fEdge->edge)->pos();

    Vec v0_pos = main_f_verts[0]->vert->pos();
    Vec v1_pos = main_f_verts[1]->vert->pos();

    Vec e0 = (vn0_pos - 2.0 * v0_pos + main_f_verts[1]->vert->pos()).normalize();
    Vec e1 = (vn1_pos - 2.0 * v1_pos + v0_pos).normalize();

    Vec new_pos = tva::spatalgs::linesClosestPoint(v0_pos, v0_pos + e0, v1_pos, v1_pos + e1);

    Vec v0_to_np = new_pos - v0_pos;
    Vec v1_to_np = new_pos - v1_pos;
    if (doesSegmentIntersectsWithFront(v0_pos + FROM_VERT_COEF * v0_to_np, new_pos + NOT_TOO_CLOSE * v0_to_np) ||
        doesSegmentIntersectsWithFront(v1_pos + FROM_VERT_COEF * v1_to_np, new_pos + NOT_TOO_CLOSE * v1_to_np))
        return false;

    out_pos = new_pos;
    return true;
}


bool shell::Facet::tryComputeNewVertPosType1(FrPlEdge* fEdge, Point& out_pos, int smallAngleIndex)
{
    auto main_vert = fEdge->edge->verts[smallAngleIndex];
    auto sec_vert  = fEdge->edge->findNot(main_vert);
    auto main_f_vert = findFrontVert(main_vert);

    auto adj_f_edges = main_f_vert->findAdjEdges();
    auto en = std::get<0>(adj_f_edges) == fEdge ? std::get<1>(adj_f_edges) : std::get<0>(adj_f_edges);
    auto vn = en->edge->findNot(main_vert);

    Vec e = (sec_vert->pos() - 2.0 * main_vert->pos() + vn->pos()).normalize();

    double av_magn = 0.5 * (fEdge->edge->magnitude() + en->edge->magnitude());
    Point new_pos = main_vert->pos() + av_magn * e;

    Vec v0_to_np = new_pos - main_vert->pos();
    Vec v1_to_np = new_pos - sec_vert->pos();
    if (doesSegmentIntersectsWithFront(main_vert->pos() + FROM_VERT_COEF * v0_to_np, new_pos + NOT_TOO_CLOSE * v0_to_np) ||
        doesSegmentIntersectsWithFront(sec_vert->pos()  + FROM_VERT_COEF * v1_to_np, new_pos + NOT_TOO_CLOSE * v1_to_np))
        return false;

    out_pos = new_pos;
    return true;
}


bool shell::Facet::tryComputeNewVertPosType0(FrPlEdge* fEdge, Point& out_pos)
{
    Point new_pos = fEdge->computeCenter() + fEdge->normal * m_preferredLength * SQRT3_2;

    Vec v0_pos = fEdge->edge->verts[0]->pos();
    Vec v1_pos = fEdge->edge->verts[1]->pos();
    Vec v_to_np0 = (new_pos - v0_pos);
    Vec v_to_np1 = (new_pos - v1_pos);
    if (doesSegmentIntersectsWithFront(v0_pos + v_to_np0 * FROM_VERT_COEF, new_pos + v_to_np0 * NOT_TOO_CLOSE) ||
        doesSegmentIntersectsWithFront(v1_pos + v_to_np1 * FROM_VERT_COEF, new_pos + v_to_np1 * NOT_TOO_CLOSE))
        return false;

    out_pos = new_pos;
    return true;
}


bool shell::Facet::tryComputeNewVertPos(FrPlEdge* fEdge, Point& out_pos)
{
    double angs_coses[2]
    {
        findFrontVert(fEdge->edge->verts[0])->angleExCos(),
        findFrontVert(fEdge->edge->verts[1])->angleExCos()
    };
    int indexes[2];
    int small_angs_num = 0;
    if (angs_coses[0] > cosDeg<120>) indexes[small_angs_num++] = 0;
    if (angs_coses[1] > cosDeg<120>) indexes[small_angs_num++] = 1;

    switch (small_angs_num)
    {
    case 0: return tryComputeNewVertPosType0(fEdge, out_pos);
    case 1: return tryComputeNewVertPosType1(fEdge, out_pos, indexes[0]);
    case 2: return tryComputeNewVertPosType2(fEdge, out_pos);
    }

    return false;
}




pair_dd shell::Facet::computeMinMaxEdgesLengths(const Vec& p0, const Vec& p1, const Vec& p2)
{
    auto min_max = computeMinMaxEdgesSqrLengths(p0, p1, p2);
    min_max.first = sqrt(min_max.first);
    min_max.second = sqrt(min_max.second);
    return min_max;
}


pair_dd shell::Facet::computeMinMaxEdgesSqrLengths(const Vec& p0, const Vec& p1, const Vec& p2)
{
    double sqr_magns[3];
    sqr_magns[0] = (p1 - p0).sqrMagnitude();
    sqr_magns[1] = (p2 - p0).sqrMagnitude();
    sqr_magns[2] = (p2 - p1).sqrMagnitude();
    return std::minmax({ sqr_magns[0], sqr_magns[1], sqr_magns[2] });
}


double shell::Facet::computeTriangSimpleQuality(const Vec& p0, const Vec& p1, const Vec& p2)
{
    auto sqr_min_max = computeMinMaxEdgesSqrLengths(p0, p1, p2);
    return sqrt(sqr_min_max.first / sqr_min_max.second);
}


double shell::Facet::computeTriangSimpleSqrQuality(const Vec& p0, const Vec& p1, const Vec& p2)
{
    auto sqr_min_max = computeMinMaxEdgesSqrLengths(p0, p1, p2);
    return sqr_min_max.first / sqr_min_max.second;
}




FrPlEdge* shell::Facet::chooseEdgeForExhaustionWithNewVert(FrPlVertex* fVert)
{
    auto adj_edges = fVert->findAdjEdges();
    return std::get<0>(adj_edges)->edge->sqrMagnitude() < std::get<1>(adj_edges)->edge->sqrMagnitude() ?
                std::get<0>(adj_edges) : std::get<1>(adj_edges);
}


void shell::Facet::exhaustWithNewVert(FrPlEdge* fEdge, const Point& vertPos)
{
    pmg::Vertex* main_verts[2] { fEdge->edge->verts[0], fEdge->edge->verts[1] };

    pmg::Vertex* new_vert = addToFront(new pmg::Vertex(vertPos))->vert;
    auto new_f_edge0 = addToFront(new pmg::Edge(main_verts[0], new_vert));
    auto new_f_edge1 = addToFront(new pmg::Edge(main_verts[1], new_vert));
    new_f_edge0->normal = computeNormalInTriang(new_f_edge0, fEdge->edge->findNot(new_f_edge0->edge)->pos());
    new_f_edge1->normal = computeNormalInTriang(new_f_edge1, fEdge->edge->findNot(new_f_edge1->edge)->pos());

    m_innerFacets.push_back(new pmg::Facet(fEdge->edge, new_f_edge0->edge, new_f_edge1->edge));

    findFrontVert(main_verts[0])->refreshAngleData();
    findFrontVert(main_verts[1])->refreshAngleData();

    removeFromFront(fEdge);
}


void shell::Facet::exhaustWithoutNewVert(FrPlVertex* fVert)
{
    auto adj_edges = fVert->findAdjEdges();
    std::pair<pmg::Vertex*, pmg::Vertex*> opp_verts =
    {
        adj_edges.first->edge->findNot(fVert->vert),
        adj_edges.second->edge->findNot(fVert->vert)
    };

    auto new_f_edge = addToFront(new pmg::Edge(opp_verts.first, opp_verts.second));
    new_f_edge->normal = computeNormalInTriang(new_f_edge, fVert->vert->pos());

    m_innerFacets.push_back(new pmg::Facet(adj_edges.first->edge, adj_edges.second->edge, new_f_edge->edge));

    findFrontVert(opp_verts.first)->refreshAngleData();
    findFrontVert(opp_verts.second)->refreshAngleData();

    removeFromFront(fVert);
    removeFromFront(adj_edges.first);
    removeFromFront(adj_edges.second);
}




bool shell::Facet::tryExhaustWithoutNewVert(FrPlVertex* fVert)
{
    if (anyVertInsidePotentialTriangCheck(fVert))
        return false;

    exhaustWithoutNewVert(fVert);
    return true;
}


bool shell::Facet::tryExhaustWithNewVert(FrPlVertex* fVert)
{
    auto exhaust_f_edge = chooseEdgeForExhaustionWithNewVert(fVert);
    Vec new_vert_pos;
    if (!tryComputeNewVertPos(exhaust_f_edge, new_vert_pos))
        return false;

    exhaustWithNewVert(exhaust_f_edge, new_vert_pos);
    return true;
}




FrPlVertex* shell::Facet::currentFrontVert(double maxCompl) const
{
    double cur_max_compl = 0.0;
    FrPlVertex* cur_max_f_edge = nullptr;
    for (auto& f_vert : m_frontVerts)
    {
        double cur_compl = f_vert->complexity();
        if (cur_compl > cur_max_compl &&
            cur_compl < maxCompl)
        {
            cur_max_compl  = cur_compl;
            cur_max_f_edge = f_vert;
        }
    }

    return cur_max_f_edge;
}


bool shell::Facet::exhaustWithoutNewVertPriorityPredicate(FrPlVertex* fEdge)
{
    if (fEdge->angleExCos() > 0.5)
        return true;

    if (fEdge->angleExCos() < cosDeg<80>)
        return false;

    auto adj_edges = fEdge->findAdjEdges();
    double div = adj_edges.first->edge->sqrMagnitude() / adj_edges.second->edge->sqrMagnitude();
    if (NINE_DIV_SIXTEEN < div && div < SIXTEEN_DIV_NINE)
        return true;

    return false;
}


bool shell::Facet::exhaustWithNewVertPriorityPredicate(FrPlVertex* fEdge)
{
    if (fEdge->angleExCos() > degToRad(110))
        return true;

    return false;
}


shell::Facet::ExhaustType shell::Facet::computeExhaustionTypeQualityPriority(
        FrPlVertex* fVert, FrPlEdge*& out_withNWFrontEdge, Point*& out_withNWNewVertPos)
{
    if (anyVertInsidePotentialTriangCheck(fVert))
        return ExhaustType::WithNewVert;

    auto opp_verts = fVert->findOppVerts();
    double without_nv_quality = computeTriangSimpleSqrQuality(
        fVert->vert->pos(),
        std::get<0>(opp_verts)->pos(),
        std::get<1>(opp_verts)->pos());

    FrPlEdge* f_edge = chooseEdgeForExhaustionWithNewVert(fVert);
    Vec new_vert_pos;
    if (!tryComputeNewVertPos(f_edge, new_vert_pos))
        return ExhaustType::DontExhaust;

    double with_nv_quality = computeTriangSimpleSqrQuality(
        f_edge->edge->verts[0]->pos(),
        f_edge->edge->verts[1]->pos(),
        new_vert_pos);

    if (without_nv_quality > with_nv_quality)
        return ExhaustType::WithoutNewVert;

    out_withNWFrontEdge = f_edge;
    out_withNWNewVertPos = new Vec(new_vert_pos);
    return ExhaustType::WithNewVert;
}




void shell::Facet::processLastFacet()
{
    pmg::Edge* edges[3];
    int i = 0;
    for (auto& f_edge : m_frontEdges)
        edges[i++] = f_edge->edge;

    m_innerFacets.push_back(new pmg::Facet(edges[0], edges[1], edges[2]));

    for (auto& f_edge : m_frontEdges)
        delete f_edge;
    m_frontEdges.clear();

    for (auto& f_vert : m_frontVerts)
        delete f_vert;
    m_frontVerts.clear();
}


void shell::Facet::processAngles()
{
    if (m_frontEdges.size() < 3)
        throw std::logic_error("Wrong input data.\nError in function: pmg::shell::Facet::processAngles");

    if (m_frontEdges.size() == 3)
    {
        processLastFacet();
        return;
    }

    double max_compl = std::numeric_limits<double>::max();
    for (FrPlVertex* cur_f_vert = currentFrontVert(max_compl);; cur_f_vert = currentFrontVert(max_compl))
    {
        if (!cur_f_vert)
            throw std::logic_error("pmg::shell::Facet::currentFrontVert returned nullptr");

        if (exhaustWithoutNewVertPriorityPredicate(cur_f_vert))
        {
            if (!tryExhaustWithoutNewVert(cur_f_vert))
            {
                max_compl = cur_f_vert->complexity();
                continue;
            }
        }
        else if (exhaustWithNewVertPriorityPredicate(cur_f_vert))
        {
            if (!tryExhaustWithNewVert(cur_f_vert))
            {
                max_compl = cur_f_vert->complexity();
                continue;
            }
        }
        else
        {
            FrPlEdge* exhaust_from_f_edge = nullptr;
            Point* new_vert_pos = nullptr;
            switch (computeExhaustionTypeQualityPriority(cur_f_vert, exhaust_from_f_edge, new_vert_pos))
            {
            case ExhaustType::WithoutNewVert:
                exhaustWithoutNewVert(cur_f_vert);
                break;

            case ExhaustType::WithNewVert:
                if (new_vert_pos)
                {
                    exhaustWithNewVert(exhaust_from_f_edge, *new_vert_pos);
                    delete new_vert_pos;
                }
                else
                {
                    if (!tryExhaustWithNewVert(cur_f_vert))
                    {
                        max_compl = cur_f_vert->complexity();
                        continue;
                    }
                }
                break;

            case ExhaustType::DontExhaust:
                max_compl = cur_f_vert->complexity();
                continue;
            }
        }
        max_compl = std::numeric_limits<double>::max();

        if (m_frontEdges.size() == 3)
        {
            processLastFacet();
            return;
        }
    }
}




void shell::Facet::smoothMesh(unsigned nIterations)
{
    for (unsigned i = 0; i < nIterations; i++)
    {
        for (auto& vert : m_innerVerts)
        {
            Vec shift;
            int delta_shifts_num = 0;
            for (auto& edge : m_innerEdges)
            {
                if (vert == edge->verts[0])
                {
                    shift += *edge->verts[1] - *vert;
                    delta_shifts_num++;
                }
                else if (vert == edge->verts[1])
                {
                    shift += *edge->verts[0] - *vert;
                    delta_shifts_num++;
                }
            }
            shift /= delta_shifts_num;
            vert->setPos(vert->pos() + shift);
        }
    }
}




pair_ff shell::Facet::find2AdjFacets(pmg::Edge* edge) const
{
    pair_ff res;
    bool not_found_yet = true;
    for (auto& facet : m_innerFacets)
    {
        if (facet->contains(edge))
        {
            if (not_found_yet)
            {
                res.first = facet;
                not_found_yet = false;
            }
            else
            {
                res.second = facet;
                return res;
            }
        }
    }

    throw std::logic_error("pmg::shell::Facet::find2AdjFacets didn't find 2 adjacent facets.");
}


bool shell::Facet::flipIfNeeded(pmg::Edge* edge)
{
    pmg::Vertex* opp_nodes[2];
    auto around_facets = find2AdjFacets(edge);
    opp_nodes[0] = std::get<0>(around_facets)->findVertNot(edge);
    opp_nodes[1] = std::get<1>(around_facets)->findVertNot(edge);

    double alpha = acos(Vec::cos(*edge->verts[0] - *opp_nodes[0], *edge->verts[1] - *opp_nodes[0]));
    double beta  = acos(Vec::cos(*edge->verts[0] - *opp_nodes[1], *edge->verts[1] - *opp_nodes[1]));

    if (alpha + beta <= M_PI)
        return false;


    auto old_edge   = std::find(m_innerEdges.begin(), m_innerEdges.end(), edge);
    auto old_facet0 = std::find(m_innerFacets.begin(), m_innerFacets.end(), std::get<0>(around_facets));
    auto old_facet1 = std::find(m_innerFacets.begin(), m_innerFacets.end(), std::get<1>(around_facets));

    pmg::Edge* new_edge = new pmg::Edge(opp_nodes[0], opp_nodes[1]);
    pmg::Facet* new_facet0 = new pmg::Facet(
            std::get<0>(around_facets)->findEdge(opp_nodes[0], edge->verts[0]),
            std::get<1>(around_facets)->findEdge(opp_nodes[1], edge->verts[0]),
            new_edge);
    pmg::Facet* new_facet1 = new pmg::Facet(
            std::get<0>(around_facets)->findEdge(opp_nodes[0], edge->verts[1]),
            std::get<1>(around_facets)->findEdge(opp_nodes[1], edge->verts[1]),
            new_edge);

    delete *old_edge;
    delete *old_facet0;
    delete *old_facet1;

    *old_edge = new_edge;
    *old_facet0 = new_facet0;
    *old_facet1 = new_facet1;

    return true;
}


void shell::Facet::delaunayPostproc()
{
    for (auto& edge : m_innerEdges)
        flipIfNeeded(edge);
}




void shell::Facet::computeFrontNormals() const
{
    for (auto& f_edge : m_frontEdges)
        f_edge->computeNormal();
}


void shell::Facet::initializeFront()
{
    std::vector<shell::Vertex*> sverts;
    for (auto& this_edge : edges)
        for (auto& svert : this_edge->verts)
            if (std::find(sverts.begin(), sverts.end(), svert) == sverts.end())
                sverts.push_back(svert);

    for (auto& svert : sverts)
        m_frontVerts.push_back(new FrPlVertex(this, svert->attachedVert));
    sverts.clear();

    for (auto& this_edge : edges)
        for (auto& vert : this_edge->innerVerts())
            m_frontVerts.push_back(new FrPlVertex(this, vert));

    for (auto& this_edge : edges)
        for (auto& edge : this_edge->innerEdges())
            m_frontEdges.push_back(new FrPlEdge(this, edge));
}