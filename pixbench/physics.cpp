#include "pixbench/ecs.h"
#include "pixbench/engine_config.h"
#include "pixbench/vector2.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include "pixbench/physics.h"


int imod(int a, int b) {
    return (a % b + b) % b;
}


Vector2 projectPointToLine(Vector2 point, Vector2 line_p1, Vector2 line_p2) {
    // special case when m = inf
    const double a_denum = (line_p2.x - line_p1.x);
    if (a_denum == 0.0) {
        return Vector2(line_p1.x, point.y);
    }
    // find A, B, C of the line
    const double a = (line_p2.y - line_p1.y)/a_denum;
    const double b = -1;
    const double c = (line_p1.y*line_p2.x - line_p2.y*line_p1.x)/(line_p2.x - line_p1.x);

    // calculate xp and yp
    const double denum = a*a + b*b;
    const double xp = (b*b*point.x - a*b*point.y - c*a)/denum;
    const double yp = (a*a*point.y - a*b*point.x - c*b)/denum;

    return Vector2(xp, yp);
}


double projectPointToLineCoordinate(
        Vector2 point,
        Vector2 line_origin,
        Vector2 line_direction_normalized
        ) {
    const Vector2 orig_2_point = point - line_origin;
    return Vector2::dotProduct(orig_2_point, line_direction_normalized);
}

double projectedPenetration(
        Vector2 verts_1[], int vert_1_counts,
        Vector2 verts_2[], int vert_2_counts,
        Edge edge) {
    double b1_min = 0.0;
    double b1_max = 0.0;
    double b2_min = 0.0;
    double b2_max = 0.0;

    // project to edge
    double combined_max = 0.0;
    double combined_min = 0.0;

    for (int i=0; i<vert_1_counts; ++i) {
        const double projected_b1 = projectPointToLineCoordinate(
                verts_1[i],
                edge.p1,
                edge.normal
                );

        if (projected_b1 < b1_min)
            b1_min = projected_b1;
        if (projected_b1 > b1_max)
            b1_max = projected_b1;

        if (projected_b1 < combined_min)
            combined_min = projected_b1;
        if (projected_b1 > combined_max)
            combined_max = projected_b1;
    }

    for (int i=0; i<vert_2_counts; ++i) {
        const double projected_b2 = projectPointToLineCoordinate(
                verts_2[i],
                edge.p1,
                edge.normal
                );

        if (projected_b2 < b2_min)
            b2_min = projected_b2;
        if (projected_b2 > b2_max)
            b2_max = projected_b2;

        if (projected_b2 < combined_min)
            combined_min = projected_b2;
        if (projected_b2 > combined_max)
            combined_max = projected_b2;
    }

    const double b1_projected_length = b1_max - b1_min;
    const double b2_projected_length = b2_max - b2_min;
    const double combined_length = combined_max - combined_min;
    const double penetration_depth = (b1_projected_length + b2_projected_length)
        - combined_length
        ;

    return penetration_depth;
}


/*
 * Given 4 points which is part of 2 linear lines,
 * calculate the intersection point between those two lines.
 * Reference: https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection
 */
Vector2 intersectionBetween2Line(
        Vector2 line1_p1,
        Vector2 line1_p2,
        Vector2 line2_p1,
        Vector2 line2_p2
        ) {
    const float denum = (
            (line1_p1.x-line1_p2.x)*(line2_p1.y-line2_p2.y) -
            (line1_p1.y-line1_p2.y)*(line2_p1.x-line2_p2.x)
            );
    const float left_num = (
            line1_p1.x*line1_p2.y - line1_p1.y*line1_p2.x
            );
    const float right_num = (
            line2_p1.x*line2_p2.y - line2_p1.y*line2_p2.x
            );

    const float ix = (
            left_num*(line2_p1.x - line2_p2.x) - (line1_p1.x - line1_p2.x)*right_num
            ) / denum;
    const float iy = (
            left_num*(line2_p1.y - line2_p2.y) - (line1_p1.y - line1_p2.y)*right_num
            ) / denum;

    return Vector2(ix, iy);
}


