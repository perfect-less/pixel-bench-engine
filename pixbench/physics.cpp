#include "pixbench/ecs.h"
#include "pixbench/engine_config.h"
#include "pixbench/renderer.h"
#include "pixbench/vector2.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <regex>
#include "pixbench/physics.h"


enum BODY {
    BODY_1,
    BODY_2
};


int imod(int a, int b) {
    return (a % b + b) % b;
}


void getAABoxOfCollider(
        Collider* coll,
        Vector2* out__min_point,
        Vector2* out__max_point
        ) {
    const Vector2 pos = coll->__transform.GlobalPosition();
    *out__min_point = Vector2(
            pos.x - coll->__bounding_radius, pos.y - coll->__bounding_radius
            );
    *out__max_point = Vector2(
            pos.x + coll->__bounding_radius, pos.y + coll->__bounding_radius
            );
}


bool axisAlignedBoundingSquareCheck(
        const Vector2* b1_pos, float b1_radius,
        const Vector2* b2_pos, float b2_radius
        ) {
    const float combined_dist_x = std::abs(b2_pos->x - b1_pos->x);
    const float combined_dist_y = std::abs(b2_pos->y - b1_pos->y);
    const float total_radius = b1_radius + b2_radius;
    return (combined_dist_x <= total_radius) && (combined_dist_y <= total_radius);
}


