






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
            int color = color_step_calc(params->model_color_bottom, params->model_color_top, percentage);
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









void mouse_rotation_handler(t_params *params)
{
    int pos[2];

    pos[0] = 0;
    pos[1] = 1;
    if (params->mouse_down)
    {
        mlx_mouse_get_pos(params->mlx, params->window, &(pos[0]), &(pos[1]));
        params->last_x = pos[0];
        params->last_y = pos[1];
        if (pos[0] <= SCREEN_WIDTH / 2 || pos[0] >= SCREEN_WIDTH / 2)
            pos[0] = SCREEN_WIDTH / 2;
        if (pos[1] >= SCREEN_HEIGHT / 2 || pos[1] <= SCREEN_HEIGHT / 2)
            pos[1] = SCREEN_HEIGHT / 2;
        params->x_rot += (pos[1] - params->last_y) * 0.05;
        params->z_rot += (pos[0] - params->last_x) * 0.05;
        mlx_mouse_move(params->mlx, params->window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    }
    else
    {
        params->last_x = pos[0];
        params->last_y = pos[1];
        params->z_rot += 0.05;
    }
}

void    apply_rotation_matricies(float x, float y, float z, float result[3][3])
{
    float rot_a[3][3];
    float rot_b[3][3];
    float rot_c[3][3];
    RotationMatrixX(deg_to_rad(x), rot_a);
    RotationMatrixY(deg_to_rad(y), rot_b);
    multiplyMatrices(rot_a, rot_b, rot_c);
    RotationMatrixZ(deg_to_rad(z), rot_b);
    multiplyMatrices(rot_c, rot_b, result);
}

#include <stdio.h>
int    cb_loop(t_params *params)
{

    int i;
    int j;
    float rotFinal[3][3];

    draw_debug_info(params);
    mouse_rotation_handler(params);
    apply_rotation_matricies(params->x_rot, params->y_rot, params->z_rot, rotFinal);
    for (int y = 0; y < 21; y++) {
        for (int x = 0; x < 21; x++)
        {
            float point[3] = {x - 10, y - 10, params->map[x][y]};
            float transformed_point[3];
            float result[2];
            multiply3DVector(rotFinal, point, transformed_point);
            t_point *p = params->screenmap[y * 21 + x];
            p->x = (int)((transformed_point[0] * (params->tile_width * params->zoom) + SCREEN_WIDTH / 2));
            p->y = (int)((transformed_point[1] * (params->tile_height * params->zoom) + SCREEN_HEIGHT / 2));
            p->height = params->map[x][y];
        }
    }
    void *image = mlx_new_image(params->mlx, SCREEN_WIDTH, SCREEN_WIDTH);
    char *data = mlx_get_data_addr(image, &params->bits_per_color, &params->line_size, &params->endianess);
    int color;
    i = 0;
    int yCoord;
    params->data = data;
    unsigned int x;
    unsigned int y;
    unsigned int idx;
    float perc;
    y = 0;
    while(y < SCREEN_HEIGHT)
    {
        x = 0;
        perc = (float)y / (SCREEN_HEIGHT - 1);
        color = color_step_calc(params->bg_color_top, params->bg_color_bottom, perc);
        while(x < SCREEN_WIDTH)
        {
            idx = (x * params->bits_per_color / 8) + (y * params->line_size);
            params->data[idx] = (color & 0xFF);
            params->data[idx + 1] = (color >> 8) & 0xFF;
            params->data[idx + 2] = (color >> 16) & 0xFF;
            params->data[idx + 3] = (color >> 24) & 0xFF;
            x++;
        }
        y++;
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
    params->x_rot = 50;
    params->y_rot = 0;
    params->z_rot = 45;
    params->zoom = 0.25;
    params->tile_width = 30;
    params->tile_height = 30;
    params->min_height = 0;
    params->max_height = 11;
    params->projection = 1;
    params->bg_color_top = 0x57212f;
    params->bg_color_bottom = 0x091027;
    params->model_color_top = 0xffffff;
    params->model_color_bottom = 0xe72337;
    params->scr_ratio = SCREEN_WIDTH / SCREEN_HEIGHT;
    
    i = 0;
    while (i < params->window_size[1])
    {
        j = 0;
        while (j < params->window_size[0])
            params->screen_buffer[i][j++] = 0;
        i++;
    }
    // i = 0;
    // while (i < params->map_dimensions[0])
    // {
    //     j = 0;
    //     while (j < params->map_dimensions[1])
    //     {
    //         int dist_to_center = fmax(abs(i - 21 / 2), abs(j - 21 / 2));
    //         params->map[i][j] = fmax(0, 10 - dist_to_center + 1);
    //         j++;
    //     }
    //     i++;
    // }
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