bool boxToBoxCollision(BoxCollider* box_1, BoxCollider* box_2, CollisionManifold* manifold__out, bool* is_body_1_the_ref) {
    // rotated vertex and edges
    Vector2 b1_verts[4];
    Vector2 b2_verts[4];
    Edge b1_edges[4];
    Edge b2_edges[4];
    for (int i=0; i<4; ++i) {
        b1_verts[i] = box_1->__polygon.getVertex(i, box_1->__transform.GlobalPosition(), box_1->__transform.rotation);
        b1_edges[i] = box_1->__polygon.getEdge(i, box_1->__transform.GlobalPosition(), box_1->__transform.rotation);

        b2_verts[i] = box_2->__polygon.getVertex(i, box_2->__transform.GlobalPosition(), box_2->__transform.rotation);
        b2_edges[i] = box_2->__polygon.getEdge(i, box_2->__transform.GlobalPosition(), box_2->__transform.rotation);
    }

    double penetration_edge_indexes[4];
    double penetration_depths[4];
    int penetration_counts = 0;

    // edge loop
    for (int i=0; i<4; ++i) {
        const Edge edge = b1_edges[i];

        // check edge direction, ignore edge pointing away from b2
        if ( Vector2::dotProduct(box_2->__transform.GlobalPosition() - edge.p1, edge.normal) < 0.0 ) {
            continue;
        }

        float b1_min = 0.0;
        float b1_max = 0.0;
        float b2_min = 0.0;
        float b2_max = 0.0;

        // project to edge
        for (int j=0; j<4; ++j) {
            const float projected_b1 = projectPointToLineCoordinate(
                    b1_verts[j],
                    edge.p1,
                    edge.normal
                    );
            const float projected_b2 = projectPointToLineCoordinate(
                    b2_verts[j],
                    edge.p1,
                    edge.normal
                    );
            if (j == 0) {
                b1_min = projected_b1;
                b1_max = projected_b1;
                b2_min = projected_b2;
                b2_max = projected_b2;
            }

            b1_min = std::min(projected_b1, b1_min);
            b1_max = std::max(projected_b1, b1_max);

            b2_min = std::min(projected_b2, b2_min);
            b2_max = std::max(projected_b2, b2_max);
        }

        // collission depth
        const float b1_projected_length = b1_max - b1_min;
        const float b2_projected_length = b2_max - b2_min;
        const float combined_length = std::max(b1_max, b2_max) - std::min(b1_min, b2_min);
        const float penetration_depth = (b1_projected_length + b2_projected_length)
            - combined_length
            ;

        if (penetration_depth < 0.0) {
            return false;
        }

        penetration_edge_indexes[penetration_counts] = i;
        penetration_depths[penetration_counts] = penetration_depth;
        penetration_counts++;
    }

    if (penetration_counts == 0) {
        return false;
    }

    // minimum depth edge
    double min_depth = penetration_depths[0];
    int min_b1_edge_index = penetration_edge_indexes[0];
    for (int i=1; i<penetration_counts; ++i) {
        if ( penetration_depths[i] >= min_depth )
            continue;

        min_depth = penetration_depths[i];
        min_b1_edge_index = penetration_edge_indexes[i];
    }
    const Edge b1_collision_edge = b1_edges[min_b1_edge_index];

    // other edge calculation
    int min_b2_edge_index = 0;
    double min_b2_edge_projected_normal = 1.0;
    for (int i=0; i<4; ++i) {
        double projected_b2_normal = projectPointToLineCoordinate(
                    b2_edges[i].normal,
                    Vector2::ZERO,
                    b1_collision_edge.normal
                    );
        if (projected_b2_normal >= min_b2_edge_projected_normal)
            continue;

        min_b2_edge_projected_normal = projected_b2_normal;
        min_b2_edge_index = i;
    }
    const Edge b2_collision_edge = b2_edges[min_b2_edge_index];

    // reference edge determination, by which produce minimum penetration depth
    const double edge_1_penetration_depth = projectedPenetration(b1_verts, 4, b2_verts, 4, b1_collision_edge);
    const double edge_2_penetration_depth = projectedPenetration(b1_verts, 4, b2_verts, 4, b2_collision_edge);

    *is_body_1_the_ref = false;
    double ref_penetration_depth = edge_2_penetration_depth;
    Edge ref_edge = b2_collision_edge;
    Edge inc_edge = b1_collision_edge;
    Edge* ref_edges = b2_edges;
    int ref_edge_index = min_b2_edge_index;
    if ( edge_1_penetration_depth < edge_2_penetration_depth ) {
        *is_body_1_the_ref = true;
        ref_penetration_depth = edge_1_penetration_depth;
        ref_edge = b1_collision_edge;
        inc_edge = b2_collision_edge;
        ref_edges = b1_edges;
        ref_edge_index = min_b1_edge_index;
    }

    // incident edge clipping, obtaining manifold

    // left clip
    const Edge left_edge = ref_edges[imod((ref_edge_index+1), 4)];
    Vector2 inc_points[2] = { inc_edge.p1, inc_edge.p2 };
    for (int i=0; i<2; ++i) {
        if ( Vector2::dotProduct(inc_points[i] - left_edge.p1, left_edge.normal) < 0.0 )
            continue;

        // clip
        Vector2 clipped_point = intersectionBetween2Line(
                inc_points[0],
                inc_points[1],
                left_edge.p1,
                left_edge.p2
                );
        inc_points[i] = clipped_point;
    }

    // right clip
    const Edge right_edge = ref_edges[imod((ref_edge_index-1), 4)];
    for (int i=0; i<2; ++i) {
        if ( Vector2::dotProduct(inc_points[i] - right_edge.p1, right_edge.normal) < 0.0 )
            continue;

        // clip
        Vector2 clipped_point = intersectionBetween2Line(
                inc_points[0],
                inc_points[1],
                right_edge.p1,
                right_edge.p2
                );
        inc_points[i] = clipped_point;
    }

    // forward exclusion
    Vector2 manifold_points[2];
    int manifold_point_counts = 0;
    for (int i=0; i<2; ++i) {
        if ( Vector2::dotProduct(inc_points[i] - ref_edge.p1, ref_edge.normal) > 0.0 ) {
            continue;
        }

        manifold_points[manifold_point_counts] = inc_points[i];
        ++manifold_point_counts;
    }

    // result packing
    manifold__out->normal = ref_edge.normal.normalized();
    manifold__out->penetration_depth = ref_penetration_depth;
    manifold__out->setPoints(manifold_points, manifold_point_counts);

    return true;
}

