#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "assg.h"
extern void set_h_max(int h_max);

typedef struct {
    int x, y, w, h;
} Wall;

static void trim_line_endings(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[--len] = '\0';
    }
}

static int manhattan_dist(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

// 利用 parent 指针回溯并打印完整路径
static void print_final_path(Node *goal_node) {
    int path_cap = 256;
    int path_len = 0;
    Node **path = (Node **)malloc(sizeof(Node *) * path_cap);
    if (!path) return;

    Node *curr = goal_node;
    while (curr != NULL) {
        if (path_len >= path_cap) {
            path_cap *= 2;
            Node **new_path = (Node **)realloc(path, sizeof(Node *) * path_cap);
            if (!new_path) {
                free(path);
                return;
            }
            path = new_path;
        }
        path[path_len++] = curr;
        curr = curr->parent;
    }

    printf("Path:\n");
    for (int i = path_len - 1; i >= 0; i--) {
        printf("(%d, %d)", path[i]->x, path[i]->y);
        if (i > 0) printf(" ");
    }
    printf("\n");

    free(path);
}

int main(void) {
    char line[256];
    int start_x = 0, start_y = 0;
    int end_x = 0, end_y = 0;

    // 1. 读取 Start
    printf("Start:\n");
    fflush(stdout);
    if (fgets(line, sizeof(line), stdin) != NULL) {
        sscanf(line, "(%d, %d)", &start_x, &start_y);
    }

    // 2. 读取 End
    printf("End:\n");
    fflush(stdout);
    if (fgets(line, sizeof(line), stdin) != NULL) {
        sscanf(line, "(%d, %d)", &end_x, &end_y);
    }

    // 3. 读取 Walls
    printf("Walls:\n");
    fflush(stdout);

    int wall_count = 0;
    int wall_cap = 16;
    Wall *walls = (Wall *)malloc(sizeof(Wall) * wall_cap);
    if (!walls) return 1;

    long long min_x = (start_x < end_x) ? start_x : end_x;
    long long max_x = (start_x > end_x) ? start_x : end_x;
    long long min_y = (start_y < end_y) ? start_y : end_y;
    long long max_y = (start_y > end_y) ? start_y : end_y;

    while (fgets(line, sizeof(line), stdin) != NULL) {
        trim_line_endings(line);
        if (strcmp(line, "done") == 0) {
            break;
        }
        int wx, wy, ww, wh;
        if (sscanf(line, "(%d, %d), %d, %d", &wx, &wy, &ww, &wh) == 4) {
            if (wall_count >= wall_cap) {
                wall_cap *= 2;
                Wall *new_walls = (Wall *)realloc(walls, sizeof(Wall) * wall_cap);
                if (!new_walls) {
                    free(walls);
                    return 1;
                }
                walls = new_walls;
            }
            walls[wall_count++] = (Wall){wx, wy, ww, wh};

            if (wx < min_x) min_x = wx;
            if (wy < min_y) min_y = wy;
            long long wall_max_x = (long long)wx + ww - 1;
            long long wall_max_y = (long long)wy + wh - 1;
            if (wall_max_x > max_x) max_x = wall_max_x;
            if (wall_max_y > max_y) max_y = wall_max_y;
        }
    }

    // 计算网格尺寸并输出 SW Corner
    int width = (int)(max_x - min_x + 1);
    int height = (int)(max_y - min_y + 1);

    printf("SW Corner:\n");
    printf("(%d, %d), %d, %d\n", (int)min_x, (int)min_y, width, height);
    // 计算包围盒四个外角到终点的最大曼哈顿距离 H_max
    int h_sw = manhattan_dist((int)min_x, (int)min_y, end_x, end_y);
    int h_se = manhattan_dist((int)max_x, (int)min_y, end_x, end_y);
    int h_nw = manhattan_dist((int)min_x, (int)max_y, end_x, end_y);
    int h_ne = manhattan_dist((int)max_x, (int)max_y, end_x, end_y);

    int h_max = h_sw;
    if (h_se > h_max) h_max = h_se;
    if (h_nw > h_max) h_max = h_nw;
    if (h_ne > h_max) h_max = h_ne;
    set_h_max(h_max);

    // 创建二维布尔网格标记墙体
    bool *is_wall = (bool *)calloc((size_t)width * height, sizeof(bool));
    if (!is_wall) {
        free(walls);
        return 1;
    }

    for (int i = 0; i < wall_count; i++) {
        for (int dy = 0; dy < walls[i].h; dy++) {
            for (int dx = 0; dx < walls[i].w; dx++) {
                int gx = (int)((long long)walls[i].x + dx - min_x);
                int gy = (int)((long long)walls[i].y + dy - min_y);
                is_wall[(size_t)gy * width + gx] = true;
            }
        }
    }
    free(walls);

    // 4. 初始化并开始 A* 搜索
    printf("Start Search:\n");

    MinHeap open_list;
    ClosedList closed_list;
    heap_init(&open_list);
    closed_list_init(&closed_list);

    int start_gx = (int)((long long)start_x - min_x);
    int start_gy = (int)((long long)start_y - min_y);
    int end_gx = (int)((long long)end_x - min_x);
    int end_gy = (int)((long long)end_y - min_y);

    if (!is_wall[(size_t)start_gy * width + start_gx] &&
        !is_wall[(size_t)end_gy * width + end_gx]) {
        int h_start = manhattan_dist(start_x, start_y, end_x, end_y);
        Node *start_node = create_node(start_x, start_y, 0, h_start, NULL);
        heap_insert(&open_list, start_node);
    }

    const int dx_dir[4] = {0, 0, -1, 1};
    const int dy_dir[4] = {1, -1, 0, 0};

    Node *goal_node = NULL;

    while (!heap_is_empty(&open_list)) {
        Node *current = heap_extract_min(&open_list);
        closed_list_add(&closed_list, current);

        // 输出本时间步加入 Closed List 的节点
        printf("(%d, %d)\n", current->x, current->y);

        // 检查是否到达终点
        if (current->x == end_x && current->y == end_y) {
            printf("G\n");
            goal_node = current;
            break;
        }

        // 遍历四个邻居
        for (int dir = 0; dir < 4; dir++) {
            long long nx = (long long)current->x + dx_dir[dir];
            long long ny = (long long)current->y + dy_dir[dir];

            if (nx < min_x || nx > max_x || ny < min_y || ny > max_y) {
                continue;
            }

            int ngx = (int)(nx - min_x);
            int ngy = (int)(ny - min_y);

            if (is_wall[(size_t)ngy * width + ngx]) {
                continue;
            }

            if (closed_list_contains(&closed_list, (int)nx, (int)ny)) {
                continue;
            }

            int tentative_g = current->g + 1;
            int open_idx = heap_find(&open_list, (int)nx, (int)ny);

            if (open_idx == -1) {
                int h = manhattan_dist((int)nx, (int)ny, end_x, end_y);
                Node *neighbor = create_node((int)nx, (int)ny, tentative_g, h, current);
                heap_insert(&open_list, neighbor);
            } else if (tentative_g < open_list.data[open_idx]->g) {
                open_list.data[open_idx]->parent = current;
                int h = open_list.data[open_idx]->h;
                heap_decrease_key(&open_list, open_idx, tentative_g, h);
            }
        }

        if (heap_is_empty(&open_list)) {
            printf("F\n");
        } else {
            printf("P\n");
        }
    }

    if (goal_node == NULL && closed_list.size == 0) {
        printf("F\n");
    }

    // 5. 打印最终路径
    if (goal_node != NULL) {
        print_final_path(goal_node);
    }

    // 释放所有内存（Open List 和 Closed List 内部节点随列表一同释放）
    free(is_wall);
    heap_free(&open_list);
    closed_list_free(&closed_list);

    return 0;
}