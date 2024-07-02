#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

/////////////////  HEADER ////////////////

#ifndef MAP
// # define MAP "./basictest.fdf"
# define MAP "./pyramide.fdf"
#endif

#ifndef BUFFER_SIZE
# define BUFFER_SIZE 20
#endif

typedef struct s_node t_node;

typedef struct s_node {
    char *content;
    t_node *next;
}   t_node;

#include "minilibx-linux/mlx.h"
#include "minilibx-linux/mlx_int.h"
#define _USE_MATH_DEFINES
#include <math.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080


enum Keyboard {
    KEYSPC = 0x20,
    KEYESC = 0xff1b,
    KEYNUM1 = 0x31,
    KEYNUM2 = 0x32
};

//gcc -L./minilibx-linux fdf.c -lmlx -lXext -lX11 -lm

typedef struct s_point{
    int x;
    int y;
    int height;
} t_point;

typedef struct s_params {
    void *mlx;
    void *window;
    int window_size[2];
    int map_dimensions[2];
    int mouse_down;
    int last_x;
    int last_y;
    unsigned int bits_per_color;
    unsigned int line_size;
    unsigned int endianess;
    float zoom;
    float x_rot;
    float y_rot;
    float z_rot;
    float tile_width;
    float tile_height;
    float scr_ratio;
    int max_height;
    int min_height;
    unsigned int bg_color_top;
    unsigned int bg_color_bottom;
    unsigned int model_color_top;
    unsigned int model_color_bottom;
    char projection;
    char *data;
    int map[21][21];
    t_point **screenmap;
    int screen_buffer[SCREEN_WIDTH][SCREEN_HEIGHT];
}   t_params;

enum Projections {
    ISOMETRIC = 1,
    PERSPECTIVE = 2
};

const char *getProjectionName(enum Projections projection)
{
    if (projection == ISOMETRIC)
        return "ISOMETRIC";
    else if (projection == PERSPECTIVE)
        return "PERSPECTIVE";
    else
        return "UNDEFINED";
}

/////////////////  HEADER ////////////////

/////////////////  INPUT /////////////////

int    cb_key(int keycode, t_params *params)
{
    int x;
    int y;
    
    printf("KEYPRESS DETECTED (%x)\n", keycode);
    if (keycode == KEYESC)
        mlx_loop_end(params->mlx);
    else if(keycode == KEYSPC)
    {
        mlx_clear_window(params->mlx, params->window);
        printf("Clearing window\n");
    }
    else if(keycode >= 0x30 && keycode <= 0x39)
        params->projection = keycode - 0x30;
    return (0);
}

float clamp(float min, float max, float value)
{
    if (value < min)
        return (min);
    else if (value > max)
        return (max);
    else
        return (value);
}

