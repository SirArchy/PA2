#include "../lib/operations.h"
#include "../lib/filesystem.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int fs_mkdir(file_system* fs, char* path) {
    // Überprüfen, ob das Dateisystem gültig ist
    if (fs == NULL) {
        return -1;
    }
    
    // Überprüfen, ob der Pfad gültig ist
    if (path == NULL || strlen(path) == 0 || path[0] != '/') {
        return -1;
    }
    
    // Analyse des Pfads, um den übergeordneten Ordner und den Namen des neuen Ordners zu extrahieren
    char parent_path[NAME_MAX_LENGTH];
    char dir_name[NAME_MAX_LENGTH];
    int path_length = strlen(path);
    int last_slash_index = -1;
    
    for (int i = path_length - 1; i >= 0; i--) {
        if (path[i] == '/') {
            last_slash_index = i;
            break;
        }
    }
    
    if (last_slash_index == -1) {
        return -1;
    }
    
    strncpy(parent_path, path, last_slash_index);
    parent_path[last_slash_index] = '\0';
    strncpy(dir_name, path + last_slash_index + 1, NAME_MAX_LENGTH);
    
    // Finden einer freien INode
    int free_inode_index = find_free_inode(fs);
    
    if (free_inode_index == -1) {
        return -1;
    }
    
    // Überprüfen, ob genügend INodes vorhanden sind
    if (free_inode_index >= fs->s_block->num_blocks) {
        return -1;
    }
    
    // Den neuen Ordner im Dateisystem erstellen
    inode* new_dir_inode = &(fs->inodes[free_inode_index]);
    new_dir_inode->n_type = directory;
    strncpy(new_dir_inode->name, dir_name, NAME_MAX_LENGTH);
    new_dir_inode->parent = fs->root_node;  // Derzeit wird der neue Ordner als direktes Kind des Wurzelverzeichnisses eingefügt
    
    // Den neuen Ordner in den übergeordneten Ordner einfügen
    inode* parent_inode = &(fs->inodes[fs->root_node]);
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        if (parent_inode->direct_blocks[i] == -1) {
            parent_inode->direct_blocks[i] = free_inode_index;
            break;
        }
    }
    
    // Den freien Block aktualisieren
    fs->free_list[free_inode_index] = 0;
    fs->s_block->free_blocks--;
    
    // Das Dateisystem speichern
    fs_dump(fs, "filesystem.fs");
    
    return 0;
}









int fs_mkfile(file_system* fs, char* path_and_name) {
    // Überprüfen, ob das Dateisystem gültig ist
    if (fs == NULL) {
        return -1;
    }
    
    // Überprüfen, ob der Pfad gültig ist
    if (path_and_name == NULL || strlen(path_and_name) == 0 || path_and_name[0] != '/') {
        return -1;
    }
    
    // Analyse des Pfads, um den übergeordneten Ordner und den Namen der neuen Datei zu extrahieren
    char parent_path[NAME_MAX_LENGTH];
    char file_name[NAME_MAX_LENGTH];
    int path_length = strlen(path_and_name);
    int last_slash_index = -1;
    
    for (int i = path_length - 1; i >= 0; i--) {
        if (path_and_name[i] == '/') {
            last_slash_index = i;
            break;
        }
    }
    
    if (last_slash_index == -1) {
        return -1;
    }
    
    strncpy(parent_path, path_and_name, last_slash_index);
    parent_path[last_slash_index] = '\0';
    strncpy(file_name, path_and_name + last_slash_index + 1, NAME_MAX_LENGTH);
    
    // Finden einer freien INode
    int free_inode_index = find_free_inode(fs);
    
    if (free_inode_index == -1) {
        return -1;
    }
    
    // Überprüfen, ob genügend INodes vorhanden sind
    if (free_inode_index >= fs->s_block->num_blocks) {
        return -1;
    }
    
    // Die neue Datei im Dateisystem erstellen
    inode* new_file_inode = &(fs->inodes[free_inode_index]);
    new_file_inode->n_type = reg_file;
    strncpy(new_file_inode->name, file_name, NAME_MAX_LENGTH);
    new_file_inode->parent = fs->root_node;
    
    // Die neue Datei in den übergeordneten Ordner einfügen
    inode* parent_inode = &(fs->inodes[fs->root_node]);
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        if (parent_inode->direct_blocks[i] == -1) {
            parent_inode->direct_blocks[i] = free_inode_index;
            break;
        }
    }
    
    // Den freien Block aktualisieren
    fs->free_list[free_inode_index] = 0;
    fs->s_block->free_blocks--;
    
    // Das Dateisystem speichern
    fs_dump(fs, "filesystem.fs");
    
    return 0;
}




