#include "game.h"
#include "agent.h"
#include "card_types.h"
#include <raymath.h>
#include <raygui.h>
#include <string.h>


void CameraUpdate(World *world) {
  Vector2 direction = Vector2Zero();

  if (IsKeyDown(KEY_A)) direction.x -= 1;
  if (IsKeyDown(KEY_D)) direction.x += 1;
  if (IsKeyDown(KEY_W)) direction.y -= 1;
  if (IsKeyDown(KEY_S)) direction.y += 1;

  direction = Vector2Normalize(direction);

  if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
    Vector2 delta = Vector2Scale(GetMouseDelta(), -1.0f/world->camera.zoom);
    world->camera.target = Vector2Add(world->camera.target, delta);
  }

  F32 speed = 3.0/world->camera.zoom;
  direction = Vector2Scale(direction, speed);


  world->camera.target = Vector2Add(world->camera.target, direction);

  world->camera.zoom = Clamp(world->camera.zoom + GetMouseWheelMove(), 10, 160);

  F32 wheel_move = GetMouseWheelMove();
  if (wheel_move != 0) {

    Vector2 screen_mouse_pos = GetMousePosition();
    Vector2 world_mouse_pos = GetScreenToWorld2D(screen_mouse_pos, world->camera);

    world->camera.offset = screen_mouse_pos;
    world->camera.target = world_mouse_pos;

    world->camera.zoom = Clamp(world->camera.zoom + wheel_move, 10, 160);
  }


}

void GamePlayUpdate(  
  World *world,
  Arena *perm_arena, 
  Arena *turn_arena, 
  Arena *temp_arena
) {
  Vector2 mouse_screen_position = GetMousePosition();
  Vector2 mouse_world_position = GetScreenToWorld2D(mouse_screen_position, world->camera);
  WorldCoord mouse_coord = WorldCoordFromVector2(mouse_world_position);




  // Card Grabbing and UI Stuff
  {
    for (EachCardNodeReverse(card, world->hand->last)) {
      if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){
        .x = card->screen_position.x,
        .y = card->screen_position.y,
        .width = 200,
        .height = 300})) {

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !world->grabbing_card) {
          world->grabbing_card = CardListRemove(world->hand, card);
          
        } else 
          card->screen_position = Vector2Lerp(
            card->screen_position,
            (Vector2){
              card->screen_position.x,
              GetScreenHeight() - 600},
              GetFrameTime() * 1.0
          );

        break;        
      } 
    }
  }

  if (world->selected_entity && !world->selected_entity->path) {
    world->selected_path = WorldCoordListFindPath(world, temp_arena, world->selected_entity->grid_pos, mouse_coord, 0);
  }

  // this is garbage will clean up later
  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && world->grabbing_card) {
    world->grabbing_card->screen_position = Vector2Add(world->grabbing_card->screen_position, GetMouseDelta());
  } else if (world->grabbing_card) {
    WorldCoord target_coord = WorldCoordFromVector2(Vector2Subtract(GetScreenToWorld2D(world->grabbing_card->screen_position, world->camera), Vector2One()));
    Entity *target = EntityFindByWorldCoord(world->entities, target_coord);

      TraceLog(LOG_INFO, "Coord: (%i, %i)", target_coord.x, target_coord.y);
    if (target)  {
      CardData card = card_archetypes[world->grabbing_card->data];
      CardListAppend(world->discard, world->grabbing_card);

      PlayCard(world, target);

    } else {
      CardListAppend(world->hand, world->grabbing_card);
    }
    world->grabbing_card = NULL;

  } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    Entity *entity_clicked = EntityFindByWorldCoord(world->entities, mouse_coord);
    if (entity_clicked) {
      world->selected_entity = entity_clicked;
    } else if (world->selected_entity) {
      // Move Entity Stuff
      Entity *entity = world->selected_entity;
      if (world->selected_path) {
        I32 movement_distance = entity->movement_left;
        Vector2 center = {0.5f, 0.5f};

        U32 i;
        Vector2 difference;
        for (i = 0; i < world->selected_path->len - 1; i++) {

            Color path_color = WHITE;
            Vector2 first_pos = Vector2FromWorldCoord(world->selected_path->ptr[i]);
            Vector2 second_pos = Vector2FromWorldCoord(world->selected_path->ptr[i + 1]);


            difference = Vector2Subtract(second_pos, Vector2FromWorldCoord(entity->grid_pos));

            if (Vector2LengthSqr(difference) > movement_distance * movement_distance)
              break;
          }

        WorldCoordList *new_path = ArenaPushNoZero(turn_arena, sizeof(WorldCoordList));
        new_path->len = i + 1;
        new_path->ptr = ArenaPushNoZero(turn_arena, sizeof(WorldCoord) * new_path->len);

        memcpy(new_path->ptr, world->selected_path->ptr, sizeof(WorldCoord) * new_path->len);


        world->selected_path = NULL;
        entity->path = new_path;
        if ((I32)Vector2Length(difference) >= entity->movement_left) {
          entity->movement_left = 0;
        }
        else entity->movement_left -= (I32)Vector2Length(difference);
      }
    }
  } 
  

  I32 c = GetCharPressed();

  if (c >= '0' && c <= '9') {
    c = c - '0';
    CardListPopAppend(world->hand, world->deck, c);
  }
}



