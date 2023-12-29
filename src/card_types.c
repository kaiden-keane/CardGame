#include "card_types.h"


const CardData card_archetypes[MAX_CARD_ARCHETYPES] =  {
  {
    .name = "Battle Inspiration",
    .cost = 0,
    .description = "Target one of your characters to gain another action.",
    .card_type = CardTypes_skill,
  },
  {
    .name = "Health Potion",
    .cost = 1,
    .description = "Give player more health",
    .card_type = CardType_item,
    .heal_self_amount = 10,
    .flags = CardFlags_heal_self,
  },
  {
    .name = "Array of arrows",
    .cost = 2,
    .description = "shoot shott array",
    .card_type = CardType_attack,
    .damage_target_amount = 10,
    .flags = CardFlags_damage_target,
  }
};

void PlayCard(World *world, Entity *target) {
  CardData card_data = card_archetypes[world->grabbing_card->data];

  if (card_data.flags & CardFlags_heal_self) {
    world->selected_entity->health += card_data.heal_self_amount;
    if (world->selected_entity->health > world->selected_entity->health_cap)
      world->selected_entity->health = world->selected_entity->health_cap;
  }
  if (card_data.flags & CardFlags_damage_self) {
    if (world->selected_entity->health >= card_data.damage_self_amount)
      world->selected_entity->health -= card_data.damage_self_amount;
    else world->selected_entity->health = 0;
  }
  if (card_data.flags & CardFlags_heal_target) {
    if (target->health > target->health_cap)
    target->health += card_data.heal_target_amount;
    if (target->health > target->health_cap)
      target->health = target->health_cap;
  }
  if (card_data.flags & CardFlags_damage_target) {
    if (target->health > card_data.damage_target_amount)
      target->health -= card_data.damage_target_amount;
    else target->health = 0;
  }
}