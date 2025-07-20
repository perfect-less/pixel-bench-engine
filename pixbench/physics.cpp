#include "pixbench/ecs.h"
#include "pixbench/vector2.h"
#include "pixbench/physics.h"


double projectPointToLineCoordinate(
        Vector2 point,
        Vector2 line_origin,
        Vector2 line_direction_normalized
        ) {
    const Vector2 orig_2_point = point - line_origin;
    return Vector2::dotProduct(orig_2_point, line_direction_normalized);
}


bool boxToBoxCollision(BoxCollider* box_1, BoxCollider* box_2, CollisionManifold* manifold__out, bool* is_body_1_the_ref) {
    // get collider
    Polygon b1_poly;
    Vector2 b1_verts[4];
    b1_verts[0] = Vector2(box_1->width / 2.0, -box_1->height / 2.0);
    b1_verts[1] = Vector2(-box_1->width / 2.0, -box_1->height / 2.0);
    b1_verts[2] = Vector2(-box_1->width / 2.0, box_1->height / 2.0);
    b1_verts[3] = Vector2(box_1->width / 2.0, box_1->height / 2.0);
    b1_poly.setVertex(b1_verts, 4);

    Polygon b2_poly;
    Vector2 b2_verts[4];
    b2_verts[0] = Vector2(box_1->width / 2.0, -box_1->height / 2.0);
    b2_verts[1] = Vector2(-box_1->width / 2.0, -box_1->height / 2.0);
    b2_verts[2] = Vector2(-box_1->width / 2.0, box_1->height / 2.0);
    b2_verts[3] = Vector2(box_1->width / 2.0, box_1->height / 2.0);
    b2_poly.setVertex(b2_verts, 4);

    // rotated vertex and edges
    Edge b1_edges[4];
    Edge b2_edges[4];
    for (int i=0; i<4; ++i) {
        b1_verts[i] = b1_poly.getVertex(i, box_1->__transform.GlobalPosition(), box_1->__transform.rotation);
        b1_edges[i] = b1_poly.getEdge(i, box_2->__transform.GlobalPosition(), box_1->__transform.rotation);

        b2_verts[i] = b2_poly.getVertex(i, box_1->__transform.GlobalPosition(), box_2->__transform.rotation);
        b2_edges[i] = b2_poly.getEdge(i, box_2->__transform.GlobalPosition(), box_2->__transform.rotation);
    }

    double penetration_edge_indexes[4];
    double penetration_depths[4];
    int penetration_counts = 0;

    // edge loop
    for (int i=0; i<4; ++i) {
        const Edge edge = b1_edges[i];

        // check edge direction, ignore edge pointing away from b2
        const double b2_origin_projected = projectPointToLineCoordinate(
                box_2->__transform.GlobalPosition(),
                edge.p1,
                edge.normal
                );
        if ( b2_origin_projected < 0.0 ) {
            continue;
        }

        double b1_min = 0.0;
        double b1_max = 0.0;
        double b2_min = 0.0;
        double b2_max = 0.0;

        // project to edge
        double projected_b1 = 0.0;
        double projected_b2 = 0.0;
        double combined_max = 0.0;
        double combined_min = 0.0;
        for (int j=0; j<4; ++j) {
            projected_b1 = projectPointToLineCoordinate(
                    b1_verts[j],
                    edge.p1,
                    edge.normal
                    );
            projected_b2 = projectPointToLineCoordinate(
                    b2_verts[j],
                    edge.p1,
                    edge.normal
                    );
            
            if (projected_b1 < b1_min)
                b1_min = projected_b1;
            if (projected_b1 > b1_max)
                b1_max = projected_b1;

            if (projected_b2 < b2_min)
                b2_min = projected_b2;
            if (projected_b2 > b2_max)
                b2_max = projected_b2;

            if (projected_b1 < combined_min)
                combined_min = projected_b1;
            if (projected_b1 > combined_max)
                combined_max = projected_b1;
            if (projected_b2 < combined_min)
                combined_min = projected_b2;
            if (projected_b2 > combined_max)
                combined_max = projected_b2;
        }

        // collission depth
        const double b1_projected_length = b1_max - b1_min;
        const double b2_projected_length = b2_max - b2_min;
        const double combined_length = combined_max - combined_min;
        const double penetration_depth = combined_length - (
                b1_projected_length + b2_projected_length
                );

        if ( penetration_depth < 0.0) {
            return false;
        }

        penetration_edge_indexes[penetration_counts] = i;
        penetration_depths[penetration_counts] = penetration_depth;
        penetration_counts++;
    }

    // minimum depth edge
    double min_depth = penetration_depths[0];
    int min_edge_index = penetration_edge_indexes[0];
    for (int i=1; i<penetration_counts; ++i) {
        if ( penetration_depths[i] >= min_depth )
            continue;

        min_depth = penetration_depths[i];
        min_edge_index = penetration_edge_indexes[i];
    }
    const Edge b1_collision_edge = b1_edges[min_edge_index];

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

    // incident edge determination, by which produce minimum penetration depth

    // incident edge clipping, obtaining manifold

    // result packing

    return false;
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


bool polygonToPolygonCollision(PolygonCollider* polygon_1, PolygonCollider* polygon_2, CollisionManifold* manifold__out, bool* is_body_1_the_ref) {
    return false;
}