Vector2 projectPointToLine(Vector2 point, Vector2 line_p1, Vector2 line_p2) {
    // special case when m = inf or m = 0
    const double a_denum = (line_p2.x - line_p1.x);
    if (a_denum == 0.0) {
        return Vector2(line_p1.x, point.y);
    }
    if (line_p1.y == line_p2.y) {
        return Vector2(point.x, line_p1.y);
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
        Vector2 line2_p2,
        bool* out__is_intersecting = nullptr
        ) {
    const float denum = (
            (line1_p1.x-line1_p2.x)*(line2_p1.y-line2_p2.y) -
            (line1_p1.y-line1_p2.y)*(line2_p1.x-line2_p2.x)
            );

    if ( denum == 0 ) {
        if (out__is_intersecting) {
            *out__is_intersecting = false;
        }
        return Vector2::ZERO;
    }

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

    if (out__is_intersecting) {
        *out__is_intersecting = true;
    }

    return Vector2(ix, iy);
}

double distancePointToInfiniteLine(const Vector2 point, const Vector2 line_p1, const Vector2 line_p2) {
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


bool isPointBetweenArbitraryBBoxCorner(
        const Vector2 point,
        const Vector2 c1,
        const Vector2 c2
        ) {

    const float min_x = std::min(c1.x, c2.x);
    const float max_x = std::max(c1.x, c2.x);
    const float min_y = std::min(c1.y, c2.y);
    const float max_y = std::max(c1.y, c2.y);

    const bool is_outside_of_x = (
            point.x < min_x || point.x > max_x
            );
    const bool is_outside_of_y = (
            point.y < min_y || point.y > max_y
            );
    return !(is_outside_of_x || is_outside_of_y);
}


// ========== Physics API ==========

bool PhysicsAPI::rayCast(Vector2 origin, Vector2 direction, float length, RaycastHit* out__raycast_hit) {

    // TMP: Do 1 long line checks
    // TODO: Break down into line segments checks.

    if ( !this->m_game || !this->m_game->physicsSystem || !this->m_game->entityManager )
        return false;

    direction = direction.normalized();
    const float ray_length = length;

#ifdef RAYCAST_SEGMENT_LENGTH
    const size_t segment_count = std::ceil(ray_length/RAYCAST_SEGMENT_LENGTH);
    for (size_t _s=0; _s<segment_count; ++_s) {

    const float segment_length = std::min(ray_length - (float)(_s*RAYCAST_SEGMENT_LENGTH), (float)RAYCAST_SEGMENT_LENGTH);
    length = segment_length;

    const Vector2 ray_origin = origin + _s * RAYCAST_SEGMENT_LENGTH * direction;
    const Vector2 ray_destination = ray_origin +
        segment_length * direction
        ;
#else
    const Vector2 ray_origin = origin;
    const Vector2 ray_destination = origin + direction*length;
#endif

    auto physics = std::static_pointer_cast<PhysicsSystem>(this->m_game->physicsSystem);
    std::bitset<MAX_COMPONENTS> component_mask = physics->__getPhysicsComponentMask();

    size_t hit_count = 0;
    float min_distance;
    Vector2 hit_point;
    Vector2 hit_normal;
    EntityID hit_ent;

    for (size_t cindex=0; cindex<MAX_COMPONENTS; ++cindex) {
        if (!(component_mask[cindex]))
            continue; // skip non-collider components

        std::bitset<MAX_COMPONENTS> mask;
        mask.reset();
        mask.set(cindex);
        for (auto ent_id : EntityView(m_game->entityManager, mask, false)) {
            Collider* collider = m_game->entityManager->getEntityComponentCasted<Collider>(
                    ent_id, cindex);
            if ( !collider )
                continue;

            // aabb precheck
            Vector2 out__min_point;
            Vector2 out__max_point;
            getAABoxOfCollider(collider, &out__min_point, &out__max_point);
            const float ray_min_x = std::min(ray_origin.x, ray_destination.x) - RAYCAST_AABBOX_MARGIN;
            const float ray_min_y = std::min(ray_origin.y, ray_destination.y) - RAYCAST_AABBOX_MARGIN;
            const float ray_max_x = std::max(ray_origin.x, ray_destination.x) + RAYCAST_AABBOX_MARGIN;
            const float ray_max_y = std::max(ray_origin.y, ray_destination.y) + RAYCAST_AABBOX_MARGIN;
            const float combined_length_x = std::max(ray_max_x, out__max_point.x) - std::min(ray_min_x, out__min_point.x);
            const float combined_length_y = std::max(ray_max_y, out__max_point.y) - std::min(ray_min_y, out__min_point.y);
            const float additive_length_x = (ray_max_x - ray_min_x) + (out__max_point.x - out__min_point.x);
            const float additive_length_y = (ray_max_y - ray_min_y) + (out__max_point.y - out__min_point.y);

            if (combined_length_x > additive_length_x || combined_length_y > additive_length_y) {
                continue;
            }

            const ColliderTag coltag = collider->getColliderTag();

            switch (coltag) {
                case COLTAG_Polygon:
                    {
                        PolygonCollider* poly_coll = static_cast<PolygonCollider*>(collider);
                        const Vector2 coll_pos = poly_coll->__transform.GlobalPosition();
                        const double coll_rot = poly_coll->__transform.rotation;
                        for (size_t i=0; i<poly_coll->__polygon.vertex_counts; ++i) {
                            const Edge edge = poly_coll->__polygon.getEdge(i, coll_pos, coll_rot);
                            if (Vector2::dotProduct(direction, edge.normal) >= 0.0)
                                continue;
                            const Vector2 intersect_point = intersectionBetween2Line(
                                    ray_origin, ray_destination,
                                    edge.p1, edge.p2
                                    );
                            const double projected_in_line = projectPointToLineCoordinate(
                                    intersect_point, ray_origin, direction
                                    );
                            const double projected_in_edge = projectPointToLineCoordinate(
                                    intersect_point, edge.p1, (edge.p2 - edge.p1).normalized()
                                    );

                            const bool is_within_both_lines =
                                projected_in_line > 0.0 && projected_in_line < length
                                &&
                                projected_in_edge > 0.0 && projected_in_edge < (edge.p2 - edge.p1).magnitude()
                                ;

                            if (is_within_both_lines
                                && (
                                    hit_count == 0 || projected_in_line < min_distance
                                )) {
                                min_distance = projected_in_line;
                                hit_point = intersect_point;
                                hit_normal = edge.normal;
                                hit_ent = poly_coll->entity();
                                ++hit_count;
                            }
                        }
                        break;
                    }
                case COLTAG_Box:
                    {
                        BoxCollider* box_coll = static_cast<BoxCollider*>(collider);
                        Vector2 coll_pos = box_coll->__transform.GlobalPosition();
                        double coll_rot = box_coll->__transform.rotation;
                        for (int i=0; i<4; ++i) {
                            const Edge edge = box_coll->__polygon.getEdge(i, coll_pos, coll_rot);
                            if (Vector2::dotProduct(direction, edge.normal) >= 0.0)
                                continue;
                            const Vector2 intersect_point = intersectionBetween2Line(
                                    ray_origin, ray_destination,
                                    edge.p1, edge.p2
                                    );
                            const float projected_in_line = projectPointToLineCoordinate(
                                    intersect_point, ray_origin, direction
                                    );
                            const float projected_in_edge = projectPointToLineCoordinate(
                                    intersect_point, edge.p1, (edge.p2 - edge.p1).normalized()
                                    );

                            const bool is_within_both_lines =
                                projected_in_line > 0.0 && projected_in_line < length
                                &&
                                projected_in_edge > 0.0 && projected_in_edge < (edge.p2 - edge.p1).magnitude()
                                ;

                            if (is_within_both_lines &&
                                (
                                    hit_count == 0 || projected_in_line < min_distance
                                )) {
                                min_distance = projected_in_line;
                                hit_point = intersect_point;
                                hit_normal = edge.normal;
                                hit_ent = box_coll->entity();
                                ++hit_count;
                            }
                        }
                        break;
                    }
                case COLTAG_Circle:
                    {
                        CircleCollider* circ_coll = static_cast<CircleCollider*>(collider);
                        const Vector2 coll_pos = circ_coll->__transform.GlobalPosition();
                        // const double coll_rot = circ_coll->__transform.rotation;
                        const double dist_to_ray = distancePointToInfiniteLine(coll_pos, ray_origin, ray_destination);
                        if ( dist_to_ray > circ_coll->radius )
                            break;

                        const Vector2 center_point = projectPointToLine(coll_pos, ray_origin, ray_destination);

                        if ( Vector2::dotProduct(center_point - ray_origin, direction) < 0.0 )
                            break;

                        const float k = std::sqrt(circ_coll->radius*circ_coll->radius - dist_to_ray*dist_to_ray);

                        const Vector2 _hit_point = center_point - k*direction;
                        const double hit_distance = (_hit_point - ray_origin).magnitude();

                        if ( hit_distance <= length && ( hit_count == 0 || hit_distance < min_distance) ) {
                            min_distance = hit_distance;
                            hit_point = _hit_point;
                            hit_normal = (_hit_point - coll_pos).normalized();
                            hit_ent = circ_coll->entity();
                            ++hit_count;
                        }

                        break;
                    }
                case COLTAG_Capsule:
                    {
                        CapsuleCollider* caps_coll = static_cast<CapsuleCollider*>(collider);
                        const Vector2 caps_pos = caps_coll->__transform.GlobalPosition();
                        const double caps_rot = caps_coll->__transform.rotation;

                        const Vector2 caps_p1 = caps_pos + Vector2::UP.rotated(caps_rot) * (caps_coll->length / 2.0);
                        const Vector2 caps_p2 = caps_pos + Vector2::DOWN.rotated(caps_rot) * (caps_coll->length / 2.0);
                        const Vector2 caps_norm = (caps_p2 - caps_p1).rotated(M_PI / 2.0).normalized();

                        // #1 check ray_line against forwarded caps_line
                        const Vector2 forward_dir = (Vector2::dotProduct(caps_norm, direction) < 0.0)
                            ? caps_norm : -1.0 * caps_norm
                            ;
                        const Vector2 forward_offset = caps_coll->radius * forward_dir;
                        const Vector2 fwd_p1 = caps_p1 + forward_offset;
                        const Vector2 fwd_p2 = caps_p2 + forward_offset;

                        bool is_intersecting;
                        const Vector2 intersect_point = intersectionBetween2Line(fwd_p1, fwd_p2, ray_origin, ray_destination, &is_intersecting);
                        const double intersect_in_line = projectPointToLineCoordinate(intersect_point, fwd_p1, (fwd_p2 - fwd_p1).normalized());
                        const bool is_within_caps_line = intersect_in_line >= 0.0 && intersect_in_line <= caps_coll->length;
                        const bool is_intersect_infront_of_ray = Vector2::dotProduct((intersect_point - ray_origin), direction) > 0.0;
                        if ( is_intersecting && is_intersect_infront_of_ray && is_within_caps_line ) {
                            const double hit_distance = (intersect_point - ray_origin).magnitude();
                            if ( hit_distance <= length && ( hit_count == 0 || hit_distance < min_distance) ) {
                                min_distance = hit_distance;
                                hit_point = intersect_point;
                                hit_normal = forward_dir;
                                hit_ent = caps_coll->entity();
                                ++hit_count;
                                break;
                            }
                        }

                        // #2 checks agains each capsules circle
                        const Vector2 caps_points[2] = {caps_p1, caps_p2};
                        for (size_t i=0; i<2; ++i) {
                            const Vector2 caps_point = caps_points[i];
                            const double dist_to_ray = distancePointToInfiniteLine(caps_point, ray_origin, ray_destination);

                            if ( dist_to_ray > caps_coll->radius )
                                continue;

                            const Vector2 center_point = projectPointToLine(caps_point, ray_origin, ray_destination);

                            if ( Vector2::dotProduct(center_point - ray_origin, direction) < 0.0 )
                                continue;

                            const float k = std::sqrt(caps_coll->radius*caps_coll->radius - dist_to_ray*dist_to_ray);

                            const Vector2 _hit_point = center_point - k*direction;
                            const double hit_distance = (_hit_point - ray_origin).magnitude();
                            if ( hit_distance <= length && ( hit_count == 0 || hit_distance < min_distance) ) {
                                min_distance = hit_distance;
                                hit_point = _hit_point;
                                hit_normal = (_hit_point - caps_point).normalized();
                                hit_ent = caps_coll->entity();
                                ++hit_count;
                            }
                        }

                        break;
                    }
            }
        }
    }

#ifndef RAYCAST_SEGMENT_LENGTH
    if ( hit_count == 0 )
        return false;
#endif

    // packing hit result
    if ( hit_count > 0 && out__raycast_hit ) {
        out__raycast_hit->normal = hit_normal;
        out__raycast_hit->point  = hit_point;
        out__raycast_hit->ent    = hit_ent;
    }

    if ( hit_count > 0 )
        return true;

#ifdef RAYCAST_SEGMENT_LENGTH
    }
#endif
    return false;
}


bool circleCastCheckAgainsEdge(
        const Vector2 ray_origin, const Vector2 ray_direction, const double ray_length, const double ray_radius,
        const Edge edge,
        Vector2* out__hit_point,
        Vector2* out__hit_normal,
        Vector2* out__hit_centroid
        ) {
    const Vector2 ray_destination = ray_origin + ray_length * ray_direction;

    if (Vector2::dotProduct(ray_direction, edge.normal) > 0.0)
        return false;

    const double edge_length = (edge.p2 - edge.p1).magnitude();
    const Vector2 edge_lengthwise_dir = (edge.p2 - edge.p1).normalized();
    const Vector2 ray_offset = -ray_radius * edge.normal;
    const Vector2 transposed_ray_origin = ray_origin + ray_offset;
    const Vector2 transposed_ray_destination = ray_destination + ray_offset;
    const Vector2 line_hit_point = intersectionBetween2Line(
            transposed_ray_origin, transposed_ray_destination,
            edge.p1, edge.p2
            );
    const double projected_line_hit_point = projectPointToLineCoordinate(
            line_hit_point, edge.p1, edge_lengthwise_dir
            );
    Vector2 centroid = line_hit_point + ray_radius*edge.normal;
    double hit_distance = ( line_hit_point - transposed_ray_origin ).magnitude();
    if ( // check if line_hit_point is still within the edge
            Vector2::dotProduct(centroid - ray_origin, ray_direction) > 0.0
            && projected_line_hit_point > 0.0 && projected_line_hit_point < edge_length
            && hit_distance > 0.0
       ) {
        *out__hit_point = line_hit_point;
        *out__hit_normal = edge.normal;
        *out__hit_centroid = centroid;
        return true;
    }
    // otherwise check the edge endpoints
    size_t viable_edge_point_counts = 0;
    Vector2 points[2] = { edge.p1, edge.p2 };
    double point_dists[2];
    for (size_t e=0; e<2; ++e) {
        const double d = distancePointToInfiniteLine(
                points[e], ray_origin, ray_destination
                );
        if ( d > ray_radius )
            continue;

        const Vector2 colliding_point = points[e];
        points[viable_edge_point_counts] = colliding_point;
        point_dists[viable_edge_point_counts] = d;
        viable_edge_point_counts++;
    }

    if ( viable_edge_point_counts == 0 ) {
        return false;
    }

    Vector2 min_point = points[0];
    double d = point_dists[0];
    if ( viable_edge_point_counts == 2 ) {
        const double p1_projected = projectPointToLineCoordinate(points[0], ray_origin, ray_direction);
        const double p2_projected = projectPointToLineCoordinate(points[1], ray_origin, ray_direction);
        if ( p2_projected < p1_projected ) {
            min_point = points[1];
            d = point_dists[1];
        }
    }

    const double k = std::sqrt(ray_radius*ray_radius - d*d);
    centroid = projectPointToLine(min_point, ray_origin, ray_destination)
        - k*ray_direction
        ;

    hit_distance = (centroid - ray_origin).magnitude();

    if (Vector2::dotProduct(centroid - ray_origin, ray_direction) > 0.0) {
        *out__hit_normal = (centroid - min_point).normalized();
        *out__hit_point = min_point;
        *out__hit_centroid = centroid;
        return true;
    }
    return false;
}


bool PhysicsAPI::circleCast(Vector2 origin, Vector2 direction, float length, float radius, RaycastHit* out__raycast_hit) {

    // TMP: Do 1 long line checks
    // TODO: Break down into line segments checks.

    if ( !this->m_game || !this->m_game->physicsSystem || !this->m_game->entityManager )
        return false;

    direction = direction.normalized();
    const Vector2 ray_origin = origin;
    const Vector2 ray_destination = origin + direction*length;

    auto physics = std::static_pointer_cast<PhysicsSystem>(this->m_game->physicsSystem);
    std::bitset<MAX_COMPONENTS> component_mask = physics->__getPhysicsComponentMask();

    size_t hit_count = 0;
    float min_distance;
    Vector2 hit_point;
    Vector2 hit_normal;
    EntityID hit_ent;

    for (size_t cindex=0; cindex<MAX_COMPONENTS; ++cindex) {
        if (!(component_mask[cindex]))
            continue; // skip non-collider components

        std::bitset<MAX_COMPONENTS> mask;
        mask.reset();
        mask.set(cindex);
        for (auto ent_id : EntityView(m_game->entityManager, mask, false)) {
            Collider* collider = m_game->entityManager->getEntityComponentCasted<Collider>(
                    ent_id, cindex);
            if ( !collider )
                continue;

            // aabb precheck
            const float bbox_margin = ( radius > RAYCAST_AABBOX_MARGIN ) ? radius : RAYCAST_AABBOX_MARGIN;
            Vector2 out__min_point;
            Vector2 out__max_point;
            getAABoxOfCollider(collider, &out__min_point, &out__max_point);
            const float ray_min_x = std::min(ray_origin.x, ray_destination.x) - bbox_margin;
            const float ray_min_y = std::min(ray_origin.y, ray_destination.y) - bbox_margin;
            const float ray_max_x = std::max(ray_origin.x, ray_destination.x) + bbox_margin;
            const float ray_max_y = std::max(ray_origin.y, ray_destination.y) + bbox_margin;
            const float combined_length_x = std::max(ray_max_x, out__max_point.x) - std::min(ray_min_x, out__min_point.x);
            const float combined_length_y = std::max(ray_max_y, out__max_point.y) - std::min(ray_min_y, out__min_point.y);
            const float additive_length_x = (ray_max_x - ray_min_x) + (out__max_point.x - out__min_point.x);
            const float additive_length_y = (ray_max_y - ray_min_y) + (out__max_point.y - out__min_point.y);

            if (combined_length_x > additive_length_x || combined_length_y > additive_length_y) {
                continue;
            }

            const ColliderTag coltag = collider->getColliderTag();

            switch (coltag) {
                case COLTAG_Polygon:
                    {
                        PolygonCollider* poly_coll = static_cast<PolygonCollider*>(collider);
                        const Vector2 coll_pos = poly_coll->__transform.GlobalPosition();
                        const double coll_rot = poly_coll->__transform.rotation;

                        for (size_t i=0; i<poly_coll->__polygon.vertex_counts; ++i) {
                            const Edge edge = poly_coll->__polygon.getEdge(
                                    i, coll_pos, coll_rot);

                            Vector2 out__hit_point;
                            Vector2 out__hit_normal;
                            Vector2 out__hit_centroid;
                            if ( !circleCastCheckAgainsEdge(
                                        ray_origin, direction, length, radius,
                                        edge,
                                        &out__hit_point, &out__hit_normal, &out__hit_centroid
                                        )
                               ) {
                                continue;
                            }

                            const double hit_distance = (out__hit_centroid - ray_origin).magnitude();
                            if ( hit_count == 0 || hit_distance < min_distance ) {
                                min_distance = hit_distance;
                                hit_point = out__hit_point;
                                hit_normal = out__hit_normal;
                                hit_ent = ent_id;
                                ++hit_count;
                            }
                        }
                    }
                    break;
                case COLTAG_Box:
                    {
                        BoxCollider* box_coll = static_cast<BoxCollider*>(collider);
                        const Vector2 coll_pos = box_coll->__transform.GlobalPosition();
                        const double coll_rot = box_coll->__transform.rotation;

                        for (size_t i=0; i<4; ++i) {
                            const Edge edge = box_coll->__polygon.getEdge(
                                    i, coll_pos, coll_rot);

                            Vector2 out__hit_point;
                            Vector2 out__hit_normal;
                            Vector2 out__hit_centroid;
                            if ( !circleCastCheckAgainsEdge(
                                        ray_origin, direction, length, radius,
                                        edge,
                                        &out__hit_point, &out__hit_normal, &out__hit_centroid
                                        )
                               ) {
                                continue;
                            }

                            const double hit_distance = (out__hit_centroid - ray_origin).magnitude();
                            if ( hit_count == 0 || hit_distance < min_distance ) {
                                min_distance = hit_distance;
                                hit_point = out__hit_point;
                                hit_normal = out__hit_normal;
                                hit_ent = ent_id;
                                ++hit_count;
                            }
                        }
                    }
                    break;
                case COLTAG_Circle:
                    {
                        CircleCollider* circ_coll = static_cast<CircleCollider*>(collider);
                        const Vector2 coll_pos = circ_coll->__transform.GlobalPosition();
                        // const double coll_rot = circ_coll->__transform.rotation;
                        const double d = distancePointToInfiniteLine(
                                coll_pos, ray_origin, ray_destination
                                );
                        if ( d > (radius + circ_coll->radius) )
                            continue;

                        const Vector2 projected_circle_center = projectPointToLine(
                                coll_pos, ray_origin, ray_destination
                                );
                        const double combined_radius = radius + circ_coll->radius;
                        const double k = std::sqrt(combined_radius*combined_radius - d*d);
                        const Vector2 centroid_point = projected_circle_center - k*direction;
                        if (Vector2::dotProduct(centroid_point - ray_origin, direction) < 0.0) {
                            break;
                        }
                        const double hit_distance = (centroid_point - ray_origin).magnitude();

                        if ( hit_distance > 0.0 && hit_distance <= length && (hit_count == 0 || hit_distance < min_distance) ) {
                            min_distance = hit_distance;
                            hit_normal = (centroid_point - coll_pos).normalized();
                            hit_point = coll_pos + hit_normal*circ_coll->radius;
                            hit_ent = ent_id;
                            ++hit_count;
                        }
                    }
                    break;
                case COLTAG_Capsule:
                    {
                        CapsuleCollider* caps_coll = static_cast<CapsuleCollider*>(collider);
                        const Vector2 caps_pos = caps_coll->__transform.GlobalPosition();
                        const double caps_rot = caps_coll->__transform.rotation;

                        const Vector2 caps_p1 = caps_pos + Vector2::UP.rotated(caps_rot) * (caps_coll->length / 2.0);
                        const Vector2 caps_p2 = caps_pos + Vector2::DOWN.rotated(caps_rot) * (caps_coll->length / 2.0);
                        const Vector2 caps_norm = (caps_p2 - caps_p1).rotated(M_PI / 2.0).normalized();

                        // #1 check ray_line against forwarded caps_line
                        const Vector2 forward_dir = (Vector2::dotProduct(caps_norm, direction) < 0.0)
                            ? caps_norm : -1.0 * caps_norm
                            ;
                        const Vector2 forward_offset = caps_coll->radius * forward_dir;
                        const Vector2 fwd_p1 = caps_p1 + forward_offset;
                        const Vector2 fwd_p2 = caps_p2 + forward_offset;

                        const Edge edge = {fwd_p1, fwd_p2, forward_dir};

                        Vector2 out__hit_point;
                        Vector2 out__hit_normal;
                        Vector2 out__hit_centroid;
                        if ( circleCastCheckAgainsEdge(
                                    ray_origin, direction, length, radius,
                                    edge,
                                    &out__hit_point, &out__hit_normal, &out__hit_centroid
                                    )
                           ) {
                            const double hit_distance = (out__hit_centroid - ray_origin).magnitude();
                            if ( hit_distance <= length && ( hit_count == 0 || hit_distance < min_distance) ) {
                                min_distance = hit_distance;
                                hit_point = out__hit_point;
                                hit_normal = out__hit_normal;
                                hit_ent = caps_coll->entity();
                                ++hit_count;
                                // break;
                            }
                        }

                        // #2 checks agains each capsules circle
                        const Vector2 caps_points[2] = {caps_p1, caps_p2};
                        for (size_t i=0; i<2; ++i) {
                            const Vector2 caps_point = caps_points[i];
                            const double d = distancePointToInfiniteLine(
                                    caps_point, ray_origin, ray_destination
                                    );
                            if ( d > (radius + caps_coll->radius) )
                                continue;

                            const Vector2 projected_circle_center = projectPointToLine(
                                    caps_point, ray_origin, ray_destination
                                    );
                            const double combined_radius = radius + caps_coll->radius;
                            const double k = std::sqrt(combined_radius*combined_radius - d*d);
                            const Vector2 centroid_point = projected_circle_center - k*direction;
                            if (Vector2::dotProduct(centroid_point - ray_origin, direction) < 0.0) {
                                break;
                            }
                            const double hit_distance = (centroid_point - ray_origin).magnitude();

                            if ( hit_distance > 0.0 && hit_distance <= length && (hit_count == 0 || hit_distance < min_distance) ) {
                                min_distance = hit_distance;
                                hit_normal = (centroid_point - caps_point).normalized();
                                hit_point = caps_point + hit_normal*caps_coll->radius;
                                hit_ent = ent_id;
                                ++hit_count;
                            }
                        }

                    }
                    break;
            }
        }
    }

    if ( hit_count == 0 )
        return false;

    if ( out__raycast_hit ) {
        out__raycast_hit->normal = hit_normal;
        out__raycast_hit->point  = hit_point;
        out__raycast_hit->ent    = hit_ent;
    }

    return true;
}


// ========== Collision ==========


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

    BODY min_penetration_body = BODY_1;
    double min_penetration_depth;
    int min_penetration_edge_index = 0;
    int penetration_counts = 0;

    // edge loop
    for (int _i=0; _i<4*2; ++_i) {
        int i = _i;
        Edge edge;
        Vector2 opp_pos = box_2->__transform.GlobalPosition();
        if (_i >= 4) {
            i = _i % 4;
            edge = b2_edges[i];
            opp_pos = box_1->__transform.GlobalPosition();
        } else {
            edge = b1_edges[i];
        }

        // check edge direction, ignore edge pointing away from b2
        if ( Vector2::dotProduct(opp_pos - edge.p1, edge.normal) < 0.0 ) {
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

        if (penetration_counts == 0 || (penetration_depth < min_penetration_depth)) {
            min_penetration_depth = penetration_depth;
            min_penetration_edge_index = i;
            ++penetration_counts;
            if (_i >= 4)
                min_penetration_body = BODY_2;
        }
    }

    if (penetration_counts == 0) {
        return false;
    }

    BODY ref_body = min_penetration_body;
    int ref_edge_index = min_penetration_edge_index;
    double ref_penetration_depth = min_penetration_depth;
    Edge ref_edge = (ref_body == BODY_1) ? b1_edges[min_penetration_edge_index] : b2_edges[min_penetration_edge_index];
    Edge* ref_edges = (ref_body == BODY_1) ? b1_edges : b2_edges;
    Edge* inc_edges = (ref_body == BODY_1) ? b2_edges : b1_edges;

    // other edge calculation
    int min_opp_edge_index = 0;
    double min_opp_edge_projected_normal = 1.0;
    for (int i=0; i<4; ++i) {
        double projected_opp_normal = projectPointToLineCoordinate(
                    inc_edges[i].normal,
                    Vector2::ZERO,
                    ref_edge.normal
                    );
        if (projected_opp_normal >= min_opp_edge_projected_normal)
            continue;

        min_opp_edge_projected_normal = projected_opp_normal;
        min_opp_edge_index = i;
    }
    const Edge inc_edge = inc_edges[min_opp_edge_index];

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

    *is_body_1_the_ref = (ref_body == BODY_1) ? true : false;

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
        *is_body_1_the_ref = true;
    } else {
        *is_body_1_the_ref = false;
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
        *is_body_1_the_ref = !(*is_body_1_the_ref);
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


bool isProjectedPointLiesWithinLine(
        const Vector2 point,
        const Vector2 p1, const Vector2 p2,
        Vector2* out__projected_point=nullptr
        ) {
    const Vector2 proj_point = projectPointToLine(
            point, p1, p2
            );
    if (out__projected_point) {
        *out__projected_point = proj_point;
    }

    return isPointBetweenArbitraryBBoxCorner(proj_point, p1, p2);
}


bool polygonToCapsuleCollision(PolygonCollider* polygon, CapsuleCollider* capsule, CollisionManifold* manifold__out, bool* is_body_1_the_ref) {
    const Vector2 caps_pos = capsule->__transform.GlobalPosition();
    const double caps_rot = capsule->__transform.rotation;

    const Vector2 caps_p1 = caps_pos + Vector2::UP.rotated(caps_rot) * (capsule->length / 2.0);
    const Vector2 caps_p2 = caps_pos + Vector2::DOWN.rotated(caps_rot) * (capsule->length / 2.0);

    const Vector2 poly_pos = polygon->__transform.GlobalPosition();
    const double poly_rot = polygon->__transform.rotation;
    for (size_t i=0; i<polygon->__polygon.vertex_counts; ++i) {
        __b1_edges_buff[i] = polygon->__polygon.getEdge(i, poly_pos, poly_rot);
        __b1_verts_buff[i] = polygon->__polygon.getVertex(i, poly_pos, poly_rot);
    }

    // manifold hunting | minimum penetration
    size_t penetration_count = 0;
    size_t contact_counts = 0;
    Vector2 contact_points[2];
    Vector2 contact_normal;
    double max_penetration;
    for (size_t i=0; i<polygon->__polygon.vertex_counts; ++i) {
        double manifold_candidate_depths[8];
        Vector2 manifold_candidate_points[8];
        Vector2 manifold_candidate_normal[8];
        size_t manifold_candidate_point_count = 0;
        const Edge edge = __b1_edges_buff[i];

        // project points to other's line
        const Vector2 points[4] = {caps_p1, caps_p2, edge.p1, edge.p2};
        Vector2 p1, p2;
        for (size_t j=0; j<4; ++j) {
            if (j < 2) {
                p1 = edge.p1;
                p2 = edge.p2;
            }
            else
            {
                p1 = caps_p1;
                p2 = caps_p2;
            }

            Vector2 projected_point;
            if ( !isProjectedPointLiesWithinLine(points[j], p1, p2, &projected_point) )
                continue;
            double _penetration_depth = capsule->radius - (projected_point - points[j]).magnitude();
            if ( !(_penetration_depth > 0.0) )
                continue;

            if (j < 2) {
                manifold_candidate_points[manifold_candidate_point_count] = projected_point;
                manifold_candidate_normal[manifold_candidate_point_count] = (projected_point - points[j]).normalized();
                manifold_candidate_depths[manifold_candidate_point_count] = _penetration_depth;
            }
            else {
                manifold_candidate_points[manifold_candidate_point_count] = points[j];
                manifold_candidate_normal[manifold_candidate_point_count] = (points[j] - projected_point).normalized();
                manifold_candidate_depths[manifold_candidate_point_count] = _penetration_depth;
            }

            ++manifold_candidate_point_count;
        }

        if (manifold_candidate_point_count == 0) {
            // checks end points against each other
            // each caps agains e1 and e2
            Vector2 ref_points[2] = {caps_p1, caps_p2};
            for (size_t _i=0; _i<2; ++_i) {
                const double pen_depth_e1 = capsule->radius - (ref_points[_i] - edge.p1).magnitude();
                const double pen_depth_e2 = capsule->radius - (ref_points[_i] - edge.p2).magnitude();

                if (pen_depth_e1 > 0.0) {
                    manifold_candidate_points[manifold_candidate_point_count] = edge.p1;
                    manifold_candidate_normal[manifold_candidate_point_count] = (edge.p1 - ref_points[_i]).normalized();
                    manifold_candidate_depths[manifold_candidate_point_count] = pen_depth_e1;
                    ++manifold_candidate_point_count;
                }
                if (pen_depth_e2 > 0.0) {
                    manifold_candidate_points[manifold_candidate_point_count] = edge.p2;
                    manifold_candidate_normal[manifold_candidate_point_count] = (edge.p2 - ref_points[_i]).normalized();
                    manifold_candidate_depths[manifold_candidate_point_count] = pen_depth_e2;
                    ++manifold_candidate_point_count;
                }
            }
        }

        if (manifold_candidate_point_count == 0) {
            continue;
        }

        // handle manifold candidates
        // pick the deepest candidate
        if (manifold_candidate_point_count >= 2) {
            // sort the first two, DESC
            if (manifold_candidate_depths[0] < manifold_candidate_depths[1]) {
                const double _tmp_depth = manifold_candidate_depths[0];
                const Vector2 _tmp_point = manifold_candidate_points[0];
                const Vector2 _tmp_normal = manifold_candidate_normal[0];

                manifold_candidate_depths[0] = manifold_candidate_depths[1];
                manifold_candidate_points[0] = manifold_candidate_points[1];
                manifold_candidate_normal[0] = manifold_candidate_normal[1];

                manifold_candidate_depths[1] = _tmp_depth;
                manifold_candidate_points[1] = _tmp_point;
                manifold_candidate_normal[1] = _tmp_normal;
            }

            for (size_t _k=2; _k<manifold_candidate_point_count; ++_k) {
                const bool larger_than_r1 = (manifold_candidate_depths[_k] > manifold_candidate_depths[0]);
                const bool larger_than_r2 = (manifold_candidate_depths[_k] > manifold_candidate_depths[1]);

                if (larger_than_r1 && larger_than_r2) {
                    manifold_candidate_depths[1] = manifold_candidate_depths[0];
                    manifold_candidate_points[1] = manifold_candidate_points[0];
                    manifold_candidate_normal[1] = manifold_candidate_normal[0];

                    manifold_candidate_depths[0] = manifold_candidate_depths[_k];
                    manifold_candidate_points[0] = manifold_candidate_points[_k];
                    manifold_candidate_normal[0] = manifold_candidate_normal[_k];
                }
                else if (larger_than_r2) {
                    manifold_candidate_depths[1] = manifold_candidate_depths[_k];
                    manifold_candidate_points[1] = manifold_candidate_points[_k];
                    manifold_candidate_normal[1] = manifold_candidate_normal[_k];
                }
            }

            if (manifold_candidate_point_count > 2) {
                manifold_candidate_point_count = 2;
            }
        }

        double avg_penetration_depth = 0.0;
        for (size_t k=0; k<manifold_candidate_point_count; ++k) {
            avg_penetration_depth += manifold_candidate_depths[k];
        }
        avg_penetration_depth /= manifold_candidate_point_count;

        if (penetration_count == 0 || avg_penetration_depth > max_penetration || manifold_candidate_point_count > contact_counts) {
            max_penetration = avg_penetration_depth;
            contact_counts = manifold_candidate_point_count;
            Vector2 added_normals = Vector2::ZERO;
            for (size_t _k=0; _k<contact_counts; ++_k) {
                contact_points[_k] = manifold_candidate_points[_k];
                added_normals += manifold_candidate_normal[_k];
            }
            contact_normal = (added_normals * (1.0 / contact_counts)).normalized();
            ++penetration_count;
        }

    }

    if (penetration_count == 0) {
        return false;
    }

    // manifold packaging
    manifold__out->setPoints(contact_points, contact_counts);
    manifold__out->penetration_depth = max_penetration;
    manifold__out->normal = contact_normal;

    *is_body_1_the_ref = false;

    return true;
}


bool boxToCapsuleCollision(BoxCollider* box, CapsuleCollider* capsule, CollisionManifold* manifold__out, bool* is_body_1_the_ref) {
    PolygonCollider poly_1;
    poly_1.setPolygon(box->__polygon);
    poly_1.__transform = box->__transform;
    const bool is_colliding = polygonToCapsuleCollision(
            &poly_1,
            capsule,
            manifold__out,
            is_body_1_the_ref
            );
    return is_colliding;
}


bool circleToCapsuleCollision(CircleCollider* circle, CapsuleCollider* capsule, CollisionManifold* manifold__out, bool* is_body_1_the_ref) {
    double distance;

    const Vector2 circ_pos = circle->__transform.GlobalPosition();

    const double caps_rot = capsule->__transform.rotation;
    const Vector2 caps_pos = capsule->__transform.GlobalPosition();
    const Vector2 caps_p1 = caps_pos + capsule->radius*Vector2::UP.rotated(caps_rot);
    const Vector2 caps_p2 = caps_pos + capsule->radius*Vector2::DOWN.rotated(caps_rot);

    Vector2 projected_point;
    const bool is_projected = isProjectedPointLiesWithinLine(circ_pos, caps_p1, caps_p2, &projected_point);

    if (is_projected) {
        distance = (projected_point - circ_pos).magnitude();
        if ( distance > (circle->radius + capsule->radius) ) {
            return false;
        }
    } else {
        const double dist_to_p1 = (caps_p1 - circ_pos).magnitude();
        const double dist_to_p2 = (caps_p2 - circ_pos).magnitude();

        if (dist_to_p1 < dist_to_p2) {
            distance = dist_to_p1;
            projected_point = caps_p1;
        } else {
            distance = dist_to_p2;
            projected_point = caps_p2;
        }

        if ( distance > (circle->radius + capsule->radius) ) {
            return false;
        }
    }

    // calculate manifold
    const Vector2 contact_normal = (projected_point - circ_pos).normalized();
    const Vector2 contact_point = circ_pos +  circle->radius*contact_normal;

    manifold__out->normal = contact_normal;
    manifold__out->setPoints(&contact_point, 1);
    manifold__out->penetration_depth = (circle->radius + capsule->radius) - distance;

    *is_body_1_the_ref = true;

    return true;
}


bool capsuleToCapsuleCollision(CapsuleCollider* capsule_1, CapsuleCollider* capsule_2, CollisionManifold* manifold__out, bool* is_body_1_the_ref) {

    const double caps1_rot = capsule_1->__transform.rotation;
    const double caps2_rot = capsule_2->__transform.rotation;
    const Vector2 caps1_pos = capsule_1->__transform.GlobalPosition();
    const Vector2 caps2_pos = capsule_2->__transform.GlobalPosition();
    const Vector2 caps1_p1 = caps1_pos + (capsule_1->length/2.0)*Vector2::UP.rotated(caps1_rot);
    const Vector2 caps1_p2 = caps1_pos + (capsule_1->length/2.0)*Vector2::DOWN.rotated(caps1_rot);
    const Vector2 caps2_p1 = caps2_pos + (capsule_2->length/2.0)*Vector2::UP.rotated(caps2_rot);
    const Vector2 caps2_p2 = caps2_pos + (capsule_2->length/2.0)*Vector2::DOWN.rotated(caps2_rot);
    const double combined_radius = capsule_1->radius + capsule_2->radius;

    struct ManifoldCandidate {
        Vector2 contact;
        Vector2 normal;
        double depth;
    };
    ManifoldCandidate manifold_candidates[8];
    size_t manifold_candidate_count = 0;
    const Vector2 points[4] = {caps1_p1, caps1_p2, caps2_p1, caps2_p2};
    Vector2 p1, p2;
    for (size_t i=0; i<4; ++i) {
        if (i < 2) {
            p1 = caps2_p1;
            p2 = caps2_p2;
        }
        else
        {
            p1 = caps1_p1;
            p2 = caps1_p2;
        }

        Vector2 projected_point;
        const bool is_projected = isProjectedPointLiesWithinLine(points[i], p1, p2, &projected_point);

        if ( is_projected && (projected_point - points[i]).magnitude() < (capsule_1->radius + capsule_2->radius) ) {
            const size_t j = manifold_candidate_count;
            if (i < 2) {
                const Vector2 normal = (projected_point - points[i]).normalized();
                manifold_candidates[j].contact = projected_point - capsule_2->radius*normal;
                manifold_candidates[j].depth = combined_radius - (projected_point - points[i]).magnitude();
                manifold_candidates[j].normal = normal;
            } else {
                const Vector2 normal = (points[i] - projected_point).normalized();
                manifold_candidates[j].contact = points[i] - capsule_2->radius*normal;
                manifold_candidates[j].depth = combined_radius - (projected_point - points[i]).magnitude();
                manifold_candidates[j].normal = normal;
            }
            ++manifold_candidate_count;
        }
    }

    if ( manifold_candidate_count == 0 ) {
        for (size_t _i=0; _i<2; ++_i) {
            const double pen_depth_e1 = combined_radius - (points[_i] - caps2_p1).magnitude();
            const double pen_depth_e2 = combined_radius - (points[_i] - caps2_p2).magnitude();

            if (pen_depth_e1 > 0.0) {
                const Vector2 normal = (caps2_p1 - points[_i]).normalized();
                manifold_candidates[manifold_candidate_count].contact = caps2_p1 - capsule_2->radius*normal;
                manifold_candidates[manifold_candidate_count].normal = normal;
                manifold_candidates[manifold_candidate_count].depth = pen_depth_e1;
                ++manifold_candidate_count;
            }
            if (pen_depth_e2 > 0.0) {
                const Vector2 normal = (caps2_p2 - points[_i]).normalized();
                manifold_candidates[manifold_candidate_count].contact = caps2_p2 -capsule_2->radius*normal;
                manifold_candidates[manifold_candidate_count].normal = normal;
                manifold_candidates[manifold_candidate_count].depth = pen_depth_e2;
                ++manifold_candidate_count;
            }
        }
    }

    if ( manifold_candidate_count == 0 ) {
        return false;
    }

    // find the two deepest manifold candidates
    if ( manifold_candidate_count >= 2 ) {
        if ( manifold_candidates[0].depth < manifold_candidates[1].depth ) {
            ManifoldCandidate _tmp_manifold=  manifold_candidates[0];
            manifold_candidates[0] = manifold_candidates[1];
            manifold_candidates[1] = _tmp_manifold;
        }

        for (size_t _k=2; _k<manifold_candidate_count; ++_k) {
            const bool larger_than_r1 = (manifold_candidates[_k].depth > manifold_candidates[0].depth);
            const bool larger_than_r2 = (manifold_candidates[_k].depth > manifold_candidates[1].depth);

            if (larger_than_r1 && larger_than_r2) {
                manifold_candidates[1] = manifold_candidates[0];
                manifold_candidates[0] = manifold_candidates[_k];
            }
            else if (larger_than_r2) {
                manifold_candidates[1] = manifold_candidates[_k];
            }
        }

        if (manifold_candidate_count > 2) {
            manifold_candidate_count = 2;
        }
    }

    double avg_penetration_depth = 0.0;
    for (size_t k=0; k<manifold_candidate_count; ++k) {
        avg_penetration_depth += manifold_candidates[k].depth;
    }
    avg_penetration_depth /= manifold_candidate_count;

    Vector2 contact_normal = Vector2::ZERO;
    Vector2 manifold_points[2];
    for (size_t i=0; i<manifold_candidate_count; ++i) {
        manifold_points[i] = manifold_candidates[i].contact;
        contact_normal += manifold_candidates[i].normal;
    }
    contact_normal = (contact_normal * (1.0 / manifold_candidate_count)).normalized();

    manifold__out->setPoints(manifold_points, manifold_candidate_count);
    manifold__out->penetration_depth = avg_penetration_depth;
    manifold__out->normal = contact_normal;

    *is_body_1_the_ref = true;

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


