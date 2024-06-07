#include "minilibx-linux/mlx.h"
#include "minilibx-linux/mlx_int.h"
#define _USE_MATH_DEFINES
#include <math.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define KEYESC  0xff1b //ESCAPE
#define KEYSPC  0x0020 //SPACE

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

//gcc -L./minilibx-linux fdf.c -lmlx -lXext -lX11 -lm

typedef struct s_point{
    int x;
    int y;
    int height;
} t_point;

typedef struct s_params {
    void *mlx;
    void *window;
    int mouse_down;
    int last_x;
    int last_y;
    float zoom;
    float x_rot;
    float y_rot;
    float z_rot;
    float tile_width;
    float tile_height;
    float scr_ratio;
    int map[20][20];
    t_point **screenmap;
}   t_params;


//////////////// INPUT /////////////////

int    cb_key(int keycode, t_params *params)
{
    int x;
    int y;
    
    // printf("KEYPRESS DETECTED (%i)\n", keycode);
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
    if (button == 1)
    {
        mlx_mouse_hide(params->mlx, params->window);
        mlx_mouse_move(params->mlx, params->window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        params->last_x = 0;
        params->last_y = 0;
        params->mouse_down = 1;
    }
    else if (button == 4)
        params->zoom = clamp(0.10, 100, params->zoom + 0.10);
    else if (button == 5)
        params->zoom = clamp(0.10, 100, params->zoom - 0.10);
    return (0);
}

int    cb_mouse_up(int button, int x, int y, t_params *params)
{
    if (button == 1)
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

void toIsometric(float vec[3], float result[2])
{
    float iso[2][3];
    int i;
    int j;

    iso[0][0] = sqrt(3)/2;
    iso[0][1] = 0;
    iso[0][2] = -sqrt(3)/2;
    iso[1][0] = 1.0 / 2;
    iso[1][1] = 1;
    iso[1][2] = 1.0 / 2;
    i = 0;
    while (i < 2)
    {
        result[i] = 0;
        j = 0;
        while (j < 3)
        {
            result[i] += iso[i][j] * vec[j];
            j++;
        }
        i++;
    }
}

typedef struct s_screenpos {
    int x;
    int y;
}   t_screenpos;

void drawLine(int x1, int y1, int x2, int y2, t_params *params) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    int e2;

    while (1) {
        mlx_pixel_put(params->mlx, params->window, x1, y1, 0xFFFFFF); // Draw the pixel
        if (x1 == x2 && y1 == y2) break; // Check if the end point is reached

        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

t_point *calculateScreenPosition(int x, int y, t_params *params)
{
    float rot_x[3][3];
    float rot_y[3][3];
    float rot_z[3][3];
    float rotXY[3][3];
    float rotFinal[3][3];
    float rotFinal2[3][3];
    float isoMatrix[3][3];
    RotationMatrixX(deg_to_rad(params->x_rot), rot_x);
    RotationMatrixY(deg_to_rad(params->y_rot), rot_y);
    RotationMatrixZ(deg_to_rad(params->z_rot), rot_z);
    multiplyMatrices(rot_x, rot_y, rotXY);
    multiplyMatrices(rotXY, rot_z, rotFinal);

    float point[3] = {x - 10, y - 10, params->map[x][y]};
    float transformed_point[3];
    // float vec[2];
    multiply3DVector(rotFinal, point, transformed_point);
    // toIsometric(transformed_point, vec);
    t_point *p = params->screenmap[y * 20 + x];
    p->x = (int)((transformed_point[0] * (params->tile_width * params->zoom) + SCREEN_WIDTH / 2));
    p->y = (int)((transformed_point[1] * (params->tile_height * params->zoom) + SCREEN_HEIGHT / 2));
    p->height = params->map[x][y];
}

int    cb_loop(t_params *params)
{
    int m_x;
    int m_y;
    mlx_mouse_get_pos(params->mlx, params->window, &m_x, &m_y);
    if (params->mouse_down)
    {
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
        params->z_rot += (m_x - params->last_x) * 0.1;
        params->x_rot += (m_y - params->last_y) * 0.1;
        mlx_mouse_move(params->mlx, params->window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    }
    else
    {
        params->last_x = m_x;
        params->last_y = m_y;
        params->z_rot += 0.0005;
    }
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 20; x++)
            calculateScreenPosition(x, y, params);
    }
    int i = 0;
    t_point *p;
    mlx_clear_window(params->mlx, params->window);
    while (params->screenmap[i] != NULL) {
        p = params->screenmap[i];
        if (0 != (i + 1) % 20) { // Draw horizontal line
            drawLine(p->x, p->y, params->screenmap[i + 1]->x, params->screenmap[i + 1]->y, params);
        }
        if (i < 20 * (20 - 1)) { // Draw vertical line
            drawLine(p->x, p->y, params->screenmap[i + 20]->x, params->screenmap[i + 20]->y, params);
        }
        // Uncomment if you want to plot the point as well
        // mlx_pixel_put(params->mlx, params->window, p->x, p->y, 0xFFFFFF);
        i++;
    }
    return (0);
}



int main()
{
    void *mlx;
    void *window;
    t_params *params;
    int loop_val;

    mlx = mlx_init();
    window = mlx_new_window(mlx, SCREEN_WIDTH, SCREEN_HEIGHT, "nbudzins's FdF");
    params = malloc(sizeof(t_params));
    params->mlx = mlx;
    params->window = window;
    params->mouse_down = 0;
    params->last_x = 0;
    params->x_rot = 90;
    params->y_rot = 0;
    params->z_rot = 0;
    params->zoom = 0.25;
    params->tile_width = 30;
    params->tile_height = 30;
    params->scr_ratio = SCREEN_WIDTH / SCREEN_HEIGHT;
    // for (int i = 0; i < 20; ++i) {
    //     for (int j = 0; j < 20; ++j)
    //         params->map[i][j] = 0;
    // }
    for (int i = 0; i < 20; ++i) {
        for (int j = 0; j < 20; ++j) {
            int dist_to_center = fmax(abs(i - 20 / 2), abs(j - 20 / 2));
            params->map[i][j] = fmax(0, 10 - dist_to_center + 1);
        }
    }
    //POINT ARRAY INIT//////////////////////////////
    t_point **ptr;
    ptr = malloc(sizeof(t_point *) * 20 * 20 + 1);
    ptr[sizeof(t_point *) * 20 * 20] = NULL;
    int i = 0;
    while (i < 20 * 20)
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
