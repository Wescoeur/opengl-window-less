#include <stdio.h>

#include "opengl-window-less.h"

// =============================================================================

int main (void) {
  OwlContext *context = owl_create_context();

  if (!context) {
    fprintf(stderr, "Unable to create context.\n");
    return -1;
  }

  owl_destroy_context(context);

  return 0;
}
