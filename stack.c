#include "stack.h"

#include <stdlib.h>
#include <string.h>

Stack stack_new() { return (Stack){.items = NULL, .num_items = 0}; }

void stack_free(Stack stack) { free(stack.items); }

void stack_push(Stack *stack, void *item) {
  stack->items = realloc(stack->items, (stack->num_items + 1) * sizeof(void *));
  stack->items[stack->num_items++] = item;
}

void *stack_pop(Stack *stack) {
  if (stack->num_items) {
    return stack->items[--stack->num_items];
  }
  return NULL;
}

void stack_swap(Stack *stack, unsigned int n) {
  if (n > 0 && n < stack->num_items) {
    void *tos = stack->items[stack->num_items - 1];
    stack->items[stack->num_items - 1] = stack->items[stack->num_items - 1 - n];
    stack->items[stack->num_items - 1 - n] = tos;
  }
}

void stack_roll_left(Stack *stack) {
  if (stack->num_items > 1) {
    void *tos = stack->items[stack->num_items - 1];
    void **rest_items = stack->items;
    stack->items = malloc(stack->num_items * sizeof(void *));
    memcpy(stack->items + 1, rest_items,
           (stack->num_items - 1) * sizeof(void *));
    free(rest_items);
    stack->items[0] = tos;
  }
}

void stack_roll_right(Stack *stack) {
  if (stack->num_items > 1) {
    void *bottom = stack->items[0];
    void **rest_items = stack->items + 1;
    stack->items = malloc(stack->num_items * sizeof(void *));
    memcpy(stack->items, rest_items, (stack->num_items - 1) * sizeof(void *));
    free(rest_items);
    stack->items[stack->num_items - 1] = bottom;
  }
}

void *stack_at(Stack *stack, unsigned int n) {
  if (n < stack->num_items) {
    return stack->items[stack->num_items - 1 - n];
  }
  return NULL;
}
