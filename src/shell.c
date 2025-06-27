#include "shell.h"
#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void shell() {
  char buf[64];
  char cmd[64];
  char arg[2][64];

  byte cwd = FS_NODE_P_ROOT;

  while (true) {
    printString("MengOS:");
    printCWD(cwd);
    printString("$ ");
    readString(buf);
    parseCommand(buf, cmd, arg);

    if (strcmp(cmd, "cd")) cd(&cwd, arg[0]);
    else if (strcmp(cmd, "ls")) ls(cwd, arg[0]);
    else if (strcmp(cmd, "mv")) mv(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cp")) cp(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cat")) cat(cwd, arg[0]);
    else if (strcmp(cmd, "mkdir")) mkdir(cwd, arg[0]);
    else if (strcmp(cmd, "clear")) clearScreen();
    else printString("Invalid command\n");
  }
}

// TODO: 4. Implement printCWD function
void printCWD(byte cwd) {
    struct node_fs node_fs_buf;
    byte dir_index_stack[FS_MAX_NODE];
    int depth = 0;
    int k;
    char full_path[256];
    int len = 0;

    // Inisialisasi full_path dengan null
    for (k = 0; k < sizeof(full_path); k++) {
        full_path[k] = 0;
    }

    // Load struktur node
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);

    // Kalau root, langsung print "/"
    if (cwd == FS_NODE_P_ROOT || cwd == 0xFF) {
        printString("/");
        return;
    }

    // Simpan urutan direktori ke dalam stack
    while (cwd != FS_NODE_P_ROOT && cwd != 0xFF && depth < FS_MAX_NODE) {
        dir_index_stack[depth++] = cwd;
        cwd = node_fs_buf.nodes[cwd].parent_index;
    }

    // Tambahkan karakter slash pertama
    full_path[len++] = '/';

    // Keluarkan stack untuk membangun path
    while (depth > 0) {
        byte current = dir_index_stack[--depth];
        char* dir_name = node_fs_buf.nodes[current].node_name;

        for (k = 0; dir_name[k] != '\0'; k++) {
            full_path[len++] = dir_name[k];
        }

        if (depth > 0) {
            full_path[len++] = '/';
        }
    }

    // Null-terminate dan print hasilnya
    full_path[len] = '\0';
    printString(full_path);
}


// TODO: 5. Implement parseCommand function
void parseCommand(char* buf, char* cmd, char arg[2][64]) {
    int segment = 0;        // 0 = cmd, 1 = arg[0], 2 = arg[1]
    int pos = 0;            // posisi karakter di segmen
    int i;

    // Inisialisasi semua string ke kosong
    for (i = 0; i < 64; i++) {
        cmd[i] = '\0';
        arg[0][i] = '\0';
        arg[1][i] = '\0';
    }

    i = 0;
    while (buf[i] != '\0') {
        if (buf[i] == ' ') {
            // Skip multiple spaces
            if (segment < 2 && pos > 0) {
                segment++;
                pos = 0;
            }
            i++;
            continue;
        }

        if (segment == 0 && pos < 63) {
            cmd[pos++] = buf[i];
        } else if (segment == 1 && pos < 63) {
            arg[0][pos++] = buf[i];
        } else if (segment == 2 && pos < 63) {
            arg[1][pos++] = buf[i];
        }

        i++;
    }
}


// TODO: 6. Implement cd function
void cd(byte* cwd, char* dirname) {
    struct node_fs snapshot;
    readSector(&snapshot.nodes[0], FS_NODE_SECTOR_NUMBER);
    readSector(&snapshot.nodes[32], FS_NODE_SECTOR_NUMBER + 1);

    if (strcmp(dirname, "/") == 0){
        *cwd = FS_NODE_P_ROOT;
        return;
    }

    if (strcmp(dirname, "..") == 0){
        if (*cwd != FS_NODE_P_ROOT){
            *cwd = snapshot.nodes[*cwd].parent_index;
        }
        return;
    }

    for (int i = 0; i < FS_MAX_NODE; i++){
        struct node_entry entry = snapshot.nodes[i];
        if (entry.parent_index == *cwd && strcmp(entry.node_name, dirname) == 0 && entry.data_index == FS_NODE_D_DIR){
            *cwd = i;
            return;
        }
    }

    printString("Error: directory not found\n");
}

