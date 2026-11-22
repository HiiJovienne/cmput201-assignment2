#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "assg.h"

#define MAX_NODES 100

//避免频繁动态内存分配）
static Node node_pool[MAX_NODES];
static int node_pool_count = 0;

static Node* open_data[MAX_NODES];
static Node* closed_data[MAX_NODES];

// 内部辅助函数 
// 优先级比较：返回 true 表示 a 的优先级高于 b（a 应该更靠近堆顶）
// 依次比较 f -> h -> x -> y，均为较小者优先
static bool has_higher_priority(const Node *a, const Node *b) {
    if (a->f != b->f) return a->f < b->f;
    if (a->h != b->h) return a->h < b->h;
    if (a->x != b->x) return a->x < b->x;
    return a->y < b->y;
}

static void swap_nodes(Node **a, Node **b) {
    Node *temp = *a;
    *a = *b;
    *b = temp;
}

// 去除行尾的 \n 和 \r
static void trim_line_endings(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[--len] = '\0';
    }
}

//  Node Constructor 

Node* create_node(int x, int y, int g, int h, Node *parent) {
    if (node_pool_count >= MAX_NODES) {
        return NULL;
    }
    Node *node = &node_pool[node_pool_count++];
    node->x = x;
    node->y = y;
    node->g = g;
    node->h = h;
    node->f = g + h;
    node->parent = parent;
    return node;
}

// MinHeap (Open List) Functions

void heap_init(MinHeap *heap) {
    if (!heap) return;
    heap->data = open_data;
    heap->size = 0;
    heap->capacity = MAX_NODES;
}

void heapify_up(MinHeap *heap, int index) {
    if (!heap || index <= 0 || index >= heap->size) return;

    while (index > 0) {
        int parent = (index - 1) / 2;
        if (has_higher_priority(heap->data[index], heap->data[parent])) {
            swap_nodes(&heap->data[index], &heap->data[parent]);
            index = parent;
        } else {
            break;
        }
    }
}

void heapify_down(MinHeap *heap, int index) {
    if (!heap || index < 0 || index >= heap->size) return;

    while (true) {
        int smallest = index;
        int left = 2 * index + 1;
        int right = 2 * index + 2;

        if (left < heap->size && has_higher_priority(heap->data[left], heap->data[smallest])) {
            smallest = left;
        }
        if (right < heap->size && has_higher_priority(heap->data[right], heap->data[smallest])) {
            smallest = right;
        }

        if (smallest != index) {
            swap_nodes(&heap->data[index], &heap->data[smallest]);
            index = smallest;
        } else {
            break;
        }
    }
}

void heap_insert(MinHeap *heap, Node *node) {
    if (!heap || !node || heap->size >= heap->capacity) return;

    heap->data[heap->size] = node;
    heap->size++;
    heapify_up(heap, heap->size - 1);
}

Node* heap_extract_min(MinHeap *heap) {
    if (!heap || heap->size == 0) return NULL;

    Node *min_node = heap->data[0];
    heap->size--;

    if (heap->size > 0) {
        heap->data[0] = heap->data[heap->size];
        heapify_down(heap, 0);
    }

    return min_node;
}

int heap_find(const MinHeap *heap, int x, int y) {
    if (!heap) return -1;

    for (int i = 0; i < heap->size; i++) {
        if (heap->data[i]->x == x && heap->data[i]->y == y) {
            return i;
        }
    }
    return -1;
}

void heap_decrease_key(MinHeap *heap, int index, int new_g, int new_h) {
    if (!heap || index < 0 || index >= heap->size) return;

    heap->data[index]->g = new_g;
    heap->data[index]->h = new_h;
    heap->data[index]->f = new_g + new_h;

    heapify_up(heap, index);
    heapify_down(heap, index);
}

bool heap_is_empty(const MinHeap *heap) {
    return (!heap || heap->size == 0);
}

void heap_free(MinHeap *heap) {
    if (!heap) return;
    heap->data = NULL;
    heap->size = 0;
    heap->capacity = 0;
}

// Closed List Functions

