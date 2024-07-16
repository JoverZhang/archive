#include <fcntl.h>
#include <scsi/sg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

static const int BLOCK_SIZE = 512;
static const int SENSE_BUFFER_SIZE = 512;

int scsi_cmd16_read(char *blkname, int lba, int num_blocks) {
  int fd = open(blkname, O_RDWR);
  if (fd < 0) {
    perror("open blkname");
    return -1;
  }

  uint8_t cmd[16] = {
      0x88, // opcode
      0x00, // flags

      0x00, // local block address
      0x00,
      0x00,
      0x00,
      (uint8_t)(lba >> 24),
      (uint8_t)(lba >> 16),
      (uint8_t)(lba >> 8),
      (uint8_t)(lba),

      (uint8_t)(num_blocks >> 24), // transfer length
      (uint8_t)(num_blocks >> 16),
      (uint8_t)(num_blocks >> 8),
      (uint8_t)num_blocks,

      0x00, // dld & group number
      0x00, // control
  };

  sg_io_hdr_t io_hdr;
  io_hdr.interface_id = 'S';
  io_hdr.cmd_len = sizeof(cmd);
  io_hdr.cmdp = cmd;

  io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
  io_hdr.dxfer_len = num_blocks * BLOCK_SIZE;
  io_hdr.dxferp = malloc(io_hdr.dxfer_len);
  if (!io_hdr.dxferp) {
    goto cleanup;
  }

  io_hdr.mx_sb_len = (uint8_t)SENSE_BUFFER_SIZE;
  io_hdr.sbp = (uint8_t *)malloc(io_hdr.mx_sb_len);
  if (!io_hdr.sbp) {
    goto cleanup;
  }

  io_hdr.timeout = 20000;

  // error
  if (ioctl(fd, SG_IO, &io_hdr) < 0) {
    perror("ioctl");
    for (int i = 0; i < SENSE_BUFFER_SIZE; i++) {
      printf("%02x ", io_hdr.sbp[i]);
    }
    goto cleanup;
  }

  // success
  for (int i = 0; i < num_blocks * BLOCK_SIZE; i++) {
    printf("%02x ", ((uint8_t *)io_hdr.dxferp)[i]);
    // 4K
    if (i % (8 * BLOCK_SIZE) == 0) {
      printf("\n");
    }
  }
  printf("\n");

  free(io_hdr.sbp);
  free(io_hdr.dxferp);
  close(fd);
  return 0;

cleanup:
  if (io_hdr.sbp)
    free(io_hdr.sbp);
  if (io_hdr.dxferp)
    free(io_hdr.dxferp);
  close(fd);
  return -1;
}

int scsi_cmd16_write(char *blkname, int lba, int num_blocks) {
  int fd = open(blkname, O_RDWR);
  if (fd < 0) {
    perror("open blkname");
    return -1;
  }

  uint8_t cmd[16] = {
      0x8A, // opcode
      0x00, // flags

      0x00, // local block address
      0x00,
      0x00,
      0x00,
      (uint8_t)(lba >> 24),
      (uint8_t)(lba >> 16),
      (uint8_t)(lba >> 8),
      (uint8_t)(lba),

      (uint8_t)(num_blocks >> 24), // transfer length
      (uint8_t)(num_blocks >> 16),
      (uint8_t)(num_blocks >> 8),
      (uint8_t)num_blocks,

      0x00, // dld & group number
      0x00, // control
  };

  uint8_t *write_buf = (uint8_t *)malloc(num_blocks * BLOCK_SIZE);
  for (int i = 0; i < num_blocks * BLOCK_SIZE; i++) {
    write_buf[i] = i % 0x100;
  }

  sg_io_hdr_t io_hdr;
  io_hdr.interface_id = 'S';
  io_hdr.cmd_len = sizeof(cmd);
  io_hdr.cmdp = cmd;

  io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
  io_hdr.dxfer_len = num_blocks * BLOCK_SIZE;
  io_hdr.dxferp = write_buf;
  if (!io_hdr.dxferp) {
    goto cleanup;
  }

  io_hdr.mx_sb_len = (uint8_t)SENSE_BUFFER_SIZE;
  io_hdr.sbp = (uint8_t *)malloc(io_hdr.mx_sb_len);
  if (!io_hdr.sbp) {
    goto cleanup;
  }

  io_hdr.timeout = 20000;

  // error
  if (ioctl(fd, SG_IO, &io_hdr) < 0) {
    perror("ioctl");
    for (int i = 0; i < SENSE_BUFFER_SIZE; i++) {
      printf("%02x ", io_hdr.sbp[i]);
    }
    goto cleanup;
  }

  free(io_hdr.sbp);
  free(io_hdr.dxferp);
  close(fd);
  return 0;

cleanup:
  if (io_hdr.sbp)
    free(io_hdr.sbp);
  if (io_hdr.dxferp)
    free(io_hdr.dxferp);
  close(fd);
  return -1;
}

int scsi_mmap_read(char *blkname, int lba, int num_blocks) {
  int fd = open(blkname, O_RDWR);
  if (fd < 0) {
    perror("open blkname");
    return -1;
  }

  uint8_t *mmaped =
      (uint8_t *)mmap(NULL, num_blocks * BLOCK_SIZE, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, lba * BLOCK_SIZE);
  if (mmaped == MAP_FAILED) {
    perror("mmap");
    goto cleanup;
  }

  for (int i = 0; i < num_blocks * BLOCK_SIZE; i++) {
    printf("%02x ", mmaped[i]);
    // 4K
    if (i % (8 * BLOCK_SIZE) == 0) {
      printf("\n");
    }
  }
  printf("\n");

  munmap(mmaped, num_blocks * BLOCK_SIZE);
  close(fd);
  return 0;

cleanup:
  if (mmaped != MAP_FAILED)
    munmap(mmaped, num_blocks * BLOCK_SIZE);
  close(fd);
  return -1;
}

int scsi_mmap_write(char *blkname, int lba, int num_blocks) {
  int fd = open(blkname, O_RDWR);
  if (fd < 0) {
    perror("open blkname");
    return -1;
  }

  uint8_t *mmaped =
      (uint8_t *)mmap(NULL, num_blocks * BLOCK_SIZE, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, lba * BLOCK_SIZE);
  if (mmaped == MAP_FAILED) {
    perror("mmap");
    goto cleanup;
  }

  for (int i = 0; i < num_blocks * BLOCK_SIZE; i++) {
    mmaped[i] = i % 0x100;
  }

  munmap(mmaped, num_blocks * BLOCK_SIZE);
  close(fd);
  return 0;

cleanup:
  if (mmaped != MAP_FAILED)
    munmap(mmaped, num_blocks * BLOCK_SIZE);
  close(fd);
  return -1;
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    fprintf(stderr,
            "Usage: %s <blkname> <local_block_address> <num_blocks> [mmap]\n",
            argv[0]);
    return 1;
  }

  char *blkname = argv[1];
  int lba = atoi(argv[2]);
  int num_blocks = atoi(argv[3]);
  int use_mmap = argc > 4 && strcmp(argv[4], "mmap") == 0;

  if (use_mmap) {
    printf("Using mmap\n");

    if (scsi_mmap_write(blkname, lba, num_blocks)) {
      return -1;
    }

    if (scsi_mmap_read(blkname, lba, num_blocks)) {
      return -1;
    }
  } else {
    printf("Using scsi_cmd16\n");
    fflush(stdout);

    if (scsi_cmd16_write(blkname, lba, num_blocks)) {
      return -1;
    }

    if (scsi_cmd16_read(blkname, lba, num_blocks)) {
      return -1;
    }
  }

  return 0;
}