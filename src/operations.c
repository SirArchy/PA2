#include "../lib/operations.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

inode *fs_traverse(file_system *fs, char *path)
{
    // Check if the path is absolute
    if (path[0] != '/')
    {
        return NULL; // Invalid path
    }

    // Start traversal from the root node
    inode *current_node = &(fs->inodes[fs->root_node]);

    // Tokenize the path
    char *token = strtok(path, "/");
    while (token != NULL)
    {
        bool found = false;
        int i;
        for (i = 0; i < DIRECT_BLOCKS_COUNT; i++)
        {
            int block_number = current_node->direct_blocks[i];
            if (block_number != -1)
            {
                inode *subdir = &(fs->inodes[block_number]);
                if (subdir->n_type == directory && strcmp(subdir->name, token) == 0)
                {
                    current_node = subdir;
                    found = true;
                    break;
                }
            }
        }

        if (!found)
        {
            return NULL; // Subdirectory not found
        }

        token = strtok(NULL, "/");
    }

    return current_node;
}



int fs_mkdir(file_system *fs, char *path)
{
    // Find the parent directory and the new directory name
    char *parent_path = strdup(path);
    char *new_dir_name = strrchr(parent_path, '/');
    if (new_dir_name == NULL) {
        free(parent_path);
        return -1; // Invalid path
    }
    *new_dir_name = '\0'; // Terminate the parent path
    new_dir_name++; // Move to the new directory name

    // Find the parent directory inode
    inode *parent_dir = fs_traverse(fs, parent_path);
    if (parent_dir == NULL || parent_dir->n_type != directory) {
        free(parent_path);
        return -1; // Parent directory not found or not a directory
    }

    // Check if the directory already exists
    int i;
    for (i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        if (parent_dir->direct_blocks[i] != -1) {
            inode *child = &(fs->inodes[parent_dir->direct_blocks[i]]);
            if (strcmp(child->name, new_dir_name) == 0) {
                free(parent_path);
                return -2; // Directory already exists
            }
        }
    }

    // Find a free inode for the new directory
    int new_dir_inode = find_free_inode(fs);
    if (new_dir_inode == -1) {
        free(parent_path);
        return -1; // No free inode available
    }

    // Initialize the new directory inode
    inode *new_dir = &(fs->inodes[new_dir_inode]);
    inode_init(new_dir);
    new_dir->n_type = directory;
    strncpy(new_dir->name, new_dir_name, NAME_MAX_LENGTH);
    new_dir->parent = (parent_dir - fs->inodes); // Extract the inode number from the address

    // Find the next free block for the new directory inode
    int new_dir_block = find_free_block(fs);
    if (new_dir_block == -1) {
        fs->inodes[new_dir_inode].n_type = free_block;
        free(parent_path);
        return -1; // No free block available
    }

    // Assign the new directory block to the new directory inode
    new_dir->direct_blocks[0] = new_dir_block;

    // Update the parent directory with the new directory inode number
    for (i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        if (parent_dir->direct_blocks[i] == -1) {
            parent_dir->direct_blocks[i] = new_dir_inode;
            break;
        }
    }

    // Update the free block count in the superblock
    fs->s_block->free_blocks--;

    free(parent_path);
    return 0; // Success
}

int find_free_block(file_system *fs)
{
    int i;
    for (i = 0; i < fs->s_block->num_blocks; i++)
    {
        if (fs->free_list[i] == 1)
        {
            fs->free_list[i] = 0; // Mark block as used
            fs->s_block->free_blocks--; // Decrease free block count in superblock
            return i; // Return the block number
        }
    }
    return -1; // No free block available
}




int fs_mkfile(file_system *fs, char *path_and_name)
{
    // Find the parent directory and the new file name
    char *parent_path = strdup(path_and_name);
    char *new_file_name = strrchr(parent_path, '/');
    if (new_file_name == NULL) {
        free(parent_path);
        return -1; // Invalid path
    }
    *new_file_name = '\0'; // Terminate the parent path
    new_file_name++; // Move to the new file name

    // Find the parent directory inode
    inode *parent_dir = fs_traverse(fs, parent_path);
    if (parent_dir == NULL || parent_dir->n_type != directory) {
        free(parent_path);
        return -1; // Parent directory not found or not a directory
    }

    // Check if the file already exists
    int i;
    for (i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        if (parent_dir->direct_blocks[i] != -1) {
            inode *child = &(fs->inodes[parent_dir->direct_blocks[i]]);
            if (strcmp(child->name, new_file_name) == 0) {
                free(parent_path);
                return -2; // File already exists
            }
        }
    }

    // Find a free inode for the new file
    int new_file_inode = find_free_inode(fs);
    if (new_file_inode == -1) {
        free(parent_path);
        return -1; // No free inode available
    }

    // Initialize the new file inode
    inode *new_file = &(fs->inodes[new_file_inode]);
    inode_init(new_file);
    new_file->n_type = reg_file;
    strncpy(new_file->name, new_file_name, NAME_MAX_LENGTH);
    new_file->parent = (parent_dir - fs->inodes); // Extract the inode number from the address

    // Update the parent directory with the new file inode number
    for (i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        if (parent_dir->direct_blocks[i] == -1) {
            parent_dir->direct_blocks[i] = new_file_inode;
            break;
        }
    }

    // Update the free block count in the superblock
    fs->s_block->free_blocks--;

    free(parent_path);
    return 0; // Success
}




