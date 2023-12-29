#define RAYGUI_IMPLEMENTATION

#include "world.h"
#include "agent.h"
#include "asset.h"
#include "game.h"
#include "editor.h"
#include <raymath.h>
#include <raygui.h>

#include <stdio.h> // Please remove this

// Coordinate Type Conversion Functions 
Vector2 Vector2FromWorldIndex(World *world, U32 index) {
  return (Vector2){
    .x = index % world->width,
    .y = index / world->width,
  };
}


Vector2 Vector2FromWorldCoord(WorldCoord coord) {
  return (Vector2) {
    .x = (F32)coord.x,
    .y = (F32)coord.y,
  };
}


WorldCoord WorldCoordFromVector2(Vector2 v) {
  return (WorldCoord){v.x, v.y};
}


WorldCoord WorldCoordFromIndex(World *world, U32 index) {
  return (WorldCoord){
    .x = index % world->width,
    .y = index / world->width,
  };
}


U32 WorldIndexFromVector2(World *world, Vector2 v) {
  return (U32)v.x + (U32)v.y * world->width;
}


U32 WorldIndexFromWorldCoord(World *world, WorldCoord coord) {
  return coord.x + coord.y * world->width;
}


bool WorldCoordEqual(WorldCoord a, WorldCoord b) {
  return a.x == b.x && a.y == b.y;
}



World WorldInit(Arena *arena, U32 width, U32 height) {
  return (World) {

      .width = width,
      .height = height,
      .tiles = ArenaPush(arena, sizeof(Tile) * width * height),
      .camera = (Camera2D) {
        .zoom = 2 * width,
      },
      .turn_count = 0,
      .entities = ArenaPush(arena, sizeof(EntityList)),
      .hand = CardListInit(arena, 0),
      .discard = CardListInit(arena, 0),
      .deck = CardListInit(arena, 30),
      .textures = ArenaPush(arena, sizeof(Texture) * MAX_TEXTURES)
    };
}


void WorldLoad(World *world, Arena* arena, const char *filepath) {
  // TODO: implement this function
}


void WorldSave(World *world, const char *filepath) {
  // TODO: implement this function
}



