/* Stack structure
 *
 * Stored as a void* array along with the number of items, with TOS at the end
 * of the array.
 */
typedef struct {
  void **items;
  unsigned int num_items;
} Stack;

/* Create a new stack */
Stack stack_new();

/* Free a stack */
void stack_free(Stack stack);

/* Push a new item to TOS */
void stack_push(Stack *stack, void *item);

/* Pop an item from TOS, returning it or NULL if the stack is empty */
void *stack_pop(Stack *stack);

/* Swap two items at TOS and TOS+n if TOS+n exists */
void stack_swap(Stack *stack, unsigned int n);

/* Move TOS to bottom */
void stack_roll_top(Stack *stack);

/* Move bottom to TOS */
void stack_roll_bottom(Stack *stack);

/* Return the item at TOS+n, or NULL if it doesn't exist */
void *stack_at(Stack *stack, unsigned int n);