bool boxToCircleCollision(BoxCollider* box, CircleCollider* circle, CollisionManifold* manifold__out, bool* is_body_1_the_ref) {
    PolygonCollider poly_1;
    poly_1.setPolygon(box->__polygon);
    poly_1.__transform = box->__transform;
    const bool is_colliding = circleToPolygonCollision(
            circle,
            &poly_1,
            manifold__out,
            is_body_1_the_ref
            );
    *is_body_1_the_ref = !(*is_body_1_the_ref);
    return is_colliding;
}

bool boxToPolygonCollision(BoxCollider* box, PolygonCollider* polygon, CollisionManifold* manifold__out, bool* is_body_1_the_ref) {
    PolygonCollider poly_1;
    poly_1.setPolygon(box->__polygon);
    poly_1.__transform = box->__transform;
    const bool is_colliding = polygonToPolygonCollision(
            &poly_1,
            polygon,
            manifold__out,
            is_body_1_the_ref
            );
    return is_colliding;
}


bool circleToCircleCollision(CircleCollider* circle_1, CircleCollider* circle_2, CollisionManifold* manifold__out, bool* is_body_1_the_ref) {
    return false;
}

double distancePointToInfiniteLine(Vector2 point, Vector2 line_p1, Vector2 line_p2) {
    return (projectPointToLine(point, line_p1, line_p2) - point).magnitude();
}

double distancePointToTerminatedLine(Vector2 point, Vector2 line_p1, Vector2 line_p2, Vector2* out_minimum_point = nullptr) {
    const float min_x = std::min(line_p1.x , line_p2.x);
    const float max_x = std::max(line_p1.x , line_p2.x);
    const float min_y = std::min(line_p1.y , line_p2.y);
    const float max_y = std::max(line_p1.y , line_p2.y);

    Vector2 proj_point = projectPointToLine(point, line_p1, line_p2);
    if ( proj_point.x < min_x || proj_point.x > max_x || proj_point.y < min_y || proj_point.y > max_y ) { // if outside of bound-box
        const double dist_to_p1 = (line_p1 - point).magnitude();
        const double dist_to_p2 = (line_p2 - point).magnitude();

        double min_dist = dist_to_p2;
        Vector2 min_point = line_p2;
        if (dist_to_p1 < dist_to_p2) {
            min_dist = dist_to_p1;
            min_point = line_p1;
        }

        if ( out_minimum_point ) {
            *out_minimum_point = min_point;
        }
        return min_dist;
    }

    if ( out_minimum_point ) {
        *out_minimum_point = proj_point;
    }
    return (proj_point - point).magnitude();
}

