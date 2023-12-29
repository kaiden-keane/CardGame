#include "editor.h"

#define GUI_ENTITY_EDITOR_IMPLEMENTATION
#include "gui/gui_entity_editor.h"

// Entity Editor GUI
GuiEntityEditorState entity_editor_state;
Entity *editing_entity = NULL;

void EntityEditorUpdateFrame(World *world) {
  if (IsKeyPressed(KEY_E) && world->selected_entity && !entity_editor_state.EntityEditorWindowBoxActive) {
    entity_editor_state = InitGuiEntityEditor();
    TraceLog(LOG_INFO, "bool: %d", entity_editor_state.EntityEditorWindowBoxActive);
    // set screen position;
    entity_editor_state.anchor = (Vector2){GetScreenWidth() - (272 + 10), 10};


    // Get Variables in there
    editing_entity = world->selected_entity;
    entity_editor_state.HealthSpinnerValue = editing_entity->health_cap;
    entity_editor_state.MovementSpinnerValue = editing_entity->movement_cap;
    strcpy(entity_editor_state.NameTextBoxText, editing_entity->name_buffer);
    entity_editor_state.EntityFactionToggleGroupActive = editing_entity->faction;
    // Flags
    entity_editor_state.EnemyableCheckBoxExChecked  = editing_entity->flags.is_enemyable;
    entity_editor_state.PlayableCheckBoxExChecked   = editing_entity->flags.is_playable;
    entity_editor_state.FlamableCheckBoxExChecked   = editing_entity->flags.is_flamable;
    entity_editor_state.FreezableCheckBoxExChecked  = editing_entity->flags.is_freezable;
    entity_editor_state.WetableCheckBoxExChecked    = editing_entity->flags.is_wetable;
  } 
}

static void EntitySaveButton() {
  editing_entity->health_cap = entity_editor_state.HealthSpinnerValue;
  editing_entity->movement_cap = entity_editor_state.MovementSpinnerValue;
  strcpy(editing_entity->name_buffer, entity_editor_state.NameTextBoxText);
  editing_entity->faction = entity_editor_state.EntityFactionToggleGroupActive;
  editing_entity->faction = entity_editor_state.EntityFactionToggleGroupActive;
  // Flags
  editing_entity->flags.is_enemyable  = entity_editor_state.EnemyableCheckBoxExChecked;
  editing_entity->flags.is_playable   = entity_editor_state.PlayableCheckBoxExChecked;
  editing_entity->flags.is_flamable   = entity_editor_state.FlamableCheckBoxExChecked;
  editing_entity->flags.is_freezable  = entity_editor_state.FreezableCheckBoxExChecked;
  editing_entity->flags.is_wetable    = entity_editor_state.WetableCheckBoxExChecked;

}

void EditorUpdateFrame(
  World *world,
  Arena *perm_arena, 
  Arena *turn_arena, 
  Arena *temp_arena
) {
  Vector2 mouse_pos = GetMousePosition();
  Vector2 mouse_world_pos = GetScreenToWorld2D(mouse_pos, world->camera);
  WorldCoord mouse_world_coord = WorldCoordFromVector2(mouse_world_pos);
  
  EntityEditorUpdateFrame(world);
  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    Entity *entity_selected = EntityFindByWorldCoord(world->entities, mouse_world_coord);
    if (entity_selected)
      world->selected_entity = entity_selected;
  }
  if (mouse_world_pos.x >= 0 && mouse_world_pos.y >= 0 && mouse_world_pos.x < world->width && mouse_world_pos.y < world->height) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        const U32 index = WorldIndexFromVector2(world, mouse_world_pos);
        world->tiles[index] = Tile_wall;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        const U32 index = WorldIndexFromVector2(world, mouse_world_pos);
        world->tiles[index] = Tile_void;
      }
  }
}

void EditorDraw(void) {

  // DrawText("Editing", 0, 0, 20, WHITE);
  GuiDrawIcon(ICON_FILE_SAVE_CLASSIC, 5, 5, 3, WHITE);
  GuiEntityEditor(&entity_editor_state);
}