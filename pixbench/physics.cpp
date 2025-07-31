#include "pixbench/ecs.h"
#include "pixbench/engine_config.h"
#include "pixbench/vector2.h"
#include <algorithm>
#include <cstddef>
#include "pixbench/physics.h"


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
    const Edge left_edge = ref_edges[(ref_edge_index+1) % 4];
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
    const Edge right_edge = ref_edges[(ref_edge_index-1) % 4];
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
    return false;
}

bool boxToPolygonCollision(BoxCollider* box, PolygonCollider* polygon, CollisionManifold* manifold__out, bool* is_body_1_the_ref) {
    return false;
}


bool circleToCircleCollision(CircleCollider* circle_1, CircleCollider* circle_2, CollisionManifold* manifold__out, bool* is_body_1_the_ref) {
    return false;
}

bool circleToPolygonCollision(CircleCollider* circle, PolygonCollider* polygon, CollisionManifold* manifold__out, bool* is_body_1_the_ref) {
    return false;
}


Vector2 __b1_verts_buff[MAX_POLYGON_VERTEX];
Vector2 __b2_verts_buff[MAX_POLYGON_VERTEX];
Edge __b1_edges_buff[MAX_POLYGON_VERTEX];
Edge __b2_edges_buff[MAX_POLYGON_VERTEX];
bool polygonToPolygonCollision(PolygonCollider* polygon_1, PolygonCollider* polygon_2, CollisionManifold* manifold__out, bool* is_body_1_the_ref) {
    // rotated vertex and edges
    size_t b1_verts_count = polygon_1->polygon.vertex_counts;
    size_t b2_verts_count = polygon_2->polygon.vertex_counts;
    for (int i=0; i<b1_verts_count; ++i) {
        __b1_verts_buff[i] = polygon_1->polygon.getVertex(
                i,
                polygon_1->__transform.GlobalPosition(),
                polygon_1->__transform.rotation
                );
        __b1_edges_buff[i] = polygon_1->polygon.getEdge(
                i,
                polygon_1->__transform.GlobalPosition(),
                polygon_1->__transform.rotation
                );
    }

    for (int i=0; i<b2_verts_count; ++i) {
        __b2_verts_buff[i] = polygon_2->polygon.getVertex(
                i,
                polygon_2->__transform.GlobalPosition(),
                polygon_2->__transform.rotation
                );
        __b2_edges_buff[i] = polygon_2->polygon.getEdge(
                i,
                polygon_2->__transform.GlobalPosition(),
                polygon_2->__transform.rotation
                );
    }

    // separating axis checks
    Vector2* least_vertex_verts;
    Edge* least_vertex_edges;
    Vector2* most_vertex_verts;
    Edge* most_vertex_edges;
    Vector2 opposing_body_centroid;

    size_t least_vertex_count;
    size_t most_vertex_count;
    if (b1_verts_count < b2_verts_count) {
        least_vertex_verts = __b1_verts_buff;
        least_vertex_edges = __b1_edges_buff;
        most_vertex_verts = __b2_verts_buff;
        most_vertex_edges = __b2_edges_buff;

        least_vertex_count = b1_verts_count;
        most_vertex_count = b2_verts_count;

        opposing_body_centroid = polygon_2->polygon.getCentroid(
                polygon_2->__transform.GlobalPosition(),
                polygon_2->__transform.rotation
                );
    }
    else {
        least_vertex_verts = __b2_verts_buff;
        least_vertex_edges = __b2_edges_buff;
        most_vertex_verts = __b1_verts_buff;
        most_vertex_edges = __b1_edges_buff;

        least_vertex_count = b2_verts_count;
        most_vertex_count = b1_verts_count;

        opposing_body_centroid = polygon_1->polygon.getCentroid(
                polygon_1->__transform.GlobalPosition(),
                polygon_1->__transform.rotation
                );
    }

    int penetration_counts = 0;
    int min_penetration_edge_index;
    double min_penetration_depth;

    for (int i=0; i<least_vertex_count; ++i) {
        const Edge edge = least_vertex_edges[i];

        // check edge direction, ignore edge pointing away from b2
        if ( Vector2::dotProduct(opposing_body_centroid - edge.p1, edge.normal) < 0.0 ) {
            continue;
        }

        float br_min;
        float br_max;
        float bo_min;
        float bo_max;

        // project to edge
        for (int j=0; j<least_vertex_count; ++j) {
            const float projected_ref = projectPointToLineCoordinate(
                    least_vertex_verts[j],
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

        for (int j=0; j<most_vertex_count; ++j) {
            const float projected_op = projectPointToLineCoordinate(
                    most_vertex_verts[j],
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

        if (i == 0 || (penetration_depth < min_penetration_depth)) {
            min_penetration_depth = penetration_depth;
            min_penetration_edge_index = i;
        }
        penetration_counts++;
    }

    if (penetration_counts == 0) {
        return false;
    }

    // opposing edge determination, the most opposed to min_penetration_edge's normal
    size_t min_projected_index = 0;
    double min_projected_normal;
    for (int i=0; i<most_vertex_count; ++i) {
        const double projected_op_normal = projectPointToLineCoordinate(
                most_vertex_edges[i].normal,
                Vector2::ZERO,
                least_vertex_edges[min_penetration_edge_index].normal
                );
        if (i != 0 && projected_op_normal >= min_projected_normal)
            continue;

        min_projected_index = i;
        min_projected_normal = projected_op_normal;
    }

    // reference edge determination, by which produce minimum penetration depth
    const double edge_l_penetration_depth = projectedPenetration(
            least_vertex_verts, least_vertex_count,
            most_vertex_verts, most_vertex_count,
            least_vertex_edges[min_penetration_edge_index]
            );
    const double edge_m_penetration_depth = projectedPenetration(
            least_vertex_verts, least_vertex_count,
            most_vertex_verts, most_vertex_count,
            most_vertex_edges[min_projected_index]
            );

    double ref_penetration_depth;
    Edge* ref_edge, inc_edge;
    Edge* ref_edges;
    int ref_edge_index, ref_edge_count;
    if (edge_l_penetration_depth < edge_m_penetration_depth) {
        ref_edges = least_vertex_edges;
        ref_penetration_depth = edge_l_penetration_depth;
        ref_edge_index = min_penetration_edge_index;
        ref_edge_count = least_vertex_count;
        ref_edge = &ref_edges[ref_edge_index];
        inc_edge = most_vertex_edges[min_projected_index];
    }
    else {
        ref_edges = most_vertex_edges;
        ref_penetration_depth = edge_m_penetration_depth;
        ref_edge_index = min_projected_index;
        ref_edge_count = most_vertex_count;
        ref_edge = &ref_edges[ref_edge_index];
        inc_edge = least_vertex_edges[min_penetration_edge_index];
    }

    // incident edge clipping
    Vector2 inc_points[2] = { inc_edge.p1, inc_edge.p2 };

    // left clip
    const Edge left_ref_edge = ref_edges[(ref_edge_index+1)%ref_edge_count];
    for (int i=0; i<2; ++i) {
        if ( Vector2::dotProduct(inc_points[i] - left_ref_edge.p1, left_ref_edge.normal) < 0.0 )
            continue;

        // clip
        Vector2 clipped_point = intersectionBetween2Line(
                inc_points[0],
                inc_points[1],
                left_ref_edge.p1,
                left_ref_edge.p2
                );
        inc_points[i] = clipped_point;
    }

    // right clip
    const Edge right_ref_edge = ref_edges[(ref_edge_index+1)%ref_edge_count];
    for (int i=0; i<2; ++i) {
        if ( Vector2::dotProduct(inc_points[i] - right_ref_edge.p1, right_ref_edge.normal) < 0.0 )
            continue;

        // clip
        Vector2 clipped_point = intersectionBetween2Line(
                inc_points[0],
                inc_points[1],
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