void EntityUpdatePathPosition(Entity *entity) {
  const F32 speed = 10.0;

  if (entity->path) {
    WorldCoord coord_end = entity->path->ptr[entity->path->len - 1];
    Vector2 vector_end = Vector2FromWorldCoord(coord_end);

    // cursed
    WorldCoord coord_current_target = (entity->path_index < entity->path->len)? entity->path->ptr[entity->path_index]: coord_end;
    Vector2 vector_current_target = Vector2FromWorldCoord(coord_current_target);
    

    Vector2 delta = Vector2Subtract(vector_current_target, entity->visual_pos);
    
    // adjust direction entity is facing
    if (delta.x < 0) entity->h_flip = -1;
    else entity->h_flip = 1;


    // fix this i guess
    F32 delta_p = speed/60;

    Vector2 velocity = Vector2Scale(Vector2Normalize(delta), delta_p);
    F32 velocity_sqr = Vector2LengthSqr(velocity);

    TraceLog(LOG_ALL, "dt: %f\n", sqrt(velocity_sqr));
    
    F32 delta_sqr = Vector2LengthSqr(delta);
    while (velocity_sqr > delta_sqr) {
      entity->visual_pos = vector_current_target;


      //think about this some
      if (entity->path_index < entity->path->len)
        entity->path_index += 1;
      else {
        goto stop_path;
      }

      F32 distance_left = sqrtf(velocity_sqr - delta_sqr);

      coord_current_target = entity->path->ptr[entity->path_index];
      vector_current_target = Vector2FromWorldCoord(coord_current_target);

      delta = Vector2Subtract(vector_current_target, entity->visual_pos);
      velocity = Vector2Scale(Vector2Normalize(delta), distance_left);

  

      velocity_sqr = Vector2LengthSqr(velocity);
      delta_sqr = Vector2LengthSqr(delta);
    }
      
    entity->visual_pos = Vector2Add(entity->visual_pos, velocity);
    entity->grid_pos = WorldCoordFromVector2(entity->visual_pos);
    

    // go to next path index
    if (WorldCoordEqual(entity->grid_pos, coord_current_target) && entity->path_index <  entity->path->len)
      entity->path_index += 1 ;

    // stop condition
    if (Vector2Equals(entity->visual_pos, vector_end)) {
      stop_path:
      entity->visual_pos = vector_end;
      entity->grid_pos = coord_end;
      entity->path = NULL;
      entity->path_index = 0;
    }
  } 
}



void EntityUpdate(EntityList *list, Arena *perm_arena) {
  // ForEachEntity(entity, list->first) {
  
  Entity *entity = list->first;
  while (entity) {
    Entity *next = entity->next;

    EntityUpdatePathPosition(entity);


    entity = next;
  }
}


void GameGuiDraw(World *world) {
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