// author: chatGPT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define BLOCK_SIZE 1024 * 1024 // 每个块的大小为1MB

// 用于表示块的结构体
typedef struct block
{
    int id;        // 块的ID
    int size;      // 块的大小
    char *content; // 块的内容
} Block;

// 用于表示块管理器的结构体
typedef struct block_manager
{
    Block **blocks; // 块的数组
    int count;      // 块的数量
} BlockManager;

// 监视目录并处理上传的文件的函数
void watch_directory(char *directory_path, BlockManager *block_manager);

// 处理上传的文件的函数
void process_file(char *file_path, BlockManager *block_manager);

// 将文件内容存储到块中的函数
void save_to_block(char *file_content, int file_size, BlockManager *block_manager);

int main()
{
    // 创建块管理器
    BlockManager *block_manager = (BlockManager *)malloc(sizeof(BlockManager));
    block_manager->count = 0;
    block_manager->blocks = NULL;

    // 监视目录并处理上传的文件
    char *directory_path = "/path/to/watch";
    watch_directory(directory_path, block_manager);

    // 释放块管理器
    for (int i = 0; i < block_manager->count; i++)
    {
        free(block_manager->blocks[i]->content);
        free(block_manager->blocks[i]);
    }
    free(block_manager->blocks);
    free(block_manager);

    return 0;
}

void watch_directory(char *directory_path, BlockManager *block_manager)
{
    // 打开目录
    DIR *directory = opendir(directory_path);
    if (directory == NULL)
    {
        printf("Failed to open directory %s\n", directory_path);
        return;
    }

    // 遍历目录中的文件和子目录
    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            // 忽略当前目录和父目录
            continue;
        }

        // 构造完整路径
        char sub_path[256];
        snprintf(sub_path, sizeof(sub_path), "%s/%s", directory_path, entry->d_name);

        if (entry->d_type == DT_DIR)
        {
            // 如果是子目录，递归处理子目录
            watch_directory(sub_path, block_manager);
        }
        else if (entry->d_type == DT_REG)
        {
            // 如果是普通文件，处理文件
            process_file(sub_path, block_manager);
        }
    }

    // 关闭目录
    closedir(directory);
}

void process_file(char *file_path, BlockManager *block_manager)
{
    // 打开文件
    FILE *file = fopen(file_path, "r");
    if (file == NULL)
    {
        printf("Failed to open file %s\n", file_path);
        return;
    }

    // 获取文件大小
    struct stat file_stat;
    stat(file_path, &file_stat);
    int file_size = file_stat.st_size;

    // 读取文件内容
    char *file_content = (char *)malloc(file_size);
    fread(file_content, file_size, 1, file);
    fclose(file);

    // 将文件内容存储到块中
    save_to_block(file_content, file_size, block_manager);

    // 释放文件内容
    free(file_content);

    // 删除文件
    remove(file_path);
}

void save_to_block(char *file_content, int file_size, BlockManager *block_manager)
{
    // 计算需要的块数量
    int block_count = file_size / BLOCK_SIZE + (file_size % BLOCK_SIZE == 0 ? 0 : 1);

    // 创建块并存储文件内容
    for (int i = 0; i < block_count; i++)
    {
        Block *block = (Block *)malloc(sizeof(Block));
        block->id = block_manager->count + 1;
        block->size = (i == block_count - 1 ? file_size % BLOCK_SIZE : BLOCK_SIZE);
        block->content = (char *)malloc(block->size);
        memcpy(block->content, file_content + i * BLOCK_SIZE, block->size);

        // 将块添加到块管理器中
        block_manager->count++;
        block_manager->blocks = (Block **)realloc(block_manager->blocks, block_manager->count * sizeof(Block *));
        block_manager->blocks[block_manager->count - 1] = block;

        printf("Block %d saved with size %d\n", block->id, block->size);
    }
}