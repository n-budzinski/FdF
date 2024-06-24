#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

///////////////// HEADER

#ifndef MAP
# define MAP "./basictest.fdf"
#endif

#ifndef BUFFER_SIZE
# define BUFFER_SIZE 20
#endif

typedef struct s_node t_node;

typedef struct s_node {
    char *content;
    t_node *next;
}   t_node;

///////////////// HEADER

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

void    append_node(t_node **list, char *content)
{
    t_node *last_node;
    t_node *node;

    node = create_node(content);
    if (*list == NULL)
    {
        *list = node;
        return ;
    }
    last_node = get_last_node(*list);
    last_node->next = node;
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

void    break_apart(t_node **list, char **leftovers, char *content, int nlpos)
{
    char *str;
    int len;

    len = ft_strlen(content);
    append_node(list, ft_strndup(content, nlpos));
    if (nlpos == len - 1)
        return ;
    str = ft_strndup(content + nlpos + 1, len - nlpos);
    *leftovers = NULL;
    free(*leftovers);
    *leftovers = str;
}

int get_joint_memsize(t_node *node)
{
    int size;

    size = 0;
    if (node == NULL)
        return (0);
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

int main()
{
    int fd;
    int flg;
    char *temp;
    t_node *buffer;
    static char *leftovers;
    t_node *node;
    char *line;
    int nlpos;


    flg = 1;
    buffer = NULL;
    leftovers = NULL;
    temp = malloc(BUFFER_SIZE + 1);
    temp[BUFFER_SIZE] = '\0';
    if (temp == NULL)
        return (0);
    fd = open(MAP, O_RDONLY);

    if (!fd)
    {
        printf("Error opening file.");
        return (0);
    }
    while (flg)
    {
        if (leftovers != NULL)
        {
            nlpos = ft_strchr(leftovers, '\n');
            break_apart(&buffer, &leftovers, leftovers, nlpos);
        }
        flg = read(fd, temp, BUFFER_SIZE);
        line = ft_strjoin(leftovers, temp);
        leftovers = NULL;
        nlpos = ft_strchr(line, '\n');
        if (nlpos == -1)
            append_node(&buffer, line);
        else
            break_apart(&buffer, &leftovers, line, nlpos);
        if (flg != BUFFER_SIZE)
            break;
    }
    printf("%s", join_nodes(buffer));

    return (0);
}