// PART basic setting and struct
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define BLOCK_SIZE 64
#define SUB_BLOCK_MANAGER_NUM 3

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
    SubBlockManager *submanagers[SUB_BLOCK_MANAGER_NUM];
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
    int permission;
    struct iNode_link *nextLink;
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
// PART tool functions
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

// -------------------------------------------------------------------------------------------
// PART inner function
struct user *findUserByUserId(int userid, UserManager *um)
{
    User *user = um->users;
    while (user != NULL)
    {
        if (user->u_id == userid)
        {
            return user;
        }
        user = user->nextUser;
    }
    return NULL;
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

void printFileFromHeadBlock(Block *b, char *fileName)
{
    FILE *fp = fopen(concatThreeStr("download", "/", fileName), "w");
    while (b != NULL)
    {
        fprintf(fp, "%s", b->content);
        b = b->nextInfile;
    }
    fclose(fp);
}


Block *findBlockHeadByFileName(int user_id, char *fileName, INodeManager *im)
{
    INode *iNode = im->iNodes;
    while (iNode != NULL)
    {
        if (iNode->owner_id == user_id && strcmp(iNode->filename, fileName) == 0)
        {
            if (iNode->available == 0)
            {
                return NULL;
            }
            return iNode->blocks;
        }
        iNode = iNode->nextINode;
    }
    return NULL;
}

// -------------------------------------------------------------------------------
// PART outer functions
void changeFIleAvailability(char *user, char *fileName, UserManager *um, INodeManager *im, int newState)
{
    int user_id = findUserIdByName(user, um);
    INode *iNode = im->iNodes;
    while (iNode != NULL)
    {
        if (iNode->owner_id == user_id && strcmp(iNode->filename, fileName) == 0)
        {
            iNode->available = newState;
            if(newState == 0){
                printf("File %s of %s deleted.\n", fileName, user);
            }
            else {
                printf("File %s of %s recovered.\n", fileName, user);
            }
            return;
        }
        iNode = iNode->nextINode;
    }
    printf("No such a file.\n");
}

int userExists(char *loginName, UserManager *um)
{
    User *user = um->users;
    while (user != NULL)
    {
        if (strcmp(user->username, loginName) == 0)
        {
            return 1;
        }
        user = user->nextUser;
    }
    return 0;
}

void shareFile(char *userDir, char *fileName, char *targetUser, UserManager *um, INodeManager *im)
{
    int ownerId = findUserIdByName(userDir, um);
    int targetId = findUserIdByName(targetUser, um);
    if (ownerId < 0 || targetId < 0)
    {
        printf("Target user doesn't exist. Can't share file.\n");
        return;
    }
    User *sharedUser = findUserByUserId(targetId, um);
    INodeLink *link = sharedUser->shared_files;
    while (link != NULL)
    {
        if (strcmp(link->node->filename, fileName) == 0 && link->node->owner_id == ownerId)
        {
            link->permission = 1;
            printf("%s shares file %s with %s successfully.\n", userDir, fileName, targetUser);
            return;
        }
        link = link->nextLink;
    }

    INode *node = im->iNodes;
    while (node != NULL)
    {
        if (strcmp(node->filename, fileName) == 0 && node->owner_id == ownerId)
        {
            INodeLink *newLink = (INodeLink *)malloc(sizeof(INodeLink));
            newLink->node = node;
            newLink->permission = 1;
            User *sharedUser = findUserByUserId(targetId, um);
            newLink->nextLink = sharedUser->shared_files;
            sharedUser->shared_files = newLink;
            printf("%s shares file %s with %s successfully.\n", userDir, fileName, targetUser);
            break;
        }
        node = node->nextINode;
    }
}

void unshareFile(char *userDir, char *fileName, char *targetUser, UserManager *um, INodeManager *im)
{
    int ownerId = findUserIdByName(userDir, um);
    int targetId = findUserIdByName(targetUser, um);
    if (ownerId < 0 || targetId < 0)
    {
        printf("Can't find this user. Can't unshare.\n");
        return;
    }
    User *sharedUser = findUserByUserId(targetId, um);
    INodeLink *link = sharedUser->shared_files;
    while (link != NULL)
    {
        if (strcmp(link->node->filename, fileName) == 0 && link->node->owner_id == ownerId)
        {
            link->permission = 0;
            printf("%s unshares file %s with %s successfully.\n", userDir, fileName, targetUser);
            return;
        }
        link = link->nextLink;
    }
    printf("Can't find this file. Can't unshare.\n");
}

void borrowFile(char *userDir, char *ownerName, char *fileName, UserManager *um, INodeManager *im)
{
    int ownerId = findUserIdByName(ownerName, um);
    int borrowerId = findUserIdByName(userDir, um);
    if (ownerId < 0 || borrowerId < 0)
    {
        printf("The Owner doesn't exist. Can't borrow file!\n");
        return;
    }
    User *borrower = findUserByUserId(borrowerId, um);
    INodeLink *link = borrower->shared_files;
    while (link != NULL)
    {
        if (strcmp(link->node->filename, fileName) == 0 && link->node->owner_id == ownerId)
        {
            if (link->permission == 0)
            {
                printf("You don't have permission. Can't borrow file!\n");
                return;
            }
            else if (link->node->available == 0)
            {
                printf("No such a file. Can't borrow file.\n");
                return;
            }
            else
            {
                printFileFromHeadBlock(link->node->blocks, fileName);
                printf("%s successfully borrows file %s from %s\n", userDir, fileName, ownerName);
                return;
            }
            break;
        }
        link = link->nextLink;
    }
    printf("No such a file. Can't borrow file.\n");
}


void printFile(char *userName, char *fileName, UserManager *um, INodeManager *im)
{
    int user_id = findUserIdByName(userName, um);
    Block *blockHead = findBlockHeadByFileName(user_id, fileName, im);
    if (blockHead == NULL)
    {
        printf("No such a file. Can't download.\n");
        return;
    }
    else
    {
        Block *block = blockHead;
        FILE *fp = fopen(concatThreeStr("download", "/", fileName), "w");
        while (block != NULL)
        {
            fprintf(fp, "%s", block->content);
            block = block->nextInfile;
        }
        fprintf(fp, "\n");
        fclose(fp);
    }
    printf("download file %s of %s finished.\n", fileName, userName);
}

// ---------------------------------------------------------------------------
// PART admin functions
void printAllUsers(UserManager *um)
{
    User *user = um->users;
    while (user != NULL)
    {
        printf("User: %s, id: %d\n", user->username, user->u_id);
        user = user->nextUser;
    }
}


void lookupFile(char *ownerName, char *fileName, UserManager *um, INodeManager *im)
{
    printFile(ownerName, fileName, um, im);
}

// ------------------------------------------------------------------------------------
// PART initial and save functions
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
        int last = 0;
        while (fgets(content, BLOCK_SIZE, fp) != NULL)
        {
            Block *newBlock = (Block *)malloc(sizeof(Block));
            newBlock->content = assignValue(newBlock->content, content);
            newBlock->nextInfile = NULL;
            int current = rand() % SUB_BLOCK_MANAGER_NUM;
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
            last = current;
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

void initBlockManager(BlockManager *bm)
{
    for (int i = 0; i < SUB_BLOCK_MANAGER_NUM; i++)
    {
        bm->submanagers[i] = (SubBlockManager *)malloc(sizeof(SubBlockManager));
        bm->submanagers[i]->blocks = NULL;
    }
    bm->block_count = 0;
}

// ---------------------------------------------------------------------
// PART backdoors function
void printAllBlocks(BlockManager *bm)
{
    printf("BlockManager:\n");
    for (int i = 0; i < SUB_BLOCK_MANAGER_NUM; i++)
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

// -------------------------------------------------------------------------------------------
// PART main
void interact(UserManager *um, BlockManager *bm, INodeManager *im, char *workDir)
{
    const char s[2] = " ";
    char str[100];
    char *userName = "log out";
    while (1)
    {
        printf("Please input your command: ");
        scanf("%[^\n]", str);
        char *command = strtok(str, s);
        if (strcmp(command, "login") == 0)
        {
            char *loginName = strtok(NULL, s);
            if (userExists(loginName, um) == 0 && strcmp(loginName, "admin") != 0)
            {
                printf("User name doesn't exist. Can't login.\n");
                getchar();
                continue;
            }
            userName = assignValue(userName, loginName);
            printf("You have logged in as %s.\n", userName);
        }
        else if (strcmp(command, "download") == 0)
        {
            char *fileName = strtok(NULL, s);
            printf("begin to download file %s of %s\n", fileName, userName);
            printFile(userName, fileName, um, im);
        }
        else if (strcmp(command, "delete") == 0)
        {
            char *fileName = strtok(NULL, s);
            printf("begin to delete file %s of %s\n", fileName, userName);
            changeFIleAvailability(userName, fileName, um, im, 0);
        }
        else if (strcmp(command, "recover") == 0)
        {
            char *fileName = strtok(NULL, s);
            printf("begin to recover file %s of %s\n", fileName, userName);
            changeFIleAvailability(userName, fileName, um, im, 1);
        }
        else if (strcmp(command, "borrow") == 0)
        {
            char *fileName = strtok(NULL, s);
            char *ownerName = strtok(NULL, s);
            printf("begin to borrow file %s from %s\n", fileName, userName);
            borrowFile(userName, ownerName, fileName, um, im);
        }
        else if (strcmp(command, "share") == 0)
        {
            char *fileName = strtok(NULL, s);
            char *shareUserName = strtok(NULL, s);
            printf("%s begins to share file %s with %s\n", userName, fileName, shareUserName);
            shareFile(userName, fileName, shareUserName, um, im);
        }
        else if (strcmp(command, "unshare") == 0)
        {
            char *fileName = strtok(NULL, s);
            char *shareUserName = strtok(NULL, s);
            printf("%s begins to unshare file %s with %s\n", userName, fileName, shareUserName);
            unshareFile(userName, fileName, shareUserName, um, im);
        }
        else if (strcmp(command, "lookup") == 0)
        {
            if (strcmp(userName, "admin") == 0)
            {
                char *fileName = strtok(NULL, s);
                char *ownerName = strtok(NULL, s);
                lookupFile(ownerName, fileName, um, im);
            }
            else
            {
                printf("You are not admin. You can't show users.\n");
            }
        }
        else if (strcmp(command, "showusers") == 0)
        {
            if (strcmp(userName, "admin") == 0)
            {
                printAllUsers(um);
            }
            else
            {
                printf("You are not admin. You can't show users.\n");
            }
        }
        else if (strcmp(command, "help") == 0)
        {
            printf("Common Users:\n");
            printf("login [username]\n");
            printf("download [filename]\n");
            printf("borrow [filename] [username] \n");
            printf("share [filename] [username]\n");
            printf("unshare [filename] [username]\n");
            printf("delete [filename] \n");
            printf("recover [filename]\n");
            printf("exit\n");

            printf("Admin only:\n");
            printf("lookup [filename] [username]\n");
            printf("showusers\n");

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
    BlockManager blockManager = {{NULL}, 0};
    UserManager userManager = {NULL, 0};
    INodeManager inodeManager = {NULL, 0};
    initBlockManager(&blockManager);
    saveWorkDir(workDir, &userManager, &blockManager, &inodeManager);
    // printFile("lihua", "abc", &userManager, &inodeManager);
    interact(&userManager, &blockManager, &inodeManager, workDir);
    /*
    printAllBlocks(&blockManager);
    printAllInodes(&inodeManager);
    */
    return 0;
}
