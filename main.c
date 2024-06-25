#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

///////////////// HEADER

#ifndef MAP
# define MAP "./basictest.fdf"
#endif

#ifndef BUFFER_SIZE
# define BUFFER_SIZE 10
#endif

typedef struct s_node t_node;

typedef struct s_node {
    char *content;
    t_node *next;
}   t_node;

///////////////// HEADER

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
        i++;
    }
    return (-1);
}

typedef struct s_point {
    int height;
    int color;
}   t_point;

int    dissect_map(t_point ****output, char *data)
{
    size_t nrows;
    size_t ncols;
    int nlpos;
    int pos;
    int i;

    pos = 0;
    nrows = ft_strchrn(data, '\n') + 1;
    **output = calloc(nrows + 1, sizeof(void *));
    if (**output == NULL)
        return (-1);
    while (data[pos] == '\n')
    {
        i = 0;
        ncols = ft_strsepn(&data[pos + 1], ' ');
        **output[i] = calloc(ncols + 1, sizeof(t_point *));
        if (**output[i] == NULL)
            return (-1);
        nlpos = ft_strchr(&data[pos + 1], '\n');
        if (nlpos == -1)
            break ;
        pos += nlpos + 1;
        i++;
    }
    return (1);
}

int main()
{
    int fd;
    char *data;
    t_point ***map;

    fd = open(MAP, O_RDONLY);
    if (fd < 0)
        return (-1);
    data = read_map(fd);
    dissect_map(&map, data);
}