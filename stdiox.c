#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h> /* DON'T FORGET TO REMOVE */

int int_length(int value)
{
    int len = 0;

    if (value == 0) {
        return 1;
    }

    if (value < 0) {
        len++;
        value = -value;
    }

    while (value != 0) {
        len++;
        value /= 10;
    }

    return len;
}

void int_to_str(int value, char* buffer)
{
    char temp[12]; // Buffer to store the temporary result (max length: -2147483648)
    int i = 0;
    int is_negative = 0;

    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    if (value < 0) {
        is_negative = 1;
        value = -value;
    }

    while (value != 0) {
        temp[i++] = '0' + value % 10;
        value /= 10;
    }

    if (is_negative) {
        temp[i++] = '-';
    }

    temp[i] = '\0';

    // Reverse the string and copy it to the buffer
    for (int j = 0; j < i; j++) {
        buffer[j] = temp[i - j - 1];
    }
    buffer[i] = '\0';
}

int fprintfx(char* filename, char format, void* data)
{
    int fd;
    char* str;
    int len;
    int ret = 0;

    // Check if data pointer is null
    if (data == NULL) {
        errno = EIO;
        return -1;
    }

    // Determine length of data
    if (format == 'd') {
        len = int_length(*(int*)data);
    } else if (format == 's') {
        len = strlen((char*)data);
    } else {
        errno = EIO;
        return -1;
    }

    // Allocate memory for string
    str = (char*)malloc(len + 1);
    if (str == NULL) {
        errno = EIO;
        return -1;
    }

    // Convert data to string
    if (format == 'd') {
        int_to_str(*(int*)data, str);
    } else if (format == 's') {
        strcpy(str, (char*)data);
    }

    // Open file
    if (strlen(filename) == 0) {
        fd = STDOUT_FILENO;
    } else {
        fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP);
        if (fd == -1) {
            free(str);
            errno = EIO;
            return -1;
        }
    }

    // Write to file
    if (write(fd, str, len) != len) {
        ret = -1;
        errno = EIO;
    }

    free(str);
    return ret;
}

int fscanfx(char* filename, char format, void* dst)
{
    int fd;
    int ret = 0;
    int read_bytes;
    size_t buffer_size = 128;
    char* buffer = (char*)malloc(buffer_size);
    if (buffer == NULL) {
        errno = EIO;
        return -1;
    }

    /* if the file name is empty, if yes, then use keyboard input;
        else try to access the filename in the path. if not available, throw;
        if it exist, coitnue; else if it doesn't exist, throw */
    if (strlen(filename) == 0) {
        fd = STDIN_FILENO;
    } else {
        if (access(filename, F_OK) == -1) {
            errno = ENOENT;
            free(buffer);
            return -1;
        }
        fd = open(filename, O_RDONLY);
        if (fd == -1) {
            errno = EIO;
            free(buffer);
            return -1;
        }
    }


    size_t total_bytes = 0; // characters read into buffer so far
    ssize_t bytes_read; // -1 if fails to read
    while ((bytes_read = read(fd, buffer + total_bytes, 1)) == 1) {
        if (buffer[total_bytes] == '\n') {
            break;
        }
        total_bytes++;
        if (total_bytes >= buffer_size) {
            buffer_size += 128;
            buffer = (char*)realloc(buffer, buffer_size);
        }
    }

    // if fails throw
    if (bytes_read == -1) {
        ret = -1;
        errno = EIO;
    } else if (bytes_read == 0) {
        ret = -1; // Reached the end of the file
    } else {
        buffer[total_bytes] = '\0';

        if (format == 'd') {
            *(int*)dst = atoi(buffer);
        } else if (format == 's') {
            strcpy((char*)dst, buffer);
        } else {
            ret = -1;
            errno = EIO;
        }
    }

    free(buffer);
    return ret;
}

int clean()
{
    DIR* dir;
    struct dirent* entry;
    int fd;

    dir = opendir("/proc/self/fd");
    if (dir == NULL) {
        errno = EIO;
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        char* endptr;
        fd = strtol(entry->d_name, &endptr, 10);

        if (*endptr == '\0' && fd >= 3) { // Successfully converted to a file descriptor and not stdin, stdout, or stderr
            struct stat fd_stat;
            if (fstat(fd, &fd_stat) == -1) {
                errno = EIO;
                closedir(dir);
                return -1;
            }

            if (S_ISREG(fd_stat.st_mode)) { // Check if it's a regular file
                if (close(fd) == -1) {
                    errno = EIO;
                    closedir(dir);
                    return -1;
                }
            }
        }
    }

    closedir(dir);
    return 0;
}

int main()
{
    int array[5] = { 1, 2, -9, 12, -3 };
    char* string = "Hello!";
    /* Print integers to stdout */
    for (size_t i = 0; i < 5; i++) {
        fprintfx("", 'd', &array[i]);
        printf("\n");
    }
    /* Print string to stdout */

    fprintfx("", 's', string);
    /* Error: unrecognized format */
    fprintfx("", 'i', string);
    /* Write integers to a text file */
    for (size_t i = 0; i < 5; i++) {
        fprintfx("text", 'd', &array[i]);
    }
    // /* Write string to a text file */
    fprintfx("text", 's', string);
    clean();
    char newstr[1024] = { 0 };
    int num;
    /* Receive a string from stdin */
    fscanfx("", 's', newstr);
    /* Print out the string to stdout */
    fprintfx("", 's', newstr);
    /* Receive an integer from stdin */
    fscanfx("", 'd', &num);
    /* Print out the integer to stdout */
    fprintfx("", 'd', &num);
    // /* Read a file */
    while (!fscanfx("text", 's', newstr)) {
        fprintfx("", 's', "Line: ");
        fprintfx("", 's', newstr);
    }
    /* Close opened files */
    clean();
    return 0;
}