void WorldDraw(World *world) {
  BeginMode2D(world->camera);
  {
    for (U32 i = 0; i < world->width * world->height; i++) {
      Vector2 tile_pos = Vector2FromWorldIndex(world, i);

      switch (world->tiles[i]) {
        case Tile_wall: DrawRectangleV(tile_pos, (Vector2){1,1}, BLACK); break;
        default: 
          DrawRectangleV(tile_pos, (Vector2){1,1}, DARKGRAY);
          DrawRectangleV(Vector2Add(tile_pos, (Vector2){0.05, 0.05}), (Vector2){0.9,0.9}, GRAY);
          break;
      }

      
    }
    


    for (EachEntity(entity, world->entities->first)) {
      if (entity->path) 
        WorldCoordListDraw(world, entity->path, entity->path_index);

      if (entity == world->selected_entity) {

        I32 movement_distance = entity->movement_left;

        if (world->selected_path) {
          for (U32 i = 0; i < world->selected_path->len - 1; i++) {
            Vector2 center = {0.5f, 0.5f};

            Color path_color = WHITE;
            Vector2 first_pos = Vector2FromWorldCoord(world->selected_path->ptr[i]);
            Vector2 second_pos = Vector2FromWorldCoord(world->selected_path->ptr[i + 1]);


            Vector2 difference = Vector2Subtract(second_pos, Vector2FromWorldCoord(entity->grid_pos));

            if (Vector2LengthSqr(difference) > movement_distance * movement_distance)
              path_color = RED;
            
            first_pos = Vector2Add(first_pos, center);
            second_pos = Vector2Add(second_pos, center);

            DrawLineEx(first_pos, second_pos, 0.1, path_color);
          }
        }
      }

      Rectangle entity_rect = (Rectangle) {
        .height = 1,
        .width = 1,
        .x = entity->visual_pos.x,
        .y = entity->visual_pos.y,
      };

      DrawTextEx(GetFontDefault(), entity->name_buffer, Vector2Subtract(entity->visual_pos, (Vector2){0,0.2}), 0.2, 0.1, WHITE);

      DrawTextEx(GetFontDefault(), TextFormat("health: %lu", entity->health), Vector2Add(entity->visual_pos, (Vector2){0, 1}), 0.4, 0.1, WHITE); // show health

      // draw textures
      Texture texture = world->textures[entity->texture];
      Rectangle texture_source_rect = {(texture.width/6) * entity->animation_state, 0, (texture.width/6) * entity->h_flip, texture.height};
      DrawTexturePro(texture, texture_source_rect, entity_rect, Vector2Zero(), 0, WHITE);
      

      if (entity == world->selected_entity) {
        DrawRectangleLinesEx(entity_rect, 0.05, GREEN );
      }
    }

    if (world->mode == WorldMode_game) {
      if (world->grabbing_card) {
        Vector2 card_world_postion = GetScreenToWorld2D(world->grabbing_card->screen_position, world->camera);
        Vector2 card_target_vector = Vector2Subtract(card_world_postion, Vector2One());

        WorldCoord card_target_coord = WorldCoordFromVector2(card_target_vector);
          Rectangle card_target_rect = (Rectangle){
            .x = card_target_coord.x,
            .y = card_target_coord.y,
            .width = 1,
            .height = 1,
          };
          DrawRectangleLinesEx(card_target_rect, 0.1, RED);

        DrawLineEx(
          card_world_postion, 
          Vector2FromWorldCoord(WorldCoordFromVector2(card_world_postion)), 
          0.07, 
          RED
        );
    }
  }
  EndMode2D();
  
  // UI drawing stuff
  

  if (world->mode == WorldMode_game){
    DrawText(TextFormat("Turn: %i", world->turn_count), 0, 0, 20, WHITE);

    CardListHandDraw(world->hand);
    if (world->grabbing_card)
      CardDraw(world->grabbing_card);

    DrawText(TextFormat("Turn: %lu", world->turn_count), 0 ,0 , 20, WHITE);

    Rectangle discard_rect = (Rectangle){
      .height = 80,
      .width = 80,
      .x =  10,
      .y = GetScreenHeight() - 90,
    };

    Rectangle deck_rect = (Rectangle){
      .height = 80,
      .width = 80,
      .x =  GetScreenWidth() - 90,
      .y = GetScreenHeight() - 90,
    };

    Rectangle end_turn_rect = (Rectangle){
      .height = 30,
      .width = 80,
      .x = GetScreenWidth() - 90,
      .y = GetScreenHeight() - 130,
    };

    DrawRectangleRounded(discard_rect, .4, 2, RAYWHITE);
    DrawTextEx(GetFontDefault(), TextFormat("%lu", world->discard->count), (Vector2){40, GetScreenHeight() - 50}, 20, 1, BLACK);
    
    DrawRectangleRounded(deck_rect, .4, 2, RAYWHITE);
    DrawTextEx(GetFontDefault(), TextFormat("%lu", world->deck->count), (Vector2){GetScreenWidth() - 40, GetScreenHeight() - 50}, 20, 1, BLACK);
    
    // Game Intermediate Mode Gui
    if (GuiButton(end_turn_rect, "End Turn")) {
      WorldUpdateTurn(world);
    }

  }

}

  if (world->mode == WorldMode_edit) {
    EditorDraw();
  }
}



void WorldUpdateFrame(
  World *world,
  Arena *perm_arena, 
  Arena *turn_arena, 
  Arena *temp_arena
) {
  // TODO: implement this function


  CameraUpdate(world);

  if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_TAB))
    world->mode = !world->mode;

  if (world->mode == WorldMode_edit)
    EditorUpdateFrame(world, perm_arena, turn_arena, temp_arena);
    

  if (world->mode == WorldMode_game) 
    GamePlayUpdate(world, perm_arena, turn_arena, temp_arena);

  EntityUpdate(world->entities, perm_arena); 
}


void WorldUpdateTurn(World *world) {
  world->turn_count += 1;
  
  for (EachEntity(entity, world->entities->first)) {
    entity->movement_left = entity->movement_cap;
  }
}