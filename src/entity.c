/*
all logic specific to entities resides here
entites are all nodes in a linked list!
*/

#include "entity.h"
#include <string.h>


/*
adds a new entity at the end of the entity list
*/
void EntitiesAppend(Entities *list, Entity *entity) {
  Entity *last = list->last; 
  if (list->last) {
    last->next = entity;
    entity->prev = last;
    list->last = entity;
    list->count += 1; 
  } else {
    list->count = 1;
    list->first = entity;
    list->last = entity;
  }
}

/*
allocates a new entity and returns its memory address
*/
Entity *EntityAlloc(Arena *arena, Entities *list, const char *name) {
  Entity *entity = NULL;

  if (list->free_list) {
    // If we have a free spot we allocate reuse memory
    entity = list->free_list;
    list->free_list = list->free_list->next;  
    memset(entity, 0, sizeof(Entity));
  } else {
    // Otherwise we just allocate new memory from a arena
    entity = ArenaPush(arena, sizeof(Entity));
  }
  
  EntitiesAppend(list, entity);

  // set up the entity
  entity->name_len = strlen(name);
  strcpy(entity->name_buffer, name);

  return entity;
}


void EntityFree(Entities *list, Entity *entity) {
  Entity *next = entity->next;
  Entity *prev = entity->prev;

  if (prev && next) {
    next->prev = prev;
    prev->next = next;
  } else if (prev) {
    prev->next = NULL;
    list->last = prev;
  } else if (next) {
    next->prev = NULL;
    list->first = next;
  } else {
    list->last = NULL;
    list->first = NULL;
  }

  list->count -= 1;

  entity->next = list->free_list;
  list->free_list = entity;
}



Entity *EntityFindByWorldCoord(Entities *list, WorldCoord coord) {
  Entity *entity = NULL;
  for (EachEntity(node, list->first)) {
    if (coord.x == node->grid_pos.x && coord.y == node->grid_pos.y) {
      entity = node;
      break;
    }
  }
  return entity;
}