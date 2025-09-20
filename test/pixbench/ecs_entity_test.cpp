#include "pixbench/ecs.h"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <memory>
#include <vector>

TEST_CASE("ecs system test", "[ecscore]") {
    /* What functionalities that we should test here?
     *   1. Create entity
     *   2. Add components to entity
     *   3. Remove component from entity
     *   4. Delete entity
     *   5. Test gaining access to component from deleted entity*/

    // Creates entity manager and entities
    EntityManager entity_manager;
    EntityID entity_1 = entity_manager.CreateEntity();
    EntityID entity_2 = entity_manager.CreateEntity();

    std::cout << "Created Entity" << std::endl;
    for (auto& entity : entity_manager.GetEntities()) {
        std::cout << "  entity id:  " << entity << std::endl;
    }
    REQUIRE(entity_manager.GetEntities().size() == 2);

    // Adding components to entity
    std::unique_ptr<GameObject> gameObject_1 = std::make_unique<GameObject>();
    std::unique_ptr<GameObject> gameObject_2 = std::make_unique<GameObject>();
    std::cout << "Created unique GameObject(s)" << std::endl;
    entity_manager.AddComponentToEntity(entity_1, std::move(gameObject_1));
    entity_manager.AddComponentToEntity(entity_2, std::move(gameObject_2));
    std::cout << "GameObject(s) added to entities" << std::endl;


    std::cout << "Entity components (GameObject)" << std::endl;
    for (auto& entity : entity_manager.GetEntities()) {
        std::cout << "  id        : " << entity << std::endl;
        std::cout << "  components: " << std::endl;
        auto components = entity_manager.GetEntityComponents(entity, TGameObject);
        std::cout << "  components.size: " << components.size() << std::endl;
        REQUIRE(components.size() == 1);
        for (auto& component_wptr : components) {
            if (auto component_sptr = component_wptr.lock()) {
                std::cout << "  .  type: " << component_sptr->getType() << std::endl;
                REQUIRE(component_sptr->getType() == TGameObject);
            }
        }
    }

    // Adding other components to entity
    std::unique_ptr<Transform> transform_1 = std::make_unique<Transform>();
    std::unique_ptr<Transform> transform_2 = std::make_unique<Transform>();
    transform_2->SetPosition(Vector2(2, 1));
    std::cout << "Created unique Transform(s)" << std::endl;
    std::cout << "transform_1->getType() = " << transform_1->getType() << std::endl;
    entity_manager.AddComponentToEntity(entity_1, std::move(transform_1));
    entity_manager.AddComponentToEntity(entity_2, std::move(transform_2));
    std::cout << "Transform(s) added to entities" << std::endl;

    std::cout << "Entity components (Transform)" << std::endl;
    for (auto& entity : entity_manager.GetEntities()) {
        std::cout << "  id        : " << entity << std::endl;
        std::cout << "  components: " << std::endl;
        auto components = entity_manager.GetEntityComponents(entity, TTransform);
        std::cout << "  components.size: " << components.size() << std::endl;
        REQUIRE(components.size() == 1);
        for (auto& component_wptr : components) {
            if (auto component_sptr = component_wptr.lock()) {
                std::cout << "  .  type: " << component_sptr->getType() << std::endl;
                REQUIRE(component_sptr->getType() == TTransform);
            }
        }
    }

    // Remove Transform from entity_2
    std::vector<std::weak_ptr<Component>> transform_e2_vec = entity_manager.GetEntityComponents(
            entity_2,
            ComponentType::TTransform
            );
    REQUIRE(transform_e2_vec.size() == 1);
    if (std::shared_ptr<Component> transform_e2_shr = transform_e2_vec[0].lock()) {
        std::shared_ptr<Transform> transform_e2 = std::static_pointer_cast<Transform>(transform_e2_shr);
        Vector2 t2_local_pos = transform_e2->LocalPosition();
        std::cout << "tranform_2.local_position: " << t2_local_pos.x << ", " << t2_local_pos.y << std::endl;
        entity_manager.RemoveComponentFromEntity(entity_2, transform_e2_vec[0]);
        std::cout << "Transform from entity_2 removed" << std::endl;
    }
    else {
        REQUIRE(false);
    }
    REQUIRE(
            entity_manager.GetEntityComponents(
                entity_2,
                ComponentType::TTransform
                ).size() == 0
           );
    
    // Entity Deletion test
    auto ent2_gameobjects = entity_manager.GetEntityComponents(entity_2, ComponentType::TGameObject);
    REQUIRE(ent2_gameobjects.size() == 1);
    auto ent2_gameobject_weak = ent2_gameobjects[0];

    entity_manager.DestroyEntity(entity_2);
    std::cout << "entity_2 Destroyed" << std::endl;
    
    if (auto ent2_gameobject_shr = ent2_gameobject_weak.lock()) {
        // the pointer should be inaccessible
        REQUIRE(false);
    }

}
