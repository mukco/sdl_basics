#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))

const int MAX_BULLETS = 100;
const int MAX_ASTEROIDS = 10;
const int SCREEN_WIDTH = 1000;
const int SCREEN_HEIGHT = 800;
const int MAX_KEYBOARD_KEYS = 350;
const int MAX_STRING_LENGTH = 100;
const float FPS = 60;
float frame_time = 0;

static TTF_Font *font;
static SDL_Texture *ast_texture;
static SDL_Texture *bullet_texture;
static SDL_Texture *player_texture;

struct Bullet
{
  int x;
  int y;
  int w;
  int h;
  SDL_Texture *texture;
};

struct Asteroid
{
  int x;
  int y;
  int w;
  int h;
  SDL_Texture *texture;
};

struct Player
{
  int x;
  int y;
  int w;
  int h;
  int frame_w;
  int frame_h;
  int frame_w_offset;
  SDL_Texture *texture;
  struct Bullet bullets[MAX_BULLETS];
};

typedef struct
{
  int up;
  int down;
  int left;
  int right;
  int fire;
  int ast_index;
  int collisions;
  int bullet_index;
  int player_lives;
  SDL_Window *window;
  struct Player *player;
  SDL_Renderer *renderer;
  SDL_GameController *controller;
  struct Asteroid asteroids[MAX_ASTEROIDS];
} App;

App app;

void init_bullet(void)
{
  struct Bullet bullet;
  bullet.w = 32;
  bullet.h = 32;
  bullet.y = app.player->y;
  bullet.x = app.player->x;
  bullet.texture = bullet_texture;

  if (app.bullet_index < MAX_BULLETS)
  {
    app.player->bullets[app.bullet_index] = bullet;
    app.bullet_index += 1;
  }

  if (app.bullet_index >= MAX_BULLETS)
  {
    app.bullet_index = 0;

    for (int i = 0; i < MAX_BULLETS; i++)
    {
      struct Bullet bullet;
      app.player->bullets[i] = bullet;
    }
  }
}

void init_player(void)
{
  static struct Player player;
  player.frame_w = 123;
  player.frame_w_offset = 0;
  player.frame_h = 50;

  player.w = player.frame_w / 3;
  player.h = player.frame_h;

  player.texture = player_texture;
  player.x = SCREEN_WIDTH / 2 - 32;
  player.y = SCREEN_HEIGHT / 2 - 32;

  app.player = &player;

  SDL_Log("Player initialized\n");
}

void maybe_gen_ast(void)
{
  struct Asteroid *ast = malloc(sizeof(struct Asteroid));
  ast->y = -10;
  ast->x = rand() % SCREEN_WIDTH;
  ast->h = 64;
  ast->w = 64;
  ast->texture = ast_texture;

  if (app.ast_index >= MAX_ASTEROIDS && app.asteroids[app.ast_index - 1].y > SCREEN_HEIGHT)
  {
    app.ast_index = 0;

    for (int i = 0; i < MAX_ASTEROIDS; i++)
    {
      struct Asteroid ast;
      app.asteroids[i] = ast;
    }
  }

  if (app.ast_index < MAX_ASTEROIDS)
  {
    app.asteroids[app.ast_index] = *ast;
    app.ast_index += 1;
  }
}

void init_font(void)
{
  font = TTF_OpenFont("/System/Library/Fonts/Supplemental/Impact.ttf", 25);
  if (!font)
  {
    printf("Error opening font");
    exit(1);
  }
}

void init_textures(void)
{
  bullet_texture = IMG_LoadTexture(app.renderer, "laser.png");
  ast_texture = IMG_LoadTexture(app.renderer, "asteroid.png");
  player_texture = IMG_LoadTexture(app.renderer, "ship_animation.png");
}

void init_app(void)
{
  if (TTF_Init() < 0)
  {
    printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
    exit(1);
  }

  if (IMG_Init(IMG_INIT_PNG) < 0)
  {
    printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
    exit(1);
  }

  if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
  {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    exit(1);
  }

  app.window = SDL_CreateWindow("Game 1", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (!app.window)
  {
    printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    exit(1);
  }

  app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!app.renderer)
  {
    printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
    exit(1);
  }

  int i, n;

  n = SDL_NumJoysticks();

  SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "%d Number of joysticks pending", n);

  for (i = 0; i < n; i++)
  {
    app.controller = SDL_GameControllerOpen(i);
  }

  app.bullet_index = 0;
  app.ast_index = 0;
  app.player_lives = 3;

  init_textures();
  init_font();
  init_player();
}

int out_of_x_bounds(void)
{
  if (app.player->x >= SCREEN_WIDTH - 64 || app.player->x < 0)
  {
    return 1;
  }
  return 0;
}

int out_of_y_bounds(void)
{
  if (app.player->y >= SCREEN_HEIGHT - 64 || app.player->y < 0)
  {
    return 1;
  }
  return 0;
}