bool circleToPolygonCollision(CircleCollider* circle, PolygonCollider* polygon, CollisionManifold* manifold__out, bool* is_body_1_the_ref) {

    // collision happens when distance between circle's centroid and any of polygon's edges is less than or equal to circle's radius.
    const Vector2 circ_pos = circle->__transform.GlobalPosition();
    const Vector2 poly_pos = polygon->__transform.GlobalPosition();
    const double poly_rot = polygon->__transform.rotation;


    double max_penetration_depth;
    Vector2 max_penetration_point;
    size_t penetration_count = 0;
    for (size_t i=0; i<(polygon->__polygon.vertex_counts); ++i) {
        const Edge edge = polygon->__polygon.getEdge(
                i, poly_pos, poly_rot
                );

        Vector2 out__corresponding_point = Vector2(100.0, 100.0);
        const double d = distancePointToTerminatedLine(
                circ_pos,
                edge.p1, edge.p2,
                &out__corresponding_point
                );

        if ( d > circle->radius )
            continue;

        const double penetration_depth = circle->radius - d;
        if ( penetration_count == 0 || (penetration_depth > max_penetration_depth) ) {
            max_penetration_depth = penetration_depth;
            max_penetration_point = out__corresponding_point;
            penetration_count++;
        }
    }

    if ( penetration_count == 0 )
        return false;

    // collision edge
    const Vector2 normal = (max_penetration_point - circ_pos).normalized();
    manifold__out->point_count = 1;
    manifold__out->points[0] = max_penetration_point;
    manifold__out->penetration_depth = max_penetration_depth;
    manifold__out->normal = normal;

    *is_body_1_the_ref = true;

    return true;
}


