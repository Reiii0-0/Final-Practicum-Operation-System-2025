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
void cd(byte* cwd, char* dirname) {}

// TODO: 7. Implement ls function
void ls(byte cwd, char* dirname) {}

// TODO: 8. Implement mv function
void mv(byte cwd, char* src, char* dst) {}

// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst) {}

// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {}

// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {}

