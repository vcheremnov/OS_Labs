#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "fileutils.h"

#define OPEN_ERROR (-1)
#define READ_ERROR (-1)
#define LSEEK_ERROR (-1)
#define CLOSE_ERROR (-1)

#define BUFFER_SIZE BUFSIZ

static char buf[BUFFER_SIZE];

int open_file(char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == OPEN_ERROR) {
        fprintf(stderr, "Failed to open %s: %s", filename, strerror(errno));
        return FAILURE_CODE;
    }
    return fd;
}

int rewind_file(int fd) {
    static const off_t BEGIN_FILE_POS = 0;

    off_t returnVal = lseek(fd, BEGIN_FILE_POS, SEEK_SET);
    if (returnVal == LSEEK_ERROR) {
        perror("rewind_file(..) error");
        return FAILURE_CODE;
    }

    return SUCCESS_CODE;
}

void close_file(int fd, char *filename) {
    int returnCode = close(fd);
    if (returnCode == CLOSE_ERROR) {
        fprintf(stderr, "Failed to close %s: %s", filename, strerror(errno));
    }
}

int fill_size_table(int fd, size_t *lineSizes, int tableMaxSize) {
    static const char LINE_END_CHAR = '\n';

    ssize_t readNum = 0;
    size_t lineSize = 0;
    int lineNo = 0;

    while ((readNum = read(fd, buf, BUFFER_SIZE)) != 0) {
        if (readNum == READ_ERROR) {
            perror("fill_line_tables(..) error while reading from a file");
            return FAILURE_CODE;
        }
        if (lineNo >= tableMaxSize) {
            fprintf(stderr, "fill_line_tables(..) error: "
                            "number of lines exceeded maximum table size\n");
            return FAILURE_CODE;
        }
        for (int bufPos = 0; bufPos < readNum; ++bufPos) {
            if (buf[bufPos] == LINE_END_CHAR) {
                lineSizes[lineNo++] = lineSize + 1;
                lineSize = 0;
                continue;
            }
            ++lineSize;
        }
    }
    if (lineSize > 0) {
        lineSizes[lineNo++] = lineSize;
    }

    rewind_file(fd);

    return lineNo;
}

void fill_offset_table(off_t *lineOffsets, size_t *lineSizes, int tableSize) {
    off_t offset = 0;
    for (int lineNo = 0; lineNo < tableSize; ++lineNo) {
        lineOffsets[lineNo] = offset;
        offset += lineSizes[lineNo];
    }
}

int print_line_from_file(int fd, off_t pos, size_t lineSize) {
    ssize_t readNum = 0;
    size_t charsRemained = lineSize, charsToRead = 0;
    while (charsRemained > 0) {
        charsToRead = charsRemained < BUFFER_SIZE ? charsRemained : BUFFER_SIZE;
        readNum = pread(fd, buf, charsToRead, pos);
        if (readNum == READ_ERROR) {
            perror("print_line_from_file(..) error while reading from a file");
            return FAILURE_CODE;
        }
        if (readNum == 0) {
            fprintf(stderr, "print_line_from_file(..) error: end of file has been reached\n");
            return FAILURE_CODE;
        }
        write(STDOUT_FILENO, buf, (size_t)readNum);
        charsRemained -= (size_t)readNum;
    }
    return SUCCESS_CODE;
}
