#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void fsInit() {
  struct map_fs map_fs_buf;
  int idx = 0;

  readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
  for (idx = 0; idx < 16; idx++) map_fs_buf.is_used[idx] = true;
  for (idx = 256; idx < 512; idx++) map_fs_buf.is_used[idx] = true;
  writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
}

// TODO: 2. Implement fsRead function
void fsRead(struct file_metadata* metadata, enum fs_return* status) {
  struct node_fs node_fs_buf;
  struct data_fs data_fs_buf;
  int i, node_found = -1;

  readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name) == 0 &&
        node_fs_buf.nodes[i].parent_index == metadata->parent_index) {
      node_found = i;
      break;
    }
  }

  if (node_found == -1) {
    *status = FS_R_NODE_NOT_FOUND;
    return;
  }

  if (node_fs_buf.nodes[node_found].data_index == FS_NODE_D_DIR) {
    *status = FS_R_TYPE_IS_DIRECTORY;
    return;
  }

  metadata->filesize = 0;
  for (i = 0; i < FS_MAX_SECTOR; i++) {
    byte sid = data_fs_buf.datas[node_fs_buf.nodes[node_found].data_index].sectors[i];
    if (sid == 0x00) break;
    readSector(metadata->buffer + i * SECTOR_SIZE, sid);
    metadata->filesize += SECTOR_SIZE;
  }

  *status = FS_SUCCESS;
}

// TODO: 3. Implement fsWrite function
void fsWrite(struct file_metadata* metadata, enum fs_return* status) {
  struct map_fs map_fs_buf;
  struct node_fs node_fs_buf;
  struct data_fs data_fs_buf;

  int i, j;
  int node_slot = -1;
  int data_slot = -1;
  int empty_blocks = 0;
  int needed_blocks = (metadata->filesize + SECTOR_SIZE - 1) / SECTOR_SIZE;

  readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
  readSector(&(data_fs_buf.datas[0]), FS_DATA_SECTOR_NUMBER);

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name) == 0 &&
        node_fs_buf.nodes[i].parent_index == metadata->parent_index) {
      *status = FS_W_NODE_ALREADY_EXISTS;
      return;
    }
    if (node_slot == -1 && strlen(node_fs_buf.nodes[i].node_name) == 0) {
      node_slot = i;
    }
  }

  if (node_slot == -1) {
    *status = FS_W_NO_FREE_NODE;
    return;
  }

  for (i = 0; i < FS_MAX_DATA; i++) {
    if (data_fs_buf.datas[i].sectors[0] == 0) {
      data_slot = i;
      break;
    }
  }

  if (data_slot == -1 && metadata->filesize > 0) {
    *status = FS_W_NO_FREE_DATA;
    return;
  }

  for (i = 0; i < SECTOR_SIZE; i++) {
    if (!map_fs_buf.is_used[i]) {
      empty_blocks++;
    }
  }

  if (empty_blocks < needed_blocks) {
    *status = FS_W_NOT_ENOUGH_SPACE;
    return;
  }

  strcpy(node_fs_buf.nodes[node_slot].node_name, metadata->node_name);
  node_fs_buf.nodes[node_slot].parent_index = metadata->parent_index;
  node_fs_buf.nodes[node_slot].data_index =
    (metadata->filesize == 0) ? FS_NODE_D_DIR : data_slot;

  if (metadata->filesize > 0) {
    j = 0;
    for (i = 0; i < SECTOR_SIZE && j < needed_blocks; i++) {
      if (!map_fs_buf.is_used[i]) {
        writeSector(metadata->buffer + j * SECTOR_SIZE, i);
        data_fs_buf.datas[data_slot].sectors[j] = i;
        map_fs_buf.is_used[i] = true;
        j++;
      }
    }
  }

  writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
  writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
  writeSector(&(data_fs_buf.datas[0]), FS_DATA_SECTOR_NUMBER);

  *status = FS_SUCCESS;
}
