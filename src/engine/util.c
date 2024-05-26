#include "engine/util.h"
#include "engine/state.h"
#include "map.h"
#include "matrix.h"
#include "vector.h"

#include <stdbool.h>

void insert_stencil_quad(mat4_t transformation) {
  if (stencil_list.head == NULL) {
    stencil_list.head  = malloc(sizeof(stencil_node_t));
    *stencil_list.head = (stencil_node_t){transformation, NULL};

    stencil_list.tail = stencil_list.head;
  } else {
    stencil_list.tail->next  = malloc(sizeof(stencil_node_t));
    *stencil_list.tail->next = (stencil_node_t){transformation, NULL};

    stencil_list.tail = stencil_list.tail->next;
  }
}

sector_t *map_get_sector(vec2_t position) {
  uint16_t id = gl_map.num_nodes - 1;
  while ((id & 0x8000) == 0) {
    if (id > gl_map.num_nodes) { return NULL; }

    gl_node_t *node = &gl_map.nodes[id];

    vec2_t delta      = vec2_sub(position, node->partition);
    bool   is_on_back = (delta.x * node->delta_partition.y -
                       delta.y * node->delta_partition.x) <= 0.f;

    if (is_on_back) {
      id = gl_map.nodes[id].back_child_id;
    } else {
      id = gl_map.nodes[id].front_child_id;
    }
  }

  if ((id & 0x7fff) >= gl_map.num_subsectors) { return NULL; }

  gl_subsector_t *subsector = &gl_map.subsectors[id & 0x7fff];
  gl_segment_t   *segment   = &gl_map.segments[subsector->first_seg];
  linedef_t      *linedef   = &map.linedefs[segment->linedef];

  sidedef_t *sidedef;
  if (segment->side == 0) {
    sidedef = &map.sidedefs[linedef->front_sidedef];
  } else {
    sidedef = &map.sidedefs[linedef->back_sidedef];
  }

  return &map.sectors[sidedef->sector_idx];
}
