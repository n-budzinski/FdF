#include "minilibx-linux/mlx.h"
#include "minilibx-linux/mlx_int.h"
#define _USE_MATH_DEFINES
#include <math.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600


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
    int bits_per_color;
    int line_size;
    int endianess;
    float zoom;
    float x_rot;
    float y_rot;
    float z_rot;
    float tile_width;
    float tile_height;
    float scr_ratio;
    int max_height;
    int min_height;
    char projection;
    char *data;
    int map[21][21];
    t_point **screenmap;
    int screen_buffer[SCREEN_WIDTH][SCREEN_HEIGHT];
}   t_params;


//////////////// INPUT /////////////////

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



int    color_step_calc( int goal,  int start, float percentage)
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

void drawLine(t_point p1, t_point p2, t_params *params) {
    t_point lp;
    t_point hp;
    t_point sp;
    if (p1.height > p2.height)
    {
        hp = p1;
        lp = p2;
        sp = p1;
    }
    else
    {
        hp = p2;
        lp = p1;
        sp = p2;
    }
    int dx = abs(hp.x - lp.x);
    int dy = abs(hp.y - lp.y);
    int sx = (lp.x < hp.x) ? 1 : -1;
    int sy = (lp.y < hp.y) ? 1 : -1;
    int err = dx - dy;
    int e2;
    float percentage;

    while (1) {
        if (lp.x >= 0 && lp.x < SCREEN_WIDTH && lp.y >= 0 && lp.y < SCREEN_HEIGHT) {
            int index = lp.x * params->bits_per_color / 8 + lp.y * params->line_size;
            float height_range = params->max_height - params->min_height;
            percentage = fabs((float)(hp.height - params->min_height) / height_range);
            int color = color_step_calc(0xd4b1bc, 0xdf3562, percentage);
            params->data[index] = (color & 0xFF);
            params->data[index + 1] = (color >> 8) & 0xFF;
            params->data[index + 2] = (color >> 16) & 0xFF;
            params->data[index + 3] = (color >> 24) & 0xFF;
        }
        if (lp.x == hp.x && lp.y == hp.y) break;

        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            lp.x += sx;
        }
        if (e2 < dx) {
            err += dx;
            lp.y += sy;
        }
    }
}