void update_player_pos(struct Player *player)
{
  if (app.up == 1 && !(player->y < 0))
  {
    player->y -= 10;
  }

  if (app.down == 1 && !(player->y > SCREEN_HEIGHT - player->h))
  {
    player->y += 10;
  }

  if (app.left == 1 && !(player->x < 0))
  {
    player->x -= 10;
  }

  if (app.right == 1 && !(player->x > SCREEN_WIDTH - player->w))
  {
    player->x += 10;
  }
}


void clear_screen(void)
{
  SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 0);
  SDL_RenderClear(app.renderer);
}

void draw(SDL_Texture *texture, SDL_Rect *dest)
{
  SDL_RenderCopy(app.renderer, texture, NULL, dest);
}

void draw_w_frame(SDL_Texture *texture, SDL_Rect *frame, SDL_Rect *dest)
{
  SDL_RenderCopy(app.renderer, texture, frame, dest);
}

void draw_player(void)
{
  update_player_pos(app.player);

  SDL_Rect dest;
  dest.w = app.player->w;
  dest.h = app.player->h;
  dest.x = app.player->x;
  dest.y = app.player->y;

  SDL_Rect frame;
  frame.x = app.player->frame_w_offset;
  frame.y = 0;
  frame.w = app.player->frame_w / 3;
  frame.h = app.player->frame_h;

  if (FPS / frame_time == 4)
  {
    frame_time = 0;
    app.player->frame_w_offset += (int)app.player->frame_w / 3;
  }

  if (app.player->frame_w_offset >= app.player->frame_w)
  {
    app.player->frame_w_offset = 0;
  }

  frame_time++;

  draw_w_frame(app.player->texture, &frame, &dest);
}

void draw_bullets(void)
{
  if (app.bullet_index == 0)
  {
    return;
  }

  for (int i = 0; i < app.bullet_index; i++)
  {

    struct Bullet *bullet = &app.player->bullets[i];

    SDL_Rect dest;
    dest.x = bullet->x;
    dest.y = bullet->y;
    dest.w = bullet->w;
    dest.h = bullet->h;

    if (bullet->y < SCREEN_HEIGHT)
    {
      bullet->y -= 10;
    }

    if (bullet->y < 0)
    {
      struct Bullet bullet;
      app.player->bullets[i] = bullet;
    }

    draw(bullet->texture, &dest);
  }
}

void draw_asteroids(void)
{
  if (app.ast_index == 0)
  {
    return;
  }

  for (int i = 0; i < app.ast_index; i++)
  {
    struct Asteroid *ast = &app.asteroids[i];

    SDL_Rect dest;
    dest.x = ast->x;
    dest.y = ast->y;
    dest.w = ast->w;
    dest.h = ast->h;

    if (ast->y <= (SCREEN_HEIGHT + 10))
    {
      ast->y += 1;
    
      if ((rand() / 10000000) % 10 == 0 && !(ast->x < 0))
      {
        ast->x -= 1;
      }

      if ((rand() / 10000000) % 10 == 0 && !(ast->x > SCREEN_WIDTH + ast->x))
      {
        ast->x += 1;
      }
      
    }

    draw(ast->texture, &dest);
  }
}

void do_key_up(SDL_KeyboardEvent *event)
{
  if (event->repeat == 0)
  {
    if (event->keysym.scancode == SDL_SCANCODE_UP)
    {
      app.up = 0;
    }

    if (event->keysym.scancode == SDL_SCANCODE_DOWN)
    {
      app.down = 0;
    }

    if (event->keysym.scancode == SDL_SCANCODE_LEFT)
    {
      app.left = 0;
    }

    if (event->keysym.scancode == SDL_SCANCODE_RIGHT)
    {
      app.right = 0;
    }
  }
}

static void do_key_down(SDL_KeyboardEvent *event)
{
  if (event->repeat == 0)
  {
    if (event->keysym.scancode == SDL_SCANCODE_UP)
    {
      app.up = 1;
    }

    if (event->keysym.scancode == SDL_SCANCODE_DOWN)
    {
      app.down = 1;
    }

    if (event->keysym.scancode == SDL_SCANCODE_LEFT)
    {
      app.left = 1;
    }

    if (event->keysym.scancode == SDL_SCANCODE_RIGHT)
    {
      app.right = 1;
    }

    if (event->keysym.scancode == SDL_SCANCODE_RETURN)
    {
      app.collisions = 0;
      app.player_lives = 3;
    }

    if (event->keysym.scancode == SDL_SCANCODE_SPACE)
    {
      init_bullet();
    }
  }
}

void do_joy_axis(SDL_JoyAxisEvent *event)
{
  if ((event->value < -32000) || (event->value > 32000))
  {
    if (event->value < 0 && event->axis == 0)
    {
      app.up = 1;
    }

    if (event->value > 0 && event->axis == 0)
    {
      app.down = 1;
    }

    if (event->value < 0 && event->axis == 1)
    {
      app.left = 1;
    }

    if (event->value > 0 && event->axis == 1)
    {
      app.right = 1;
    }

    int x_axis = SDL_GameControllerGetAxis(app.controller, SDL_CONTROLLER_AXIS_LEFTX);
    SDL_Log("Joy x axis event up %d \n\n", x_axis);

    int y_axis = SDL_GameControllerGetAxis(app.controller, SDL_CONTROLLER_AXIS_LEFTY);
    SDL_Log("Joy y axis event up %d \n\n", y_axis);
  }

  SDL_Log("This is the event value %d \n\n", event->axis);
}