Vector2 __b1_verts_buff[MAX_POLYGON_VERTEX];
Vector2 __b2_verts_buff[MAX_POLYGON_VERTEX];
Edge __b1_edges_buff[MAX_POLYGON_VERTEX];
Edge __b2_edges_buff[MAX_POLYGON_VERTEX];
bool polygonToPolygonCollision(PolygonCollider* polygon_1, PolygonCollider* polygon_2, CollisionManifold* manifold__out, bool* is_body_1_the_ref) {
    // rotated vertex and edges
    size_t b1_verts_count = polygon_1->__polygon.vertex_counts;
    size_t b2_verts_count = polygon_2->__polygon.vertex_counts;
    for (int i=0; i<b1_verts_count; ++i) {
        __b1_verts_buff[i] = polygon_1->__polygon.getVertex(
                i,
                polygon_1->__transform.GlobalPosition(),
                polygon_1->__transform.rotation
                );
        __b1_edges_buff[i] = polygon_1->__polygon.getEdge(
                i,
                polygon_1->__transform.GlobalPosition(),
                polygon_1->__transform.rotation
                );
    }

    for (int i=0; i<b2_verts_count; ++i) {
        __b2_verts_buff[i] = polygon_2->__polygon.getVertex(
                i,
                polygon_2->__transform.GlobalPosition(),
                polygon_2->__transform.rotation
                );
        __b2_edges_buff[i] = polygon_2->__polygon.getEdge(
                i,
                polygon_2->__transform.GlobalPosition(),
                polygon_2->__transform.rotation
                );
    }

    enum BODY {
        BODY_1,
        BODY_2
    };

    // separating axis checks
    Vector2* ref_vertex_verts;
    Edge* ref_vertex_edges;
    Vector2* opp_vertex_verts;
    Edge* opp_vertex_edges;
    Vector2 opposing_body_centroid;

    size_t ref_vertex_count;
    size_t opp_vertex_count;
    ref_vertex_verts = __b1_verts_buff;
    ref_vertex_edges = __b1_edges_buff;
    opp_vertex_verts = __b2_verts_buff;
    opp_vertex_edges = __b2_edges_buff;

    ref_vertex_count = b1_verts_count;
    opp_vertex_count = b2_verts_count;

    opposing_body_centroid = polygon_2->__polygon.getCentroid(
            polygon_2->__transform.GlobalPosition(),
            polygon_2->__transform.rotation
            );

    int penetration_counts = 0;
    int min_penetration_edge_index = 0;
    double min_penetration_depth;
    BODY min_penetration_body = BODY::BODY_1;

    for (int i=0; i<ref_vertex_count; ++i) { // body 1 as ref
        const Edge edge = ref_vertex_edges[i];

        // check edge direction, ignore edge pointing away from b2
        if ( Vector2::dotProduct(opposing_body_centroid - edge.p1, edge.normal) < 0.0 ) {
            continue;
        }

        float br_min;
        float br_max;
        float bo_min;
        float bo_max;

        // project to edge
        for (int j=0; j<ref_vertex_count; ++j) {
            const float projected_ref = projectPointToLineCoordinate(
                    ref_vertex_verts[j],
                    edge.p1,
                    edge.normal
                    );
            if (j == 0) {
                br_min = projected_ref;
                br_max = projected_ref;
            }
            br_min = std::min(projected_ref, br_min);
            br_max = std::max(projected_ref, br_max);
        }

        for (int j=0; j<opp_vertex_count; ++j) {
            const float projected_op = projectPointToLineCoordinate(
                    opp_vertex_verts[j],
                    edge.p1,
                    edge.normal
                    );
            if (j == 0) {
                bo_min = projected_op;
                bo_max = projected_op;
            }
            bo_min = std::min(projected_op, bo_min);
            bo_max = std::max(projected_op, bo_max);
        }

        // collission depth
        const float br_projected_length = br_max - br_min;
        const float bo_projected_length = bo_max - bo_min;
        const float combined_length = std::max(br_max, bo_max) - std::min(br_min, bo_min);
        const float penetration_depth = (br_projected_length + bo_projected_length)
            - combined_length
            ;

        if (penetration_depth < 0.0) {
            return false;
        }

        if (penetration_counts == 0 || (penetration_depth < min_penetration_depth)) {
            min_penetration_depth = penetration_depth;
            min_penetration_edge_index = i;
            min_penetration_body = BODY::BODY_1;
        }
        penetration_counts++;
    }

    ref_vertex_verts = __b2_verts_buff;
    ref_vertex_edges = __b2_edges_buff;
    opp_vertex_verts = __b1_verts_buff;
    opp_vertex_edges = __b1_edges_buff;

    ref_vertex_count = b2_verts_count;
    opp_vertex_count = b1_verts_count;

    opposing_body_centroid = polygon_1->__polygon.getCentroid(
            polygon_1->__transform.GlobalPosition(),
            polygon_1->__transform.rotation
            );

     for (int i=0; i<ref_vertex_count; ++i) { // body 2 as ref
        const Edge edge = ref_vertex_edges[i];

        // check edge direction, ignore edge pointing away from b2
        if ( Vector2::dotProduct(opposing_body_centroid - edge.p1, edge.normal) < 0.0 ) {
            continue;
        }

        float br_min;
        float br_max;
        float bo_min;
        float bo_max;

        // project to edge
        for (int j=0; j<ref_vertex_count; ++j) {
            const float projected_ref = projectPointToLineCoordinate(
                    ref_vertex_verts[j],
                    edge.p1,
                    edge.normal
                    );
            if (j == 0) {
                br_min = projected_ref;
                br_max = projected_ref;
            }
            br_min = std::min(projected_ref, br_min);
            br_max = std::max(projected_ref, br_max);
        }

        for (int j=0; j<opp_vertex_count; ++j) {
            const float projected_op = projectPointToLineCoordinate(
                    opp_vertex_verts[j],
                    edge.p1,
                    edge.normal
                    );
            if (j == 0) {
                bo_min = projected_op;
                bo_max = projected_op;
            }
            bo_min = std::min(projected_op, bo_min);
            bo_max = std::max(projected_op, bo_max);
        }

        // collission depth
        const float br_projected_length = br_max - br_min;
        const float bo_projected_length = bo_max - bo_min;
        const float combined_length = std::max(br_max, bo_max) - std::min(br_min, bo_min);
        const float penetration_depth = (br_projected_length + bo_projected_length)
            - combined_length
            ;

        if (penetration_depth < 0.0) {
            return false;
        }

        if (penetration_counts == 0 || (penetration_depth < min_penetration_depth)) {
            min_penetration_depth = penetration_depth;
            min_penetration_edge_index = i;
            min_penetration_body = BODY::BODY_2;
        }
        penetration_counts++;
    }

    if (penetration_counts == 0) {
        return false;
    }

    // reframe based on min penetration body 
    if (min_penetration_body == BODY::BODY_1) {
        ref_vertex_verts = __b1_verts_buff;
        ref_vertex_edges = __b1_edges_buff;
        opp_vertex_verts = __b2_verts_buff;
        opp_vertex_edges = __b2_edges_buff;

        ref_vertex_count = b1_verts_count;
        opp_vertex_count = b2_verts_count;

        opposing_body_centroid = polygon_2->__polygon.getCentroid(
                polygon_2->__transform.GlobalPosition(),
                polygon_2->__transform.rotation
                );
    }

    // opposing edge determination, the most opposed to min_penetration_edge's normal
    size_t min_projected_index = 0;
    double min_projected_normal;
    for (int i=0; i<opp_vertex_count; ++i) {
        const double projected_op_normal = projectPointToLineCoordinate(
                opp_vertex_edges[i].normal,
                Vector2::ZERO,
                ref_vertex_edges[min_penetration_edge_index].normal
                );
        if (i != 0 && projected_op_normal >= min_projected_normal)
            continue;

        min_projected_index = i;
        min_projected_normal = projected_op_normal;
    }

    // reference edge determination, by which produce minimum penetration depth
    const double ref_edge_penetration_depth = projectedPenetration(
            ref_vertex_verts, ref_vertex_count,
            opp_vertex_verts, opp_vertex_count,
            ref_vertex_edges[min_penetration_edge_index]
            );
    const double opp_edge_penetration_depth = projectedPenetration(
            ref_vertex_verts, ref_vertex_count,
            opp_vertex_verts, opp_vertex_count,
            opp_vertex_edges[min_projected_index]
            );

    double ref_penetration_depth;
    Edge* ref_edge, inc_edge;
    Edge* ref_edges;
    int ref_edge_index, ref_edge_count;
    if (ref_edge_penetration_depth < opp_edge_penetration_depth) {
        ref_edges = ref_vertex_edges;
        ref_penetration_depth = ref_edge_penetration_depth;
        ref_edge_index = min_penetration_edge_index;
        ref_edge_count = ref_vertex_count;
        ref_edge = &ref_edges[ref_edge_index];
        inc_edge = opp_vertex_edges[min_projected_index];
    }
    else {
        ref_edges = opp_vertex_edges;
        ref_penetration_depth = opp_edge_penetration_depth;
        ref_edge_index = min_projected_index;
        ref_edge_count = opp_vertex_count;
        ref_edge = &ref_edges[ref_edge_index];
        inc_edge = ref_vertex_edges[min_penetration_edge_index];
    }

    // incident edge clipping
    Vector2 inc_points[2] = { inc_edge.p1, inc_edge.p2 };

    // left clip
    const Edge left_ref_edge = ref_edges[imod((ref_edge_index+1), ref_edge_count)];
    for (int i=0; i<2; ++i) {
        if ( Vector2::dotProduct(inc_points[i] - left_ref_edge.p1, left_ref_edge.normal) < 0.0 )
            continue;

        // clip
        Vector2 clipped_point = intersectionBetween2Line(
                inc_edge.p1,
                inc_edge.p2,
                left_ref_edge.p1,
                left_ref_edge.p2
                );
        inc_points[i] = clipped_point;
    }

    // right clip
    const Edge right_ref_edge = ref_edges[imod((ref_edge_index-1), ref_edge_count)];
    for (int i=0; i<2; ++i) {
        if ( Vector2::dotProduct(inc_points[i] - right_ref_edge.p1, right_ref_edge.normal) < 0.0 )
            continue;

        // clip
        Vector2 clipped_point = intersectionBetween2Line(
                inc_edge.p1,
                inc_edge.p2,
                right_ref_edge.p1,
                right_ref_edge.p2
                );
        inc_points[i] = clipped_point;
    }

    // forward exclusion
    Vector2 manifold_points[2];
    int manifold_point_counts = 0;
    for (int i=0; i<2; ++i) {
        if ( Vector2::dotProduct(inc_points[i] - ref_edge->p1, ref_edge->normal) > 0.0 ) {
            continue;
        }

        manifold_points[manifold_point_counts] = inc_points[i];
        ++manifold_point_counts;
    }

    // result packing
    manifold__out->normal = ref_edge->normal.normalized();
    manifold__out->penetration_depth = ref_penetration_depth;
    manifold__out->setPoints(manifold_points, manifold_point_counts);

    return true;
}


// === Physics Debug Drawing ===


void phydebDrawCross(
        SDL_Renderer* renderer,
        Vector2* center,
        float cross_width
        ) {
    SDL_RenderLine(
            renderer,
            center->x - cross_width/2, center->y - cross_width/2,
            center->x + cross_width/2, center->y + cross_width/2
            );
    SDL_RenderLine(
            renderer,
            center->x + cross_width/2, center->y - cross_width/2,
            center->x - cross_width/2, center->y + cross_width/2
            );
}