char *fs_list(file_system *fs, char *path)
{
    // Find the directory inode
    inode *dir = fs_traverse(fs, path);
    if (dir == NULL || dir->n_type != directory)
    {
        return NULL; // Directory not found or invalid path
    }

    // Count the number of files and directories in the directory
    int count = 0;
    int i;
    for (i = 0; i < DIRECT_BLOCKS_COUNT; i++)
    {
        if (dir->direct_blocks[i] != -1)
        {
            inode *child = &(fs->inodes[dir->direct_blocks[i]]);
            if (child->n_type == directory || child->n_type == reg_file)
            {
                count++;
            }
        }
    }

    // Allocate memory for the result string
    char *result = (char *)malloc((count * (NAME_MAX_LENGTH + 5) + 1) * sizeof(char));
    if (result == NULL)
    {
        return NULL; // Memory allocation failed
    }

    // Create the result string
    count = 0;
    for (i = 0; i < DIRECT_BLOCKS_COUNT; i++)
    {
        if (dir->direct_blocks[i] != -1)
        {
            inode *child = &(fs->inodes[dir->direct_blocks[i]]);
            if (child->n_type == directory)
            {
                snprintf(result + (count * (NAME_MAX_LENGTH + 5)), NAME_MAX_LENGTH + 5, "DIR %s\n", child->name);
                count++;
            }
            else if (child->n_type == reg_file)
            {
                snprintf(result + (count * (NAME_MAX_LENGTH + 5)), NAME_MAX_LENGTH + 5, "FIL %s\n", child->name);
                count++;
            }
        }
    }

    return result;
}




int fs_writef(file_system *fs, char *filename, char *text)
{
    // Find the file inode
    inode *file = fs_traverse(fs, filename);
    if (file == NULL || file->n_type != reg_file)
    {
        return -1; // File not found or invalid path
    }

    // Calculate the number of blocks needed for the text
    int num_blocks = (strlen(text) / BLOCK_SIZE) + 1;

    // Find the free blocks and update the free list
    int *free_blocks = (int *)malloc(num_blocks * sizeof(int));
    if (free_blocks == NULL)
    {
        return -1; // Memory allocation failed
    }

    int count = 0;
    int i;
    for (i = 0; i < fs->s_block->num_blocks; i++)
    {
        if (fs->free_list[i] == 1)
        {
            free_blocks[count] = i;
            fs->free_list[i] = 0;
            fs->s_block->free_blocks--;
            count++;

            if (count == num_blocks)
            {
                break;
            }
        }
    }

    // Write the text to the data blocks
    int remaining_text_size = strlen(text);
    int block_index = 0;
    while (remaining_text_size > 0 && block_index < num_blocks)
    {
        data_block *block = &(fs->data_blocks[free_blocks[block_index]]);
        int write_size = MIN(remaining_text_size, BLOCK_SIZE);
        memcpy(block->block, text, write_size);
        block->size = write_size;
        remaining_text_size -= write_size;
        text += write_size;
        block_index++;
    }

    // Update the file inode
    file->size = strlen(text);
    file->direct_blocks[0] = free_blocks[0];

    free(free_blocks);
    return strlen(text); // Return the number of written chars
}




