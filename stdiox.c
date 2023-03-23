/*
name: alim
date: march 21, 2023
pledge: i pledge my honor that i have abided by the stevens honor system
extra credit: done
*/

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

int float_length(float value, int precision)
{
    int len = 0;
    int integer_part = (int)value;

    if (precision < 0) {
        precision = 0;
    }

    len = int_length(integer_part);

    if (precision > 0) {
        len++; // For the decimal point
        len += precision; // For the number of digits after the decimal point
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

void float_to_str(float value, char* buffer, int precision)
{
   int is_negative = 0;
    if (value < 0) {
        is_negative = 1;
        value = -value;
    }

    int integer_part = (int)value;
    float fractional_part = value - integer_part;

    if (is_negative) {
        char temp[12];
        int_to_str(-integer_part, temp);
        strcpy(buffer, temp);
    } else {
        int_to_str(integer_part, buffer);
    }


    if (precision > 0) {
        int int_len = int_length(integer_part);
        if (is_negative) {
            int_len++;
        }
        buffer[int_len] = '.';
        buffer[int_len + 1] = '\0';

        for (int i = 0; i < precision; i++) {
            fractional_part *= 10;
            int digit = (int)fractional_part;
            buffer[int_len + 1 + i] = '0' + digit;
            fractional_part -= digit;
        }

        buffer[int_len + 1 + precision] = '\0';
    }
}

int fprintfx(char* filename, char format, void* data)
{
    int FLOAT_PRECISION = 6; //change if needed

    int fd = STDIN_FILENO;
    int ret = 0;
    int read_bytes;
    
    if (strlen(filename) != 0) {

        if (access(filename, F_OK) == -1) {
            errno = ENOENT;
            return -1;
        }

        fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP);
        if (fd == -1) {
            errno = EIO;
            return -1;
        }
    }
    char* str;
    int len;

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
    } else if (format =='f') {
        len = float_length(*(float*)data, FLOAT_PRECISION);
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
    } else if (format == 'f') {
        float_to_str(*(float*)data, str, FLOAT_PRECISION);
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

    char ewline = '\n';
    write(fd, &ewline, 1);

    free(str);
    return ret;
}

int fscanfx(char* filename, char format, void* dst)
{
    int fd = STDIN_FILENO;
    int ret = 0;
    int read_bytes;
    size_t buffer_size = 128;
    char* buffer = (char*)malloc(buffer_size);
    if (buffer == NULL) {
        errno = EIO;
        return -1;
    }

    if (dst == NULL) {
        free(buffer);
        return 0;
    }
    
    if (strlen(filename) != 0) {
        if (access(filename, F_OK) == -1) {
            errno = ENOENT;
            free(buffer);
            return -1;
        }
        
        DIR* processes = opendir("/proc/self/fd");
        struct dirent* entry;
        struct stat fd_stat;
        struct stat dirent_stat;
        int found = 0;
        stat(filename, &fd_stat);
        while((entry = readdir(processes)) != NULL) {
            fstat(atoi(entry->d_name), &dirent_stat);
            if (fd_stat.st_ino == dirent_stat.st_ino) {
                fd = atoi(entry->d_name);
                found = 1;
                break;
            }
        }
        closedir(processes);
        if (found == 0) fd = open(filename, O_RDONLY);
    } else {
        fd = STDIN_FILENO;
    }

    size_t total_bytes = 0; // characters read into buffer so far
    ssize_t bytes_read; // -1 if fails to read
    while ((bytes_read = read(fd, buffer + total_bytes, 1)) == 1) {
        if (buffer[total_bytes] == '\n') {
            break;
        }
        if (bytes_read == 0) {
            break;
        }    
        total_bytes++;
        if (total_bytes >= buffer_size - 1) { // Leave room for the null terminator
            buffer_size += 128; 
            buffer = (char*)realloc(buffer, buffer_size);
        }
    }

    // if fails throw
    if (bytes_read == -1) {
        ret = -1;
        errno = EIO;
    } else if (bytes_read == 0 && total_bytes == 0) { // !!!
        free(buffer);
        return 1; // Reached the end of the file
    }
    if (buffer[total_bytes - 1] == '\n') {
        buffer[total_bytes - 1] = '\0';
    } 
    buffer[total_bytes] = '\0';
    if (format == 'd') {
        *(int*)dst = atoi(buffer);
    } else if (format == 's') {
        strcpy((char*)dst, buffer);
    } else if (format == 'f') {
        *(float*)dst = atof(buffer);
    }
        else { // wrong format
        errno = EIO;
        free(buffer);
        return -1;
    }
    
    free(buffer);
    //printf("\n%d", ret);
    return ret;
}

int clean()
{
    DIR* dir;
    struct dirent* entry;
    int fd;

    dir = opendir("/proc/self/fd"); // linux only

    if (dir == NULL) {
        errno = EIO;
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        fd = atoi(entry->d_name);
        if (fd > 2) {
            if (close(fd) == -1) { // *
                closedir(dir);
                errno = EIO;
                return -1;
            }
        }
    }

    closedir(dir);
    return 0;
}

// int main()
// {
//     int array[5] = { 1, 2, -9, 12, -3 };
//     char* string = "Hello!";
//     /* Print integers to stdout */
//     for (size_t i = 0; i < 5; i++) {
//         fprintfx("", 'd', &array[i]);
//     }
//     /* Print string to stdout */

//     fprintfx("", 's', string);
//     /* Error: unrecognized format */
//     fprintfx("", 'i', string);
//     /* Write integers to a text file */
//     for (size_t i = 0; i < 5; i++) {
//         fprintfx("text", 'd', &array[i]);
//     }
//     // /* Write string to a text file */
//     fprintfx("text", 's', string);
//     clean();
//     char newstr[1024] = { 0 };
//     int num;
//     /* Receive a string from stdin */
//     fscanfx("", 's', newstr);
//     /* Print out the string to stdout */
//     fprintfx("", 's', newstr);
//     /* Receive an integer from stdin */
//     fscanfx("", 'd', &num);
//     /* Print out the integer to stdout */
//     fprintfx("", 'd', &num);
//     // /* Read a file */
//     while (fscanfx("text", 's', newstr) == 0) {
//         fprintfx("", 's', "Line: ");
//         fprintfx("", 's', newstr);
//     }

//     /* Close opened files */
//     clean();
//     return 0;
// }