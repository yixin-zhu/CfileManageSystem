#ifdef DEBUG
#define debug(str) printf("%s\n", str)
#else
#define debug(str) /* do nothing */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define BLOCK_SIZE 64
#define SUB_BLOCK_MANAGWER_NUM 3

typedef struct block
{
    char *content; // 块的内容
    struct block *nextInfile;
    struct block *nextInBlockManager;
} Block;

typedef struct sub_block_manager
{
    Block *blocks;
} SubBlockManager;

typedef struct block_manager
{
    SubBlockManager *submanagers[SUB_BLOCK_MANAGWER_NUM];
    int block_count;
} BlockManager;

typedef struct iNode
{
    char *filename;
    int owner_id;
    Block *blocks;
    struct iNode *nextINode;
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
    INode *owned_files; // not in use now
    INodeLink *shared_files;
    struct user *nextUser;
} User;

typedef struct user_manager
{
    User *users;
    int user_count;
} UserManager;

// -------------------------------------------------------------------------------------------
char *concatTwoStr(char *str1, char *str2)
{
    char *result = malloc(strlen(str1) + strlen(str2) + 1); //+1 for the zero-terminator
    strcpy(result, str1);
    strcat(result, str2);
    return result;
}

char *concatThreeStr(char *str1, char *str2, char *str3)
{
    char *result = malloc(strlen(str1) + strlen(str2) + strlen(str3) + 1); //+1 for the zero-terminator
    strcpy(result, str1);
    strcat(result, str2);
    strcat(result, str3);
    return result;
}

char *assignValue(char *str, char *value)
{
    str = (char *)malloc(sizeof(char) * (strlen(value) + 1));
    strcpy(str, value);
    return str;
}

void addNewUser(char *userName, UserManager *um)
{
    int index = um->user_count;
    User *newUser = (User *)malloc(sizeof(User));
    newUser->username = assignValue(newUser->username, userName);
    newUser->u_id = index;
    newUser->owned_files = NULL;
    newUser->shared_files = NULL;
    newUser->nextUser = um->users;
    um->users = newUser;
    um->user_count++;
}

void printAllUsers(UserManager *um)
{
    User *user = um->users;
    while (user != NULL)
    {
        printf("User: %s, id: %d\n", user->username, user->u_id);
        user = user->nextUser;
    }
}

int findUserIdByName(char *userName, UserManager *um)
{
    User *user = um->users;
    while (user != NULL)
    {
        if (strcmp(user->username, userName) == 0)
        {
            return user->u_id;
        }
        user = user->nextUser;
    }
    return -1;
}

struct block *saveFileToBlocks(char *composeFileName, BlockManager *bm)
{
    FILE *fp = fopen(composeFileName, "rb");
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
            Block *newBlock = (Block *)malloc(sizeof(Block));
            newBlock->content = assignValue(newBlock->content, content);
            newBlock->nextInfile = NULL;
            int last = (index + SUB_BLOCK_MANAGWER_NUM - 1) % SUB_BLOCK_MANAGWER_NUM;
            int current = index % SUB_BLOCK_MANAGWER_NUM;
            if (flag > 0)
            {
                bm->submanagers[last]->blocks->nextInfile = newBlock;
            }
            else
            {
                headBlock = newBlock;
                flag = 1;
            }
            newBlock->nextInBlockManager = bm->submanagers[current]->blocks;
            bm->submanagers[current]->blocks = newBlock;
            index++;
        }
        bm->block_count = index;
    }
    return headBlock;
}

void saveFileMetaToInode(char *composeFileName, char *fileName, char *userDir, UserManager *um, INodeManager *im, BlockManager *bm)
{
    int index = im->iNode_count;
    INode *newINode = (INode *)malloc(sizeof(INode));
    newINode->filename = assignValue(newINode->filename, fileName);
    newINode->owner_id = findUserIdByName(userDir, um);
    newINode->blocks = saveFileToBlocks(composeFileName, bm);
    newINode->available = 1;
    newINode->nextINode = im->iNodes;
    im->iNodes = newINode;
    im->iNode_count++;
}