uint8_t *fs_readf(file_system *fs, char *filename, int *file_size)
{
    // Find the file inode
    inode *file = fs_traverse(fs, filename);
    if (file == NULL || file->n_type != reg_file)
    {
        return NULL; // File not found or invalid path
    }

    // Allocate memory for the buffer
    uint8_t *buffer = (uint8_t *)malloc(file->size * sizeof(uint8_t));
    if (buffer == NULL)
    {
        return NULL; // Memory allocation failed
    }

    // Read the data from the data blocks
    int remaining_size = file->size;
    int buffer_index = 0;
    int block_index = 0;
    while (remaining_size > 0 && block_index < DIRECT_BLOCKS_COUNT)
    {
        int block_number = file->direct_blocks[block_index];
        if (block_number == -1)
        {
            break;
        }

        data_block *block = &(fs->data_blocks[block_number]);
        int read_size = MIN(remaining_size, block->size);
        memcpy(buffer + buffer_index, block->block, read_size);
        buffer_index += read_size;
        remaining_size -= read_size;
        block_index++;
    }

    *file_size = file->size;
    return buffer;
}




int fs_rm(file_system *fs, char *path)
{
    // Find the file or directory inode to be removed
    inode *to_remove = fs_traverse(fs, path);
    if (to_remove == NULL)
    {
        return -1; // File or directory not found
    }

    // Recursive removal for directories
    if (to_remove->n_type == directory)
    {
        // Check if the directory is empty
        int empty = 1;
        int i;
        for (i = 0; i < DIRECT_BLOCKS_COUNT; i++)
        {
            if (to_remove->direct_blocks[i] != -1)
            {
                empty = 0;
                break;
            }
        }

        if (!empty)
        {
            return -1; // Directory not empty, cannot remove
        }

        // Remove the directory entry from the parent directory
        inode *parent_dir = &(fs->inodes[to_remove->parent]);
        for (i = 0; i < DIRECT_BLOCKS_COUNT; i++)
        {
            if (parent_dir->direct_blocks[i] == (to_remove - fs->inodes))
            {
                parent_dir->direct_blocks[i] = -1;
                break;
            }
        }
    }

    // Free the blocks used by the file or directory
    int i;
    for (i = 0; i < DIRECT_BLOCKS_COUNT; i++)
    {
        int block_number = to_remove->direct_blocks[i];
        if (block_number != -1)
        {
            fs->free_list[block_number] = 1;
            fs->s_block->free_blocks++;
            memset(&(fs->data_blocks[block_number]), 0, sizeof(data_block));
        }
    }

    // Mark the inode as free
    inode_init(to_remove);

    return 0; // Success
}




int fs_import(file_system *fs, char *int_path, char *ext_path)
{
    // Find the internal file inode
    inode *int_file = fs_traverse(fs, int_path);
    if (int_file == NULL || int_file->n_type != reg_file)
    {
        return -1; // Internal file not found or invalid path
    }

    // Open the external file for reading
    FILE *ext_file = fopen(ext_path, "rb");
    if (ext_file == NULL)
    {
        return -1; // External file not found or cannot be opened
    }

    // Get the size of the external file
    fseek(ext_file, 0, SEEK_END);
    long ext_file_size = ftell(ext_file);
    fseek(ext_file, 0, SEEK_SET);

    // Check if the external file size exceeds the internal file size
    if (ext_file_size > int_file->size)
    {
        fclose(ext_file);
        return -1; // External file size exceeds internal file size
    }

    // Read the data from the external file and write it to the internal file
    int remaining_size = ext_file_size;
    int buffer_size = BLOCK_SIZE;
    uint8_t *buffer = (uint8_t *)malloc(buffer_size * sizeof(uint8_t));
    if (buffer == NULL)
    {
        fclose(ext_file);
        return -1; // Memory allocation failed
    }

    while (remaining_size > 0)
    {
        int read_size = MIN(remaining_size, buffer_size);
        fread(buffer, sizeof(uint8_t), read_size, ext_file);
        fs_writef(fs, int_path, (char *)buffer);
        remaining_size -= read_size;
    }

    fclose(ext_file);
    free(buffer);
    return 0; // Success
}




int fs_export(file_system *fs, char *int_path, char *ext_path)
{
    // Find the internal file inode
    inode *int_file = fs_traverse(fs, int_path);
    if (int_file == NULL || int_file->n_type != reg_file)
    {
        return -1; // Internal file not found or invalid path
    }

    // Open the external file for writing
    FILE *ext_file = fopen(ext_path, "wb");
    if (ext_file == NULL)
    {
        return -1; // External file cannot be opened for writing
    }

    // Read the data from the internal file and write it to the external file
    int file_size;
    uint8_t *buffer = fs_readf(fs, int_path, &file_size);
    if (buffer == NULL)
    {
        fclose(ext_file);
        return -1; // Failed to read the internal file
    }

    fwrite(buffer, sizeof(uint8_t), file_size, ext_file);

    fclose(ext_file);
    free(buffer);
    return 0; // Success
}