char *fs_list(file_system *fs, char *path) {
    // Überprüfen, ob das Dateisystem gültig ist
    if (fs == NULL) {
        return NULL;
    }
    
    // Überprüfen, ob der Pfad gültig ist
    if (path == NULL || strlen(path) == 0 || path[0] != '/') {
        return NULL;
    }
    
    // Den übergeordneten Ordner finden
    inode *parent_inode = NULL;
    if (strcmp(path, "/") == 0) {
        parent_inode = &(fs->inodes[fs->root_node]);
    } else {
        // Analyse des Pfads, um den übergeordneten Ordner zu finden
        char parent_path[NAME_MAX_LENGTH];
        char dir_name[NAME_MAX_LENGTH];
        int path_length = strlen(path);
        int last_slash_index = -1;
        
        for (int i = path_length - 1; i >= 0; i--) {
            if (path[i] == '/') {
                last_slash_index = i;
                break;
            }
        }
        
        if (last_slash_index == -1) {
            return NULL;
        }
        
        strncpy(parent_path, path, last_slash_index);
        parent_path[last_slash_index] = '\0';
        strncpy(dir_name, path + last_slash_index + 1, NAME_MAX_LENGTH);
        
        // Den übergeordneten Ordner finden
        int parent_inode_index = find_inode_by_path(fs, parent_path);
        if (parent_inode_index == -1) {
            return NULL;
        }
        
        parent_inode = &(fs->inodes[parent_inode_index]);
        
        // Überprüfen, ob der übergeordnete Ordner ein Verzeichnis ist
        if (parent_inode->n_type != directory) {
            return NULL;
        }
    }
    
    // Die Dateien und Verzeichnisse im übergeordneten Ordner sammeln und nach INode-Nummer sortieren
    int num_elements = 0;
    int elements_capacity = 10;
    char **elements = malloc(elements_capacity * sizeof(char *));
    
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        int inode_index = parent_inode->direct_blocks[i];
        if (inode_index != -1) {
            inode *element_inode = &(fs->inodes[inode_index]);
            char *element_name = element_inode->name;
            
            char *formatted_element = malloc(NAME_MAX_LENGTH + 5);
            if (element_inode->n_type == directory) {
                snprintf(formatted_element, NAME_MAX_LENGTH + 5, "DIR %s", element_name);
            } else {
                snprintf(formatted_element, NAME_MAX_LENGTH + 5, "FIL %s", element_name);
            }
            
            elements[num_elements] = formatted_element;
            num_elements++;
            
            // Überprüfen, ob die Kapazität der Elemente-Liste erreicht ist und sie bei Bedarf erweitern
            if (num_elements >= elements_capacity) {
                elements_capacity *= 2;
                elements = realloc(elements, elements_capacity * sizeof(char *));
            }
        }
    }
    
    // Die Elemente nach INode-Nummer sortieren
    qsort(elements, num_elements, sizeof(char *), compare_elements);
    
    // Die Namen der Elemente in einen String zusammenführen
    int string_length = 0;
    for (int i = 0; i < num_elements; i++) {
        string_length += strlen(elements[i]) + 1;  // +1 für das Zeilenumbruchzeichen
    }
    
    char *result = malloc(string_length + 1);  // +1 für das Abschlusszeichen
    result[0] = '\0';
    
    for (int i = 0; i < num_elements; i++) {
        strcat(result, elements[i]);
        strcat(result, "\n");
    }
    
    // Die Elemente-Liste und den übergeordneten Ordner freigeben
    free(elements);
    
    return result;
}



int fs_writef(file_system *fs, char *filename, char *text) {
    // Überprüfen, ob das Dateisystem gültig ist
    if (fs == NULL) {
        return -1;
    }
    
    // Überprüfen, ob der Dateiname und der Text gültig sind
    if (filename == NULL || strlen(filename) == 0) {
        return -1;
    }
    
    if (text == NULL) {
        return -1;
    }
    
    // Den übergeordneten Ordner finden
    char parent_path[NAME_MAX_LENGTH];
    char file_name[NAME_MAX_LENGTH];
    int last_slash_index = -1;
    
    int path_length = strlen(filename);
    for (int i = path_length - 1; i >= 0; i--) {
        if (filename[i] == '/') {
            last_slash_index = i;
            break;
        }
    }
    
    if (last_slash_index == -1) {
        strncpy(file_name, filename, NAME_MAX_LENGTH);
        strncpy(parent_path, "/", NAME_MAX_LENGTH);
    } else {
        strncpy(file_name, filename + last_slash_index + 1, NAME_MAX_LENGTH);
        strncpy(parent_path, filename, last_slash_index);
        parent_path[last_slash_index] = '\0';
    }
    
    // Den übergeordneten Ordner finden
    int parent_inode_index = find_inode_by_path(fs, parent_path);
    if (parent_inode_index == -1) {
        printf("Parent directory not found\n");
        return -1;
    }
    
    inode *parent_inode = &(fs->inodes[parent_inode_index]);
    
    // Überprüfen, ob der übergeordnete Ordner ein Verzeichnis ist
    if (parent_inode->n_type != directory) {
        return -1;
    }
    
    // Die Datei in den übergeordneten Ordner finden
    int file_inode_index = find_inode_by_name(fs, parent_inode, file_name);
    if (file_inode_index == -1) {
        return -1;
    }
    
    inode *file_inode = &(fs->inodes[file_inode_index]);
    
    // Überprüfen, ob die INode eine reguläre Datei ist
    if (file_inode->n_type != reg_file) {
        return -1;
    }
    
    // Den Text in die Datei schreiben
    int text_length = strlen(text);
    int remaining_length = text_length;
    int block_index = 0;
    
    while (remaining_length > 0) {
        int data_block_index = file_inode->direct_blocks[block_index];
        
        if (data_block_index == -1) {
            // Finden eines freien Datenblocks
            data_block_index = find_free_block(fs);
            
            if (data_block_index == -1) {
                return -1;
            }
            
            // Den Datenblock in den direkten Blocks der INode speichern
            file_inode->direct_blocks[block_index] = data_block_index;
            
            // Den freien Block in der Freiliste markieren
            fs->free_list[data_block_index] = 0;
            fs->s_block->free_blocks--;
        }
        
        // Den Text in den Datenblock schreiben
        int write_length = remaining_length > BLOCK_SIZE ? BLOCK_SIZE : remaining_length;
        strncpy(fs->data_blocks[data_block_index].block, text + (text_length - remaining_length), write_length);
        remaining_length -= write_length;
        
        // Die Größe der Datei aktualisieren
        file_inode->size += write_length;
        
        block_index++;
    }
    
    // Das Dateisystem speichern
    fs_dump(fs, "filesystem.fs");
    
    return 0;
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