// TODO: 7. Implement ls function
void ls(byte cwd, char* dirname) {
    struct node_fs snapshot;
    readSector(&snapshot.nodes[0], FS_NODE_SECTOR_NUMBER);
    readSector(&snapshot.nodes[32], FS_NODE_SECTOR_NUMBER + 1);
    byte target_dir = cwd;

    if (!(strcmp(dirname, ".") == 0 || strcmp(dirname, "") == 0)){
        int found = 0;
        for (int i=0; i < FS_MAX_NODE; i++){
            struct node_entry entry = snapshot.nodes[i];
            if (entry.parent_index == cwd && entry.data_index == FS_NODE_D_DIR && strcmp(entry.node_name, dirname) == 0){
                target_dir = i;
                found = 1;
                break;
            }
        }
        if (!found){
            printString("Error: Directory not found\n");
            return;
        }
    }

    for (int i=0; i < FS_MAX_NODE; i++){
        struct node_entry item = snapshot.nodes[i];
        if (item.parent_index == target_dir){
            printString(item.node_name);
            printString(" ");
        }
    }

    printString("\n");
}

// TODO: 8. Implement mv function
void mv(byte cwd, char* src, char* dst) {
    struct node_fs fs_buf;
    int src_index = -1, target_parent = -1, slash_pos = -1;
    char dst_dir[MAX_FILENAME];
    char new_name[MAX_FILENAME];
    readSector(&fs_buf.nodes[0], FS_NODE_SECTOR_NUMBER);
    readSector(&fs_buf.nodes[32], FS_NODE_SECTOR_NUMBER + 1);

    for (int i = 0; i < FS_MAX_NODE; i++){
        if (fs_buf.nodes[i].parent_index == cwd && strcmp(fs_buf.nodes[i].node_name, src) == 0){
            src_index = i;
            break;
        }
    }

    if (src_index == -1){
        printString("Error: Source file not found\n");
        return;
    }

    if (fs_buf.nodes[src_index].data_index == FS_NODE_D_DIR){
        printString("Error: Cannot move directory\n");
        return;
    }

    int len = strlen(dst);
    for (int i = 0; i < len; i++){
        if (dst[i] == '/') slash_pos = i;
    }

    if (dst[0] == '/') {
        target_parent = FS_NODE_P_ROOT;
        if (len > 1) {
            strcpy(new_name, dst + 1);
        } else {
            strcpy(new_name, src);
        }
    }else if (strncmp(dst, "../", 3) == 0){
        target_parent = fs_buf.nodes[cwd].parent_index;
        strcpy(new_name, dst + 3);
    }else if (slash_pos != -1){
        strncpy(dst_dir, dst, slash_pos);
        dst_dir[slash_pos] = '\0';

        for (int i = 0; i < FS_MAX_NODE; i++) {
            if (fs_buf.nodes[i].parent_index == cwd && strcmp(fs_buf.nodes[i].node_name, dst_dir) == 0 && fs_buf.nodes[i].data_index == FS_NODE_D_DIR){
                target_parent = i;
                break;
            }
        }

        if (target_parent == -1) {
            printString("Error: Destination path not found\n");
            return;
        }

        strcpy(new_name, dst + slash_pos + 1);
    } else {
        target_parent = cwd;
        strcpy(new_name, dst);
    }

    if (strlen(new_name) == 0) {
        printString("Error: Invalid output name\n");
        return;
    }

    strcpy(fs_buf.nodes[src_index].node_name, new_name);
    fs_buf.nodes[src_index].parent_index = target_parent;

    writeSector(&fs_buf.nodes[0], FS_NODE_SECTOR_NUMBER);
    writeSector(&fs_buf.nodes[32], FS_NODE_SECTOR_NUMBER + 1);

    printString("File moved successfully\n");
}

// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst) {
    struct node_fs fs_buf;
    struct file_metadata src_meta, dst_meta;
    enum fs_return read_status, write_status;

    int src_idx = -1, slash_pos = -1;
    byte dst_parent = cwd;
    char dir_name[MAX_FILENAME];
    char out_name[MAX_FILENAME];
    readSector(&fs_buf.nodes[0], FS_NODE_SECTOR_NUMBER);
    readSector(&fs_buf.nodes[32], FS_NODE_SECTOR_NUMBER + 1);

    for (int i=0; i < FS_MAX_NODE; i++){
        if (fs_buf.nodes[i].parent_index == cwd && strcmp(fs_buf.nodes[i].node_name, src) == 0){
            src_idx = i;
            break;
        }
    }

    if (src_idx == -1){
        printString("Error: Source file not found\n");
        return;
    }

    if (fs_buf.nodes[src_idx].data_index == FS_NODE_D_DIR){
        printString("Error: Cannot copy directory\n");
        return;
    }

    int len = strlen(dst);
    for (int i = 0; i < len; i++){
        if (dst[i] == '/') slash_pos = i;
    }

    if (dst[0] == '/'){
        dst_parent = FS_NODE_P_ROOT;
        if (len > 1) strcpy(out_name, dst + 1);
        else strcpy(out_name, src);
    }else if (strncmp(dst, "../", 3) == 0){
        dst_parent = fs_buf.nodes[cwd].parent_index;
        strcpy(out_name, dst + 3);
    }else if (slash_pos != -1){
        strncpy(dir_name, dst, slash_pos);
        dir_name[slash_pos] = '\0';
        int found = 0;
        for (int i = 0; i < FS_MAX_NODE; i++){
            if (fs_buf.nodes[i].parent_index == cwd && strcmp(fs_buf.nodes[i].node_name, dir_name) == 0 && fs_buf.nodes[i].data_index == FS_NODE_D_DIR){
                dst_parent = i;
                found = 1;
                break;
            }
        }
        if (!found){
            printString("Error: Destination directory not found\n");
            return;
        }
        strcpy(out_name, dst + slash_pos + 1);
        
    } else {
        strcpy(out_name, dst);
    }

    if (strlen(out_name) == 0){
        printString("Error: Invalid destination name\n");
        return;
    }

    memset(&src_meta, 0, sizeof(struct file_metadata));
    strcpy(src_meta.node_name, fs_buf.nodes[src_idx].node_name);
    src_meta.parent_index = fs_buf.nodes[src_idx].parent_index;

    fsRead(&src_meta, &read_status);
    if (read_status != FS_SUCCESS) {
        printString("Error: Failed to read source file\n");
        return;
    }

    memset(&dst_meta, 0, sizeof(struct file_metadata));
    strcpy(dst_meta.node_name, out_name);
    dst_meta.parent_index = dst_parent;
    memcpy(dst_meta.buffer, src_meta.buffer, src_meta.filesize);
    dst_meta.filesize = src_meta.filesize;

    fsWrite(&dst_meta, &write_status);
    if (write_status != FS_SUCCESS) {
        printString("Error: Failed to write destination file\n");
        return;
    }

    printString("File copied successfully\n");
}

// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {
    struct file_metadata file_info;
    enum fs_return status;

    memset(&file_info, 0, sizeof(struct file_metadata));
    file_info.parent_index = cwd;
    strcpy(file_info.node_name, filename);
    fsRead(&file_info, &status);

    if (status == FS_SUCCESS){
        printString(file_info.buffer);
        printString("\n");
    } else if (status == FS_R_NODE_NOT_FOUND){
        printString("File not found\n");
    } else if (status == FS_R_TYPE_IS_DIRECTORY){
        printString("Cannot display a directory\n");
    } else {
        printString("An error occurred while reading the file\n");
    }
}

// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {
    struct node_fs fs_buf;
    int empty_slot = -1;
    readSector(&fs_buf.nodes[0], FS_NODE_SECTOR_NUMBER);
    readSector(&fs_buf.nodes[32], FS_NODE_SECTOR_NUMBER + 1);

    for (int i=0; i < FS_MAX_NODE; i++){
        if (fs_buf.nodes[i].parent_index == cwd && strcmp(fs_buf.nodes[i].node_name, dirname) == 0){
            printString("Error: directory already exists\n");
            return;
        }
    }

    for (int i=0; i < FS_MAX_NODE; i++){
        if (fs_buf.nodes[i].node_name[0] == '\0'){
            empty_slot = i;
            break;
        }
    }

    if (empty_slot == -1) {
        printString("Error: no space for new directory\n");
        return;
    }

    fs_buf.nodes[empty_slot].parent_index = cwd;
    strcpy(fs_buf.nodes[empty_slot].node_name, dirname);
    fs_buf.nodes[empty_slot].data_index = FS_NODE_D_DIR;

    writeSector(&fs_buf.nodes[0], FS_NODE_SECTOR_NUMBER);
    writeSector(&fs_buf.nodes[32], FS_NODE_SECTOR_NUMBER + 1);

    printString("Directory created successfully\n");
}
