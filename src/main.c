#include <raylib.h>
#include <raymath.h>
#include "arena.h"
#include "card.h"
#include "world.h"
#include "rayutil.h"
#include "asset.h"

// temporary
#include "serialization.h"

// Ok I have this idea to combine a card game with a 
// turn based strategy game I think the idea would be 
// kinda interesting

// Supporters would be like spirits that you can call on 
// and items would be physical items that you throw on the grid

// we could have emergent game play like throwing a flamable liquid 
// on the ground then lighting it on fire would cause the whole area to 
// light up

// we could possibly save on art assets by having a paper mario kinda animation 
// thing going on. 

// Rules each player controls three characters 
// == Card Types == 
// Item Cards
// - weapons - each character can hold one weapon at a time


int main() {

  TraceLogLevel(LOG_ALL);
  InitWindow(1240, 720, "Card Game");
  SetTargetFPS(60);

  TraceLog(LOG_INFO, "size of effect flags: %d", sizeof(EffectFlags));
  
  // This is all of our memory management 
  Arena *perm_arena = ArenaInit(Megabyte(100)); // this is for permanent allocations *eg. world data, entities, cards, 
  Arena *turn_arena = ArenaInit(Megabyte(100)); // This is for allocations that will only last a turn *eg. A path list
  Arena *temp_arena = ArenaInit(Megabyte(100)); // this is for frame allocations any thing that is only for a frame
  

  World world = WorldInit(perm_arena, 20, 20);

  AssetLoadTexture(&world);

  world.tiles[57] = Tile_wall;

  Entity *player = EntityAlloc(perm_arena, world.entities, "kaiden");
  player->health_cap = 50;
  player->health = 20;
  player->movement_cap = 10;
  player->movement_left = 10;
  player->h_flip = 1;
  player->animation_state = AnimationState_running;

  Entity *enemy = EntityAlloc(perm_arena, world.entities, "susan");
  enemy->health_cap = 10;
  enemy->health = 7;
  enemy->movement_cap = 5;
  enemy->movement_left = 5;
  enemy->grid_pos = (WorldCoord){10, 10};
  enemy->visual_pos = (Vector2){10, 10};
  enemy->h_flip = 1;
  enemy->animation_state = AnimationState_attacking;
  
  SerializeWorld(temp_arena, &world, "test.world");
  LoadWorld(temp_arena, &world, "test.world");

  while (!WindowShouldClose()) {
    // Update 
    ArenaReset(temp_arena);
    
    WorldUpdateFrame(&world, perm_arena, turn_arena, temp_arena);

    // Draw 
    BeginDrawing();
      ClearBackground(BLACK);
      
      WorldDraw(&world);

    EndDrawing();
  }
}