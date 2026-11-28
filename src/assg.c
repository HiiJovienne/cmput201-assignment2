#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "assg.h"

static int global_h_max = 0;

void set_h_max(int h_max) {
    global_h_max = h_max;
}

// 计算地形权重 w = floor(5 - 4 * (h / H_max))
static int calculate_weight(int h) {
    if (global_h_max <= 0) return 5;
    // 使用整数运算实现精确向下取整：floor(5 - 4h/H_max) = 5 - ceil(4h/H_max)
    int sub = (4 * h + global_h_max - 1) / global_h_max;
    int w = 5 - sub;
    if (w < 1) w = 1;
    if (w > 5) w = 5;
    return w;
}

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

// --- Node Constructor (使用 Weighted A* 公式 f = g + w * h) ---
Node* create_node(int x, int y, int g, int h, Node *parent) {
    Node *node = (Node *)malloc(sizeof(Node));
    if (!node) return NULL;

    int w = calculate_weight(h);
    node->x = x;
    node->y = y;
    node->g = g;
    node->h = h;
    node->f = g + w * h;
    node->parent = parent;
    return node;
}

// --- MinHeap (Open List) Functions ---
void heap_init(MinHeap *heap) {
    if (!heap) return;
    heap->capacity = 256;
    heap->size = 0;
    heap->data = (Node **)malloc(sizeof(Node *) * heap->capacity);
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
    if (!heap || !node) return;
    if (heap->size >= heap->capacity) {
        heap->capacity *= 2;
        Node **new_data = (Node **)realloc(heap->data, sizeof(Node *) * heap->capacity);
        if (!new_data) return;
        heap->data = new_data;
    }

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

    int w = calculate_weight(new_h);
    heap->data[index]->g = new_g;
    heap->data[index]->h = new_h;
    heap->data[index]->f = new_g + w * new_h;

    heapify_up(heap, index);
    heapify_down(heap, index);
}

bool heap_is_empty(const MinHeap *heap) {
    return (!heap || heap->size == 0);
}

void heap_free(MinHeap *heap) {
    if (!heap) return;
    if (heap->data) {
        for (int i = 0; i < heap->size; i++) {
            free(heap->data[i]);
        }
        free(heap->data);
    }
    heap->data = NULL;
    heap->size = 0;
    heap->capacity = 0;
}

// --- Closed List Functions ---
void closed_list_init(ClosedList *list) {
    if (!list) return;
    list->capacity = 256;
    list->size = 0;
    list->data = (Node **)malloc(sizeof(Node *) * list->capacity);
}

void closed_list_add(ClosedList *list, Node *node) {
    if (!list || !node) return;
    if (list->size >= list->capacity) {
        list->capacity *= 2;
        Node **new_data = (Node **)realloc(list->data, sizeof(Node *) * list->capacity);
        if (!new_data) return;
        list->data = new_data;
    }
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
    if (list->data) {
        for (int i = 0; i < list->size; i++) {
            free(list->data[i]);
        }
        free(list->data);
    }
    list->data = NULL;
    list->size = 0;
    list->capacity = 0;
}