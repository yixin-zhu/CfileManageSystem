#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define BLOCK_SIZE 64
typedef struct block
{
    char *content; // 块的内容
    struct block *nextInfile;
    struct block *nextInBlockManager;
} Block;

typedef struct block_manager
{
    Block *blocks;
    int block_count;
} BlockManager;

typedef struct iNode
{
    char *filename;
    int owner_id;
    Block *blocks;
    struct iNode *next;
    int available;
} INode;

typedef struct iNode_manager
{
    INode *iNodes;
    int iNode_count;
} INodeManager;

typedef struct iNode_link
{
    INode *node;
} INodeLink;

typedef struct user
{
    int u_id;
    char *username;
    INode *owned_files;
    INodeLink *shared_files;
} User;

typedef struct user_manager
{
    User *users;
    int user_count;
} UserManager;

// -------------------------------------------------------------------------------------------
void addNewUser(char *userName, UserManager *um)
{
    int index = um->user_count;
    um->users = (User *)realloc(um->users, sizeof(User) * (index + 1));
    um->users[index].username = (char *)malloc(sizeof(char) * (strlen(userName) + 1));
    strcpy(um->users[index].username, userName);
    um->users[index].u_id = index;
    um->users[index].owned_files = NULL;
    um->users[index].shared_files = NULL;
    um->user_count++;
}

void printAllUsers(UserManager *um)
{
    for (int i = 0; i < um->user_count; i++)
    {
        printf("User %d: %s\n", um->users[i].u_id, um->users[i].username);
    }
}

int findUserIdByName(char *userName, UserManager *um)
{
    for (int i = 0; i < um->user_count; i++)
    {
        if (strcmp(um->users[i].username, userName) == 0)
        {
            return um->users[i].u_id;
        }
    }
    return -1;
}

struct block *saveFileToBlocks(char *composeFileName, BlockManager *bm);
void saveFileMetaToInode(char *composeFileName, char *fileName, char *userDir, UserManager *um, INodeManager *im, BlockManager *bm)
{
    int index = im->iNode_count;
    im->iNodes = (INode *)realloc(im->iNodes, sizeof(INode) * (index + 1));
    im->iNodes[index].filename = (char *)malloc(sizeof(char) * (strlen(fileName) + 1));
    strcpy(im->iNodes[index].filename, fileName);
    if (index > 0)
    {
        im->iNodes[index - 1].next = &im->iNodes[index];
    }

    im->iNodes[index].owner_id = findUserIdByName(userDir, um);
    im->iNodes[index].blocks = saveFileToBlocks(composeFileName, bm);
    im->iNodes[index].next = NULL;
    im->iNodes[index].available = 1;
    im->iNode_count++;
}

void saveUserFiles(char *workdir, char *userDir, UserManager *um, BlockManager *bm, INodeManager *im)
{
    addNewUser(userDir, um);
    char *composeDir = (char *)malloc(sizeof(char) * (strlen(workdir) + strlen(userDir) + 2));
    strcpy(composeDir, workdir);
    strcat(composeDir, "/");
    strcat(composeDir, userDir);
    DIR *directory = opendir(composeDir);
    if (directory == NULL)
    {
        printf("In saveUserFiles. Can't open this user dir.\n");
    }
    else
    {
        struct dirent *entry;
        while ((entry = readdir(directory)) != NULL)
        {
            if (entry->d_type == DT_REG)
            {
                char *composeFileName = (char *)malloc(sizeof(char) * (strlen(composeDir) + strlen(entry->d_name) + 2));
                strcpy(composeFileName, composeDir);
                strcat(composeFileName, "/");
                strcat(composeFileName, entry->d_name);
                saveFileMetaToInode(composeFileName, entry->d_name, userDir, um, im, bm);
            }
            else
            {
                // printf("Not File: %s\n", entry->d_name);
            }
        }
    }
}

struct block *saveFileToBlocks(char *composeFileName, BlockManager *bm)
{
    FILE *fp = fopen(composeFileName, "r");
    Block *headBlock = NULL;
    if (fp == NULL)
    {
        printf("In saveFileToBlocks. Can't open this file.\n");
    }
    else
    {
        char *content = (char *)malloc(sizeof(char) * BLOCK_SIZE);
        int index = bm->block_count;
        int flag = 0;
        while (fgets(content, BLOCK_SIZE, fp) != NULL)
        {
            bm->blocks = (Block *)realloc(bm->blocks, sizeof(Block) * (index + 1));
            bm->blocks[index].content = (char *)malloc(sizeof(char) * (strlen(content) + 1));
            strcpy(bm->blocks[index].content, content);
            bm->blocks[index].nextInfile = NULL;
            bm->blocks[index].nextInBlockManager = NULL;
            if (index > 0)
            {
                bm->blocks[index - 1].nextInBlockManager = &bm->blocks[index];
            }
            if (flag > 0)
            {
                bm->blocks[index - 1].nextInfile = &bm->blocks[index];
            }
            else
            {
                headBlock = &bm->blocks[index];
                flag = 1;
            }
            index++;
        }
        bm->block_count = index;
    }
    return headBlock;
}

void saveWorkDir(char *workDir, UserManager *userManager, BlockManager *blockManager, INodeManager *inodeManager)
{
    DIR *directory = opendir(workDir);
    if (directory == NULL)
    {
        printf("In saveWorkDir. Can't open this work dir.\n");
    }
    else
    {
        struct dirent *entry;
        while ((entry = readdir(directory)) != NULL)
        {
            if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            {
                saveUserFiles(workDir, entry->d_name, userManager, blockManager, inodeManager);
            }
            else
            {
                // printf("Not user: %s\n", entry->d_name);
            }
        }
    }
}

Block *findBlockHeadByFileName(int user_id, char *fileName, INodeManager *im)
{
    for (int i = 0; i < im->iNode_count; i++)
    {
        if (strcmp(im->iNodes[i].filename, fileName) == 0 && im->iNodes[i].owner_id == user_id)
        {
            return im->iNodes[i].blocks;
        }
    }
    return NULL;
}


void printFile(char *workDir, char *userName, char *fileName, UserManager *um, INodeManager *im)
{
    int user_id = findUserIdByName(userName, um);
    Block *blockHead = findBlockHeadByFileName(user_id, fileName, im);
    if (blockHead == NULL)
    {
        printf("In printFile. Can't find this file.\n");
    }
    else
    {
        Block *block = blockHead;
        printf("head is not null\n");
        while (block != NULL)
        {
            printf("c");
            printf("bc:%s",block->content);
            block = block->nextInfile;
        }
        printf("d");
    }
}

void printAllBlocks(BlockManager *bm)
{
    printf("BlockManager:\n");
    for (int i = 0; i < bm->block_count; i++)
    {
        printf("Block %d: %s\n", i, bm->blocks[i].content);
    }
}

void printAllInodes(INodeManager *im)
{
    printf("INodeManager:\n");
    for (int i = 0; i < im->iNode_count; i++)
    {
        printf("INode %d: %s\n", i, im->iNodes[i].filename);
    }
}

int main()
{
    char *workDir = "work";
    BlockManager blockManager = {NULL, 0};
    UserManager userManager = {NULL, 0};
    INodeManager inodeManager = {NULL, 0};
    saveWorkDir(workDir, &userManager, &blockManager, &inodeManager);
    printAllUsers(&userManager);
    printAllBlocks(&blockManager);
    printAllInodes(&inodeManager);
    // printFile(workDir, "0tEsKcHiy_enkQjo2Jpr", "abc", &userManager, &inodeManager);

    return 0;
}