int    cb_mouse_down(int button, int x, int y, t_params *params)
{
    if (button == 3)
    {
        mlx_mouse_hide(params->mlx, params->window);
        mlx_mouse_move(params->mlx, params->window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        params->last_x = 0;
        params->last_y = 0;
        params->mouse_down = 1;
    }
    else if (button == 4)
        params->zoom = clamp(0.2, 2, params->zoom + 0.1);
    else if (button == 5)
        params->zoom = clamp(0.2, 2, params->zoom - 0.1);
    return (0);
}

int    cb_mouse_up(int button, int x, int y, t_params *params)
{
    if (button == 3)
        mlx_mouse_show(params->mlx, params->window);
        params->mouse_down = 0;
    return (0);
}

//////////////// INPUT /////////////////

float deg_to_rad(float deg)
{
    return (deg * M_PI / 180.0);
}

void RotationMatrixX(float rads, float R[3][3])
{
    R[0][0] = 1;
    R[0][1] = 0;
    R[0][2] = 0;
    R[1][0] = 0;
    R[1][1] = cos(rads);
    R[1][2] = -sin(rads);
    R[2][0] = 0;
    R[2][1] = sin(rads);
    R[2][2] = cos(rads);
}

void RotationMatrixY(float rads, float R[3][3])
{
    R[0][0] = cos(rads);
    R[0][1] = 0;
    R[0][2] = sin(rads);
    R[1][0] = 0;
    R[1][1] = 1;
    R[1][2] = 0;
    R[2][0] = -sin(rads);
    R[2][1] = 0;
    R[2][2] = cos(rads);
}

void RotationMatrixZ(float rads, float R[3][3])
{
    R[0][0] = cos(rads);
    R[0][1] = -sin(rads);
    R[0][2] = 0;
    R[1][0] = sin(rads);
    R[1][1] = cos(rads);
    R[1][2] = 0;
    R[2][0] = 0;
    R[2][1] = 0;
    R[2][2] = 1;
}

//Rotation matrix for isometric projection
void IsometricMatrix(float R[3][3])
{
    R[0][0] = sqrt(3)/2;
    R[0][1] = 0;
    R[0][2] = -sqrt(3)/2;
    R[1][0] = 1.0 / 2;
    R[1][1] = 1;
    R[1][2] = 1.0 / 2;
}

//Multiply matrices
void    multiplyMatrices(float a[3][3], float b[3][3], float result[3][3])
{
    int i;
    int j;
    int k;

    i = 0;
    while(i < 3)
    {
        j = 0;
        while(j < 3)
        {
            result[i][j] = 0;
            k = 0;
            while (k < 3)
            {
                result[i][j] += a[i][k] * b[k][j];
                k++;
            }
            j++;
        }
        i++;
    }
}

//Multiply resulting rotation matrix by 3D vector
void    multiply3DVector(float r[3][3], float vec[3], float result[3])
{
    int i;
    int j;
    i = 0;
    while(i < 3)
    {
        j = 0;
        result[i] = 0;
        while(j < 3)
        {
            result[i] += r[i][j] * vec[j];
            j++;
        }
        i++;
    }
}

unsigned int    color_step_calc(unsigned int start, unsigned int goal, float percentage)
{
    int goal_rgba[4];
    int init_rgba[4];
    int curr_rgba[4];
    int current_color;

    goal_rgba[0] = goal >> 24 & 0xFF;
    goal_rgba[1] = goal >> 16 & 0xFF;
    goal_rgba[2] = goal >> 8 & 0xFF;
    goal_rgba[3] = goal & 0xFF;
    init_rgba[0] = start >> 24 & 0xFF;
    init_rgba[1] = start >> 16 & 0xFF;
    init_rgba[2] = start >> 8 & 0xFF;
    init_rgba[3] = start & 0xFF;
    curr_rgba[0] = (int)(init_rgba[0] + (goal_rgba[0] - init_rgba[0]) * percentage);
    curr_rgba[1] = (int)(init_rgba[1] + (goal_rgba[1] - init_rgba[1]) * percentage);
    curr_rgba[2] = (int)(init_rgba[2] + (goal_rgba[2] - init_rgba[2]) * percentage);
    curr_rgba[3] = (int)(init_rgba[3] + (goal_rgba[3] - init_rgba[3]) * percentage);
    current_color = (curr_rgba[0] << 24) | 
    (curr_rgba[1] << 16) | 
    (curr_rgba[2] << 8) | 
    curr_rgba[3] & 0xFF;
    return(current_color);
}

void    draw_debug_info(t_params *params)
{
    char *projection;

    projection = ft_strjoin("PROJECTION TYPE: ", getProjectionName(params->projection));
    mlx_string_put(params->mlx, params->window, 10, 10, 0xFFFFFF, projection);
    free(projection);
}

size_t	ft_strlcpy(char *dst, const char *src, size_t size)
{
	size_t	i;
	size_t	len;

	len = ft_strlen(src);
	i = 0;
	if (size > 0)
	{
		while (i < size - 1 && src[i])
		{
			dst[i] = src[i];
			i++;
		}
		dst[i] = 0x00;
	}
	return (len);
}

int ft_strlen(char *str)
{
    int i;

    if (str == NULL)
        return (-1);
    i = 0;
    while (str[i])
        i++;
    return (i);
}

int ft_strchr(char *str, char c)
{
    int i;

    i = 0;
    if (str && str[i])
    {
        while (str[i])
        {
            if (str[i] == c)
                return (i);
            i++;
        }
    }
    return (-1);
}

t_node *create_node(char *content)
{
    t_node *node;

    node = malloc(sizeof(t_node));
    if (node == NULL)
        return (NULL);
    node->next = NULL;
    node->content = content;
    return (node);
}

t_node *get_last_node(t_node *node)
{
    if (node == NULL)
        return (NULL);
    while (node->next)
        node = node->next;
    return (node);
}

void    free_list(t_node *node)
{
    t_node *temp;
    while (node)
    {
        temp = node;
        node = node->next;
        free(temp->content);
        free(temp);
    }
}

void    append_node(t_node **list, t_node *node)
{
    t_node *last_node;

    if (*list == NULL)
        *list = node;
    else
    {
        last_node = get_last_node(*list);
        last_node->next = node;
    }
}

void    prepend_node(t_node **list, t_node *node)
{
    if (*list == NULL)
        *list = node; 
    else
    {
        node->next = *list;
        *list = node;
    }
}

char    *ft_strdup(char *src)
{
    char *str;
    int i;

    if (src == NULL)
        return (NULL);
    i = ft_strlen(src);
    str = malloc(i + 1);
    if (str == NULL)
        return (NULL);
    str[i] = '\0';
    while (i >= 0)
    {
        str[i] = src[i];
        i--;
    }
    return (str);
}

char    *ft_strndup(char *src, int amt)
{
    char *str;
    int i;

    i = 0;
    str = malloc(amt + 1);
    while (src[i] && i != amt)
    {
        str[i] = src[i];
        i++;
    }
    str[i] = '\0';
    return (str);
}

void ft_strcpy(char *dst, char *src)
{
    int i;

    if (dst == NULL || src == NULL)
        return ;
    i = 0;
    while (src[i])
    {
        dst[i] = src[i];
        i++;
    }
}

char *ft_strjoin(char *s1, char *s2)
{
    char *str;
    int s1len;
    int s2len;

    if (s1 == NULL && s2 == NULL)
        return (NULL);
    if (s1 == NULL)
        return (s2);
    if (s2 == NULL)
        return (s1);
    s1len = ft_strlen(s1);
    s2len = ft_strlen(s2);
    str = malloc(s1len + s2len + 1);
    ft_strcpy(str, s1);
    ft_strcpy(str + s1len, s2);
    str[s1len + s2len] = '\0';
    return (str);
}

char    *get_line(char **str)
{
    char *line;
    char *temp;
    int nl;

    if (*str == NULL)
        return (NULL);
    nl = ft_strchr(*str, '\n');
    if (nl == -1)
        return (*str);
    else
        line = ft_strndup(*str, nl);
    temp = ft_strdup(&(*str)[nl]);
    free(*str);
    *str = temp;
    return (line);
}

int get_joint_memsize(t_node *node)
{
    int size;

    size = 0;
    while (node)
    {
        size += ft_strlen(node->content);
        node = node->next;
    }
    return (size);
}

char    *join_nodes(t_node *node)
{
    char    *str;
    int memsize;
    int cur;
    int len;

    memsize = get_joint_memsize(node);
    str = malloc(memsize + 1);
    str[memsize] = '\0';
    cur = 0;
    while (node)
    {
        ft_strcpy(&str[cur], node->content);
        cur += ft_strlen(node->content);
        node = node->next;
    } 
    return (str);
}

int init_buffer(char **buffer)
{
    *buffer = malloc(BUFFER_SIZE + 1);
    if (*buffer == NULL)
        return (-1);
    (*buffer)[BUFFER_SIZE] = '\0';
    return (1);
}

char *read_map(int fd)
{
    int flg;
    char *buffer;
    t_node *temp;
    t_node *list;

    flg = 1;
    list = NULL;
    if (init_buffer(&buffer) == -1)
        return (NULL);
    while (flg)
    {
        flg = read(fd, buffer, BUFFER_SIZE);
        if (flg != -1)
        {
            buffer[flg] = '\0';
            temp = create_node(ft_strdup(buffer));
            append_node(&list, temp);
        }
        if (flg != BUFFER_SIZE)
            break;
    }
    free(buffer);
    buffer = join_nodes(list);
    free_list(list);
    return (buffer);
}

//Count total occurences of a character in a given string
int ft_strchrn(char *str, char c)
{
    int i;
    int n;

    i = 0;
    n = 0;
    while (str[i] != '\0')
    {
        if (str[i] == c)
            n++;
        i++;
    }
    return (n);
}

//Count unique separators within a string
size_t ft_strsepn(char *str, char c)
{
    int i;
    size_t n;

    i = 0;
    n = 0;
    while (str[i] != '\0')
    {
        if (str[i] == c)
        {
            n++;
            while (str[i] == c && str[i] != '\0')
                i++;
        }
        if (str[i] != '\0')
            i++;
    }
    return (n);
}

typedef struct s_point {
    int height;
    int color;
}   t_point;

size_t count_rows(char *data)
{
    size_t rows = 0;
    int i = 0;
    int pos;

    while (data[i] != '\0')
    {
        pos = ft_strchr(&data[i], '\n');
        if (pos == -1)
        {
            if (data[i] != '\0')
                rows++;
            break;
        }
        else
        {
            rows++;
            i += pos + 1;
        }
    }
    return (rows);
}

int ft_isdigit(char c)
{
    if (c >= '0' && c <= '9')
        return (1);
    return (-1);
}

int ft_isalpha(char c)
{
    if (c >= 'a' && c <= 'z')
        return (1);
    else if (c >= 'A' && c <= 'Z')
        return (1);
    return (-1);
}

int ft_isalnum(char c)
{
    if (ft_isdigit(c) || ft_isalpha(c))
        return (1);
    return (-1);
}

int ft_atoi(char *str)
{
    int neg;
    int value;

    value = 0;
    neg = 1;
    while (*str == '+' || *str == '-')
    {
        if (*str == '-')
            neg *= -1;
        str++;
    }
    while (*str >= '0' && *str <= '9')
    {
        value *= 10;
        value += *str - '0';
        str++;
    }
    value *= neg;
    return (value);
}

int ft_hatoi(char *str)
{
    int value;
    int digit;

    printf("%s\n", str);
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
        str += 2;
    value = 0;
    while (ft_isalnum(*str))
    {
        if ('0' <= *str && *str <= '9')
            digit = *str - '0';
        else if ('a' <= *str && *str <= 'f')
            digit = *str - 'a' + 10;
        else if ('A' <= *str && *str <= 'F')
            digit = *str - 'A' + 10;
        else
            break;
        value *= 16;
        value += digit;
        str++;
    }
    return (value);
}

int read_values(t_point **point, char *str)
{
    int comma;

    comma = ft_strchr(str, ',');
    if (comma != -1)
        (*point)->color = ft_hatoi(str + comma + 1);
    (*point)->height = ft_atoi(str);
    return (1);
}

int fill_row(t_point ***row, size_t ncols, char *data)
{
    int i;
    int j;
    int col;
    char *ptr;

    *row = calloc(ncols + 1, sizeof(t_point *));
    if (*row == NULL)
        return (-1);
    i = 0;
    j = 0;
    col = 0;
    while (col < ncols && data[i] != '\0' && data[i] != '\n')
    {
        while (data[j] != ' ' && data[j] != '\0' && data[j] != '\n')
            j++;
        (*row)[col] = malloc(sizeof(t_point));
        if ((*row)[col] == NULL)
            return (-1);
        ptr = ft_strndup(data + i, j - i);
        read_values(&(*row)[col], ptr);
        free(ptr);
        i = j++ + 1;
        col++;
    }
    if (col < ncols)
        return (-1);
    return (1);
}

typedef t_point ***map;

int    dissect_map(map *map, char *data, size_t *nrows, size_t *ncols)
{
    int nlpos;
    int row;
    int pos;

    pos = 0;
    nlpos = 0;
    *nrows = count_rows(data);
    *ncols = ((ft_strsepn(data, ' ') + 1 + *nrows) / *nrows);
    *map = calloc(*nrows + 1, sizeof(t_point **));
    if (*map == NULL || *ncols == 0 || *nrows == 0)
        return (-1);
    row = 0;
    while (row < *nrows && nlpos != -1)
    {
        nlpos = ft_strchr(&data[pos], '\n');
        if (nlpos != -1)
            data[pos + nlpos] = '\0';
        printf("%s\n", &data[pos]);
        if (*ncols != ft_strsepn(&data[pos], ' ') + 1)
            return (-1);
        if (fill_row(&(*map)[row++], *ncols, &data[pos]) == -1)
            return (-1);
        pos += nlpos + 1;
    }
    return (1);
}

void free_map(t_point ***map, size_t nrows, size_t ncols)
{
    size_t i, j;

    i = 0;
    while(map[i] != NULL)
    {
        j = 0;
        while(map[i][j] != NULL)
        {
            free(map[i][j++]);
        }
        free(map[i++]);
    }
    free(map);
}

int main()
{
    int fd;
    char *data;
    map map;
    size_t nr;
    size_t nc;  

    fd = open(MAP, O_RDONLY);
    if (fd < 0)
    {
        printf("CANNOT OPEN %s\n", MAP);
        return (-1);
    }
    printf("PROCESSING MAP FILE\n");
    data = read_map(fd);
    if (data == NULL)
        printf("MAP READ ERROR\n");
    if (dissect_map(&map, data, &nr, &nc) == 1)
        printf("READING COMPLETE\n");
    else
        printf("INVALID MAP FILE\n");

    // int i;
    // int j;

    // i = 0;
    // while (map[i] != NULL)
    // {
    //     j = 0;
    //     while (map[i][j] != NULL)
    //     {
    //         printf("HEIGHT: %i COLOR: %i\n", map[i][j]->height, map[i][j]->color);
    //         j++;
    //     }
    //     i++;
    // }
    free_map(map, nr, nc);
    free(data);
    return (0);
}