enum Projections {
    ISOMETRIC = 0,
    PERSPECTIVE = 1
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

int	ft_strlen(const char *c)
{
	int	i;

	i = 0;
	while (c[i] != '\0')
		i++;
	return (i);
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

char	*ft_strjoin(char const *s1, char const *s2)
{
	size_t	s1l;
	size_t	s2l;
	size_t	tl;
	void	*ptr;

	s1l = ft_strlen(s1);
	s2l = ft_strlen(s2);
	tl = s1l + s2l + 1;
	ptr = (char *)malloc(tl);
	if (ptr != NULL)
	{
		ft_strlcpy(ptr, s1, s1l + 1);
		ft_strlcpy(ptr + s1l, s2, s2l + 1);
	}
	return (ptr);
}

void    draw_debug_info(t_params *params)
{
    char *projection;

    projection = ft_strjoin("PROJECTION TYPE: ", getProjectionName(params->projection));
    mlx_string_put(params->mlx, params->window, 10, 10, 0xFFFFFF, projection);
    free(projection);
}

void mouse_down_hander(t_params *params)
{
    int m_x;
    int m_y;

    params->last_x = m_x;
    params->last_y = m_y;
    if (m_x <= SCREEN_WIDTH / 2)
        m_x = SCREEN_WIDTH / 2;
    else if(m_x >= SCREEN_WIDTH / 2)
        m_x = SCREEN_WIDTH / 2;
    if (m_y >= SCREEN_HEIGHT / 2)
        m_y = SCREEN_HEIGHT / 2;
    else if (m_y <= SCREEN_HEIGHT / 2)
        m_y = SCREEN_HEIGHT / 2;
    params->z_rot += (m_x - params->last_x) * 0.05;
    params->x_rot += (m_y - params->last_y) * 0.05;
    mlx_mouse_move(params->mlx, params->window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
}

#include <stdio.h>
int    cb_loop(t_params *params)
{

    int i;
    int j;

    draw_debug_info(params);
    mlx_mouse_get_pos(params->mlx, params->window, &m_x, &m_y);
    if (params->mouse_down)
    {
        mouse_down_handler
    }
    else
    {
        params->last_x = m_x;
        params->last_y = m_y;
        params->z_rot += 0.020;
    }
    float rot_x[3][3];
    float rot_y[3][3];
    float rot_z[3][3];
    float rotXY[3][3];
    float rotFinal[3][3];
    float rotFinal2[3][3];
    float isoFinal[3][3];
    RotationMatrixX(deg_to_rad(params->x_rot), rot_x);
    RotationMatrixY(deg_to_rad(params->y_rot), rot_y);
    RotationMatrixZ(deg_to_rad(params->z_rot), rot_z);
    IsometricMatrix(isoFinal);
    multiplyMatrices(rot_x, rot_y, rotXY);
    multiplyMatrices(rotXY, rot_z, rotFinal);
    // multiplyMatrices(rotFinal, isoFinal, rotFinal2);
    for (int y = 0; y < 21; y++) {
        for (int x = 0; x < 21; x++)
        {
            float point[3] = {x - 10, y - 10, params->map[x][y]};
            float transformed_point[3];
            float result[2];
            multiply3DVector(rotFinal, point, transformed_point);
            // isometricProjection(transformed_point, result);
            t_point *p = params->screenmap[y * 21 + x];
            p->x = (int)((transformed_point[0] * (params->tile_width * params->zoom) + SCREEN_WIDTH / 2));
            p->y = (int)(((transformed_point[1]) * (params->tile_height * params->zoom) + SCREEN_HEIGHT / 2));
            p->height = params->map[x][y];
        }
    }
    void *image = mlx_new_image(params->mlx, SCREEN_WIDTH, SCREEN_WIDTH);
    char *data = mlx_get_data_addr(image, &params->bits_per_color, &params->line_size, &params->endianess);
    int color = mlx_get_color_value(params->mlx, 0x380c43);
    i = 0;
    params->data = data;
    while (i + 4 < SCREEN_HEIGHT * SCREEN_WIDTH * 4)
    {
            params->data[i] = (color & 0xFF);
            params->data[i + 1] = (color >> 8) & 0xFF;
            params->data[i + 2] = (color >> 16) & 0xFF;
            params->data[i + 4] = (color >> 24) & 0xFF;
            i+=4;
    }
    t_point *p;
    i = 0;
    while (params->screenmap[i] != NULL) {
        p = params->screenmap[i];
        if (0 != (i + 1) % 21) {

            drawLine(*p, *params->screenmap[i + 1], params);
        }
        if (i < 21 * (21 - 1)) {
            drawLine(*p, *params->screenmap[i + 21], params);
        }
        i++;
    }
    mlx_clear_window(params->mlx, params->window);
    mlx_put_image_to_window(params->mlx, params->window, image, 0, 0);
    mlx_destroy_image(params->mlx, image);
    i = 0;
    while (i < SCREEN_WIDTH)
    {
        j = 0;
        while (j < SCREEN_HEIGHT)
        {
            if (params->screen_buffer[i][j])
            {
                mlx_pixel_put(params->mlx, params->window, i, j, params->screen_buffer[i][j]);
                params->screen_buffer[i][j] = 0;
            }
            j++;
        }
        i++;
    }
}



int main()
{
    void *mlx;
    void *window;
    t_params *params;
    int loop_val;
    int i;
    int j;

    mlx = mlx_init();
    window = mlx_new_window(mlx, SCREEN_WIDTH, SCREEN_HEIGHT, "nbudzins's FdF");
    params = malloc(sizeof(t_params));
    params->mlx = mlx;
    params->window = window;
    params->map_dimensions[0] = 21;
    params->map_dimensions[1] = 21;
    params->mouse_down = 0;
    params->last_x = 0;
    params->x_rot = 0;
    params->y_rot = 0;
    params->z_rot = 0;
    params->zoom = 0.25;
    params->tile_width = 30;
    params->tile_height = 30;
    params->min_height = 0;
    params->max_height = 15;
    params->projection = 0;
    params->scr_ratio = SCREEN_WIDTH / SCREEN_HEIGHT;
    
    i = 0;
    while (i < params->window_size[1])
    {
        j = 0;
        while (j < params->window_size[0])
            params->screen_buffer[i][j++] = 0;
        i++;
    }
    i = 0;
    while (i < params->map_dimensions[0])
    {
        j = 0;
        while (j < params->map_dimensions[1])
        {
            int dist_to_center = fmax(abs(i - 21 / 2), abs(j - 21 / 2));
            params->map[i][j] = fmax(0, 10 - dist_to_center + 1);
            j++;
        }
        i++;
    }
    //POINT ARRAY INIT//////////////////////////////
    t_point **ptr;
    ptr = malloc(sizeof(t_point *) * 21 * 21 + 1);
    ptr[sizeof(t_point *) * 21 * 21] = NULL;
    i = 0;
    while (i < 21 * 21)
        ptr[i++] = malloc(sizeof(t_point));
    params->screenmap = ptr;
    /////////////////////////////////////////////////
    //MOUSE
    mlx_hook(window, 4, (1L<<2)	,&cb_mouse_down, params);
    mlx_hook(window, 5, (1L<<3)	,&cb_mouse_up, params);
    mlx_loop_hook(mlx, &cb_loop, params);
    //KEYS
    mlx_key_hook(window, &cb_key, params);
    mlx_loop(mlx);
    return (0);
}
