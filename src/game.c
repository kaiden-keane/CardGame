/*
all the general game logic will be in this file
*/

#include "game.h"
#include "agent.h"
#include "card_types.h"
#include <raymath.h>
#include <raygui.h>
#include <string.h>
#include <stdlib.h> // for quicksort()
#include <stdio.h> // for putchar()


/*
gets user input related to the camera location and updates it
*/
void CameraUpdate(World *world) {
  Vector2 direction = Vector2Zero();

  if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  direction.x -= 1;
  if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) direction.x += 1;
  if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    direction.y -= 1;
  if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  direction.y += 1;

  direction = Vector2Normalize(direction);


  if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
    Vector2 delta = Vector2Scale(GetMouseDelta(), -1.0f/world->camera.zoom);
    world->camera.target = Vector2Add(world->camera.target, delta);
  }

  F32 speed = 8.0/world->camera.zoom;

  if (IsKeyDown(KEY_LEFT_SHIFT)){
    speed *= 2; 
  }
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

/*
moves the entity AFTER its new location has already been decided
collects no user input
*/
void EntityMoveEntity(Arena *turn_arena, World *world) {
  Entity *entity = world->selected_entity;
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


/*
updates everything related to gameplay every turn
handles user input related to movement and cards
*/
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
    for (EachCardNodeReverse(card, world->player.hand->last)) {
      if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){
        .x = card->screen_position.x,
        .y = card->screen_position.y,
        .width = 200,
        .height = 300})) {

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !world->grabbing_card) {
          world->grabbing_card = CardListRemove(world->player.hand, card);
          
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

  if (world->selected_entity && !world->selected_entity->path && world->selected_entity == world->turn_data.characters[world->turn_data.current_character]) {
    world->selected_path = WorldCoordListFindPath(world, temp_arena, world->selected_entity->grid_pos, mouse_coord, 0);
  }

  
  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && world->grabbing_card) {
    world->grabbing_card->screen_position = Vector2Add(world->grabbing_card->screen_position, GetMouseDelta());
  }
  else if (world->grabbing_card) {
    WorldCoord target_coord = WorldCoordFromVector2(Vector2Subtract(GetScreenToWorld2D(world->grabbing_card->screen_position, world->camera), Vector2One()));
    Entity *target = EntityFindByWorldCoord(world->entities, target_coord);

      TraceLog(LOG_INFO, "Coord: (%i, %i)", target_coord.x, target_coord.y);
    if (target)  {
      CardData card = card_archetypes[world->grabbing_card->data];
      CardListAppend(world->player.discard, world->grabbing_card);

      PlayCard(world, target);
    }
    else {
      CardListAppend(world->player.hand, world->grabbing_card);
    }
    world->grabbing_card = NULL;

  }
  else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    Entity *entity_clicked = EntityFindByWorldCoord(world->entities, mouse_coord);
    // check so we dont select another entity while pathfinding
    if (entity_clicked && world->selected_path == NULL) {
      world->selected_entity = entity_clicked;
    }
    else if (world->selected_entity && world->selected_entity == world->turn_data.characters[world->turn_data.current_character]) {
      if (world->selected_path) {
        EntityMoveEntity(turn_arena, world);
      }
    }
  } 

  if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
    world->selected_entity = NULL;
    world->selected_path = NULL;
  }

  I32 c = GetCharPressed();

  if (c >= '0' && c <= '9') {
    c = c - '0';
    CardListPopAppend(world->player.hand, world->player.deck, c);
  }
}


/*
moves the entity through its path
*/
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

int EntityCompareY(void const *entity_a, void const *entity_b) {
  Entity const *a = entity_a;
  Entity const *b = entity_b;

  return a->grid_pos.y - b->grid_pos.y;
}

/*
sorts the entities by their Y value
*/
void EntitySort(Entities *list, Arena *temp_arena) {
  TempArena temp = TempArenaInit(temp_arena);

  Entity **sort_array = ArenaPush(temp_arena, sizeof(Entity*) * list->count);
  
  U32 i = 0;
  for (EachEntity(entity, list->first)) {
    sort_array[i] = entity;
    i += 1;
  }

  qsort(sort_array, list->count, sizeof(Entity*), EntityCompareY);

  for (i = 0; i < list->count; i += 1) {
    if (i > 0) sort_array[i]->prev = sort_array[i - 1];
    if (i < list->count - 1) sort_array[i]->next = sort_array[i + 1];
  }
  // put the caps on
  sort_array[0]->prev = NULL;
  sort_array[list->count - 1]->next = NULL;

  list->first = sort_array[0];
  list->last = sort_array[list->count - 1]; 


  TempArenaDeinit(temp);
}


/*
goes through every entity and updates changes each frame
*/
void EntityUpdate(World *world, Arena *perm_arena) {
  Entities *list = world->entities;
  EntitySort(list, perm_arena);
  if (world->entities->grid)
    memset(world->entities->grid, 0, sizeof(Entity*) * world->width * world->height);
  else
    world->entities->grid = ArenaPush(perm_arena, sizeof(Entity*) * world->width * world->height);


  Entity *entity = list->first;
  while (entity) {
    Entity *next = entity->next;
    
    world->entities->grid[WorldIndexFromWorldCoord(world, entity->grid_pos)] = entity;
    
    EntityUpdatePathPosition(entity);


    entity = next;
  }
}

/*
CURRENTLY EVER USED
draws all the gui
*/
void GameGuiDraw(World *world, Arena *turn_arena) {
  DrawText(TextFormat("Turn: %i", world->turn_count), 0, 0, 20, WHITE);

  CardListHandDraw(world->player.hand);
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

  //debug
  Rectangle print_entity_grid_rect = (Rectangle){
    .height = 30,
    .width = 80,
    .x = GetScreenWidth() - 90,
    .y = GetScreenHeight() - 170,
  };

  DrawRectangleRounded(discard_rect, .4, 2, RAYWHITE);
  DrawTextEx(GetFontDefault(), TextFormat("%lu", world->player.discard->count), (Vector2){40, GetScreenHeight() - 50}, 20, 1, BLACK);
  
  DrawRectangleRounded(deck_rect, .4, 2, RAYWHITE);
  DrawTextEx(GetFontDefault(), TextFormat("%lu", world->player.deck->count), (Vector2){GetScreenWidth() - 40, GetScreenHeight() - 50}, 20, 1, BLACK);
  
  // Game Intermediate Mode Gui
  if (GuiButton(end_turn_rect, "End Turn")) {
    EndTurn(world);
    WorldUpdateTurn(world, turn_arena);
  }

  if (GuiButton(print_entity_grid_rect, "Print Entities")) {
    for (U32 y = 0; y < world->height ; y++) {
      for (U32 x = 0; x < world->width; x++) {
        U32 index = WorldIndexFromWorldCoord(world, (WorldCoord){x, y});
        if (world->entities->grid[index]) putchar(world->entities->grid[index]->name_buffer[0]);
        else putchar('*');
      }
      putchar('\n');
    } 
  }
}