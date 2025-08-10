/*
 * Pixel Bench Core Engine Configs
 *
 * Created     : 2025-04-15
 * Last Updated: 2025-04-15
 * */

#ifndef PIXBENCH_ENGINE_CONFIG
#define PIXBENCH_ENGINE_CONFIG

#include "SDL3_mixer/SDL_mixer.h"
#include <cstddef>


/* Used by ECS system to determined the absolute maximum numbers of entities 
 * in the game. You can increase this value if you encounters "Can't add entity,
 * maximum numbers of entities reached."
 * */
const size_t MAX_ENTITIES = 10000;

/* Used by ECS system to determined the maximum limit on numbers of components
 * usable in the game. If you encounter "Can't add component, maximum numbers 
 * of components reached." when you add your script as component. You can 
 * either:
 * 1. use `entity_manager.AssignScript<ScriptType>(entity_id)`, or
 * 2. you can in crease this number and still use 
 *    entity_manager.RegisterScriptAsComponent<ScriptType>() and then assign 
 *    it to an entity with 
 *    entity_manager.AssignComponentToEntity<ScriptType>(entity_id)
 * */
const size_t MAX_COMPONENTS = 30;



/*
 * AUDIO
 */
const MIX_InitFlags AUDIO_MIX_INIT_FLAGS = MIX_INIT_OGG | MIX_INIT_WAVPACK;
const size_t AUDIO_NUM_CHANNELS = 8;



/*
 * PHYSICS
 */
const size_t FIXED_UPDATE_RATE = 60;
const size_t MAX_POLYGON_VERTEX = 16;


// Uncomment this to draw collider debug lines
#define PHYSICS_DEBUG_DRAW
// #define PHYSICS_DEBUG_DRAW_SHOW_ENTITY_ID


#endif