void closed_list_init(ClosedList *list) {
    if (!list) return;
    list->data = closed_data;
    list->size = 0;
    list->capacity = MAX_NODES;
}

void closed_list_add(ClosedList *list, Node *node) {
    if (!list || !node || list->size >= list->capacity) return;
    list->data[list->size++] = node;
}

bool closed_list_contains(const ClosedList *list, int x, int y) {
    if (!list) return false;

    for (int i = 0; i < list->size; i++) {
        if (list->data[i]->x == x && list->data[i]->y == y) {
            return true;
        }
    }
    return false;
}

void closed_list_free(ClosedList *list) {
    if (!list) return;
    list->data = NULL;
    list->size = 0;
    list->capacity = 0;
}


int main(void) {
    MinHeap open_list;
    ClosedList closed_list;

    heap_init(&open_list);
    closed_list_init(&closed_list);

    char line[256];
    bool is_inserting = false;

    while (fgets(line, sizeof(line), stdin) != NULL) {
        trim_line_endings(line);

        // 忽略标记行，不产生输出
        if (strcmp(line, "Commands:") == 0) {
            continue;
        }

        // 退出批量插入阶段，不产生输出
        if (strcmp(line, "done") == 0) {
            is_inserting = false;
            continue;
        }

        // 批量插入状态：解析 x, y, g, h 并插入 Open List
        if (is_inserting) {
            int x, y, g, h;
            if (sscanf(line, "%d, %d, %d, %d", &x, &y, &g, &h) == 4) {
                Node *node = create_node(x, y, g, h, NULL);
                if (node != NULL) {
                    heap_insert(&open_list, node);
                }
            }
            continue;
        }

        // 命令分派
        if (strcmp(line, "[O] Insert:") == 0) {
            is_inserting = true;
        } 
        else if (strcmp(line, "[O] Extract") == 0) {
            Node *min_node = heap_extract_min(&open_list);
            if (min_node != NULL) {
                closed_list_add(&closed_list, min_node);
                printf("Extracted Node (%d, %d), f = %d, g = %d, h = %d\n",
                       min_node->x, min_node->y, min_node->f, min_node->g, min_node->h);
            }
        } 
        else if (strncmp(line, "[O] Find", 8) == 0) {
            int x, y;
            if (sscanf(line, "[O] Find (%d, %d)", &x, &y) == 2) {
                int idx = heap_find(&open_list, x, y);
                if (idx != -1) {
                    printf("Node (%d, %d) has been found\n", x, y);
                } else {
                    printf("Node (%d, %d) has not been found\n", x, y);
                }
            }
        } 
        else if (strncmp(line, "[C] Find", 8) == 0) {
            int x, y;
            if (sscanf(line, "[C] Find (%d, %d)", &x, &y) == 2) {
                if (closed_list_contains(&closed_list, x, y)) {
                    printf("Node (%d, %d) has been found\n", x, y);
                } else {
                    printf("Node (%d, %d) has not been found\n", x, y);
                }
            }
        } 
        else if (strcmp(line, "[O] IsEmpty") == 0) {
            printf("%s\n", heap_is_empty(&open_list) ? "True" : "False");
        } 
        else if (strcmp(line, "[C] IsEmpty") == 0) {
            printf("%s\n", (closed_list.size == 0) ? "True" : "False");
        } 
        else if (strncmp(line, "[O] DecreaseKey", 15) == 0) {
            int x, y, new_g;
            if (sscanf(line, "[O] DecreaseKey (%d, %d), new_g = %d", &x, &y, &new_g) == 3) {
                int idx = heap_find(&open_list, x, y);
                if (idx != -1) {
                    int old_h = open_list.data[idx]->h;
                    heap_decrease_key(&open_list, idx, new_g, old_h);
                    int updated_idx = heap_find(&open_list, x, y);
                    if (updated_idx != -1) {
                        Node *updated = open_list.data[updated_idx];
                        printf("Decreased key for Node (%d, %d): f = %d, g = %d, h = %d\n",
                               updated->x, updated->y, updated->f, updated->g, updated->h);
                    }
                }
            }
        }
    }

    heap_free(&open_list);
    closed_list_free(&closed_list);

    return 0;
}