void saveUserFiles(char *workdir, char *userDir, UserManager *um, BlockManager *bm, INodeManager *im)
{
    addNewUser(userDir, um);
    char *composeDir = concatThreeStr(workdir, "/", userDir);
    DIR *directory = opendir(composeDir);
    if (directory == NULL)
    {
        printf("In saveUserFiles. Can't open the user's dir.\n");
    }
    else
    {
        struct dirent *entry;
        while ((entry = readdir(directory)) != NULL)
        {
            if (entry->d_type == DT_REG)
            {
                char *composeFileName = concatThreeStr(composeDir, "/", entry->d_name);
                saveFileMetaToInode(composeFileName, entry->d_name, userDir, um, im, bm);
            }
            else
            {
                // printf("Not File: %s\n", entry->d_name);
            }
        }
    }
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
    INode *iNode = im->iNodes;
    while (iNode != NULL)
    {
        if (iNode->owner_id == user_id && strcmp(iNode->filename, fileName) == 0)
        {
            return iNode->blocks;
        }
        iNode = iNode->nextINode;
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
        while (block != NULL)
        {
            printf("%s", block->content);
            block = block->nextInfile;
        }
        printf("\n");
    }
}

void printAllBlocks(BlockManager *bm)
{
    printf("BlockManager:\n");
    for (int i = 0; i < SUB_BLOCK_MANAGWER_NUM; i++)
    {
        printf("SubBlockManager%d:\n", i);
        Block *block = bm->submanagers[i]->blocks;
        int count = 0;
        while (block != NULL)
        {
            printf("Block%d: %s\n", count, block->content);
            block = block->nextInBlockManager;
            count++;
        }
    }
}

void printAllInodes(INodeManager *im)
{
    printf("INodeManager:\n");
    INode *iNode = im->iNodes;
    int count = 0;
    while (iNode != NULL)
    {
        printf("INode%d: %s\n", count, iNode->filename);
        iNode = iNode->nextINode;
        count++;
    }
}

void initBlockManager(BlockManager *bm)
{
    for (int i = 0; i < SUB_BLOCK_MANAGWER_NUM; i++)
    {
        bm->submanagers[i] = (SubBlockManager *)malloc(sizeof(SubBlockManager));
        bm->submanagers[i]->blocks = NULL;
    }
    bm->block_count = 0;
}

void interact(UserManager *um, BlockManager *bm, INodeManager *im, char *workDir)
{
    const char s[2] = " ";
    char str[100];
    char *userName;
    while (1)
    {
        printf("Please input your command: ");
        scanf("%[^\n]", str);
        char *command = strtok(str, s);
        if (strcmp(command, "login") == 0)
        {
            userName = strtok(NULL, s);
            printf("You have logged in as %s.\n", userName);
        }
        else if (strcmp(command, "getfile") == 0)
        {
            char *userName = strtok(NULL, s);
            char *fileName = strtok(NULL, s);
            printFile(workDir, userName, fileName, um, im);
        }
        else if (strcmp(command, "help") == 0)
        {
            printf("HELP:\n");
        }
        else if (strcmp(command, "exit") == 0)
        {
            printf("Goodbye!");
            break;
        }
        else
        {
            printf("Wrong command! Please enter again!\n");
        }
        getchar();
    }
}

int main()
{
    char *workDir = "work";
    BlockManager blockManager = {NULL, 0};
    UserManager userManager = {NULL, 0};
    INodeManager inodeManager = {NULL, 0};
    initBlockManager(&blockManager);
    saveWorkDir(workDir, &userManager, &blockManager, &inodeManager);

    interact(&userManager, &blockManager, &inodeManager, workDir);
    /*
    
    printAllUsers(&userManager);
    printAllBlocks(&blockManager);
    printAllInodes(&inodeManager);
    printFile(workDir, "0tEsKcHiy_enkQjo2Jpr", "abc", &userManager, &inodeManager);
    */
    return 0;
}
