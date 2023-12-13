#include "card_types.h" 
#include "rayutil.h"
// #include "card.h"

#include <string.h>
#include <raymath.h>

#define EachNode(node) void *__next__ = node; node; node = __next__, __next__ = node->next 



// this function needs lots of testing
void CardListPopAppend(CardList *destination_list, CardList *source_list, U16 count) {

  Card *first = source_list->last;
  Card *last = source_list->last;
  int i = 0;

  while (first && i < count) {

    // I am reset the screen coords to the bottom right 
    // because I dont really know where else to do that
    first->screen_position = (Vector2){
      .x = GetScreenWidth(),
      .y = GetScreenHeight(),
    };

    i += 1;
    if (i < count)
    first = first->prev;
  }



  // TODO what if the card is the first item;
  Card *prev = first->prev;
  prev->next = NULL;
  first->prev = NULL;

  source_list->last = prev;


  if (destination_list->last) {
    Card *previous_last = destination_list->last;
    previous_last->next = first;
    first->prev = previous_last;
  } else {
    destination_list->first = first;
  }
  
  destination_list->last = last;

  destination_list->count += i + 1;
  source_list->count -= i + 1;

}


void CardDraw(Card *card) {
  Rectangle rect = (Rectangle){
    .width  = DefaultCardSize.x,
    .height = DefaultCardSize.y,
    .x  = card->screen_position.x,
    .y = card->screen_position.y,
  };
  DrawRectangleRec(rect, RAYWHITE);
  DrawRectangleLinesEx(rect, 2, BLACK);
  
  Rectangle description_rect = (Rectangle){
    .x = rect.x + 15,
    .y = rect.y + 100,
    .width = DefaultCardSize.x - 30,
    .height = rect.y + DefaultCardSize.y - 5,
  };
  DrawText(card_archetypes[card->data].name, card->screen_position.x + 10, card->screen_position.y + 10, 20, BLACK);
  DrawTextInRectangle(GetFontDefault(), card_archetypes[card->data].description, description_rect, 15, BLACK);
}



void CardListHandDraw(CardList *hand) {
  Card *node = hand->first;
  const I32 count = hand->count;
  const I32 hand_layout_width = (GetScreenWidth())/3;
  const I32 hand_layout_x = GetScreenWidth()/6;

  U32 hand_position = 0;
  // for (EachNode(node)) {
  while (node) {
    CardDraw(node);
    
    node->screen_position = Vector2Lerp(
      node->screen_position,
      (Vector2){hand_layout_x + (hand_layout_width/count) * hand_position, GetScreenHeight() - 100},
      2.0 * GetFrameTime()
    );

    hand_position++;
    node = node->next;
  }
  
};

CardList *CardListInit(Arena *arena, U32 count) {
  CardList *deck = ArenaPush(arena, sizeof(CardList));
  if (count == 0) {
    memset(deck, 0, sizeof(CardList));
    return deck;
  }

  Card *cards = ArenaPush(arena, sizeof(Card) * count);

  deck->count = count;
  deck->first = &cards[0];
  deck->last = &cards[count - 1];
  for (size_t i = 0; i < count; i++) 
  {



    cards[i].data = GetRandomValue(0, MAX_CARD_ARCHETYPES - 1);

    if (i > 0)
      cards[i].prev = &cards[i - 1];
    if (i < count - 1)
      cards[i].next =  &cards[i + 1];
  } 

  return deck;
}

// This function takes a 
void CardListShuffle(Arena *temp_arena, CardList *list) {
  TempArena temp = TempArenaInit(temp_arena);
  Card **card_p_list = ArenaPushNoZero(temp_arena, sizeof(Card*) * list->count); 
  
  // We put all the pointers in a temporary list
  Card *node = list->first;
  for (size_t i = 0; i < list->count; i++) {
    Card *next = node->next;

    card_p_list[i] = node;

    node = next;
  }

  // Shuffle all the pointers
  for (size_t i = 0; i < list->count; i++) {
    int index = GetRandomValue(0, list->count - 1);

    Card *swap = card_p_list[index];
    card_p_list[index] = card_p_list[i];
    card_p_list[i] = swap;

  }

  // Use Array Ordering to update links
  Card *first = card_p_list[0];
  Card *last = card_p_list[list->count - 1];
  for (size_t i = 0; i < list->count; i++) {   
    node = card_p_list[i];

    if (i > 0)
      node->prev = card_p_list[i - 1];
    else
      node->prev = NULL;
    
    if (i < list->count - 1)
      node->next = card_p_list[i + 1];
    else 
      node->next = NULL;

  }


  list->first = first;
  list->last = last;


  TempArenaDeinit(temp);
} 

void CardListAppend(CardList *list, Card *card) {
  if (list->last) {
    Card *last = list->last;
    last->next = card;
    card->prev = last;
    list->last = card;
    list->count += 1;
  } else {
    list->last = card;
    list->first = card;
    list->count = 1;
  }
}

// please make sure when calling function that the card 
// is in the list.
Card *CardListRemove(CardList *list, Card* card) {
  Card *next = card->next;
  Card *prev = card->prev;
  card->next = NULL;
  card->prev = NULL;

  list->count -= 1;

  if (next && prev) {
    prev->next = next;
    next->prev = prev;
  } else if (next) {
    // so if this is the start of the list
    next->prev = NULL;
    list->first = next;
  } else if (prev) {
    // if this is the last card in the list;
    prev->next = NULL;
    list->last = prev;
  } else {
    // if this is the only item in the list
    list->first = NULL;
    list->last = NULL;
  }


  return card;
}