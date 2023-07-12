#include "../lib/operations.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



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
	new_dir->parent = fs_get_inode_number(fs, parent_dir);

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


int
fs_mkfile(file_system *fs, char *path_and_name)
{
	return -1;
}

char *
fs_list(file_system *fs, char *path)
{
	return NULL;
}

int
fs_writef(file_system *fs, char *filename, char *text)
{
	return -1;
}

uint8_t *
fs_readf(file_system *fs, char *filename, int *file_size)
{
	return NULL;	
}


int
fs_rm(file_system *fs, char *path)
{
	return -1;
}

int
fs_import(file_system *fs, char *int_path, char *ext_path)
{
	return -1;
}

int
fs_export(file_system *fs, char *int_path, char *ext_path)
{
	return -1;
}
