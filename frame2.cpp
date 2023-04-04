// author: chatGPT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define BLOCK_SIZE 1024 * 1024 // 每个块的大小为1MB


// 用于表示块的结构体
typedef struct block {
    int id; // 块的ID
    int size; // 块的大小
    char* content; // 块的内容
    char* owner; // 文件的所有者
    bool is_shared; // 是否共享
} Block;

// 用于表示块管理器的结构体
typedef struct block_manager {
    Block** blocks; // 块的数组
    int count; // 块的数量
} BlockManager;

void process_file(char* file_path, BlockManager* block_manager, char* username) {
    // 获取文件的元数据
    struct stat file_stat;
    stat(file_path, &file_stat);
    
    // 获取文件大小
    int file_size = file_stat.st_size;

    // 读取文件内容
    char* file_content = (char*)malloc(file_size);
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        printf("Failed to open file %s\n", file_path);
        free(file_content);
        return;
    }
    fread(file_content, file_size, 1, file);
    fclose(file);

    // 将文件内容存储到块中
    save_to_block(file_content, file_size, block_manager, username);

    // 释放文件内容
    free(file_content);

    // 删除文件
    remove(file_path);
}


void save_to_block(char* file_content, int file_size, BlockManager* block_manager, char* username) {
    // 计算需要的块数量
    int block_count = file_size / BLOCK_SIZE + (file_size % BLOCK_SIZE == 0 ? 0 : 1);

    // 创建块并存储文件内容
    for (int i = 0; i < block_count; i++) {
        Block* block = (Block*)malloc(sizeof(Block));
        block->id = block_manager->count + 1;
        block->size = (i == block_count - 1 ? file_size % BLOCK_SIZE : BLOCK_SIZE);
        block->content = (char*)malloc(block->size);
        memcpy(block->content, file_content + i * BLOCK_SIZE, block->size);
        block->owner = (char*)malloc(strlen(username) + 1);
        strcpy(block->owner, username);
        block->is_shared = false; // 默认不共享

        // 将块添加到块管理器中
        block_manager->count++;
        block_manager->blocks = (Block**)realloc(block_manager->blocks, block_manager->count * sizeof(Block*));
        block_manager->blocks[block_manager->count - 1] = block;

        printf("Block %d saved with size %d, owned by %s\n", block->id, block->size, block->owner);
    }
}

bool authenticate_user(char* username, char* password) {
    // TODO: 实现用户认证逻辑
    return true; // 通过认证
}

void share_file(BlockManager* block_manager, int block_id, char* username, bool is_shared) {
    // 查找块
    Block* block = NULL;
    for (int i = 0; i < block_manager->count; i++) {
        if (block_manager->blocks[i]->id == block_id) {
            block = block_manager->blocks[i];
            break;
        }
    }

    if (block == NULL) {
        printf("Block %d not found\n", block_id);
        return;
    }

    // 检查用户是否具有访问该文件的权限
    if (strcmp(block->owner, username) != 0 && !authenticate_user(username, NULL)) {
        printf("User %s does not have access to block %d\n", username, block_id);
        return;
    }

    // 更新共享信息
    block->is_shared = is_shared;

    printf("Block %d %s\n", block_id, (is_shared ? "shared" : "unshared"));
}

void download_file(BlockManager* block_manager, int block_id, char* username) {
    // 查找块
    Block* block = NULL;
    for (int i = 0; i < block_manager->count; i++) {
        if (block_manager->blocks[i]->id == block_id) {
            block = block_manager->blocks[i];
            break;
        }
    }

    if (block == NULL) {
        printf("Block %d not found\n", block_id);
        return;
    }

    // 检查用户是否具有访问该文件的权限
    if (strcmp(block->owner, username) != 0 && !block->is_shared) {
        printf("User %s does not have access to block %d\n", username, block_id);
        return;
    }

    // 下载文件
    char filename[256];
    snprintf(filename, sizeof(filename), "block%d", block_id); // 根据块ID构造文件名
    FILE* file = fopen(filename, "w");
    fwrite(block->content, block->size, 1, file);
    fclose(file);

    printf("Block %d downloaded\n", block_id);
}