void do_input(void)
{
  SDL_Event e;
  while (SDL_PollEvent(&e))
  {
    switch (e.type)
    {
    case SDL_QUIT:
      exit(0);
      break;

    case SDL_KEYDOWN:
      do_key_down(&e.key);
      break;

    case SDL_KEYUP:
      do_key_up(&e.key);
      break;

    // case SDL_JOYBUTTONDOWN:
    //   doButtonDown(&e.jbutton);
    //   break;

    // case SDL_JOYBUTTONUP:
    //   doButtonUp(&e.jbutton);
    //   break;

    case SDL_CONTROLLERAXISMOTION:
      do_joy_axis(&e.jaxis);
      break;

    case SDL_WINDOWEVENT:
      break;
    }
  }
}

int ast_bull_collision(struct Asteroid *a, struct Bullet *b)
{
  return (
             MAX(a->x, b->x) < MIN(a->x + a->w, b->x + b->w)) &&
         (MAX(a->y, b->y) < MIN(a->y + a->h, b->y + b->h));
}

int ply_ast_collision(struct Player *a, struct Asteroid *b)
{
  return (
             MAX(a->x, b->x) < MIN(a->x + a->w, b->x + b->w)) &&
         (MAX(a->y, b->y) < MIN(a->y + a->h, b->y + b->h));
}

void detect_collisions()
{
  for (int i = 0; i < app.ast_index; i++)
  {
    struct Asteroid *ast = &app.asteroids[i];
    if (ply_ast_collision(app.player, ast))
    {
      app.player->x = SCREEN_WIDTH / 2 - 32;
      app.player->y = SCREEN_HEIGHT / 2 - 32;

      app.player_lives--;

      app.collisions++;
      SDL_Log("Player Asteroid Collision Detected: %d \n", app.collisions);
    }

    for (int j = 0; j < app.bullet_index; j++)
    {
      struct Bullet *bullet = &app.player->bullets[j];
      if (ast_bull_collision(ast, bullet))
      {
        struct Asteroid newAst;
        app.asteroids[i] = newAst;

        struct Bullet newBullet;
        app.player->bullets[j] = newBullet;

        app.collisions++;
        SDL_Log("Bullet Asteroid Collision Detected: %d \n", app.collisions);
      }
    }
  }
}

void draw_text(char text[MAX_STRING_LENGTH], int x, int y)
{

  SDL_Color color = {255, 255, 255, 255};

  SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
  if (!surface)
  {
    printf("Texture error: %s\n", SDL_GetError());
    exit(1);
  }

  SDL_Texture *texture = SDL_CreateTextureFromSurface(app.renderer, surface);
  if (!texture)
  {
    printf("Texture error: %s\n", SDL_GetError());
    exit(1);
  }

  SDL_Rect rect;
  rect.x = x;
  rect.y = y;
  rect.w = 100;
  rect.h = 100;

  SDL_QueryTexture(texture, NULL, NULL, &rect.w, &rect.h);
  draw(texture, &rect);
  SDL_FreeSurface(surface);
  SDL_DestroyTexture(texture);
}

char collision_count[MAX_STRING_LENGTH];
char player_lives_count[MAX_STRING_LENGTH];

void render(void)
{
  SDL_RenderPresent(app.renderer);
  SDL_Delay(19);
}

void game_over_screen(void)
{
  do_input();
  clear_screen();
  draw_text("Game Over", (SCREEN_WIDTH / 2) - 50, (SCREEN_HEIGHT - 100) / 2);
  draw_text("Press Enter To Continue", (SCREEN_WIDTH / 2) - 110, (SCREEN_HEIGHT - 100) / 2 + 100);
  draw_asteroids();
  render();
}

void screen(void)
{
  sprintf(collision_count, "collision detected:  %d", app.collisions);
  sprintf(player_lives_count, "player lives:  %d", app.player_lives);
  do_input();
  clear_screen();
  draw_text(collision_count, 100, 100);
  draw_text(player_lives_count, 100, 125);
  draw_player();
  draw_bullets();
  draw_asteroids();
  detect_collisions();
  render();
}

void do_asteroid(void)
{
  int rand_number = rand() / 10000;
  if (rand_number % 100 == 2)
  {
    maybe_gen_ast();
  }
}

void loop()
{
  for (;;)
  {
    do_asteroid();
    if (app.player_lives == 0)
    {
      game_over_screen();
    }
    else
    {
      screen();
    }
  }
}

int main(int argc, char *argv[])
{
  memset(&app, 0, sizeof(App));
  init_app();
  loop();
  return 0;
}